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
//**  Copyright (C) 2018-2020 Ketmar Dark
//**
//**  This program is free software: you can redistribute it and/or modify
//**  it under the terms of the GNU General Public License as published by
//**  the Free Software Foundation, version 3 of the License ONLY.
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
#include "gl_local.h"
#include "gl_poly_adv_render.h"

#define VV_SMAP_PRECLEAR


//==========================================================================
//
//  VOpenGLDrawer::BeginLightShadowVolumes
//
//==========================================================================
void VOpenGLDrawer::BeginLightShadowMaps (const TVec &LightPos, const float Radius, const TVec &aconeDir, const float aconeAngle, int swidth, int sheight) {
  const bool flt = gl_shadowmap_filter.asBool();
  if (flt != cubemapLinearFiltering) {
    cubemapLinearFiltering = flt;
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexId);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (cubemapLinearFiltering ? GL_LINEAR : GL_NEAREST));
    GLDRW_CHECK_ERROR("set shadowmap mag filter");
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (cubemapLinearFiltering ? GL_LINEAR : GL_NEAREST));
    GLDRW_CHECK_ERROR("set shadowmap min filter");
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  GLDRW_RESET_ERROR();
  p_glBindFramebuffer(GL_FRAMEBUFFER, cubeFBO);
  GLDRW_CHECK_ERROR("set cube FBO");
  //p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, cubeDepthTexId, 0);
  //GLDRW_CHECK_ERROR("set framebuffer depth texture");
  glReadBuffer(GL_NONE);
  GLDRW_CHECK_ERROR("set cube FBO read buffer");

  ScrWdt = shadowmapSize;
  ScrHgt = shadowmapSize;

  smapLightPos = LightPos;
  smapLightRadius = Radius;
  smapLastTexinfo.initLastUsed();
  smapLastSprTexinfo.initLastUsed();

  // temp (it should be already disabled)
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_SCISSOR_TEST);
  GLDisableOffset();
  GLDisableBlend();

  //glDepthMask(GL_TRUE); // due to shadow volumes pass settings
  glEnableDepthWrite();
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.0f);
  if (p_glClipControl) p_glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE); // restore "normal" depth control
  glDepthRange(0.0f, 1.0f);
  glDepthFunc(GL_LESS);

  //glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // black background
  glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

  //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
  glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);

  glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  glEnable(GL_TEXTURE_2D);

  glGetIntegerv(GL_VIEWPORT, savedSMVPort);
  glViewport(0, 0, shadowmapSize, shadowmapSize);

  #ifdef VV_SMAP_PRECLEAR
  for (unsigned int fc = 0; fc < 6; ++fc) {
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, cubeDepthTexId[fc], 0);
    GLDRW_CHECK_ERROR("set framebuffer depth texture");
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X+fc, cubeTexId, 0);
    GLDRW_CHECK_ERROR("set cube FBO face");
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    GLDRW_CHECK_ERROR("set cube FBO draw buffer");
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    GLDRW_CHECK_ERROR("clear cube FBO");
  }
  #endif

  setupSpotLight(LightPos, Radius, aconeDir, aconeAngle);

  CalcShadowMapProjectionMatrix(smapProj, Radius, swidth, sheight, PixelAspect);

  VMatrix4 lview2;
  CalcModelMatrix(lview2, TVec(0, 0, 0), TAVec(0, 0, 0), false);
  TVec lpp = lview2*LightPos;

  //SurfShadowMap.Activate();
  SurfShadowMap.SetLightView(lview2);
  SurfShadowMap.SetLightPos(lpp);
  SurfShadowMap.SetLightRadius(Radius);

  //SurfShadowMapTex.Activate();
  SurfShadowMapTex.SetLightView(lview2);
  SurfShadowMapTex.SetLightPos(lpp);
  SurfShadowMapTex.SetLightRadius(Radius);
  SurfShadowMapTex.SetTexture(0);

  //SurfShadowMapSpr.Activate();
  SurfShadowMapSpr.SetLightView(lview2);
  SurfShadowMapSpr.SetLightPos(lpp);
  SurfShadowMapSpr.SetLightRadius(Radius);
  SurfShadowMapSpr.SetTexture(0);

  //glDisable(GL_CULL_FACE);
  GLDRW_CHECK_ERROR("finish cube FBO setup");
}


//==========================================================================
//
//  VOpenGLDrawer::EndLightShadowMaps
//
//==========================================================================
void VOpenGLDrawer::EndLightShadowMaps () {
  //currentActiveFBO = nullptr;
  //mainFBO.activate();
  //for (unsigned int fc = 0; fc < 6; ++fc)
  {
    p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
  }
  GLDRW_CHECK_ERROR("reset cube FBO");
  ReactivateCurrentFBO();
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  RestoreDepthFunc();
  if (p_glClipControl) p_glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); // actually, this is better even for "normal" cases
  glClearDepth(!useReverseZ ? 1.0f : 0.0f);
  glViewport(savedSMVPort[0], savedSMVPort[1], savedSMVPort[2], savedSMVPort[3]);
  glEnable(GL_CULL_FACE);
  //glDepthMask(GL_FALSE);
  glDisableDepthWrite();
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // black background
  glDisable(GL_TEXTURE_2D);
}


