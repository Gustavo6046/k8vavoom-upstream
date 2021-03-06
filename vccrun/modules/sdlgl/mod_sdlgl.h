//**************************************************************************
//**
//**  ##   ##    ##    ##   ##   ####     ####   ###     ###
//**  ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**   ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**   ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**    ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**     #    ##    ##    #      ####     ####   ##       ##
//**
//**  $Id$
//**
//**  Copyright (C) 1999-2006 Jānis Legzdiņš
//**  Copyright (C) 2018 Ketmar Dark
//**
//**  This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**  This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************
#ifndef VCCMOD_SDLGL_HEADER_FILE
#define VCCMOD_SDLGL_HEADER_FILE

#include "../../../libs/imago/imago.h"
#include "../../vcc_run.h"

#if defined (VCCRUN_HAS_SDL) && defined(VCCRUN_HAS_OPENGL)
#ifdef VAVOOM_CUSTOM_SPECIAL_SDL
# include <SDL.h>
#else
# include <SDL2/SDL.h>
#endif
#ifdef USE_GLAD
# include "glad.h"
#else
# include <GL/gl.h>
# include <GL/glext.h>
#endif


// ////////////////////////////////////////////////////////////////////////// //
struct event_t;
class VFont;
class VOpenGLTexture;


// ////////////////////////////////////////////////////////////////////////// //
class VGLVideo : public VObject {
  DECLARE_ABSTRACT_CLASS(VGLVideo, VObject, 0)
  NO_DEFAULT_CONSTRUCTOR(VGLVideo)

public:
  // (srcColor * <srcFactor>) <op> (dstColor * <dstFactor>)
  // If you want alpha blending use <srcFactor> of SRC_ALPHA and <dstFactor> of ONE_MINUS_SRC_ALPHA:
  //   (srcColor * srcAlpha) + (dstColor * (1-srcAlpha))
  enum {
    BlendNone, // disabled
    BlendNormal, // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    BlendBlend, // glBlendFunc(GL_SRC_ALPHA, GL_ONE)
    BlendFilter, // glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR)
    BlendInvert, // glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO)
    BlendParticle, // glBlendFunc(GL_DST_COLOR, GL_ZERO)
    BlendHighlight, // glBlendFunc(GL_DST_COLOR, GL_ONE);
    BlendDstMulDstAlpha, // glBlendFunc(GL_ZERO, GL_DST_ALPHA);
    InvModulate, // glBlendFunc(GL_ZERO, GL_SRC_COLOR)
    Premult, // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //
    BlendMax,
  };

  enum {
    BlendFunc_Add,
    BlendFunc_Sub,
    BlendFunc_SubRev,
    BlendFunc_Min,
    BlendFunc_Max,
  };

  enum {
    ZFunc_Never,
    ZFunc_Always,
    ZFunc_Equal,
    ZFunc_NotEqual,
    ZFunc_Less, // default
    ZFunc_LessEqual,
    ZFunc_Greater,
    ZFunc_GreaterEqual,
    ZFunc_Max,
  };

  enum {
    STC_Keep,
    STC_Zero,
    STC_Replace,
    STC_Incr,
    STC_IncrWrap,
    STC_Decr,
    STC_DecrWrap,
    STC_Invert,
  };

  enum {
    STC_Never,
    STC_Less,
    STC_LEqual,
    STC_Greater,
    STC_GEqual,
    STC_Equal,
    STC_NotEqual,
    STC_Always,
  };

  enum {
    CMask_Red   = 0x01,
    CMask_Green = 0x02,
    CMask_Blue  = 0x04,
    CMask_Alpha = 0x08,
  };

private:
  static bool mInited;
  static bool mPreInitWasDone;
  static int stencilBits;
  static int mWidth, mHeight;
  static bool doGLSwap;
  static bool doRefresh;
  static bool quitSignal;
  static bool hasNPOT; // NPOT texture support
  static VMethod *onDrawVC;
  static VMethod *onEventVC;
  static VMethod *onNewFrameVC;

