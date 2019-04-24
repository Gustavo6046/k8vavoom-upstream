//**************************************************************************
//**
//**    ##   ##    ##    ##   ##   ####     ####   ###     ###
//**    ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**     ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**     ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**      ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**       #    ##    ##    #      ####     ####   ##       ##
//**
//**  Copyright (C) 1999-2006 Jānis Legzdiņš
//**  Copyright (C) 2018-2019 Ketmar Dark
//**
//**  This program is free software: you can redistribute it and/or modify
//**  it under the terms of the GNU General Public License as published by
//**  the Free Software Foundation, either version 3 of the License, or
//**  (at your option) any later version.
//**
//**  This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**  You should have received a copy of the GNU General Public License
//**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//**
//**************************************************************************
//**
//**  BSP traversal, handling of LineSegs for rendering.
//**
//**************************************************************************
#include <limits.h>
#include <float.h>

#include "gamedefs.h"
#include "r_local.h"

#define HORIZON_SURF_SIZE (sizeof(surface_t) + sizeof(TVec) * 3)

//#define VRBSP_DISABLE_SKY_PORTALS


VCvarB r_draw_pobj("r_draw_pobj", true, "Render polyobjects?", CVAR_PreInit);
static VCvarI r_maxmirrors("r_maxmirrors", "1", "Maximum allowed mirrors.", CVAR_Archive);
VCvarI r_max_portal_depth("r_max_portal_depth", "1", "Maximum allowed portal depth (-1: infinite)", CVAR_Archive);
static VCvarB r_allow_horizons("r_allow_horizons", true, "Allow horizon portal rendering?", CVAR_Archive);
static VCvarB r_allow_mirrors("r_allow_mirrors", true, "Allow mirror portal rendering (SLOW)?", CVAR_Archive);

static VCvarB r_disable_sky_portals("r_disable_sky_portals", false, "Disable rendering of sky portals.", 0/*CVAR_Archive*/);

static VCvarB dbg_max_portal_depth_warning("dbg_max_portal_depth_warning", false, "Show maximum allowed portal depth warning?", 0/*CVAR_Archive*/);

static VCvarB r_flood_renderer("r_flood_renderer", false, "Use new floodfill renderer?", CVAR_PreInit);

VCvarB VRenderLevelShared::times_render_highlevel("times_render_highlevel", false, "Show high-level render times.", 0/*CVAR_Archive*/);
VCvarB VRenderLevelShared::times_render_lowlevel("times_render_lowlevel", false, "Show low-level render times.", 0/*CVAR_Archive*/);
VCvarB VRenderLevelShared::r_disable_world_update("r_disable_world_update", false, "Disable world updates.", 0/*CVAR_Archive*/);

extern int light_reset_surface_cache; // in r_light_reg.cpp
extern VCvarB r_decals_enabled;
extern VCvarB r_draw_adjacent_subsector_things;
extern VCvarB w_update_in_renderer;
extern VCvarB clip_frustum;
extern VCvarB clip_frustum_bsp;
extern VCvarB clip_frustum_mirror;
extern VCvarB clip_use_1d_clipper;

// to clear portals
static bool oldMirrors = true;
static bool oldHorizons = true;
static int oldMaxMirrors = -666;
static int oldPortalDepth = -666;


//==========================================================================
//
//  VRenderLevelShared::SurfCheckAndQueue
//
//  this checks if surface is not queued twice
//
//==========================================================================
void VRenderLevelShared::SurfCheckAndQueue (TArray<surface_t *> &queue, surface_t *surf) {
  check(surf);

  VTexture *tex = surf->texinfo->Tex;
  if (!tex || tex->Type == TEXTYPE_Null) return;

  if (surf->queueframe == currQueueFrame) {
    if (surf->seg) {
      //abort();
      GCon->Logf(NAME_Warning, "subsector %d, seg %d surface queued for rendering twice",
        (int)(ptrdiff_t)(surf->seg-Level->Segs),
        (int)(ptrdiff_t)(surf->subsector-Level->Subsectors));
    } else {
      GCon->Logf(NAME_Warning, "subsector %d surface queued for rendering twice",
        (int)(ptrdiff_t)(surf->subsector-Level->Subsectors));
    }
    return;
  }
  surf->queueframe = currQueueFrame;

  if (!(surf->drawflags&surface_t::DF_MASKED)) {
    // check for non-solid texture
    if (surf->texinfo->Alpha < 1.0f || tex->isTransparent()) surf->drawflags |= surface_t::DF_MASKED;
  }

  queue.append(surf);
  //GCon->Logf("frame %u: queued surface with texinfo %p", currQueueFrame, surf->texinfo);
}


//==========================================================================
//
//  VRenderLevelShared::QueueSimpleSurf
//
//==========================================================================
void VRenderLevelShared::QueueSimpleSurf (surface_t *surf) {
  SurfCheckAndQueue(DrawSurfList, surf);
}


//==========================================================================
//
//  VRenderLevelShared::QueueSkyPortal
//
//==========================================================================
void VRenderLevelShared::QueueSkyPortal (surface_t *surf) {
  SurfCheckAndQueue(DrawSkyList, surf);
}


//==========================================================================
//
//  VRenderLevelShared::QueueHorizonPortal
//
//==========================================================================
void VRenderLevelShared::QueueHorizonPortal (surface_t *surf) {
  SurfCheckAndQueue(DrawHorizonList, surf);
}