//==========================================================================
//
//  VOpenGLDrawer::SetupLightShadowMap
//
//==========================================================================
void VOpenGLDrawer::SetupLightShadowMap (unsigned int facenum) {
  p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, cubeDepthTexId[facenum], 0);
  GLDRW_CHECK_ERROR("set framebuffer depth texture");

  //!p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X+facenum, cubeTexId, 0);
  //!GLDRW_CHECK_ERROR("set cube FBO face");

  p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X+facenum, cubeTexId, 0);
  GLDRW_CHECK_ERROR("set cube FBO face");
  //glDrawBuffer(GL_COLOR_ATTACHMENT0);
  //GLDRW_CHECK_ERROR("set cube FBO draw buffer");

  #ifndef VV_SMAP_PRECLEAR
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  GLDRW_CHECK_ERROR("clear cube FBO");
  #endif

  /*
  const TAVec viewAngles[6] = {
    //    pitch    yaw   roll
    TAVec(  0.0f, -90.0f,   0.0f), // right
    TAVec(  0.0f,  90.0f,   0.0f), // left
    TAVec( 90.0f,   0.0f,   0.0f), // top
    TAVec(-90.0f,   0.0f,   0.0f), // bottom
    TAVec(  0.0f,   0.0f,   0.0f), // back
    TAVec(  0.0f, 180.0f,   0.0f), // front
  };
  */

  VMatrix4 lview;
  CalcModelMatrix(lview, smapLightPos, VDrawer::CubeMapViewAngles[facenum], false);
  VMatrix4 lmpv = smapProj*lview;
  SurfShadowMap.SetLightMPV(lmpv);
  SurfShadowMapTex.SetLightMPV(lmpv);
  SurfShadowMapSpr.SetLightMPV(lmpv);

  //SurfShadowMap.UploadChangedUniforms();
  //GLDRW_CHECK_ERROR("update cube FBO shader");
}


//==========================================================================
//
//  VOpenGLDrawer::RenderSurfaceShadowMap
//
//==========================================================================
void VOpenGLDrawer::RenderSurfaceShadowMap (const surface_t *surf) {
  if (gl_dbg_wireframe) return;
  if (surf->count < 3) return; // just in case

  if (spotLight && !isSurfaceInSpotlight(surf)) return;

  const unsigned vcount = (unsigned)surf->count;
  const SurfVertex *sverts = surf->verts;
  const SurfVertex *v = sverts;

  const texinfo_t *currTexinfo = surf->texinfo;
  #if 1
  if (currTexinfo->Tex->isTransparent()) {
    SurfShadowMapTex.Activate();
    const bool textureChanged = smapLastTexinfo.needChange(*currTexinfo, updateFrame);
    if (textureChanged) {
      smapLastTexinfo.updateLastUsed(*currTexinfo);
      //SetTexture(currTexinfo->Tex, currTexinfo->ColorMap);
      SetShadowTexture(currTexinfo->Tex);
      SurfShadowMapTex.SetTex(currTexinfo);
    }
  } else {
    SurfShadowMap.Activate();
  }
  #else
  SurfShadowMapTex.Activate();
  const bool textureChanged = smapLastTexinfo.needChange(*currTexinfo, updateFrame);
  if (textureChanged) {
    //SetTexture(currTexinfo->Tex, currTexinfo->ColorMap);
    SetShadowTexture(currTexinfo->Tex);
    SurfShadowMapTex.SetTex(currTexinfo);
  }
  #endif

  //if (gl_smart_reject_shadows && !AdvRenderCanSurfaceCastShadow(surf, LightPos, Radius)) return;

  currentActiveShader->UploadChangedUniforms();
  //currentActiveShader->UploadChangedAttrs();

  //glBegin(GL_POLYGON);
  glBegin(GL_TRIANGLE_FAN);
    for (unsigned i = 0; i < vcount; ++i, ++v) glVertex(v->vec());
  glEnd();
}


//==========================================================================
//
//  VOpenGLDrawer::DrawSpriteShadowMap
//
//==========================================================================
void VOpenGLDrawer::DrawSpriteShadowMap (const TVec *cv, VTexture *Tex, const TVec &sprnormal,
                                         const TVec &saxis, const TVec &taxis, const TVec &texorg)
{
  if (gl_dbg_wireframe) return;
  if (!Tex || Tex->Type == TEXTYPE_Null) return; // just in case

  if (spotLight && !isSpriteInSpotlight(cv)) return;

  if (Tex->isTransparent()) {
    // create fake texinfo
    texinfo_t currTexinfo;
    currTexinfo.saxis = saxis;
    currTexinfo.soffs = 0;
    currTexinfo.taxis = taxis;
    currTexinfo.toffs = 0;
    currTexinfo.saxisLM = currTexinfo.taxisLM = TVec(0, 0, 0);
    currTexinfo.Tex = Tex;
    currTexinfo.noDecals = 0;
    currTexinfo.Alpha = 1.1f;
    currTexinfo.Additive = 0;
    currTexinfo.ColorMap = 0;

    SurfShadowMapSpr.Activate();
    // activate shader, check for texture change
    const bool textureChanged = smapLastSprTexinfo.needChange(currTexinfo, updateFrame);
    if (true || textureChanged) {
      smapLastSprTexinfo.updateLastUsed(currTexinfo);
      SetShadowTexture(Tex); //FIXME: this should be "no-repeat"
    }
    SurfShadowMapSpr.SetSpriteTex(texorg, saxis, taxis, tex_iw, tex_ih);
  } else {
    SurfShadowMap.Activate();
  }

  currentActiveShader->UploadChangedUniforms();
  //currentActiveShader->UploadChangedAttrs();

  glBegin(GL_TRIANGLE_FAN);
    glVertex(cv[0]);
    glVertex(cv[1]);
    glVertex(cv[2]);
    glVertex(cv[3]);
  glEnd();
}