  static int currFrameTime;
  static int prevFrameTime;
  static int mBlendMode;
  static int mBlendFunc;

  static vuint32 colorARGB; // a==0: opaque
  static VFont *currFont;
  static bool smoothLine;
  static bool directMode;
  static bool depthTest;
  static bool stencilEnabled;
  static int depthFunc;
  static int currZ;
  static int swapInterval;
  static bool texFiltering;
  static int colorMask;
  static int alphaTestFunc;
  static float alphaFuncVal;
  static bool in3dmode;
  friend class VOpenGLTexture;

public:
  static float currZFloat;

  static inline void forceGLTexFilter () {
    if (mInited) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (texFiltering ? GL_LINEAR : GL_NEAREST));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (texFiltering ? GL_LINEAR : GL_NEAREST));
    }
  }

private:
  static inline void forceColorMask () {
    if (mInited) {
      glColorMask(
        (colorMask&CMask_Red ? GL_TRUE : GL_FALSE),
        (colorMask&CMask_Green ? GL_TRUE : GL_FALSE),
        (colorMask&CMask_Blue ? GL_TRUE : GL_FALSE),
        (colorMask&CMask_Alpha ? GL_TRUE : GL_FALSE));
    }
  }

  static void forceAlphaFunc ();
  static void forceBlendFunc ();

  static inline bool getTexFiltering () {
    return texFiltering;
  }

  static inline void setTexFiltering (bool filterit) {
    //if (texFiltering == filterit) return;
    texFiltering = filterit;
    /*
    if (mInited) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (filterit ? GL_LINEAR : GL_NEAREST));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (filterit ? GL_LINEAR : GL_NEAREST));
    }
    */
  }

  static inline void realizeZFunc () {
    if (mInited) {
      switch (depthFunc) {
        case ZFunc_Never: glDepthFunc(GL_NEVER); break;
        case ZFunc_Always: glDepthFunc(GL_ALWAYS); break;
        case ZFunc_Equal: glDepthFunc(GL_EQUAL); break;
        case ZFunc_NotEqual: glDepthFunc(GL_NOTEQUAL); break;
        case ZFunc_Less: glDepthFunc(GL_LESS); break;
        case ZFunc_LessEqual: glDepthFunc(GL_LEQUAL); break;
        case ZFunc_Greater: glDepthFunc(GL_GREATER); break;
        case ZFunc_GreaterEqual: glDepthFunc(GL_GEQUAL); break;
      }
    }
  }

  static inline void realiseGLColor () {
    if (mInited) {
      glColor4f(
        ((colorARGB>>16)&0xff)/255.0f,
        ((colorARGB>>8)&0xff)/255.0f,
        (colorARGB&0xff)/255.0f,
        1.0f-(((colorARGB>>24)&0xff)/255.0f)
      );
      setupBlending();
    }
  }

  // returns `true` if drawing will have any effect
  static inline bool setupBlending () {
    if (mBlendMode == BlendNone) {
      glDisable(GL_BLEND);
      return true;
    } else if (mBlendMode == BlendNormal) {
      if ((colorARGB&0xff000000u) == 0) {
        // opaque
        glDisable(GL_BLEND);
        return true;
      } else {
        // either alpha, or completely transparent
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        return ((colorARGB&0xff000000u) != 0xff000000u);
      }
    } else {
      glEnable(GL_BLEND);
           if (mBlendMode == BlendBlend) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      else if (mBlendMode == BlendFilter) glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
      else if (mBlendMode == BlendInvert) glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
      else if (mBlendMode == BlendParticle) glBlendFunc(GL_DST_COLOR, GL_ZERO);
      else if (mBlendMode == BlendHighlight) glBlendFunc(GL_DST_COLOR, GL_ONE);
      else if (mBlendMode == BlendDstMulDstAlpha) glBlendFunc(GL_ZERO, GL_DST_ALPHA);
      else if (mBlendMode == InvModulate) glBlendFunc(GL_ZERO, GL_SRC_COLOR);
      else if (mBlendMode == Premult) glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      return ((colorARGB&0xff000000u) != 0xff000000u);
    }
  }

