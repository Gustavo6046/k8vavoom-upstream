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
//**
//**  OpenGL driver, main module
//**
//**************************************************************************
#include <limits.h>
#include <float.h>
#include <stdarg.h>

#include "gl_local.h"
#include "../r_local.h" /* for VRenderLevelShared */


//==========================================================================
//
//  VOpenGLDrawer::CheckExtension
//
//==========================================================================
bool VOpenGLDrawer::CheckExtension (const char *ext) {
  if (!ext || !ext[0]) return false;
  TArray<VStr> Exts;
  VStr((char *)glGetString(GL_EXTENSIONS)).Split(' ', Exts);
  for (int i = 0; i < Exts.Num(); ++i) if (Exts[i] == ext) return true;
  return false;
}


//==========================================================================
//
//  VOpenGLDrawer::SupportsShadowVolumeRendering
//
//==========================================================================
bool VOpenGLDrawer::SupportsShadowVolumeRendering () {
  return (HaveStencilWrap && /*p_glStencilFuncSeparate*/p_glStencilOpSeparate && HaveDepthClamp);
}


//==========================================================================
//
//  VOpenGLDrawer::UseFrustumFarClip
//
//==========================================================================
bool VOpenGLDrawer::UseFrustumFarClip () {
  if (CanUseRevZ()) return false;
  if (RendLev && RendLev->IsShadowVolumeRenderer() && !HaveDepthClamp) return false;
  return true;
}


//==========================================================================
//
//  VOpenGLDrawer::GetProjectionMatrix
//
//==========================================================================
void VOpenGLDrawer::GetProjectionMatrix (VMatrix4 &mat) {
  glGetFloatv(GL_PROJECTION_MATRIX, mat[0]);
}


//==========================================================================
//
//  VOpenGLDrawer::GetModelMatrix
//
//==========================================================================
void VOpenGLDrawer::GetModelMatrix (VMatrix4 &mat) {
  glGetFloatv(GL_MODELVIEW_MATRIX, mat[0]);
}


//==========================================================================
//
//  VOpenGLDrawer::SetProjectionMatrix
//
//==========================================================================
void VOpenGLDrawer::SetProjectionMatrix (const VMatrix4 &mat) {
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(mat[0]);
  glMatrixMode(GL_MODELVIEW);
}


//==========================================================================
//
//  VOpenGLDrawer::SetModelMatrix
//
//==========================================================================
void VOpenGLDrawer::SetModelMatrix (const VMatrix4 &mat) {
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(mat[0]);
}


//==========================================================================
//
//  VOpenGLDrawer::LoadVPMatrices
//
//  call this before doing light scissor calculations (can be called once per scene)
//  sets `projMat` and `modelMat`
//  scissor setup will use those matrices (but won't modify them)
//
//==========================================================================
/*
void VOpenGLDrawer::LoadVPMatrices () {
  glGetFloatv(GL_PROJECTION_MATRIX, vpmats.projMat[0]);
  glGetFloatv(GL_MODELVIEW_MATRIX, vpmats.modelMat[0]);
  GLint vport[4];
  glGetIntegerv(GL_VIEWPORT, vport);
  vpmats.vport.setOrigin(vport[0], vport[1]);
  vpmats.vport.setSize(vport[2], vport[3]);
}
*/


//==========================================================================
//
//  VOpenGLDrawer::PushDepthMask
//
//==========================================================================
void VOpenGLDrawer::PushDepthMask () {
  if (depthMaskSP >= MaxDepthMaskStack) Sys_Error("OpenGL: depth mask stack overflow");
  glGetIntegerv(GL_DEPTH_WRITEMASK, &depthMaskStack[depthMaskSP]);
  ++depthMaskSP;
}


//==========================================================================
//
//  VOpenGLDrawer::PopDepthMask
//
//==========================================================================
void VOpenGLDrawer::PopDepthMask () {
  if (depthMaskSP == 0) Sys_Error("OpenGL: depth mask stack underflow");
  glDepthMask(depthMaskStack[--depthMaskSP]);
}


//==========================================================================
//
//  VOpenGLDrawer::RestoreDepthFunc
//
//==========================================================================
void VOpenGLDrawer::RestoreDepthFunc () {
  glDepthFunc(!CanUseRevZ() ? GL_LEQUAL : GL_GEQUAL);
}


