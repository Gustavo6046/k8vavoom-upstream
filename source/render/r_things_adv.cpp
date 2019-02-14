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
//**  Refresh of things, i.e. objects represented by sprites.
//**
//**  Sprite rotation 0 is facing the viewer, rotation 1 is one angle turn
//**  CLOCKWISE around the axis. This is not the same as the angle, which
//**  increases counter clockwise (protractor). There was a lot of stuff
//**  grabbed wrong, so I changed it...
//**
//**************************************************************************
#include "gamedefs.h"
#include "r_local.h"
#include "sv_local.h"


enum {
  SPR_VP_PARALLEL_UPRIGHT,
  SPR_FACING_UPRIGHT,
  SPR_VP_PARALLEL,
  SPR_ORIENTED,
  SPR_VP_PARALLEL_ORIENTED,
  SPR_VP_PARALLEL_UPRIGHT_ORIENTED,
};


extern VCvarB r_chasecam;
extern VCvarB r_draw_mobjs;
extern VCvarB r_draw_psprites;
extern VCvarB r_models;
extern VCvarB r_view_models;
extern VCvarB r_model_shadows;
extern VCvarB r_model_light;
extern VCvarB r_sort_sprites;
extern VCvarB r_fix_sprite_offsets;
extern VCvarI r_sprite_fix_delta;
extern VCvarB r_drawfuzz;
extern VCvarF r_transsouls;
extern VCvarI crosshair;
extern VCvarF crosshair_alpha;
extern VCvarI r_max_model_lights;
extern VCvarI r_max_model_shadows;


//==========================================================================
//
//  VAdvancedRenderLevel::RenderThingAmbient
//
//==========================================================================
void VAdvancedRenderLevel::RenderThingAmbient (VEntity *mobj) {
  int RendStyle = mobj->RenderStyle;
  float Alpha = mobj->Alpha;
  bool Additive = false;

  if (RendStyle == STYLE_SoulTrans) {
    RendStyle = STYLE_Translucent;
    Alpha = r_transsouls;
  } else if (RendStyle == STYLE_OptFuzzy) {
    RendStyle = (r_drawfuzz ? STYLE_Fuzzy : STYLE_Translucent);
  }

  switch (RendStyle) {
    case STYLE_None:
      return;
    case STYLE_Normal:
      Alpha = 1.0f;
      break;
    case STYLE_Fuzzy:
      Alpha = FUZZY_ALPHA;
      break;
    case STYLE_Add:
      if (Alpha == 1.0f) Alpha -= 0.0002f;
      Additive = true;
      break;
  }
  Alpha = MID(0.0f, Alpha, 1.0f);
  if (!Alpha) return;

  // setup lighting
  vuint32 light;
  if (RendStyle == STYLE_Fuzzy) {
    light = 0;
  } else if ((mobj->State->Frame&VState::FF_FULLBRIGHT) ||
             (mobj->EntityFlags&(VEntity::EF_FullBright|VEntity::EF_Bright)))
  {
    light = 0xffffffff;
  } else {
    if (!r_model_light || !r_model_shadows) {
      // use old way of lighting
      light = LightPoint(mobj->Origin, mobj);
    } else {
      light = LightPointAmbient(mobj->Origin, mobj);
    }
  }

  float TimeFrac = 0;
  if (mobj->State->Time > 0) {
    TimeFrac = 1.0f-(mobj->StateTime/mobj->State->Time);
    TimeFrac = MID(0.0f, TimeFrac, 1.0f);
  }

  DrawEntityModel(mobj, light, 0, Alpha, Additive, TimeFrac, RPASS_Ambient);
}