//==========================================================================
//
//  VRenderLevelShared::DrawSurfaces
//
//==========================================================================
void VRenderLevelShared::DrawSurfaces (subsector_t *sub, sec_region_t *secregion, seg_t *seg,
  surface_t *InSurfs, texinfo_t *texinfo, VEntity *SkyBox, int LightSourceSector, int SideLight,
  bool AbsSideLight, bool CheckSkyBoxAlways)
{
  surface_t *surfs = InSurfs;
  if (!surfs) return;

  if (texinfo->Tex->Type == TEXTYPE_Null) return;

  sec_params_t *LightParams = (LightSourceSector == -1 ? secregion->params : &Level->Sectors[LightSourceSector].params);
  int lLev = (AbsSideLight ? 0 : LightParams->lightlevel)+SideLight;
  lLev = (FixedLight ? FixedLight : lLev+ExtraLight);
  lLev = MID(0, lLev, 255);
  if (r_darken) lLev = light_remap[lLev];
  vuint32 Fade = GetFade(secregion);

  if (SkyBox && (SkyBox->EntityFlags&VEntity::EF_FixedModel)) SkyBox = nullptr;
  bool IsStack = SkyBox && SkyBox->eventSkyBoxGetAlways();
  if (texinfo->Tex == GTextureManager[skyflatnum] || (IsStack && CheckSkyBoxAlways)) { //k8: i hope that the parens are right here
    VSky *Sky = nullptr;
    if (!SkyBox && (sub->sector->Sky&SKY_FROM_SIDE) != 0) {
      int Tex;
      bool Flip;
      if (sub->sector->Sky == SKY_FROM_SIDE) {
        Tex = Level->LevelInfo->Sky2Texture;
        Flip = true;
      } else {
        side_t *Side = &Level->Sides[(sub->sector->Sky&(SKY_FROM_SIDE-1))-1];
        Tex = Side->TopTexture;
        Flip = !!Level->Lines[Side->LineNum].arg3;
      }
      if (GTextureManager[Tex]->Type != TEXTYPE_Null) {
        for (int i = 0; i < SideSkies.Num(); ++i) {
          if (SideSkies[i]->SideTex == Tex && SideSkies[i]->SideFlip == Flip) {
            Sky = SideSkies[i];
            break;
          }
        }
        if (!Sky) {
          Sky = new VSky;
          Sky->Init(Tex, Tex, 0, 0, false, !!(Level->LevelInfo->LevelInfoFlags&VLevelInfo::LIF_ForceNoSkyStretch), Flip, false);
          SideSkies.Append(Sky);
        }
      }
    }

    if (!Sky && !SkyBox) {
      InitSky();
      Sky = &BaseSky;
    }

    VPortal *Portal = nullptr;
    if (SkyBox) {
      for (int i = 0; i < Portals.Num(); ++i) {
        if (Portals[i] && Portals[i]->MatchSkyBox(SkyBox)) {
          Portal = Portals[i];
          break;
        }
      }
      if (!Portal) {
        if (IsStack) {
          Portal = new VSectorStackPortal(this, SkyBox);
          Portals.Append(Portal);
        } else {
#if !defined(VRBSP_DISABLE_SKY_PORTALS)
          if (!r_disable_sky_portals) {
            Portal = new VSkyBoxPortal(this, SkyBox);
            Portals.Append(Portal);
          }
#endif
        }
      }
    } else {
      for (int i = 0; i < Portals.Num(); ++i) {
        if (Portals[i] && Portals[i]->MatchSky(Sky)) {
          Portal = Portals[i];
          break;
        }
      }
#if !defined(VRBSP_DISABLE_SKY_PORTALS)
      if (!Portal && !r_disable_sky_portals) {
        Portal = new VSkyPortal(this, Sky);
        Portals.Append(Portal);
      }
#endif
    }
    if (!Portal) return;
    //GCon->Log("----");
    int doRenderSurf = -1;
    do {
      if (!surfs->IsVisible(vieworg)) {
        // viewer is in back side or on plane
        //GCon->Logf("  SURF SKIP!");
        continue;
      }
      Portal->Surfs.Append(surfs);
      if (doRenderSurf < 0) {
        doRenderSurf = (IsStack && CheckSkyBoxAlways && SkyBox->eventSkyBoxGetPlaneAlpha() ? 1 : 0);
      }
      //if (IsStack && CheckSkyBoxAlways && SkyBox->eventSkyBoxGetPlaneAlpha())
      if (doRenderSurf) {
        //GCon->Logf("  SURF!");
        surfs->Light = (lLev<<24)|LightParams->LightColour;
        surfs->Fade = Fade;
        surfs->dlightframe = sub->dlightframe;
        surfs->dlightbits = sub->dlightbits;
        DrawTranslucentPoly(surfs, surfs->verts, surfs->count,
          0, SkyBox->eventSkyBoxGetPlaneAlpha(), false, 0,
          false, 0, Fade, TVec(), 0, TVec(), TVec(), TVec());
      }
      surfs = surfs->next;
    } while (surfs);
    return;
  } // done skybox rendering

  for (; surfs; surfs = surfs->next) {
    //if (!surfs->IsVisible(vieworg)) continue;

    surfs->Light = (lLev<<24)|LightParams->LightColour;
    surfs->Fade = Fade;
    surfs->dlightframe = sub->dlightframe;
    surfs->dlightbits = sub->dlightbits;
    // alpha: 1.0 is masked wall, 1.1 is solid wall
    if (texinfo->Alpha <= 1.0f) surfs->drawflags |= surface_t::DF_MASKED; else surfs->drawflags &= ~surface_t::DF_MASKED;

    if (texinfo->Alpha >= 1.0f) {
      if (PortalLevel == 0) {
        world_surf_t &S = WorldSurfs.Alloc();
        S.Surf = surfs;
        S.Type = 0;
      } else {
        QueueWorldSurface(surfs);
      }
    } else {
      DrawTranslucentPoly(surfs, surfs->verts, surfs->count,
        0, texinfo->Alpha, texinfo->Additive, 0, false, 0, Fade,
        TVec(), 0, TVec(), TVec(), TVec());
    }
  }
}