private:
  static void initMethods ();
  static void onNewFrame ();
  static void onDraw ();
  static void onEvent (event_t &evt);

  // returns `true`, if there is SDL event
  static bool doFrameBusiness (SDL_Event &ev);

public:
  static bool canInit ();
  static bool hasOpenGL ();
  static bool isInitialized ();
  static int getWidth ();
  static int getHeight ();

  static void fuckfucksdl ();

  static void getMousePosition (int *mx, int *my);

  static bool open (const VStr &winname, int width, int height, int fullscreen);
  static void close ();

  static void clear (int rgb=0);

  static void dispatchEvents ();
  static void runEventLoop ();

  static void setFont (VName fontname);

  static inline void setColor (vuint32 clr) { if (colorARGB != clr) { colorARGB = clr; realiseGLColor(); } }
  static inline vuint32 getColor () { return colorARGB; }

  static inline int getBlendMode () { return mBlendMode; }
  static inline void setBlendMode (int v) { if (v >= BlendNormal && v <= BlendMax && v != mBlendMode) { mBlendMode = v; setupBlending(); } }

  static inline bool isFullyOpaque () { return ((colorARGB&0xff000000) == 0); }
  static inline bool isFullyTransparent () { return ((colorARGB&0xff000000) == 0xff000000); }

  static void drawTextAt (int x, int y, const VStr &text);
  // if clr high byte is ignored; 0 means "default color"
  static void drawTextAtTexture (VOpenGLTexture *tx, int x, int y, const VStr &text, const vuint32 clr=0);

  // returns timer id or 0
  // if id <= 0, creates new unique timer id
  // if interval is < 1, returns with error and won't create timer
  static int CreateTimerWithId (int id, int intervalms, bool oneshot=false);
  static bool DeleteTimer (int id); // `true`: deleted, `false`: no such timer
  static bool IsTimerExists (int id);
  static bool IsTimerOneShot (int id);
  static int GetTimerInterval (int id); // 0: no such timer
  // returns success flag; won't do anything if interval is < 1
  static bool SetTimerInterval (int id, int intervalms);

  static void sendPing ();
  static void postSocketEvent (int code, int sockid, int data, bool wantAck);

  static int getFrameTime ();
  static void setFrameTime (int newft);

  // static
  //DECLARE_FUNCTION(performPreInit)
  DECLARE_FUNCTION(canInit)
  DECLARE_FUNCTION(hasOpenGL)
  DECLARE_FUNCTION(isInitialized)
  DECLARE_FUNCTION(screenWidth)
  DECLARE_FUNCTION(screenHeight)

  DECLARE_FUNCTION(getRealWindowSize)

  DECLARE_FUNCTION(isMouseCursorVisible)
  DECLARE_FUNCTION(hideMouseCursor)
  DECLARE_FUNCTION(showMouseCursor)

  DECLARE_FUNCTION(get_frameTime)
  DECLARE_FUNCTION(set_frameTime)

  DECLARE_FUNCTION(openScreen)
  DECLARE_FUNCTION(closeScreen)

  DECLARE_FUNCTION(setScale)

  DECLARE_FUNCTION(runEventLoop)

  DECLARE_FUNCTION(requestRefresh)
  DECLARE_FUNCTION(requestQuit)
  DECLARE_FUNCTION(resetQuitRequest)

  DECLARE_FUNCTION(forceSwap)

  DECLARE_FUNCTION(glHasExtension)

  DECLARE_FUNCTION(get_glHasNPOT)

  DECLARE_FUNCTION(get_directMode)
  DECLARE_FUNCTION(set_directMode)

  DECLARE_FUNCTION(get_depthTest)
  DECLARE_FUNCTION(set_depthTest)

  DECLARE_FUNCTION(get_depthFunc)
  DECLARE_FUNCTION(set_depthFunc)

  DECLARE_FUNCTION(get_currZ)
  DECLARE_FUNCTION(set_currZ)

  DECLARE_FUNCTION(get_scissorEnabled)
  DECLARE_FUNCTION(set_scissorEnabled)
  DECLARE_FUNCTION(getScissor)
  DECLARE_FUNCTION(setScissor)
  DECLARE_FUNCTION(copyScissor)

  enum {
    GLMatrixMode_ModelView,
    GLMatrixMode_Projection,
  };
  DECLARE_FUNCTION(set_glMatrixMode)

  DECLARE_FUNCTION(glPushMatrix)
  DECLARE_FUNCTION(glPopMatrix)
  DECLARE_FUNCTION(glLoadIdentity)
  DECLARE_FUNCTION(glScale)
  DECLARE_FUNCTION(glTranslate)
  DECLARE_FUNCTION(glRotate)

  DECLARE_FUNCTION(glSetup2D)
  DECLARE_FUNCTION(glSetup3D)

  enum {
    GLFaceCull_None,
    GLFaceCull_Front,
    GLFaceCull_Back,
  };
  DECLARE_FUNCTION(set_glCullFace)

  DECLARE_FUNCTION(glSetTexture)
  DECLARE_FUNCTION(glSetColor)

  enum {
    GLBegin_Points,
    GLBegin_Lines,
    GLBegin_LineStrip,
    GLBegin_LineLoop,
    GLBegin_Triangles,
    GLBegin_TriangleStrip,
    GLBegin_TriangleFan,
    GLBegin_Quads,
    GLBegin_QuadStrip,
    GLBegin_Polygon,
  };

  DECLARE_FUNCTION(glBegin)
  DECLARE_FUNCTION(glEnd)
  DECLARE_FUNCTION(glVertex)

  enum {
    FM_None,
    FM_Linear,
    FM_Exp,
    FM_Exp2,
  };

  DECLARE_FUNCTION(set_glFogMode)
  DECLARE_FUNCTION(set_glFogDensity)
  DECLARE_FUNCTION(set_glFogStart)
  DECLARE_FUNCTION(set_glFogEnd)
  DECLARE_FUNCTION(glSetFogColor)

  DECLARE_FUNCTION(clearScreen)

  DECLARE_FUNCTION(get_stencil)
  DECLARE_FUNCTION(set_stencil)
  DECLARE_FUNCTION(stencilOp)
  DECLARE_FUNCTION(stencilFunc)

  DECLARE_FUNCTION(get_alphaTestFunc)
  DECLARE_FUNCTION(set_alphaTestFunc)
  DECLARE_FUNCTION(get_alphaTestVal)
  DECLARE_FUNCTION(set_alphaTestVal)

  DECLARE_FUNCTION(get_realStencilBits)
  DECLARE_FUNCTION(get_framebufferHasAlpha)

  DECLARE_FUNCTION(get_smoothLine)
  DECLARE_FUNCTION(set_smoothLine)

  DECLARE_FUNCTION(get_colorARGB) // aarrggbb
  DECLARE_FUNCTION(set_colorARGB) // aarrggbb

  DECLARE_FUNCTION(get_blendMode)
  DECLARE_FUNCTION(set_blendMode)

  DECLARE_FUNCTION(get_blendFunc)
  DECLARE_FUNCTION(set_blendFunc)

  DECLARE_FUNCTION(get_colorMask)
  DECLARE_FUNCTION(set_colorMask)

  DECLARE_FUNCTION(get_textureFiltering)
  DECLARE_FUNCTION(set_textureFiltering)

  DECLARE_FUNCTION(get_swapInterval)
  DECLARE_FUNCTION(set_swapInterval)

  DECLARE_FUNCTION(get_fontName)
  DECLARE_FUNCTION(set_fontName)

  DECLARE_FUNCTION(loadFontDF)
  DECLARE_FUNCTION(loadFontPCF)

  DECLARE_FUNCTION(fontHeight)
  DECLARE_FUNCTION(getCharInfo)
  DECLARE_FUNCTION(charWidth)
  DECLARE_FUNCTION(spaceWidth)
  DECLARE_FUNCTION(textWidth)
  DECLARE_FUNCTION(textHeight)
  DECLARE_FUNCTION(drawTextAt)
  DECLARE_FUNCTION(drawTextAtTexture)

  DECLARE_FUNCTION(drawLine)
  DECLARE_FUNCTION(drawRect)
  DECLARE_FUNCTION(fillRect)

  DECLARE_FUNCTION(getMousePos)
};