//==========================================================================
//
//  VAdvancedRenderLevel::RenderMobjsAmbient
//
//==========================================================================
void VAdvancedRenderLevel::RenderMobjsAmbient () {
  if (!r_draw_mobjs || !r_models) return;
  for (TThinkerIterator<VEntity> Ent(Level); Ent; ++Ent) {
    if (*Ent == ViewEnt && (!r_chasecam || ViewEnt != cl->MO)) continue; // don't draw camera actor
    if (Ent->EntityFlags&(VEntity::EF_NoSector|VEntity::EF_Invisible)) continue;
    if (!Ent->State) continue;
    // skip things in subsectors that are not visible
    const int SubIdx = (int)(ptrdiff_t)(Ent->SubSector-Level->Subsectors);
    if (!(BspVis[SubIdx>>3]&(1<<(SubIdx&7)))) continue;
    RenderThingAmbient(*Ent);
  }
}


//==========================================================================
//
//  VAdvancedRenderLevel::RenderThingTextures
//
//==========================================================================
void VAdvancedRenderLevel::RenderThingTextures (VEntity *mobj) {
  int RendStyle = mobj->RenderStyle;
  float Alpha = mobj->Alpha;
  bool Additive = false;

  if (RendStyle == STYLE_SoulTrans) {
    RendStyle = STYLE_Translucent;
    Alpha = r_transsouls;
  } else if (RendStyle == STYLE_OptFuzzy) {
    RendStyle = (r_drawfuzz ? STYLE_Fuzzy : STYLE_Translucent);
  }

  switch (RendStyle) {
    case STYLE_None:
      return;
    case STYLE_Normal:
      Alpha = 1.0f;
      break;
    case STYLE_Translucent:
      Alpha = mobj->Alpha;
      break;
    case STYLE_Fuzzy:
      Alpha = FUZZY_ALPHA;
      break;
    case STYLE_Add:
      if (Alpha == 1.0f) Alpha -= 0.0002f;
      Additive = true;
      break;
  }
  Alpha = MID(0.0f, Alpha, 1.0f);
  if (!Alpha) return;

  float TimeFrac = 0;
  if (mobj->State->Time > 0) {
    TimeFrac = 1.0f-(mobj->StateTime/mobj->State->Time);
    TimeFrac = MID(0.0f, TimeFrac, 1.0f);
  }

  DrawEntityModel(mobj, 0xffffffff, 0, Alpha, Additive, TimeFrac, RPASS_Textures);
}


//==========================================================================
//
//  VAdvancedRenderLevel::RenderMobjsTextures
//
//==========================================================================
void VAdvancedRenderLevel::RenderMobjsTextures () {
  if (!r_draw_mobjs || !r_models) return;
  for (TThinkerIterator<VEntity> Ent(Level); Ent; ++Ent) {
    if (*Ent == ViewEnt && (!r_chasecam || ViewEnt != cl->MO)) continue; // don't draw camera actor
    if (Ent->EntityFlags&(VEntity::EF_NoSector|VEntity::EF_Invisible)) continue;
    if (!Ent->State) continue;
    // skip things in subsectors that are not visible
    const int SubIdx = (int)(ptrdiff_t)(Ent->SubSector-Level->Subsectors);
    if (!(BspVis[SubIdx>>3]&(1<<(SubIdx&7)))) continue;
    RenderThingTextures(*Ent);
  }
}


//==========================================================================
//
//  VAdvancedRenderLevel::IsTouchedByLight
//
//==========================================================================
bool VAdvancedRenderLevel::IsTouchedByLight (VEntity *Ent, bool Count) {
  const TVec Delta = Ent->Origin-CurrLightPos;
  const float Dist = Ent->Radius+CurrLightRadius;
  if (Delta.x > Dist || Delta.y > Dist) return false;
  if (Delta.z < -CurrLightRadius) return false;
  if (Delta.z > CurrLightRadius+Ent->Height) return false;
  //Delta.z = 0;
  if (Delta.Length2DSquared() > Dist*Dist) return false;
  if (Count) Ent->NumTouchingLights += 1;
  return true;
}