//==========================================================================
//
//  VRenderLevelShared::RenderHorizon
//
//==========================================================================
void VRenderLevelShared::RenderHorizon (subsector_t *sub, sec_region_t *secregion, subregion_t *subregion, drawseg_t *dseg) {
  seg_t *seg = dseg->seg;

  if (!dseg->HorizonTop) {
    dseg->HorizonTop = (surface_t *)Z_Malloc(HORIZON_SURF_SIZE);
    dseg->HorizonBot = (surface_t *)Z_Malloc(HORIZON_SURF_SIZE);
    memset((void *)dseg->HorizonTop, 0, HORIZON_SURF_SIZE);
    memset((void *)dseg->HorizonBot, 0, HORIZON_SURF_SIZE);
  }

  // horizon is not supported in sectors with slopes, so just use TexZ
  float TopZ = secregion->eceiling.splane->TexZ;
  float BotZ = secregion->efloor.splane->TexZ;
  float HorizonZ = vieworg.z;

  // handle top part
  if (TopZ > HorizonZ) {
    sec_surface_t *Ceil = (subregion->fakeceil ? subregion->fakeceil : subregion->realceil);
    if (Ceil) {
      // calculate light and fade
      sec_params_t *LightParams = Ceil->esecplane.splane->LightSourceSector != -1 ?
        &Level->Sectors[Ceil->esecplane.splane->LightSourceSector].params :
        secregion->params;
      int lLev = (FixedLight ? FixedLight : MIN(255, LightParams->lightlevel+ExtraLight));
      if (r_darken) lLev = light_remap[lLev];
      vuint32 Fade = GetFade(secregion);

      surface_t *Surf = dseg->HorizonTop;
      Surf->plane = *(TPlane *)(dseg->seg);
      Surf->texinfo = &Ceil->texinfo;
      Surf->HorizonPlane = Ceil->esecplane.splane; //FIXME: 3dfloor
      Surf->Light = (lLev<<24)|LightParams->LightColour;
      Surf->Fade = Fade;
      Surf->count = 4;
      TVec *svs = &Surf->verts[0];
      svs[0] = *seg->v1; svs[0].z = MAX(BotZ, HorizonZ);
      svs[1] = *seg->v1; svs[1].z = TopZ;
      svs[2] = *seg->v2; svs[2].z = TopZ;
      svs[3] = *seg->v2; svs[3].z = MAX(BotZ, HorizonZ);
      if (Ceil->esecplane.splane->pic == skyflatnum) {
        // if it's a sky, render it as a regular sky surface
        DrawSurfaces(sub, secregion, nullptr, Surf, &Ceil->texinfo, secregion->eceiling.splane->SkyBox, -1,
          seg->sidedef->Light, !!(seg->sidedef->Flags&SDF_ABSLIGHT),
          false);
      } else {
        if (PortalLevel == 0) {
          world_surf_t &S = WorldSurfs.Alloc();
          S.Surf = Surf;
          S.Type = 2;
        } else {
          QueueHorizonPortal(Surf);
        }
      }
    }
  }

  // handle bottom part
  if (BotZ < HorizonZ) {
    sec_surface_t *Floor = (subregion->fakefloor ? subregion->fakefloor : subregion->realfloor);
    if (Floor) {
      // calculate light and fade
      sec_params_t *LightParams = Floor->esecplane.splane->LightSourceSector != -1 ?
        &Level->Sectors[Floor->esecplane.splane->LightSourceSector].params :
        secregion->params;
      int lLev = (FixedLight ? FixedLight : MIN(255, LightParams->lightlevel+ExtraLight));
      if (r_darken) lLev = light_remap[lLev];
      vuint32 Fade = GetFade(secregion);

      surface_t *Surf = dseg->HorizonBot;
      Surf->plane = *(TPlane *)(dseg->seg);
      Surf->texinfo = &Floor->texinfo;
      Surf->HorizonPlane = Floor->esecplane.splane; //FIXME: 3dfloor
      Surf->Light = (lLev<<24)|LightParams->LightColour;
      Surf->Fade = Fade;
      Surf->count = 4;
      TVec *svs = &Surf->verts[0];
      svs[0] = *seg->v1; svs[0].z = BotZ;
      svs[1] = *seg->v1; svs[1].z = MIN(TopZ, HorizonZ);
      svs[2] = *seg->v2; svs[2].z = MIN(TopZ, HorizonZ);
      svs[3] = *seg->v2; svs[3].z = BotZ;
      if (Floor->esecplane.splane->pic == skyflatnum) {
        // if it's a sky, render it as a regular sky surface
        DrawSurfaces(sub, secregion, nullptr, Surf, &Floor->texinfo, secregion->efloor.splane->SkyBox, -1,
          seg->sidedef->Light, !!(seg->sidedef->Flags&SDF_ABSLIGHT),
          false);
      } else {
        if (PortalLevel == 0) {
          world_surf_t &S = WorldSurfs.Alloc();
          S.Surf = Surf;
          S.Type = 2;
        } else {
          QueueHorizonPortal(Surf);
        }
      }
    }
  }
}


//==========================================================================
//
//  VRenderLevelShared::RenderMirror
//
//==========================================================================
void VRenderLevelShared::RenderMirror (subsector_t *sub, sec_region_t *secregion, drawseg_t *dseg) {
  seg_t *seg = dseg->seg;
  if (MirrorLevel < r_maxmirrors && r_allow_mirrors) {
    if (!dseg->mid) return;

    VPortal *Portal = nullptr;
    for (int i = 0; i < Portals.Num(); ++i) {
      if (Portals[i] && Portals[i]->MatchMirror(seg)) {
        Portal = Portals[i];
        break;
      }
    }
    if (!Portal) {
      Portal = new VMirrorPortal(this, seg);
      Portals.Append(Portal);
    }

    surface_t *surfs = dseg->mid->surfs;
    do {
      Portal->Surfs.Append(surfs);
      surfs = surfs->next;
    } while (surfs);
  } else {
    if (dseg->mid) DrawSurfaces(sub, secregion, seg, dseg->mid->surfs, &dseg->mid->texinfo,
      secregion->eceiling.splane->SkyBox, -1, seg->sidedef->Light,
      !!(seg->sidedef->Flags&SDF_ABSLIGHT), false);
  }
}