//==========================================================================
//
//  VOpenGLDrawer::RestoreDepthFunc
//
//==========================================================================
void VOpenGLDrawer::SetupTextureFiltering (int level) {
  // for anisotropy, we require trilinear filtering
  if (anisotropyExists) {
    /*
    if (gl_texture_filter_anisotropic > 1) {
      // turn on trilinear filtering
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      // setup anisotropy level
      glTexParameterf(GL_TEXTURE_2D, GLenum(GL_TEXTURE_MAX_ANISOTROPY_EXT),
        (gl_texture_filter_anisotropic > max_anisotropy ? max_anisotropy : gl_texture_filter_anisotropic)
      );
      return;
    }
    // we have anisotropy, but it is turned off
    //glTexParameterf(GL_TEXTURE_2D, GLenum(GL_TEXTURE_MAX_ANISOTROPY_EXT), 1); // 1 is minimum, i.e. "off"
    */
    // but newer OpenGL versions allows anisotropy filtering even for "nearest" mode,
    // so setup it in any case
    glTexParameterf(GL_TEXTURE_2D, GLenum(GL_TEXTURE_MAX_ANISOTROPY_EXT),
      (gl_texture_filter_anisotropic > max_anisotropy ? max_anisotropy : gl_texture_filter_anisotropic)
    );
  }
  int mipfilter, maxfilter;
  // setup filtering
  switch (level) {
    case 1: // nearest mipmap
      maxfilter = GL_NEAREST;
      mipfilter = GL_NEAREST_MIPMAP_NEAREST;
      break;
    case 2: // linear nearest
      maxfilter = GL_LINEAR;
      mipfilter = GL_LINEAR_MIPMAP_NEAREST;
      break;
    case 3: // bilinear
      maxfilter = GL_LINEAR;
      mipfilter = GL_LINEAR;
      break;
    case 4: // trilinear
      maxfilter = GL_LINEAR;
      mipfilter = GL_LINEAR_MIPMAP_LINEAR;
      break;
    default: // nearest, no mipmaps
      maxfilter = GL_NEAREST;
      mipfilter = GL_NEAREST;
      break;
  }
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipfilter);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, maxfilter);
}


//==========================================================================
//
//  VOpenGLDrawer::SetupBlending
//
//==========================================================================
void VOpenGLDrawer::SetupBlending (const RenderStyleInfo &ri) {
  switch (ri.translucency) {
    case RenderStyleInfo::Translucent: // normal translucency
    case RenderStyleInfo::Shaded: // normal translucency
    case RenderStyleInfo::Fuzzy: // normal translucency
      //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      break;
    case RenderStyleInfo::Additive: // additive translucency
    case RenderStyleInfo::AddShaded: // additive translucency
      glBlendFunc(GL_ONE, GL_ONE); // our source rgb is already premultiplied
      break;
    case RenderStyleInfo::DarkTrans: // translucent-dark (k8vavoom special)
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      break;
    case RenderStyleInfo::Subtractive: // subtractive translucency
      //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glBlendFunc(GL_ONE, GL_ONE); // dunno, looks like it
      if (p_glBlendEquationSeparate) {
        //glBlendEquationSeparate(GL_FUNC_SUBTRACT, GL_FUNC_ADD);
        p_glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
      } else {
        // at least something
        p_glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
      }
      break;
    default: // normal
      /*
      if (trans) {
        //restoreBlend = true; // default blending
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      }
      */
      break;
  }
}


//==========================================================================
//
//  VOpenGLDrawer::RestoreBlending
//
//==========================================================================
void VOpenGLDrawer::RestoreBlending (const RenderStyleInfo &ri) {
  switch (ri.translucency) {
    case RenderStyleInfo::Additive: // additive translucency
    case RenderStyleInfo::AddShaded: // additive translucency
    case RenderStyleInfo::DarkTrans: // translucent-dark (k8vavoom special)
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      break;
    case RenderStyleInfo::Subtractive: // subtractive translucency
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      p_glBlendEquation(GL_FUNC_ADD);
      break;
    default:
      break;
  }
}


//==========================================================================
//
//  VOpenGLDrawer::DisableClipPlanes
//
//==========================================================================
void VOpenGLDrawer::DisableClipPlanes () {
  glDisable(GL_CLIP_PLANE0);
}