//==========================================================================
//
//  VAdvancedRenderLevel::RenderThingLight
//
//==========================================================================
void VAdvancedRenderLevel::RenderThingLight (VEntity *mobj) {
  // use advanced lighting style
  int RendStyle = mobj->RenderStyle;
  float Alpha = mobj->Alpha;
  bool Additive = false;

  if (RendStyle == STYLE_SoulTrans) {
    RendStyle = STYLE_Translucent;
    Alpha = r_transsouls;
  } else if (RendStyle == STYLE_OptFuzzy) {
    RendStyle = (r_drawfuzz ? STYLE_Fuzzy : STYLE_Translucent);
  }

  switch (RendStyle) {
    case STYLE_None:
      return;
    case STYLE_Normal:
      Alpha = 1.0f;
      break;
    case STYLE_Fuzzy:
      Alpha = FUZZY_ALPHA;
      break;
    case STYLE_Add:
      if (Alpha == 1.0f) Alpha -= 0.0002f;
      Additive = true;
      break;
  }
  Alpha = MID(0.0f, Alpha, 1.0f);
  if (!Alpha) return;

  if (Alpha < 1.0f && RendStyle == STYLE_Fuzzy) return;

  float TimeFrac = 0;
  if (mobj->State->Time > 0) {
    TimeFrac = 1.0f-(mobj->StateTime/mobj->State->Time);
    TimeFrac = MID(0.0f, TimeFrac, 1.0f);
  }

  DrawEntityModel(mobj, 0xffffffff, 0, Alpha, Additive, TimeFrac, RPASS_Light);
}


//==========================================================================
//
//  VAdvancedRenderLevel::ResetMobjsLightCount
//
//==========================================================================
void VAdvancedRenderLevel::ResetMobjsLightCount () {
  if (!r_draw_mobjs || !r_models) return;

  for (TThinkerIterator<VEntity> Ent(Level); Ent; ++Ent) {
    if (Ent->NumTouchingLights == 0) continue; // no need to do anything
    if (Ent->EntityFlags&(VEntity::EF_NoSector|VEntity::EF_Invisible)) continue;
    if (!Ent->State) continue;
    if (!IsTouchedByLight(*Ent, false)) continue;
    Ent->NumTouchingLights = 0;
  }
}


//==========================================================================
//
//  VAdvancedRenderLevel::RenderMobjsLight
//
//==========================================================================
void VAdvancedRenderLevel::RenderMobjsLight () {
  if (!r_draw_mobjs || !r_models || !r_model_light) return;

  for (TThinkerIterator<VEntity> Ent(Level); Ent; ++Ent) {
    if (Ent->NumTouchingLights > r_max_model_lights) continue; // limit maximum lights for this Entity
    if (*Ent == ViewEnt && (!r_chasecam || ViewEnt != cl->MO)) continue; // don't draw camera actor
    if (Ent->EntityFlags&(VEntity::EF_NoSector|VEntity::EF_Invisible)) continue;
    if (!Ent->State) continue;
    // skip things in subsectors that are not visible
    const int SubIdx = (int)(ptrdiff_t)(Ent->SubSector-Level->Subsectors);
    if (!(LightBspVis[SubIdx>>3]&(1<<(SubIdx&7)))) continue;
    if (!IsTouchedByLight(*Ent, true)) continue;
    RenderThingLight(*Ent);
  }
}


//==========================================================================
//
//  VAdvancedRenderLevel::RenderThingShadow
//
//==========================================================================
void VAdvancedRenderLevel::RenderThingShadow (VEntity *mobj) {
  int RendStyle = mobj->RenderStyle;
  float Alpha = mobj->Alpha;
  bool Additive = false;

  if (RendStyle == STYLE_SoulTrans) {
    RendStyle = STYLE_Translucent;
    Alpha = r_transsouls;
  } else if (RendStyle == STYLE_OptFuzzy) {
    RendStyle = (r_drawfuzz ? STYLE_Fuzzy : STYLE_Translucent);
  }

  switch (RendStyle) {
    case STYLE_None:
      return;
    case STYLE_Normal:
      Alpha = 1.0f;
      break;
    case STYLE_Fuzzy:
      Alpha = FUZZY_ALPHA;
      break;
    case STYLE_Add:
      if (Alpha == 1.0f) Alpha -= 0.0002f;
      Additive = true;
      break;
  }
  Alpha = MID(0.0f, Alpha, 1.0f);
  if (!Alpha) return;

  float TimeFrac = 0;
  if (mobj->State->Time > 0) {
    TimeFrac = 1.0f-(mobj->StateTime/mobj->State->Time);
    TimeFrac = MID(0.0f, TimeFrac, 1.0f);
  }

  DrawEntityModel(mobj, 0xffffffff, 0, Alpha, Additive, TimeFrac, RPASS_ShadowVolumes);
}