//==========================================================================
//
//  VRenderLevelShared::RenderLine
//
//  Clips the given segment and adds any visible pieces to the line list.
//
//==========================================================================
void VRenderLevelShared::RenderLine (subsector_t *sub, sec_region_t *secregion, subregion_t *subregion, drawseg_t *dseg) {
  seg_t *seg = dseg->seg;
  line_t *linedef = seg->linedef;

  if (!linedef) return; // miniseg

  if (seg->PointOnSide(vieworg)) {
    // viewer is in back side or on plane
    // gozzo 3d floors should be rendered regardless from orientation
    segpart_t *sp = dseg->extra;
    if (sp && sp->texinfo.Tex && (sp->texinfo.Alpha < 1.0f || sp->texinfo.Tex->isTransparent())) {
      side_t *sidedef = seg->sidedef;
      //GCon->Logf("00: extra for seg #%d (line #%d)", (int)(ptrdiff_t)(seg-Level->Segs), (int)(ptrdiff_t)(linedef-Level->Lines));
      for (; sp; sp = sp->next) {
        DrawSurfaces(sub, secregion, seg, sp->surfs, &sp->texinfo, secregion->eceiling.splane->SkyBox,
          -1, sidedef->Light, !!(sidedef->Flags&SDF_ABSLIGHT), false);
        //GCon->Logf("  extra for seg #%d (%p)", (int)(ptrdiff_t)(seg-Level->Segs), sp);
      }
      //GCon->Logf("01: extra for seg #%d", (int)(ptrdiff_t)(seg-Level->Segs));
    }
    return;
  }

  if (MirrorClipSegs && clip_frustum && clip_frustum_mirror && clip_frustum_bsp && view_frustum.planes[5].isValid()) {
    // clip away segs that are behind mirror
    if (view_frustum.planes[5].PointOnSide(*seg->v1) && view_frustum.planes[5].PointOnSide(*seg->v2)) return; // behind mirror
  }

/*
    k8: i don't know what Janis wanted to accomplish with this, but it actually
        makes clipping WORSE due to limited precision
  if (seg->backsector) {
    // just apply this to sectors without slopes
    if (seg->frontsector->floor.normal.z == 1.0f && seg->backsector->floor.normal.z == 1.0f &&
        seg->frontsector->ceiling.normal.z == -1.0f && seg->backsector->ceiling.normal.z == -1.0f)
    {
      // clip sectors that are behind rendered segs
      TVec v1 = *seg->v1;
      TVec v2 = *seg->v2;
      TVec r1 = vieworg-v1;
      TVec r2 = vieworg-v2;
      float D1 = DotProduct(Normalise(CrossProduct(r1, r2)), vieworg);
      float D2 = DotProduct(Normalise(CrossProduct(r2, r1)), vieworg);

      // there might be a better method of doing this, but this one works for now...
           if (D1 > 0.0f && D2 < 0.0f) v2 += ((v2-v1)*D1)/(D1-D2);
      else if (D2 > 0.0f && D1 < 0.0f) v1 += ((v2-v1)*D1)/(D2-D1);

      if (!ViewClip.IsRangeVisible(ViewClip.PointToClipAngle(v2), ViewClip.PointToClipAngle(v1))) return;
    }
  } else {
    // clip sectors that are behind rendered segs
    TVec v1 = *seg->v1;
    TVec v2 = *seg->v2;
    TVec r1 = vieworg-v1;
    TVec r2 = vieworg-v2;
    float D1 = DotProduct(Normalise(CrossProduct(r1, r2)), vieworg);
    float D2 = DotProduct(Normalise(CrossProduct(r2, r1)), vieworg);

    // there might be a better method of doing this, but this one works for now...
         if (D1 > 0.0f && D2 < 0.0f) v2 += ((v2-v1)*D1)/(D1-D2);
    else if (D2 > 0.0f && D1 < 0.0f) v1 += ((v2-v1)*D1)/(D2-D1);

    if (!ViewClip.IsRangeVisible(ViewClip.PointToClipAngle(v2), ViewClip.PointToClipAngle(v1))) return;
  }
*/
  if (!ViewClip.IsRangeVisible(*seg->v2, *seg->v1)) return;

  // k8: this drops some segs that may leak without proper frustum culling
  if (!ViewClip.CheckSegFrustum(seg)) return;

  //FIXME this marks all lines
  // mark the segment as visible for auto map
  //linedef->flags |= ML_MAPPED;
  if (!(linedef->flags&ML_MAPPED)) {
    // this line is at least partially mapped; let automap drawer do the rest
    linedef->exFlags |= ML_EX_PARTIALLY_MAPPED|ML_EX_CHECK_MAPPED;
  }
  seg->flags |= SF_MAPPED;

  side_t *sidedef = seg->sidedef;

  VEntity *SkyBox = secregion->eceiling.splane->SkyBox;
  if (!seg->backsector) {
    // single sided line
    if (seg->linedef->special == LNSPEC_LineHorizon && r_allow_horizons) {
      RenderHorizon(sub, secregion, subregion, dseg);
    } else if (seg->linedef->special == LNSPEC_LineMirror) {
      RenderMirror(sub, secregion, dseg);
    } else {
      if (dseg->mid) DrawSurfaces(sub, secregion, seg, dseg->mid->surfs, &dseg->mid->texinfo, SkyBox, -1, sidedef->Light, !!(sidedef->Flags&SDF_ABSLIGHT), false);
    }
    if (dseg->topsky) DrawSurfaces(sub, secregion, nullptr, dseg->topsky->surfs, &dseg->topsky->texinfo, SkyBox, -1, sidedef->Light, !!(sidedef->Flags&SDF_ABSLIGHT), false);
  } else {
    // two sided line
    if (dseg->top) DrawSurfaces(sub, secregion, seg, dseg->top->surfs, &dseg->top->texinfo, SkyBox, -1, sidedef->Light, !!(sidedef->Flags&SDF_ABSLIGHT), false);
    if (dseg->topsky) DrawSurfaces(sub, secregion, nullptr, dseg->topsky->surfs, &dseg->topsky->texinfo, SkyBox, -1, sidedef->Light, !!(sidedef->Flags&SDF_ABSLIGHT), false);
    if (dseg->bot) DrawSurfaces(sub, secregion, seg, dseg->bot->surfs, &dseg->bot->texinfo, SkyBox, -1, sidedef->Light, !!(sidedef->Flags&SDF_ABSLIGHT), false);
    if (dseg->mid) DrawSurfaces(sub, secregion, seg, dseg->mid->surfs, &dseg->mid->texinfo, SkyBox, -1, sidedef->Light, !!(sidedef->Flags&SDF_ABSLIGHT), false);
    for (segpart_t *sp = dseg->extra; sp; sp = sp->next) {
      DrawSurfaces(sub, secregion, seg, sp->surfs, &sp->texinfo, SkyBox, -1, sidedef->Light, !!(sidedef->Flags&SDF_ABSLIGHT), false);
    }
  }
}