//==========================================================================
//
//  VOpenGLDrawer::SetupClipPlanes
//
//==========================================================================
void VOpenGLDrawer::SetupClipPlanes () {
  if (MirrorClip && viewfrustum.planes[TFrustum::Forward].isValid()) {
    glEnable(GL_CLIP_PLANE0);
    const GLdouble eq[4] = {
      viewfrustum.planes[TFrustum::Forward].normal.x,
      viewfrustum.planes[TFrustum::Forward].normal.y,
      viewfrustum.planes[TFrustum::Forward].normal.z,
      -viewfrustum.planes[TFrustum::Forward].dist,
    };
    glClipPlane(GL_CLIP_PLANE0, eq);
  }
  #if 0 /*k8: i am too dumb to figure this out*/
  else if (/*!MirrorClip && viewfrustum.planes[TFrustum::Forward].isValid()*/true) {
    //GCon->Logf(NAME_Debug, "mclip: (%g,%g,%g) : %g", viewfrustum.planes[TFrustum::Forward].normal.x, viewfrustum.planes[TFrustum::Forward].normal.y, viewfrustum.planes[TFrustum::Forward].normal.z, viewfrustum.planes[TFrustum::Forward].dist);

    // do not transform matrix
    glLoadIdentity();

    glEnable(GL_CLIP_PLANE0);
    const GLdouble eq[4] = {
      0.0f, //viewfrustum.planes[TFrustum::Forward].normal.x,
      0.0f, //viewfrustum.planes[TFrustum::Forward].normal.y,
      -1.0f, //viewfrustum.planes[TFrustum::Forward].normal.z,
      8.0f, //-viewfrustum.planes[TFrustum::Forward].dist,
    };
    glClipPlane(GL_CLIP_PLANE0, eq);

    glLoadMatrixf(vpmats.modelMat[0]);
  }
  #endif
  else {
    glDisable(GL_CLIP_PLANE0);
  }
}


//==========================================================================
//
//  VOpenGLDrawer::ReadScreen
//
//==========================================================================
void *VOpenGLDrawer::ReadScreen (int *bpp, bool *bot2top) {
  GLint oldbindtex = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldbindtex);

  glBindTexture(GL_TEXTURE_2D, GetMainFBO()->getColorTid());
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  void *dst = Z_Malloc(GetMainFBO()->getWidth()*GetMainFBO()->getHeight()*3);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, dst);
  glBindTexture(GL_TEXTURE_2D, oldbindtex);

  *bpp = 24;
  *bot2top = true;
  return dst;
}


//==========================================================================
//
//  VOpenGLDrawer::ReadBackScreen
//
//==========================================================================
void VOpenGLDrawer::ReadBackScreen (int Width, int Height, rgba_t *Dest) {
  ReadFBOPixels(GetMainFBO(), Width, Height, Dest);
}


//==========================================================================
//
//  VOpenGLDrawer::SetFade
//
//==========================================================================
void VOpenGLDrawer::SetFade (vuint32 NewFade) {
  if ((vuint32)CurrentFade == NewFade) return;
  if (NewFade) {
    //static GLenum fogMode[4] = { GL_LINEAR, GL_LINEAR, GL_EXP, GL_EXP2 };
    float fogColor[4];

    fogColor[0] = float((NewFade>>16)&255)/255.0f;
    fogColor[1] = float((NewFade>>8)&255)/255.0f;
    fogColor[2] = float(NewFade&255)/255.0f;
    fogColor[3] = float((NewFade>>24)&255)/255.0f;
    //glFogi(GL_FOG_MODE, fogMode[r_fog&3]);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fogColor);
    if (NewFade == FADE_LIGHT) {
      glFogf(GL_FOG_DENSITY, 0.3f);
      glFogf(GL_FOG_START, 1.0f);
      glFogf(GL_FOG_END, 1024.0f*r_fade_factor);
    } else {
      glFogf(GL_FOG_DENSITY, r_fog_density);
      glFogf(GL_FOG_START, r_fog_start);
      glFogf(GL_FOG_END, r_fog_end);
    }
    //glHint(GL_FOG_HINT, r_fog < 4 ? GL_DONT_CARE : GL_NICEST);
    //glHint(GL_FOG_HINT, GL_DONT_CARE);
    glHint(GL_FOG_HINT, GL_NICEST);
    glEnable(GL_FOG);
  } else {
    glDisable(GL_FOG);
  }
  CurrentFade = NewFade;
}