//==========================================================================
//
//  VAdvancedRenderLevel::RenderMobjsShadow
//
//==========================================================================
void VAdvancedRenderLevel::RenderMobjsShadow () {
  if (!r_draw_mobjs || !r_models || !r_model_shadows) return;

  for (TThinkerIterator<VEntity> Ent(Level); Ent; ++Ent) {
    if (Ent->NumTouchingLights > r_max_model_shadows) continue; // limit maximum shadows for this Entity
    if (Ent->EntityFlags&(VEntity::EF_NoSector|VEntity::EF_Invisible)) continue;
    if (!Ent->State) continue;
    // skip things in subsectors that are not visible
    const int SubIdx = (int)(ptrdiff_t)(Ent->SubSector-Level->Subsectors);
    if (!(LightVis[SubIdx>>3]&(1<<(SubIdx&7)))) continue;
    if (!IsTouchedByLight(*Ent, true)) continue;
    RenderThingShadow(*Ent);
  }
}


//==========================================================================
//
//  VAdvancedRenderLevel::RenderThingFog
//
//==========================================================================
void VAdvancedRenderLevel::RenderThingFog (VEntity *mobj) {
  int RendStyle = mobj->RenderStyle;
  float Alpha = mobj->Alpha;
  bool Additive = false;

  if (RendStyle == STYLE_SoulTrans) {
    RendStyle = STYLE_Translucent;
    Alpha = r_transsouls;
  } else if (RendStyle == STYLE_OptFuzzy) {
    RendStyle = (r_drawfuzz ? STYLE_Fuzzy : STYLE_Translucent);
  }

  switch (RendStyle) {
    case STYLE_None:
      return;
    case STYLE_Normal:
      Alpha = 1.0f;
      break;
    case STYLE_Fuzzy:
      Alpha = FUZZY_ALPHA;
      break;
    case STYLE_Add:
      if (Alpha == 1.0f) Alpha -= 0.0002f;
      Additive = true;
      break;
  }
  Alpha = MID(0.0f, Alpha, 1.0f);
  if (!Alpha) return;

  vuint32 Fade = GetFade(SV_PointInRegion(mobj->Sector, mobj->Origin));
  if (!Fade) return;

  float TimeFrac = 0;
  if (mobj->State->Time > 0) {
    TimeFrac = 1.0f-(mobj->StateTime/mobj->State->Time);
    TimeFrac = MID(0.0f, TimeFrac, 1.0f);
  }

  DrawEntityModel(mobj, 0xffffffff, Fade, Alpha, Additive, TimeFrac, RPASS_Fog);
}


//==========================================================================
//
//  VAdvancedRenderLevel::RenderMobjsFog
//
//==========================================================================
void VAdvancedRenderLevel::RenderMobjsFog () {
  if (!r_draw_mobjs || !r_models) return;

  for (TThinkerIterator<VEntity> Ent(Level); Ent; ++Ent) {
    if (*Ent == ViewEnt && (!r_chasecam || ViewEnt != cl->MO)) continue; // don't draw camera actor
    if (Ent->EntityFlags&(VEntity::EF_NoSector|VEntity::EF_Invisible)) continue;
    if (!Ent->State) continue;
    // skip things in subsectors that are not visible
    const int SubIdx = (int)(ptrdiff_t)(Ent->SubSector-Level->Subsectors);
    if (!(BspVis[SubIdx>>3]&(1<<(SubIdx&7)))) continue;
    RenderThingFog(*Ent);
  }
}