//==========================================================================
//
//  VRenderLevelShared::RenderSecSurface
//
//==========================================================================
void VRenderLevelShared::RenderSecSurface (subsector_t *sub, sec_region_t *secregion, sec_surface_t *ssurf, VEntity *SkyBox) {
  if (!ssurf) return;
  TSecPlaneRef plane(ssurf->esecplane);

  if (!plane.splane->pic) return;

  if (plane.PointOnSide(vieworg)) return; // viewer is in back side or on plane

  if (r_allow_mirrors && MirrorLevel < r_maxmirrors && plane.splane->MirrorAlpha < 1.0f) {
    VPortal *Portal = nullptr;
    for (int i = 0; i < Portals.Num(); ++i) {
      if (Portals[i] && Portals[i]->MatchMirror(plane.splane)) {
        Portal = Portals[i];
        break;
      }
    }
    if (!Portal) {
      Portal = new VMirrorPortal(this, plane.splane);
      Portals.Append(Portal);
    }

    surface_t *surfs = ssurf->surfs;
    do {
      Portal->Surfs.Append(surfs);
      surfs = surfs->next;
    } while (surfs);

    if (plane.splane->MirrorAlpha <= 0.0f) return;
    // k8: is this right?
    ssurf->texinfo.Alpha = plane.splane->MirrorAlpha;
  } else {
    // this is NOT right!
    //ssurf->texinfo.Alpha = 1.0f;
    if (plane.splane->MirrorAlpha < 1.0f) {
      if (ssurf->texinfo.Alpha >= 1.0f) {
        //GCon->Logf("MALPHA=%f", plane.splane->MirrorAlpha);
        // darken it a little to simulate mirror
        sec_params_t *oldRegionLightParams = secregion->params;
        sec_params_t newLight = (plane.splane->LightSourceSector >= 0 ? Level->Sectors[plane.splane->LightSourceSector].params : *oldRegionLightParams);
        newLight.lightlevel = (int)((float)newLight.lightlevel*plane.splane->MirrorAlpha);
        secregion->params = &newLight;
        // take light from `secregion->params`
        DrawSurfaces(sub, secregion, nullptr, ssurf->surfs, &ssurf->texinfo, SkyBox, -1, 0, false, true);
        // and resore rregion
        secregion->params = oldRegionLightParams;
        return;
      }
    }
  }

  DrawSurfaces(sub, secregion, nullptr, ssurf->surfs, &ssurf->texinfo, SkyBox, plane.splane->LightSourceSector, 0, false, true);
}


//==========================================================================
//
//  VRenderLevelShared::RenderSubRegion
//
//  Determine floor/ceiling planes.
//  Draw one or more line segments.
//
//==========================================================================
void VRenderLevelShared::RenderSubRegion (subsector_t *sub, subregion_t *region, bool &addPoly, bool useClipper) {
  //const float d = DotProduct(vieworg, region->floor->secplane->normal)-region->floor->secplane->dist;
  sec_surface_t *floor = (region->fakefloor ? region->fakefloor : region->realfloor);
  float d;
  if (floor) {
    d = floor->PointDist(vieworg);
    if (region->next && d <= 0.0f) {
      if (useClipper && !ViewClip.ClipCheckRegion(region->next, sub)) return;
      RenderSubRegion(sub, region->next, addPoly, useClipper);
    }
  } else {
    d = 1.0f;
  }

  check(sub->sector != nullptr);

  subregion_t *subregion = region;
  sec_region_t *secregion = region->secregion;

  if (addPoly && sub->poly && r_draw_pobj) {
    // render the polyobj in the subsector first
    addPoly = false;
    seg_t **polySeg = sub->poly->segs;
    for (int polyCount = sub->poly->numsegs; polyCount--; ++polySeg) {
      RenderLine(sub, secregion, subregion, (*polySeg)->drawsegs);
    }
  }

  int count = sub->numlines;
  drawseg_t *ds = region->lines;
  while (count--) {
    RenderLine(sub, secregion, subregion, ds);
    ++ds;
  }

  if (region->realfloor) RenderSecSurface(sub, secregion, region->realfloor, secregion->efloor.splane->SkyBox);
  if (region->realceil) RenderSecSurface(sub, secregion, region->realceil, secregion->eceiling.splane->SkyBox);

  if (region->fakefloor) RenderSecSurface(sub, secregion, region->fakefloor, secregion->efloor.splane->SkyBox);
  if (region->fakeceil) RenderSecSurface(sub, secregion, region->fakeceil, secregion->eceiling.splane->SkyBox);

  if (region->next && d > 0.0f) {
    if (useClipper && !ViewClip.ClipCheckRegion(region->next, sub)) return;
    RenderSubRegion(sub, region->next, addPoly, useClipper);
  }
}


//==========================================================================
//
//  VRenderLevelShared::RenderMarkAdjSubsectorsThings
//
//  used for "better things rendering"
//
//==========================================================================
void VRenderLevelShared::RenderMarkAdjSubsectorsThings (int num) {
  BspVisThing[((unsigned)num)>>3] |= 1U<<(num&7);
  if (r_draw_adjacent_subsector_things) {
    subsector_t *sub = &Level->Subsectors[num];
    int sgcount = sub->numlines;
    if (sgcount) {
      const seg_t *seg = &Level->Segs[sub->firstline];
      for (; sgcount--; ++seg) {
        if (seg->linedef && !(seg->linedef->flags&ML_TWOSIDED)) continue; // don't go through solid walls
        const seg_t *pseg = seg->partner;
        if (!pseg || !pseg->front_sub) continue;
        const unsigned psidx = (unsigned)(ptrdiff_t)(pseg->front_sub-Level->Subsectors);
        BspVisThing[psidx>>3] |= 1U<<(psidx&7);
      }
    }
  }
}


//==========================================================================
//
//  VRenderLevelShared::RenderSubsector
//
//==========================================================================
void VRenderLevelShared::RenderSubsector (int num, bool useClipper) {
  subsector_t *sub = &Level->Subsectors[num];
  //r_sub = sub;

  if (!sub->sector->linecount) return; // skip sectors containing original polyobjs

  if (Level->HasPVS() && sub->VisFrame != currVisFrame) {
    if (r_draw_adjacent_subsector_things) {
      if (!useClipper || ViewClip.ClipCheckSubsector(sub)) {
        RenderMarkAdjSubsectorsThings(num);
        if (useClipper && clip_use_1d_clipper) ViewClip.ClipAddSubsectorSegs(sub, (MirrorClipSegs && view_frustum.planes[5].isValid() ? &view_frustum.planes[5] : nullptr));
      }
    } else if (useClipper && clip_use_1d_clipper) {
      ViewClip.ClipAddSubsectorSegs(sub, (MirrorClipSegs && view_frustum.planes[5].isValid() ? &view_frustum.planes[5] : nullptr));
    }
    return;
  }

  if (useClipper && !ViewClip.ClipCheckSubsector(sub)) return;

  if (sub->parent) sub->parent->VisFrame = currVisFrame; // for one-sector degenerate maps
  sub->VisFrame = currVisFrame;

  // mark this subsector as rendered
  BspVis[((unsigned)num)>>3] |= 1U<<(num&7);

  // mark thing subsectors
  RenderMarkAdjSubsectorsThings(num);

  // update world
  if (w_update_in_renderer && sub->updateWorldFrame != updateWorldFrame) {
    UpdateSubsector(num, nullptr); // trigger BSP updating
  }

  bool addPoly = true;
  RenderSubRegion(sub, sub->regions, addPoly, useClipper);

  // add subsector's segs to the clipper
  // clipping against mirror is done only for vertical mirror planes
  if (useClipper && clip_use_1d_clipper) {
    ViewClip.ClipAddSubsectorSegs(sub, (MirrorClipSegs && view_frustum.planes[5].isValid() ? &view_frustum.planes[5] : nullptr));
  }
}