// ////////////////////////////////////////////////////////////////////////// //
// refcounted object
class VOpenGLTexture {
private:
  int rc;
  VStr mPath;

public:
  struct TexQuad {
    int x0, y0, x1, y1;
    float tx0, ty0, tx1, ty1;
  };

  const VStr &getPath () const { return mPath; }
  int getRC () const { return rc; }

public:
  VImage *img;
  GLuint tid; // !0: texture loaded
  bool mTransparent; // fully
  bool mOpaque; // fully
  bool mOneBitAlpha; // this means that the only alpha values are 255 or 0
  VOpenGLTexture *prev;
  VOpenGLTexture *next;

  bool getTransparent () const { return mTransparent; }
  bool getOpaque () const { return mOpaque; }
  bool get1BitAlpha () const { return mOneBitAlpha; }

private:
  void registerMe ();
  void analyzeImage ();

  VOpenGLTexture (int awdt, int ahgt); // dimensions must be valid!

public:
  VOpenGLTexture (VImage *aimg, const VStr &apath);
  ~VOpenGLTexture (); // don't call this manually!

  void addRef ();
  void release (); //WARNING: can delete `this`!

  void update ();

  static VOpenGLTexture *Load (const VStr &fname);
  static VOpenGLTexture *CreateEmpty (VName txname, int wdt, int hgt);