//==========================================================================
//
//  VOpenGLDrawer::DebugRenderScreenRect
//
//==========================================================================
void VOpenGLDrawer::DebugRenderScreenRect (int x0, int y0, int x1, int y1, vuint32 color) {
  glPushAttrib(/*GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_VIEWPORT_BIT|GL_TRANSFORM_BIT*/GL_ALL_ATTRIB_BITS);
  bool oldBlend = blendEnabled;

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  //glColor4f(((color>>16)&0xff)/255.0f, ((color>>8)&0xff)/255.0f, (color&0xff)/255.0f, ((color>>24)&0xff)/255.0f);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  GLEnableBlend();
  //glDisable(GL_STENCIL_TEST);
  //glDisable(GL_SCISSOR_TEST);
  glDisable(GL_TEXTURE_2D);
  glDepthMask(GL_FALSE); // no z-buffer writes
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  //p_glUseProgramObjectARB(0);

  DrawFixedCol.Activate();
  DrawFixedCol.SetColor(
    (GLfloat)(((color>>16)&255)/255.0f),
    (GLfloat)(((color>>8)&255)/255.0f),
    (GLfloat)((color&255)/255.0f), ((color>>24)&0xff)/255.0f);
  DrawFixedCol.UploadChangedUniforms();

  SetOrthoProjection(0, getWidth(), getHeight(), 0);
  glBegin(GL_QUADS);
    glVertex2i(x0, y0);
    glVertex2i(x1, y0);
    glVertex2i(x1, y1);
    glVertex2i(x0, y1);
  glEnd();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopAttrib();
  p_glUseProgramObjectARB(0);
  currentActiveShader = nullptr;
  blendEnabled = oldBlend;
}


//==========================================================================
//
//  VOpenGLDrawer::ForceClearStencilBuffer
//
//==========================================================================
void VOpenGLDrawer::ForceClearStencilBuffer () {
  NoteStencilBufferDirty();
  ClearStencilBuffer();
}


//==========================================================================
//
//  VOpenGLDrawer::ForceMarkStencilBufferDirty
//
//==========================================================================
void VOpenGLDrawer::ForceMarkStencilBufferDirty () {
  NoteStencilBufferDirty();
}


//==========================================================================
//
//  VOpenGLDrawer::EnableBlend
//
//==========================================================================
void VOpenGLDrawer::EnableBlend () {
  GLEnableBlend();
}


//==========================================================================
//
//  VOpenGLDrawer::DisableBlend
//
//==========================================================================
void VOpenGLDrawer::DisableBlend () {
  GLDisableBlend();
}


//==========================================================================
//
//  VOpenGLDrawer::GLSetBlendEnabled
//
//==========================================================================
void VOpenGLDrawer::SetBlendEnabled (const bool v) {
  GLSetBlendEnabled(v);
}


//==========================================================================
//
//  VOpenGLDrawer::GLEnableOffset
//
//==========================================================================
void VOpenGLDrawer::GLEnableOffset () {
  if (!offsetEnabled) {
    offsetEnabled = true;
    glEnable(GL_POLYGON_OFFSET_FILL);
    //glPolygonOffset(afactor, aunits); // just in case
  }
}


//==========================================================================
//
//  VOpenGLDrawer::GLDisableOffset
//
//==========================================================================
void VOpenGLDrawer::GLDisableOffset () {
  if (offsetEnabled) {
    offsetEnabled = false;
    glDisable(GL_POLYGON_OFFSET_FILL);
    //glPolygonOffset(0, 0); // just in case
  }
}


//==========================================================================
//
//  VOpenGLDrawer::GLPolygonOffset
//
//==========================================================================
void VOpenGLDrawer::GLPolygonOffset (const float afactor, const float aunits) {
  if (afactor != offsetFactor || aunits != offsetUnits || !offsetEnabled) {
    offsetFactor = afactor;
    offsetUnits = aunits;
    glPolygonOffset(afactor, aunits);
    if (!offsetEnabled) {
      offsetEnabled = true;
      glEnable(GL_POLYGON_OFFSET_FILL);
    }
  }
}


//==========================================================================
//
//  VOpenGLDrawer::ClearScreen
//
//==========================================================================
void VOpenGLDrawer::ClearScreen (unsigned clearFlags) {
  GLuint flags = 0;
  if (clearFlags&CLEAR_COLOR) flags |= GL_COLOR_BUFFER_BIT;
  if (clearFlags&CLEAR_DEPTH) flags |= GL_DEPTH_BUFFER_BIT;
  if (clearFlags&CLEAR_STENCIL) flags |= GL_STENCIL_BUFFER_BIT;
  if (flags) glClear(flags);
}