//==========================================================================
//
//  bspCalcZExtents
//
//==========================================================================
static __attribute__((unused)) void bspCalcZExtents (VLevel *level, int bspnum, float *minz, float *maxz) {
  if ((bspnum&NF_SUBSECTOR) == 0) {
    node_t *bsp = &level->Nodes[bspnum];
    bspCalcZExtents(level, bsp->children[0], minz, maxz);
    bspCalcZExtents(level, bsp->children[1], minz, maxz);
  } else {
    const subsector_t *sub = &level->Subsectors[bspnum&(~NF_SUBSECTOR)];
    if (sub->sector) level->CalcSectorBoundingHeight(sub->sector, minz, maxz);
  }
}


//==========================================================================
//
//  VRenderLevelShared::RenderBSPNode
//
//  Renders all subsectors below a given node, traversing subtree
//  recursively. Just call with BSP root.
//
//==========================================================================
void VRenderLevelShared::RenderBSPNode (int bspnum, const float *bbox, unsigned AClipflags, bool onlyClip) {
#ifdef VV_CLIPPER_FULL_CHECK
  if (ViewClip.ClipIsFull()) return;
#endif

  if (bspnum == -1) {
    RenderSubsector(0);
    return;
  }

  if (!ViewClip.ClipIsBBoxVisible(bbox)) return;

  unsigned clipflags = AClipflags;
  if (!onlyClip) {
    // cull the clipping planes if not trivial accept
    if (clipflags && clip_frustum && clip_frustum_bsp) {
      //static float newbbox[6];
      //memcpy(newbbox, bbox, sizeof(float)*6);
      //newbbox[2] = -32767.0f;
      //newbbox[5] = +32767.0f;
      const TClipPlane *cp = &view_frustum.planes[0];
      for (unsigned i = view_frustum.planeCount; i--; ++cp) {
        if (!(clipflags&cp->clipflag)) continue; // don't need to clip against it
        //k8: this check is always true, because view origin is outside of frustum (oops)
        //if (cp->PointOnSide(vieworg)) continue; // viewer is in back side or on plane (k8: why check this?)
#if defined(FRUSTUM_BOX_OPTIMISATION)
        // check reject point
        if (cp->PointOnSide(TVec(bbox[cp->pindex[0]], bbox[cp->pindex[1]], bbox[cp->pindex[2]]))) {
          // completely outside of any plane means "invisible"
          check(cp->PointOnSide(TVec(bbox[cp->pindex[3+0]], bbox[cp->pindex[3+1]], bbox[cp->pindex[3+2]])));
          return;
        }
        // is node entirely on screen?
        // k8: don't do this: frustum test are cheap, and we can hit false positive easily
        /*
        if (!cp->PointOnSide(TVec(bbox[cp->pindex[3+0]], bbox[cp->pindex[3+1]], bbox[cp->pindex[3+2]]))) {
          // yes, don't check this plane
          clipflags ^= cp->clipflag;
        }
        */
#elif 0
        int cres = cp->checkBoxEx(bbox);
        if (cres == TFrustum::OUTSIDE) return;
        // k8: don't do this: frustum test are cheap, and we can hit false positive easily
        if (cres == TFrustum::INSIDE) clipflags ^= cp->clipflag; // don't check this plane anymore
#else
        if (!cp->checkBox(bbox)) {
          //HACK: this should be done in world update, but just in case it failed to do its work...
          //      seems to be fixed
          /*
          float newbbox[6];
          memcpy(newbbox, bbox, sizeof(float)*6);
          newbbox[2] = -32767.0f;
          newbbox[5] = +32767.0f;
          if (cp->checkBox(newbbox)) {
            float minz = 999999.0f;
            float maxz = -999999.0f;
            bspCalcZExtents(Level, bspnum, &minz, &maxz);
            newbbox[2] = MIN(minz, bbox[2]);
            newbbox[5] = MAX(maxz, bbox[5]);
            if (cp->checkBox(newbbox)) {
              GCon->Logf("BBOX! (%f,%f) : (%f,%f) : %d", bbox[2], bbox[5], minz, maxz, (int)cp->checkBox(newbbox));
              if ((bspnum&NF_SUBSECTOR) == 0) {
                node_t *bsp = &Level->Nodes[bspnum];
                node_t *child = bsp;
                for (bsp = bsp->parent; bsp; child = bsp, bsp = bsp->parent) {
                  if (bsp->children[0] == (unsigned)(ptrdiff_t)(child-Level->Nodes)) {
                    bsp->bbox[0][2] = newbbox[2];
                    bsp->bbox[0][5] = newbbox[5];
                  } else if (bsp->children[1] == (unsigned)(ptrdiff_t)(child-Level->Nodes)) {
                    bsp->bbox[1][2] = newbbox[2];
                    bsp->bbox[1][5] = newbbox[5];
                  }
                }
              }
              continue;
            }
          }
          */
          if (cp != &view_frustum.planes[TFrustum::Back]) {
            // this node is out of frustum, clip with it
            onlyClip = true;
            break;
          }
          return;
        }
#endif
      }
    }
  }

  // found a subsector?
  if ((bspnum&NF_SUBSECTOR) == 0) {
    // nope
    node_t *bsp = &Level->Nodes[bspnum];
    // decide which side the view point is on
    if (!onlyClip) {
      //int side = bsp->PointOnSide(vieworg);
      const float dist = DotProduct(vieworg, bsp->normal)-bsp->dist;
      unsigned side = (unsigned)(dist <= 0.0f);
      // if we are on a plane, do forward node first (this doesn't really matter, but why not?)
      if (dist == 0.0f) side = bsp->PointOnSide(vieworg+viewforward*2);
      if (bsp->children[side]&NF_SUBSECTOR) bsp->VisFrame = currVisFrame;
      // recursively divide front space (toward the viewer)
      RenderBSPNode(bsp->children[side], bsp->bbox[side], clipflags);
      // possibly divide back space (away from the viewer)
      side ^= 1;
      return RenderBSPNode(bsp->children[side], bsp->bbox[side], clipflags);
    } else {
      RenderBSPNode(bsp->children[0], bsp->bbox[0], clipflags, true);
      return RenderBSPNode(bsp->children[1], bsp->bbox[1], clipflags, true);
    }
  } else {
    if (onlyClip) {
      if (clip_use_1d_clipper) {
        subsector_t *sub = &Level->Subsectors[bspnum&(~NF_SUBSECTOR)];
        ViewClip.ClipAddSubsectorSegs(sub, (MirrorClipSegs && view_frustum.planes[5].isValid() ? &view_frustum.planes[5] : nullptr), true);
      }
    } else {
      return RenderSubsector(bspnum&(~NF_SUBSECTOR));
    }
  }
}