  inline int getWidth () const { return (img ? img->width : 0); }
  inline int getHeight () const { return (img ? img->height : 0); }

  PropertyRO<const VStr &, VOpenGLTexture> path {this, &VOpenGLTexture::getPath};
  PropertyRO<int, VOpenGLTexture> width {this, &VOpenGLTexture::getWidth};
  PropertyRO<int, VOpenGLTexture> height {this, &VOpenGLTexture::getHeight};

  // that is:
  //  isTransparent==true: no need to draw anything
  //  isOpaque==true: no need to do alpha-blending
  //  isOneBitAlpha==true: no need to do advanced alpha-blending

  PropertyRO<bool, VOpenGLTexture> isTransparent {this, &VOpenGLTexture::getTransparent};
  PropertyRO<bool, VOpenGLTexture> isOpaque {this, &VOpenGLTexture::getOpaque};
  // this means that the only alpha values are 255 or 0
  PropertyRO<bool, VOpenGLTexture> isOneBitAlpha {this, &VOpenGLTexture::get1BitAlpha};

  // angle is in degrees
  void blitExt (int dx0, int dy0, int dx1, int dy1, int x0, int y0, int x1, int y1, float angle) const;
  void blitAt (int dx0, int dy0, float scale=1, float angle=0) const;

  // this uses integer texture coords
  void blitExtRep (int dx0, int dy0, int dx1, int dy1, int x0, int y0, int x1, int y1) const;

  void blitWithLightmap (TexQuad *t0, VOpenGLTexture *lmap, TexQuad *t1) const;
};


// ////////////////////////////////////////////////////////////////////////// //
// VaVoom C wrapper
class VGLTexture : public VObject {
  DECLARE_CLASS(VGLTexture, VObject, 0)
  NO_DEFAULT_CONSTRUCTOR(VGLTexture)

