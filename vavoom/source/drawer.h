//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
//**
//**	$Id$
//**
//**	Copyright (C) 1999-2006 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

struct surface_t;
struct surfcache_t;
struct VModel;
struct mmdl_t;
class VPortal;

struct particle_t
{
	//	Drawing info
	TVec		org;	//	position
	vuint32		colour;	//	ARGB colour
	float		Size;
	//	Handled by refresh
	particle_t*	next;	//	next in the list
	TVec		vel;	//	velocity
	TVec		accel;	//	acceleration
	float		die;	//	cl.time when particle will be removed
	int			type;
	float		ramp;
	float		gravity;
};

struct refdef_t
{
	int			x;
	int			y;
	int			width;
	int			height;
	float		fovx;
	float		fovy;
	bool		drawworld;
	bool		DrawCamera;
};

class VRenderLevelDrawer : public VRenderLevelPublic
{
public:
	virtual bool BuildLightMap(surface_t*, int) = 0;
};

class VDrawer
{
public:
	bool			HasStencil;

	VDrawer()
	: HasStencil(false)
	{}
	virtual ~VDrawer()
	{}

	virtual void Init() = 0;
	virtual void InitData() = 0;
	virtual bool SetResolution(int, int, int, bool) = 0;
	virtual void InitResolution() = 0;
	virtual void NewMap() = 0;
	virtual void StartUpdate() = 0;
	virtual void Update() = 0;
	virtual void BeginDirectUpdate() = 0;
	virtual void EndDirectUpdate() = 0;
	virtual void Shutdown() = 0;
	virtual void* ReadScreen(int*, bool*) = 0;
	virtual void ReadBackScreen(int, int, rgba_t*) = 0;
	virtual void FreeSurfCache(surfcache_t*) = 0;

	//	Rendring stuff
	virtual void SetupView(VRenderLevelDrawer*, const refdef_t*) = 0;
	virtual void SetupViewOrg() = 0;
	virtual void WorldDrawing() = 0;
	virtual void EndView() = 0;

	//	Texture stuff
	virtual void PrecacheTexture(VTexture*) = 0;

	//	Polygon drawing
	virtual void DrawPolygon(surface_t*, int) = 0;
	virtual void DrawSkyPortal(surface_t*, int) = 0;
	virtual void BeginSky() = 0;
	virtual void DrawSkyPolygon(surface_t*, bool, VTexture*, float, VTexture*,
		float, int) = 0;
	virtual void EndSky() = 0;
	virtual void DrawHorizonPolygon(surface_t*, int) = 0;
	virtual void DrawMaskedPolygon(surface_t*, float, bool) = 0;
	virtual void DrawSpritePolygon(TVec*, VTexture*, float, bool,
		VTextureTranslation*, int, vuint32, vuint32, const TVec&, float,
		const TVec&, const TVec&, const TVec&) = 0;
	virtual void DrawAliasModel(const TVec&, const TAVec&, const TVec&,
		const TVec&, mmdl_t*, int, VTexture*, VTextureTranslation*, int,
		vuint32, vuint32, float, bool, bool) = 0;
	virtual bool StartPortal(VPortal*, bool) = 0;
	virtual void EndPortal(VPortal*, bool) = 0;

	//	Particles
	virtual void StartParticles() = 0;
	virtual void DrawParticle(particle_t *) = 0;
	virtual void EndParticles() = 0;

	//	Drawing
	virtual void DrawPic(float, float, float, float, float, float, float,
		float, VTexture*, VTextureTranslation*, float) = 0;
	virtual void DrawPicShadow(float, float, float, float, float, float,
		float, float, VTexture*, float) = 0;
	virtual void FillRectWithFlat(float, float, float, float, float, float,
		float, float, VTexture*) = 0;
	virtual void FillRect(float, float, float, float, vuint32) = 0;
	virtual void ShadeRect(int, int, int, int, float) = 0;
	virtual void DrawConsoleBackground(int) = 0;
	virtual void DrawSpriteLump(float, float, float, float, VTexture*,
		VTextureTranslation*, bool) = 0;

	//	Automap
	virtual void StartAutomap() = 0;
	virtual void DrawLine(int, int, vuint32, int, int, vuint32) = 0;
	virtual void EndAutomap() = 0;
};

//	Drawer types, menu system uses these numbers.
enum
{
	DRAWER_Software,
	DRAWER_OpenGL,
	DRAWER_Direct3D,

	DRAWER_MAX
};

//	Drawer description.
struct FDrawerDesc
{
	const char*		Name;
	const char*		Description;
	const char*		CmdLineArg;
	VDrawer*		(*Creator)();

	FDrawerDesc(int Type, const char* AName, const char* ADescription,
		const char* ACmdLineArg, VDrawer* (*ACreator)());
};

//	Drawer driver declaration macro.
#define IMPLEMENT_DRAWER(TClass, Type, Name, Description, CmdLineArg) \
static VDrawer* Create##TClass() \
{ \
	return new TClass(); \
} \
FDrawerDesc TClass##Desc(Type, Name, Description, CmdLineArg, Create##TClass);

extern VDrawer			*Drawer;