// ////////////////////////////////////////////////////////////////////////// //
// returns the closest point to `this` on the line segment from `a` to `b`
static TVec projectOnLine (const TVec &p, const TVec &a, const TVec b) {
  const TVec ab = b-a; // vector from a to b
  // squared distance from a to b
  const float absq = ab.dot(ab);
  if (fabs(absq) < 0.001f) return a; // a and b are the same point (roughly)
  const TVec ap = p-a; // vector from a to p
  const float t = ap.dot(ab)/absq;
  if (t < 0) return a; // "before" a on the line
  if (t > 1) return b; // "after" b on the line
  // projection lies "inbetween" a and b on the line
  return a+t*ab;
}


struct SubInfo {
  subsector_t *sub;
  float minDistSq; // minimum distance to view origin (used to sort subsectors)
  bool seenSeg;
  float bbox[6];

  SubInfo () {}
  SubInfo (ENoInit) {}
  SubInfo (const VLevel *Level, const TVec &origin, subsector_t *asub) {
    sub = asub;
    minDistSq = FLT_MAX;
    seenSeg = false;
    float bestMini = FLT_MAX;
    bool seenMini = false;
    float bestNonMini = FLT_MAX;
    bool seenNonMini = false;
    const seg_t *seg = &Level->Segs[asub->firstline];
    for (unsigned i = asub->numlines; i--; ++seg) {
      const line_t *line = seg->linedef;
      //if (seg->PointOnSide(origin)) continue; // cannot see
      if (!line) {
        // miniseg
        seenSeg = true;
        seenMini = true;
        const TVec proj = projectOnLine(origin, *seg->v1, *seg->v2);
        const float distSq = proj.length2DSquared();
        bestMini = MIN(distSq, bestMini);
      } else {
        // normal seg
        seenSeg = true;
        seenNonMini = true;
        /*
        const TVec v1 = *seg->v1-origin, v2 = seg->v2-origin;
        float distSq = v1.length2DSquared();
        minDistSq = MIN(distSq, minDistSq);
        float distSq = v2.length2DSquared();
        minDistSq = MIN(distSq, minDistSq);
        */
        /*
        const float distSq = fabsf(DotProduct(origin, seg->normal)-seg->dist);
        minDistSq = MIN(distSq, minDistSq);
        */
        const TVec proj = projectOnLine(origin, *seg->v1, *seg->v2);
        const float distSq = proj.length2DSquared();
        bestNonMini = MIN(distSq, bestNonMini);
      }
    }
    if (seenNonMini) {
      minDistSq = bestNonMini;
    } else if (seenMini) {
      minDistSq = bestMini;
    }
    minDistSq = MIN(bestMini, bestNonMini);
  }
};

extern "C" {
  static int subinfoCmp (const void *a, const void *b, void *udata) {
    if (a == b) return 0;
    const SubInfo *aa = (const SubInfo *)a;
    const SubInfo *bb = (const SubInfo *)b;
    if (aa->minDistSq < bb->minDistSq) return -1;
    if (aa->minDistSq > bb->minDistSq) return 1;
    return 0;
  }
}