  friend class VGLVideo;

public:
  static bool savingAllowed;

private:
  VOpenGLTexture *tex;
  int id;

public:
  virtual void Destroy () override;

public:
  DECLARE_FUNCTION(Destroy)
  DECLARE_FUNCTION(Load) // native final static GLTexture Load (string fname);
  DECLARE_FUNCTION(GetById); // native final static GLTexture GetById (int id);
  DECLARE_FUNCTION(width)
  DECLARE_FUNCTION(height)
  DECLARE_FUNCTION(isTransparent)
  DECLARE_FUNCTION(isOpaque)
  DECLARE_FUNCTION(isOneBitAlpha)
  DECLARE_FUNCTION(blitExt)
  DECLARE_FUNCTION(blitExtRep)
  DECLARE_FUNCTION(blitAt)

  DECLARE_FUNCTION(blitWithLightmap)

  DECLARE_FUNCTION(CreateEmpty) // native final static GLTexture CreateEmpty (int wdt, int hgt, optional name txname);
  DECLARE_FUNCTION(setPixel) // native final static void setPixel (int x, int y, int argb); // aarrggbb; a==0 is completely opaque
  DECLARE_FUNCTION(getPixel) // native final static int getPixel (int x, int y); // aarrggbb; a==0 is completely opaque
  DECLARE_FUNCTION(upload) // native final static void upload ();
  DECLARE_FUNCTION(smoothEdges) // native final void smoothEdges (); // call after manual texture building

  DECLARE_FUNCTION(saveAsPNG)

  friend class VFont;
};


// ////////////////////////////////////////////////////////////////////////// //
// base class for fonts
class VFont {
protected:
  static VFont *fontList;

public:
  struct FontChar {
    vint32 ch;
    vint32 width, height; // height may differ from font height
    //vint32 advance; // horizontal advance to print next char
    vint32 topofs; // offset from font top (i.e. y+topofs should be used to draw char)
    vint32 leftbear, rightbear; // positive means "more to the respective bbox side"
    vint32 ascent, descent; // both are positive, which actually means "offset from baseline to the respective direction"
    float tx0, ty0; // texture coordinates, [0..1)
    float tx1, ty1; // texture coordinates, [0..1) -- cached for convenience
    VOpenGLTexture *tex; // don't destroy this!

    FontChar () : ch(-1), width(0), height(0), tex(nullptr) {}
  };

public:
  VName name;
  VFont *next;
  VOpenGLTexture *tex;
  bool singleTexture;

  // font characters (cp1251)
  TArray<FontChar> chars;
  vint32 spaceWidth; // width of the space character
  vint32 fontHeight; // height of the font
  vint32 minWidth, maxWidth, avgWidth;

protected:
  VFont (); // this inits nothing, and intended to be used in `LoadXXX()`

public:
  //VFont (VName aname, const VStr &fnameIni, const VStr &fnameTexture);
  ~VFont ();

  static VFont *LoadDF (VName aname, const VStr &fnameIni, const VStr &fnameTexture);
  static VFont *LoadPCF (VName aname, const VStr &filename);

  const FontChar *getChar (vint32 ch) const;
  vint32 charWidth (vint32 ch) const;
  vint32 textWidth (const VStr &s) const;
  vint32 textHeight (const VStr &s) const;
  // will clear lines; returns maximum text width
  //int splitTextWidth (const VStr &text, TArray<VSplitLine> &lines, int maxWidth) const;

  inline VName getName () const { return name; }
  inline vint32 getSpaceWidth () const { return spaceWidth; }
  inline vint32 getHeight () const { return fontHeight; }
  inline vint32 getMinWidth () const { return minWidth; }
  inline vint32 getMaxWidth () const { return maxWidth; }
  inline vint32 getAvgWidth () const { return avgWidth; }
  inline const VOpenGLTexture *getTexture () const { return tex; }

public:
  static VFont *findFont (VName name);
};


#endif

#endif