//==========================================================================
//
//  VRenderLevelShared::RenderBspWorld
//
//==========================================================================
void VRenderLevelShared::RenderBspWorld (const refdef_t *rd, const VViewClipper *Range) {
  static const float dummy_bbox[6] = { -99999, -99999, -99999, 99999, 99999, 99999 };

  // if we hit a cache overflow, render everything again, to avoid partial frames
  do {
    if (light_reset_surface_cache) return;

    //view_frustum.setupBoxIndicies(); // done automatically
    ViewClip.ClearClipNodes(vieworg, Level);
    ViewClip.ClipInitFrustumRange(viewangles, viewforward, viewright, viewup, rd->fovx, rd->fovy);
    if (Range) ViewClip.ClipToRanges(*Range); // range contains a valid range, so we must clip away holes in it
    memset(BspVis, 0, VisSize);
    memset(BspVisThing, 0, VisSize);
    if (PortalLevel == 0) {
      if (WorldSurfs.NumAllocated() < 4096) WorldSurfs.Resize(4096);
    }
    MirrorClipSegs = (MirrorClip && !view_frustum.planes[5].normal.z);
    if (!clip_frustum_mirror) {
      MirrorClipSegs = false;
      view_frustum.planes[5].clipflag = 0;
    }

    if (r_flood_renderer && Level->NumSubsectors) {
      GCon->Log("============");
      // start from r_viewleaf
      static TArray<SubInfo> queue;
      static TArray<vuint8> addedSubs;
      if (addedSubs.length() != Level->NumSubsectors) addedSubs.setLength(Level->NumSubsectors);
      memset(addedSubs.ptr(), 0, Level->NumSubsectors);
      queue.reset();
      // start from this subsector
      queue.append(SubInfo(Level, vieworg, r_viewleaf));
      addedSubs[(unsigned)(ptrdiff_t)(r_viewleaf-Level->Subsectors)] = 1;
      // add other subsectors
      int queuenum = 0;
      while (queuenum < queue.length()) {
        subsector_t *currsub = queue[queuenum++].sub;
        if (currsub->VisFrame == currVisFrame) continue; // already processed
        currsub->VisFrame = currVisFrame; // mark as processed
        if (!currsub->sector->linecount) continue; // skip sectors containing original polyobjs
        //const unsigned csnum = (unsigned)(ptrdiff_t)(currsub-Level->Subsectors);
        // check clipper
        float bbox[6];
        Level->GetSubsectorBBox(currsub, bbox);
        if (!view_frustum.checkBox(bbox)) continue;
        // travel to other subsectors
        //GCon->Log("...");
        const seg_t *seg = &Level->Segs[currsub->firstline];
        for (unsigned i = currsub->numlines; i--; ++seg) {
          const line_t *line = seg->linedef;
          if (line) {
            // not a miniseg; check for two-sided
            if (!(line->flags&ML_TWOSIDED)) {
              // not a two-sided, nowhere to travel
              continue;
            }
          }
          if (!seg->partner) continue; // just in case
          if (seg->PointOnSide(vieworg)) continue; // cannot see
          // closed door/lift?
          if (seg->backsector && seg->backsector != seg->frontsector &&
              (line->flags&(ML_TWOSIDED|ML_3DMIDTEX)) == ML_TWOSIDED)
          {
            if (VViewClipper::IsSegAClosedSomething(&view_frustum, seg)) continue;
          }
          // it is visible, travel to partner subsector
          subsector_t *ps = seg->partner->front_sub;
          if (!ps || ps == currsub) continue; // just in case
          if (ps->VisFrame == currVisFrame) continue; // already processed
          const unsigned psnum = (unsigned)(ptrdiff_t)(ps-Level->Subsectors);
          if (addedSubs.ptr()[psnum]) continue; // already added
          addedSubs.ptr()[psnum] = 1;
          SubInfo si = SubInfo(Level, vieworg, ps);
          if (si.seenSeg) {
            //GCon->Log(" +++");
            memcpy(si.bbox, bbox, sizeof(float)*6);
            queue.append(si);
          }
        }
      }
      //GCon->Logf("found #%d subsectors", queue.length());

      // sort subsectors
      if (queue.length() > 1) {
        timsort_r(queue.ptr()+1, queue.length()-1, sizeof(SubInfo), &subinfoCmp, nullptr);
      }

      // render subsectors using clipper
      // no need to check frustum, it is already done
      {
        const SubInfo *si = queue.ptr();
        for (int sicount = queue.length(); sicount--; ++si) {
          //if (!ViewClip.ClipIsBBoxVisible(si->bbox)) continue;
          const subsector_t *currsub = si->sub;
          const int snum = (int)(ptrdiff_t)(currsub-Level->Subsectors);
          if (!ViewClip.ClipCheckSubsector(currsub)) {
            GCon->Logf("SKIP SUB #%d", snum);
            continue;
          }
          // render it, and add to clipper
          RenderSubsector((int)(ptrdiff_t)(currsub-Level->Subsectors), false);
          GCon->Logf("RENDER SUB #%d (before)", snum); ViewClip.Dump();
          ViewClip.ClipAddSubsectorSegs(currsub, (MirrorClipSegs && view_frustum.planes[5].isValid() ? &view_frustum.planes[5] : nullptr));
          GCon->Logf("RENDER SUB #%d (after)", snum); ViewClip.Dump();
        }
      }
      //ViewClip.Dump();
    } else {
      // head node is the last node output
      RenderBSPNode(Level->NumNodes-1, dummy_bbox, (MirrorClip ? 0x3f : 0x1f));
    }

    if (PortalLevel == 0) {
      // draw the most complex sky portal behind the scene first, without the need to use stencil buffer
      VPortal *BestSky = nullptr;
      int BestSkyIndex = -1;
      for (int i = 0; i < Portals.Num(); ++i) {
        if (Portals[i] && Portals[i]->IsSky() && (!BestSky || BestSky->Surfs.Num() < Portals[i]->Surfs.Num())) {
          BestSky = Portals[i];
          BestSkyIndex = i;
        }
      }
      if (BestSky) {
        PortalLevel = 1;
        BestSky->Draw(false);
        delete BestSky;
        BestSky = nullptr;
        Portals.RemoveIndex(BestSkyIndex);
        PortalLevel = 0;
      }

      //fprintf(stderr, "WSL=%d\n", WorldSurfs.Num());
      const int wslen = WorldSurfs.Num();
      for (int i = 0; i < wslen; ++i) {
        switch (WorldSurfs[i].Type) {
          case 0:
            QueueWorldSurface(WorldSurfs[i].Surf);
            break;
          case 1:
            QueueSkyPortal(WorldSurfs[i].Surf);
            break;
          case 2:
            QueueHorizonPortal(WorldSurfs[i].Surf);
            break;
        }
      }
      //WorldSurfs.Clear();
      WorldSurfs.resetNoDtor();
    }
  } while (light_reset_surface_cache);
}


//==========================================================================
//
//  VRenderLevelShared::RenderPortals
//
//==========================================================================
void VRenderLevelShared::RenderPortals () {
  if (PortalLevel == 0) {
    if (oldMaxMirrors != r_maxmirrors || oldPortalDepth != r_max_portal_depth ||
        oldHorizons != r_allow_horizons || oldMirrors != r_allow_mirrors)
    {
      //GCon->Logf("portal settings changed, resetting portal info...");
      for (int i = 0; i < Portals.Num(); ++i) {
        if (Portals[i]) {
          delete Portals[i];
          Portals[i] = nullptr;
        }
      }
      Portals.Clear();
      // save cvars
      oldMaxMirrors = r_maxmirrors;
      oldPortalDepth = r_max_portal_depth;
      oldHorizons = r_allow_horizons;
      oldMirrors = r_allow_mirrors;
      return;
    }
  }

  ++PortalLevel;

  if (r_max_portal_depth < 0 || PortalLevel <= r_max_portal_depth) {
    //FIXME: disable decals for portals
    //       i should rewrite decal rendering, so we can skip stencil buffer
    //       (or emulate stencil buffer with texture and shaders)
    bool oldDecalsEnabled = r_decals_enabled;
    r_decals_enabled = false;
    for (int i = 0; i < Portals.Num(); ++i) {
      if (Portals[i] && Portals[i]->Level == PortalLevel) Portals[i]->Draw(true);
    }
    r_decals_enabled = oldDecalsEnabled;
  } else {
    if (dbg_max_portal_depth_warning) GCon->Logf(NAME_Warning, "portal level too deep (%d)", PortalLevel);
  }

  for (int i = 0; i < Portals.Num(); ++i) {
    if (Portals[i] && Portals[i]->Level == PortalLevel) {
      delete Portals[i];
      Portals[i] = nullptr;
    }
  }

  --PortalLevel;
  if (PortalLevel == 0) Portals.Clear();
}
