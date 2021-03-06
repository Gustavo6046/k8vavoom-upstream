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
//**  Copyright (C) 2018-2021 Ketmar Dark
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

#define MAXPLAYERS  (8)

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger, but we do not have any moving sectors nearby
#define MAXRADIUS  (32.0f)

// mapblocks are used to check movement against lines and things
#define MapBlock(x_)  (((int)floorf(x_))>>7)

class VDecalDef;
struct opening_t;


// WARNING! keep in sync with VavoomC code!
enum {
  // vanilla flags
  MTF_AMBUSH                = 0x0008, // deaf monsters/do not react to sound
  MTF_UNTRANSLATED_MP       = 0x0010,
  // Boom
  MTF_UNTRANSLATED_NO_DM    = 0x0020,
  MTF_UNTRANSLATED_NO_COOP  = 0x0040,
  // MBF
  MTF_UNTRANSLATED_FRIENDLY = 0x0080,

  MTF_DORMANT     = 0x0010, // the thing is dormant

  // translated k8vavoom flags
  MTF_GSINGLE     = 0x0100, // appearing in game modes
  MTF_GCOOP       = 0x0200,
  MTF_GDEATHMATCH = 0x0400,
  MTF_SHADOW      = 0x0800,
  MTF_ALTSHADOW   = 0x1000,
  MTF_FRIENDLY    = 0x2000,
  MTF_STANDSTILL  = 0x4000,

  MTF2_FIGHTER    = 0x000010000, // thing appearing in player classes
  MTF2_CLERIC     = 0x000020000,
  MTF2_MAGE       = 0x000040000,
};


//==========================================================================
//
//  Structures for level network replication
//
//==========================================================================
struct rep_line_t {
  float alpha;
};

struct rep_side_t {
  side_tex_params_t Top;
  side_tex_params_t Bot;
  side_tex_params_t Mid;
  int TopTexture;
  int BottomTexture;
  int MidTexture;
  vuint32 Flags;
  int Light;
};

struct rep_sector_t {
  int floor_pic;
  float floor_dist;
  float floor_xoffs;
  float floor_yoffs;
  float floor_XScale;
  float floor_YScale;
  float floor_Angle;
  float floor_BaseAngle;
  float floor_BaseYOffs;
  VEntity *floor_SkyBox;
  float floor_MirrorAlpha;
  int ceil_pic;
  float ceil_dist;
  float ceil_xoffs;
  float ceil_yoffs;
  float ceil_XScale;
  float ceil_YScale;
  float ceil_Angle;
  float ceil_BaseAngle;
  float ceil_BaseYOffs;
  VEntity *ceil_SkyBox;
  float ceil_MirrorAlpha;
  int Sky;
  sec_params_t params;
};

struct rep_polyobj_t {
  TVec startSpot;
  float angle;
};

struct rep_light_t {
  TVec Origin;
  float Radius;
  vuint32 Color;
  //VEntity *Owner;
  vuint32 OwnerUId; // 0: no owner
  TVec ConeDir;
  float ConeAngle;
  sector_t *LevelSector;
  float LevelScale;
  //int NextUIdIndex; // -1 or next light for this OwnerUId
  enum {
    LightChanged = 1u<<0,
    LightActive  = 1u<<1,
  };
  vuint32 Flags;

  inline bool IsChanged () const noexcept { return (Flags&LightChanged); }
  inline bool IsActive () const noexcept { return (Flags&LightActive); }
};


struct VSndSeqInfo {
  VName Name;
  vint32 OriginId;
  TVec Origin;
  vint32 ModeNum;
  TArray<VName> Choices;
};


struct VCameraTextureInfo {
  VEntity *Camera;
  int TexNum;
  int FOV;
};


// ////////////////////////////////////////////////////////////////////////// //
class VLevel;
class VLevelInfo;

class VLevelScriptThinker : public VSerialisable {
public:
  bool destroyed; // script `Destroy()` method should set this (and check to avoid double destroying)
  VLevel *XLevel; // level object
  VLevelInfo *Level; // level info object

public:
  inline VLevelScriptThinker () : destroyed(false), XLevel(nullptr), Level(nullptr) {}
  virtual ~VLevelScriptThinker ();

  inline void DestroyThinker () { Destroy(); }

  // it is guaranteed that `Destroy()` will be called by master before deleting the object
  virtual void Destroy () = 0;
  virtual void ClearReferences () = 0;
  virtual void Tick (float DeltaTime) = 0;

  virtual VName GetName () = 0;
  virtual int GetNumber () = 0;

  virtual VStr DebugDumpToString () = 0;
};

extern VLevelScriptThinker *AcsCreateEmptyThinker ();
//FIXME
extern void AcsSuspendScript (VAcsLevel *acslevel, int number, int map);
extern void AcsTerminateScript (VAcsLevel *acslevel, int number, int map);
extern bool AcsHasScripts (VAcsLevel *acslevel);


//==========================================================================
//
//                  LEVEL
//
//==========================================================================
enum {
  MAX_LEVEL_TRANSLATIONS      = 0xffff,
  MAX_BODY_QUEUE_TRANSLATIONS = 0xff,
};


struct VSectorLink {
  vint32 index;
  vint32 mts; // bit 30: set if ceiling; low byte: movetype
  vint32 next; // index, or -1
};


struct VCtl2DestLink {
  vint32 src; // sector number, just in case (can be -1 for unused slots)
  vint32 dest; // sector number (can be -1 for unused slots)
  vint32 next; // index in `ControlLinks` or -1
};


struct VLightParams {
  TVec Origin;
  float Radius;
  vuint32 Color;
  TVec coneDirection;
  float coneAngle;
  sector_t *LevelSector;
  float LevelScale; // <0: attenuated

  inline VLightParams () noexcept
    : Origin(0.0f, 0.0f, 0.0f)
    , Radius(0.0f)
    , Color(0u)
    , coneDirection(0.0f, 0.0f, 0.0f)
    , coneAngle(0.0f)
    , LevelSector(nullptr)
    , LevelScale(0.0f)
  {}

  inline VLightParams (const rep_light_t &L) noexcept {
    Origin = L.Origin;
    Radius = L.Radius;
    Color = L.Color;
    coneDirection = L.ConeDir;
    coneAngle = L.ConeAngle;
    LevelSector = L.LevelSector;
    LevelScale = L.LevelScale;
  }
};


class VLevel : public VGameObject {
  DECLARE_CLASS(VLevel, VGameObject, 0)
  NO_DEFAULT_CONSTRUCTOR(VLevel)

  friend class VUdmfParser;

  VName MapName;
  VStr MapHash;
  VStr MapHashMD5;
  vuint32 LSSHash; // xxHash32 of linedefs, sidedefs, sectors (in this order)
  vuint32 SegHash; // xxHash32 of segs

  // flags
  enum {
    LF_ForServer = 1u<<0, // true if this level belongs to the server
    LF_Extended  = 1u<<1, // true if level was in Hexen format (or extended namespace for UDMF)
    LF_GLNodesV5 = 1u<<2, // true if using version 5 GL nodes
    LF_TextMap   = 1u<<3, // UDMF format map
    // used in map fixer
    LF_ForceRebuildNodes                = 1u<<4,
    LF_ForceAllowSeveralPObjInSubsector = 1u<<5, // this is not used anymore, but kept for compatibility
    LF_ForceNoTexturePrecache           = 1u<<6,
    LF_ForceNoPrecalcStaticLights       = 1u<<7,
    LF_ForceNoDeepwaterFix              = 1u<<8,
    LF_ForceNoFloorFloodfillFix         = 1u<<9,
    LF_ForceNoCeilingFloodfillFix       = 1u<<10,
    // used only in VavoomC
    LF_ConvertSectorLightsToStatic      = 1u<<11,
  };
  vuint32 LevelFlags;

  VName UDMFNamespace; // locased

  // MAP related Lookup tables
  // store VERTEXES, LINEDEFS, SIDEDEFS, etc.

  TVec *Vertexes;
  vint32 NumVertexes;

  sector_t *Sectors;
  vint32 NumSectors;

  side_t *Sides;
  vint32 NumSides;

  line_t *Lines;
  vint32 NumLines;

  seg_t *Segs;
  vint32 NumSegs;

  subsector_t *Subsectors;
  vint32 NumSubsectors;

  node_t *Nodes;
  vint32 NumNodes;

  // !!! Used only during level loading
  mthing_t *Things;
  vint32 NumThings;

  // BLOCKMAP
  // created from axis aligned bounding box of the map, a rectangular array of blocks of size ...
  // used to speed up collision detection by spatial subdivision in 2D
  vint32 BlockMapLumpSize;
  vint32 *BlockMapLump; // offsets in blockmap are from here
  vint32 *BlockMap;   // int for larger maps
  vint32 BlockMapWidth;  // Blockmap size.
  vint32 BlockMapHeight; // size in mapblocks
  float BlockMapOrgX; // origin of block map
  float BlockMapOrgY;
  VEntity **BlockLinks;   // for thing chains
  polyblock_t **PolyBlockMap;

  // REJECT
  // for fast sight rejection
  // speeds up enemy AI by skipping detailed LineOf Sight calculation
  // without special effect, this could be used as a PVS lookup as well
  vuint8 *RejectMatrix;
  vint32 RejectMatrixSize;

  // strife conversations
  FRogueConSpeech *GenericSpeeches;
  vint32 NumGenericSpeeches;

  FRogueConSpeech *LevelSpeeches;
  vint32 NumLevelSpeeches;

  // list of all poly-objects on the level
  polyobj_t **PolyObjs;
  vint32 NumPolyObjs;

  // anchor points of poly-objects
  PolyAnchorPoint_t *PolyAnchorPoints;
  vint32 NumPolyAnchorPoints;

  // sound environments for sector zones
  vint32 *Zones;
  vint32 NumZones;

  VThinker *ThinkerHead;
  VThinker *ThinkerTail;

  VLevelInfo *LevelInfo;
  VWorldInfo *WorldInfo;

  VAcsLevel *Acs;

  VRenderLevelPublic *Renderer;
  VNetContext *NetContext;

  rep_line_t *BaseLines;
  rep_side_t *BaseSides;
  rep_sector_t *BaseSectors;
  rep_polyobj_t *BasePolyObjs;

  TArray<rep_light_t> StaticLights;
  TMapNC<vuint32, int> *StaticLightsMap; // key: owner; value: index in `StaticLights`

  TArray<VSndSeqInfo> ActiveSequences;
  TArray<VCameraTextureInfo> CameraTextures;

  float NextTime; // current game time, in seconds (set *BEFORE* all tickers called)
  float Time; // current game time, in seconds (incremented *AFTER* all tickers called)
  vint32 TicTime; // game time, in tics (35 per second)

  msecnode_t *SectorList;
  // phares 3/21/98
  // Maintain a freelist of msecnode_t's to reduce memory allocs and frees
  msecnode_t *HeadSecNode;

  // Translations controlled by ACS scripts
  TArray<VTextureTranslation *> Translations;
  TArray<VTextureTranslation *> BodyQueueTrans;

  VState *CallingState;
  VStateCall *StateCall;

  vint32 NextSoundOriginID;

  decal_t *decanimlist;
  vint32 decanimuid;

  //vint32 tagHashesSaved; // for savegame compatibility
  TagHash *lineTags;
  TagHash *sectorTags;

  TArray<vint32> sectorlinkStart;
  TArray<VSectorLink> sectorlinks;

  TArray<VLevelScriptThinker *> scriptThinkers;

  vuint32 csTouchCount;
  // for ChangeSector; Z_Malloc'ed, NumSectors elements
  // bit 31 is return value from `ChangeSectorInternal()`
  // if other bits are equal to csTouchCount
  vuint32 *csTouched;

  // sectors with fake floors/ceilings, so world updater can skip iterating over all of them
  TArray<vint32> FakeFCSectors;
  //TArray<vint32> TaggedSectors;

  // this is used to keep control->dest 3d floor links
  // [0..NumSectors) is entry point, then follow next index (-1 means 'end')
  // order is undefined
  TArray<VCtl2DestLink> ControlLinks;

  // array of cached subsector centers (used in VC code, no need to save/load it)
  TArray<TVec> ssCenters;

  vint32 tsVisitedCount;

  // set in `LoadMap()`, can be used by renderer to load/save lightmap cache
  VStr cacheFileBase; // can be empty, otherwise it is path and file name w/o extension
  enum {
    CacheFlag_Ignore = 1u<<0,
  };
  vuint32 cacheFlags;

  TArray<VMapMarkerInfo> MapMarkers;

protected:
  // temporary working set for decal spreader
  struct DecalLineInfo {
    line_t *nline;
    int nside;
    bool isv1;
    bool isbackside;
  };

  static TArray<DecalLineInfo> connectedLines;

public:
  // iterator
  struct CtlLinkIter {
    VLevel *level;
    int srcidx;
    int curridx;

    CtlLinkIter () = delete;

    CtlLinkIter (VLevel *alevel, int asrcidx) : level(alevel), srcidx(asrcidx), curridx(asrcidx) {
      if (!alevel || asrcidx < 0 || asrcidx >= alevel->ControlLinks.length()) {
        curridx = -1;
      } else if (alevel->ControlLinks[asrcidx].dest < 0) {
        // no linked sectors for this source
        curridx = -1;
      }
    }

    inline bool isEmpty () const { return (!level || curridx < 0); }
    inline sector_t *getDestSector () const { return (level && curridx >= 0 ? level->Sectors+level->ControlLinks[curridx].dest : nullptr); }
    inline int getDestSectorIndex () const { return (level && curridx >= 0 ? level->ControlLinks[curridx].dest : -1); }
    inline void next () { if (level && curridx >= 0) curridx = level->ControlLinks[curridx].next; }
  };

private:
  // returns `false` for duplicate/invalid link
  bool AppendControlLink (const sector_t *src, const sector_t *dest);
  inline CtlLinkIter IterControlLinks (const sector_t *src) { return CtlLinkIter(this, (src ? (int)(ptrdiff_t)(src-Sectors) : -1)); }

  // `-1` for `crunch` means "ignore stuck mobj"
  bool ChangeSectorInternal (sector_t *sector, int crunch);
  void ChangeOneSectorInternal (sector_t *sector);

private:
  void xxHashLinedef (XXH32_state_t *xctx, const line_t &line) const {
    if (!xctx) return;
    // sides, front and back sectors
    XXH32_update(xctx, &line.sidenum[0], sizeof(line.sidenum[0]));
    XXH32_update(xctx, &line.sidenum[1], sizeof(line.sidenum[1]));
    vuint32 fsnum = (line.frontsector ? (vuint32)(ptrdiff_t)(line.frontsector-Sectors) : 0xffffffffU);
    XXH32_update(xctx, &fsnum, sizeof(fsnum));
    vuint32 bsnum = (line.backsector ? (vuint32)(ptrdiff_t)(line.backsector-Sectors) : 0xffffffffU);
    XXH32_update(xctx, &bsnum, sizeof(bsnum));
  }

  void xxHashSidedef (XXH32_state_t *xctx, const side_t &side) const {
    if (!xctx) return;
    // sector, line
    vuint32 secnum = (side.Sector ? (vuint32)(ptrdiff_t)(side.Sector-Sectors) : 0xffffffffU);
    XXH32_update(xctx, &secnum, sizeof(secnum));
    XXH32_update(xctx, &side.LineNum, sizeof(side.LineNum));
  }

  void xxHashSectordef (XXH32_state_t *xctx, const sector_t &sec) const {
    if (!xctx) return;
    // number of lines in sector
    XXH32_update(xctx, &sec.linecount, sizeof(sec.linecount));
  }

  void xxHashPlane (XXH32_state_t *xctx, const TPlane &pl) const {
    if (!xctx) return;
    XXH32_update(xctx, &pl.normal.x, sizeof(pl.normal.x));
    XXH32_update(xctx, &pl.normal.y, sizeof(pl.normal.y));
    XXH32_update(xctx, &pl.normal.z, sizeof(pl.normal.z));
    XXH32_update(xctx, &pl.dist, sizeof(pl.dist));
  }

  // seg hash is using to restore decals
  // it won't prevent level loading
  void xxHashSegdef (XXH32_state_t *xctx, const seg_t &seg) const {
    if (!xctx) return;
    xxHashPlane(xctx, seg);
    vuint32 v0idx = (vuint32)(ptrdiff_t)(seg.v1-Vertexes);
    XXH32_update(xctx, &v0idx, sizeof(v0idx));
    vuint32 v1idx = (vuint32)(ptrdiff_t)(seg.v2-Vertexes);
    XXH32_update(xctx, &v1idx, sizeof(v1idx));
    //!XXH32_update(xctx, &seg.offset, sizeof(seg.offset));
    //!XXH32_update(xctx, &seg.length, sizeof(seg.length));
    vuint32 sdnum = (seg.sidedef ? (vuint32)(ptrdiff_t)(seg.sidedef-Sides) : 0xffffffffU);
    XXH32_update(xctx, &sdnum, sizeof(sdnum));
    vuint32 ldnum = (seg.linedef ? (vuint32)(ptrdiff_t)(seg.linedef-Lines) : 0xffffffffU);
    XXH32_update(xctx, &ldnum, sizeof(ldnum));
    vuint32 fsnum = (seg.frontsector ? (vuint32)(ptrdiff_t)(seg.frontsector-Sectors) : 0xffffffffU);
    XXH32_update(xctx, &fsnum, sizeof(fsnum));
    vuint32 bsnum = (seg.backsector ? (vuint32)(ptrdiff_t)(seg.backsector-Sectors) : 0xffffffffU);
    XXH32_update(xctx, &bsnum, sizeof(bsnum));
    vuint32 pnum = (seg.partner ? (vuint32)(ptrdiff_t)(seg.partner-Segs) : 0xffffffffU);
    XXH32_update(xctx, &pnum, sizeof(pnum));
    XXH32_update(xctx, &seg.side, sizeof(seg.side));
  }

public:
  // if `ImmediateRun` is true, init some script variables, but don't register thinker
  void AddScriptThinker (VLevelScriptThinker *sth, bool ImmediateRun);
  void PromoteImmediateScriptThinker (VLevelScriptThinker *sth); // WARNING! does no checks!
  void RemoveScriptThinker (VLevelScriptThinker *sth); // won't call `Destroy()`, won't call `delete`
  void RunScriptThinkers (float DeltaTime);

  void SuspendNamedScriptThinkers (VStr name, int map);
  void TerminateNamedScriptThinkers (VStr name, int map);

  vint32 nextVisitedCount ();

public:
  // basic coldet code
  enum CD_HitType {
    CD_HT_None,
    CD_HT_Point,
    // sides
    CD_HT_Top,
    CD_HT_Bottom,
    CD_HT_Left,
    CD_HT_Right,
  };

  static float SweepLinedefAABB (const line_t *ld, TVec vstart, TVec vend, TVec bmin, TVec bmax,
                                 TPlane *hitPlane=nullptr, TVec *contactPoint=nullptr, CD_HitType *hitType=nullptr,
                                 int *hitplanenum=nullptr);

public:
  void ResetValidCount ();
  void IncrementValidCount ();

  void ResetSZValidCount ();
  void IncrementSZValidCount ();

  // this saves everything except thinkers, so i can load it for further experiments
  void DebugSaveLevel (VStream &strm);

  // this is slow!
  float CalcSkyHeight () const;

  // some sectors (like doors) has floor and ceiling on the same level, so
  // we have to look at neighbour sector to get height.
  // note that if neighbour sector is closed door too, we can safely use
  // our zero height, as camera cannot see through top/bottom textures.
  //void CalcSectorBoundingHeight (sector_t *sector, float *minz, float *maxz);

  void UpdateSubsectorBBox (int num, float bbox[6], const float skyheight);
  void RecalcWorldNodeBBox (int bspnum, float bbox[6], const float skyheight);
  // this also fixes 2d node bounding boxes (those can be wrong due to integers in ajbsp)
  void RecalcWorldBBoxes ();

  // recalcs world bboxes if some cvars changed
  void CheckAndRecalcWorldBBoxes ();

  void UpdateSectorHeightCache (sector_t *sector);
  void GetSubsectorBBox (subsector_t *sub, float bbox[6]);
  void CalcSecMinMaxs (sector_t *sector); // also, update BSP bounding boxes

public:
  virtual void PostCtor () override;
  virtual void SerialiseOther (VStream &Strm) override;
  virtual void ClearReferences () override;
  virtual void Destroy () override;

  // map loader
  void LoadMap (VName MapName);

  // vanilla buggy algo
  subsector_t *PointInSubsector_Buggy (const TVec &point) const noexcept;
  // bugfixed algo
  subsector_t *PointInSubsector (const TVec &point) const noexcept;

  bool IsPointInSubsector2D (const subsector_t *sub, TVec in) const noexcept;
  // checks all subsectors
  bool IsPointInSector2D (const sector_t *sec, TVec in) const noexcept;

  inline const vuint8 IsRejectedVis (const sector_t *from, const sector_t *dest) const noexcept {
    if (!from || !dest) return 0xff; // this is REJECT matrix, not ACCEPT
    if (RejectMatrix) {
      const unsigned s1 = (unsigned)(ptrdiff_t)(from-Sectors);
      const unsigned s2 = (unsigned)(ptrdiff_t)(dest-Sectors);
      const unsigned pnum = s1*(unsigned)NumSectors+s2;
      return RejectMatrix[pnum>>3]&(1u<<(pnum&7));
    }
    return 0x00; // this is REJECT matrix, not ACCEPT
  }

  void ResetStaticLights ();

  // returns static light id
  void AddStaticLightRGB (VEntity *Ent, const VLightParams &lpar);
  void MoveStaticLightByOwner (VEntity *Ent, const TVec &Origin);
  void RemoveStaticLightByOwner (VEntity *Ent);

  // returns static light id
  void AddStaticLightRGB (vuint32 owneruid, const VLightParams &lpar);
  void MoveStaticLightByOwner (vuint32 owneruid, const TVec &Origin);
  void RemoveStaticLightByOwner (vuint32 owneruid);

  VThinker *SpawnThinker (VClass *AClass, const TVec &AOrigin=TVec(0, 0, 0),
                          const TAVec &AAngles=TAVec(0, 0, 0), mthing_t *mthing=nullptr,
                          bool AllowReplace=true, vuint32 srvUId=0);

  void AddThinker (VThinker *Th);
  void RemoveThinker (VThinker *Th);
  void DestroyAllThinkers ();

  // called from netcode for client connection, after `LevelInfo` was received
  void UpdateThinkersLevelInfo ();

  void TickDecals (float DeltaTime); // this should be called in `CL_UpdateMobjs()`
  void TickWorld (float DeltaTime);

  // poly-objects
  void SpawnPolyobj (float x, float y, int tag, bool crush, bool hurt);
  void AddPolyAnchorPoint (float x, float y, int tag);
  void InitPolyobjs ();
  polyobj_t *GetPolyobj (int polyNum) noexcept; // actually, tag
  int GetPolyobjMirror (int poly); // tag again
  bool MovePolyobj (int num, float x, float y, bool forced=false); // tag (GetPolyobj)
  bool RotatePolyobj (int num, float angle); // tag (GetPolyobj)

  // `-1` for `crunch` means "ignore stuck mobj"
  bool ChangeSector (sector_t *sector, int crunch);

  // returns `false` on hit
  bool TraceLine (linetrace_t &Trace, const TVec &Start, const TVec &End, unsigned PlaneNoBlockFlags, unsigned moreLineBlockFlags=0);

  // doesn't check pvs or reject
  // `Sector` can be `nullptr`
  // if `orgdirRight` is zero vector, don't do extra checks
  bool CastCanSee (sector_t *Sector, const TVec &org, float myheight, const TVec &orgdirFwd, const TVec &orgdirRight,
                   const TVec &dest, float radius, float height, bool skipBaseRegion=false, sector_t *DestSector=nullptr,
                   bool allowBetterSight=false, bool ignoreBlockAll=false, bool ignoreFakeFloors=false);
  // this is used to trace light rays (via blockmap)
  bool CastLightRay (bool textureCheck, sector_t *Sector, const TVec &org, const TVec &dest, sector_t *DestSector=nullptr);

  void SetCameraToTexture (VEntity *, VName, int);

  msecnode_t *AddSecnode (sector_t *, VEntity *, msecnode_t *);
  msecnode_t *DelSecnode (msecnode_t *);
  void DelSectorList ();

  int FindSectorFromTag (sector_t *&sector, int tag, int start=-1);
  line_t *FindLine (int, int *);
  void SectorSetLink (int controltag, int tag, int surface, int movetype);

  inline bool IsForServer () const { return !!(LevelFlags&LF_ForServer); }
  inline bool IsForClient () const { return !(LevelFlags&LF_ForServer); }

  void SaveCachedData (VStream *strm);
  bool LoadCachedData (VStream *strm);
  void ClearAllMapData (); // call this if cache is corrupted

  void FixKnownMapErrors ();

  void AddExtraFloor (line_t *line, sector_t *dst);

  void CalcLine (line_t *line); // this calls `CalcLineCDPlanes()`
  void CalcLineCDPlanes (line_t *line);
  void CalcSegLenOfs (seg_t *seg); // only length and offset
  void CalcSeg (seg_t *seg); // plane, direction, but length and offset should be already set

private:
  void AddExtraFloorSane (line_t *line, sector_t *dst); // k8vavoom
  void AddExtraFloorShitty (line_t *line, sector_t *dst); // gozzo

public:
  static void dumpRegion (const sec_region_t *reg);
  static void dumpSectorRegions (const sector_t *dst);

  static bool CheckPlanePass (const TSecPlaneRef &plane, const TVec &linestart, const TVec &lineend, TVec &currhit, bool &isSky);

  // set `SPF_IGNORE_FAKE_FLOORS` is flagmask to ignore fake floors
  // set `SPF_IGNORE_BASE_REGION` is flagmask to ignore base region
  static bool CheckPassPlanes (sector_t *sector, TVec linestart, TVec lineend, unsigned flagmask,
                               TVec *outHitPoint, TVec *outHitNormal, bool *outIsSky, TPlane *outHitPlane);

public:
  #define VL_ITERATOR(arrname_,itername_,itertype_) \
    class arrname_##Iterator { \
      friend class VLevel; \
    private: \
      const VLevel *level; \
    public: \
      arrname_##Iterator (const VLevel *alevel) : level(alevel) {} \
    public: \
      inline itertype_ *begin () const { return &level->arrname_[0]; } \
      /*inline const itertype_ *begin () const { return &level->arrname_[0]; }*/ \
      inline itertype_ *end () const { return &level->arrname_[level->Num##arrname_]; } \
      /*inline const itertype_ *end () const { return &level->arrname_[level->Num##arrname_]; }*/ \
    }; \
    inline arrname_##Iterator itername_ () { return arrname_##Iterator(this); } \
    inline const arrname_##Iterator itername_ () const { return arrname_##Iterator(this); }

  // iterators: `for (auto &&it : XXX())` (`it` is a reference!)
  VL_ITERATOR(Vertexes, allVertices, TVec)
  VL_ITERATOR(Sectors, allSectors, sector_t)
  VL_ITERATOR(Sides, allSides, side_t)
  VL_ITERATOR(Lines, allLines, line_t)
  VL_ITERATOR(Segs, allSegs, seg_t)
  VL_ITERATOR(Subsectors, allSubsectors, subsector_t)
  VL_ITERATOR(Nodes, allNodes, node_t)
  VL_ITERATOR(Things, allThings, mthing_t)
  VL_ITERATOR(Zones, allZones, vint32)
  VL_ITERATOR(PolyObjs, allPolyObjs, polyobj_t *)
  VL_ITERATOR(PolyAnchorPoints, allPolyAnchorPoints, PolyAnchorPoint_t)
  VL_ITERATOR(GenericSpeeches, allGenericSpeeches, FRogueConSpeech)
  VL_ITERATOR(LevelSpeeches, allLevelSpeeches, FRogueConSpeech)

  inline VertexesIterator allVertexes () { return VertexesIterator(this); }
  inline const VertexesIterator allVertexes () const { return VertexesIterator(this); }

  #undef VL_ITERATOR


  #define VL_ITERATOR_INDEX(arrname_,itername_,itertype_) \
    class arrname_##IndexIterator { \
      friend class VLevel; \
    private: \
      const VLevel *level; \
      int idx; \
    public: \
      arrname_##IndexIterator (const VLevel *alevel, bool asEnd=false) : level(alevel), idx(asEnd ? alevel->Num##arrname_ : 0) {} \
      arrname_##IndexIterator (const arrname_##IndexIterator &it) : level(it.level), idx(it.idx) {} \
    public: \
      inline bool operator == (const arrname_##IndexIterator &b) const { return (idx == b.idx); } \
      inline bool operator != (const arrname_##IndexIterator &b) const { return (idx != b.idx); } \
      inline arrname_##IndexIterator operator * () const { return arrname_##IndexIterator(*this); } /* required for iterator */ \
      inline void operator ++ () { ++idx; } /* this is enough for iterator */ \
      inline arrname_##IndexIterator begin () { return arrname_##IndexIterator(*this); } \
      inline arrname_##IndexIterator end () { return arrname_##IndexIterator(level, true); } \
      inline itertype_ *value () const { return &level->arrname_[idx]; } \
      /*inline const itertype_ *value () const { return &level->arrname_[idx]; }*/ \
      inline int index () const { return idx; } \
    }; \
    inline arrname_##IndexIterator itername_ () { return arrname_##IndexIterator(this); } \
    inline arrname_##IndexIterator itername_ () const { return arrname_##IndexIterator(this); } \

  VL_ITERATOR_INDEX(Vertexes, allVerticesIdx, TVec)
  VL_ITERATOR_INDEX(Sectors, allSectorsIdx, sector_t)
  VL_ITERATOR_INDEX(Sides, allSidesIdx, side_t)
  VL_ITERATOR_INDEX(Lines, allLinesIdx, line_t)
  VL_ITERATOR_INDEX(Segs, allSegsIdx, seg_t)
  VL_ITERATOR_INDEX(Subsectors, allSubsectorsIdx, subsector_t)
  VL_ITERATOR_INDEX(Nodes, allNodesIdx, node_t)
  VL_ITERATOR_INDEX(Things, allThingsIdx, mthing_t)
  VL_ITERATOR_INDEX(Zones, allZonesIdx, vint32)
  VL_ITERATOR_INDEX(PolyObjs, allPolyObjsIdx, polyobj_t *)
  VL_ITERATOR_INDEX(PolyAnchorPoints, allPolyAnchorPointsIdx, PolyAnchorPoint_t)
  VL_ITERATOR_INDEX(GenericSpeeches, allGenericSpeechesIdx, FRogueConSpeech)
  VL_ITERATOR_INDEX(LevelSpeeches, allLevelSpeechesIdx, FRogueConSpeech)

  inline VertexesIndexIterator allVertexesIdx () { return VertexesIndexIterator(this); }
  inline VertexesIndexIterator allVertexesIdx () const { return VertexesIndexIterator(this); }

  #undef VL_ITERATOR_INDEX

  // subsector segs
  class SubSegsIndexIterator {
    friend class VLevel;
  private:
    const VLevel *level;
    seg_t *curr;
    seg_t *last;
  public:
    SubSegsIndexIterator (const VLevel *alevel, const subsector_t *sub) : level(alevel) {
      if (sub && sub->numlines > 0) {
        curr = &alevel->Segs[sub->firstline];
        last = curr+sub->numlines;
      } else {
        curr = nullptr;
        last = nullptr;
      }
    }
    //SubSegsIndexIterator (const SubSegsIndexIterator &it) : level(it.level), curr(it.curr), last(it.last) {}
  public:
    inline seg_t *begin () { return curr; }
    inline seg_t *end () { return last; }
  };

  inline SubSegsIndexIterator allSubSegs (const subsector_t *sub) const { return SubSegsIndexIterator(this, sub); }
  inline SubSegsIndexIterator allSubSegs (int subidx) const {
    const subsector_t *sub = (subidx >= 0 && subidx < NumSubsectors ? &Subsectors[subidx] : nullptr);
    return SubSegsIndexIterator(this, sub);
  }

  static constexpr float BigDecalWidth = 34.0f;
  static constexpr float BigDecalHeight = 34.0f;

  // this will remove dead and over-the-limit decals (including animated)
  // called from `AddDecal()`
  void CleanupDecals (seg_t *seg);

private:
  // map loaders
  void LoadVertexes (int, int, int &);
  void LoadSectors (int);
  void LoadSideDefs (int);
  void LoadLineDefs1 (int, int, const VMapInfo &);
  void LoadLineDefs2 (int, int, const VMapInfo &);
  void LoadGLSegs (int, int);
  void LoadSubsectors (int);
  void LoadNodes (int);
  bool LoadCompressedGLNodes (int Lump, char hdr[4]);
  void LoadBlockMap (int);
  void LoadReject (int);
  void LoadThings1 (int);
  void LoadThings2 (int);
  void LoadLoadACS (int lacsLump, int XMapLump); // load libraries from 'loadacs'
  void LoadACScripts (int BehLump, int XMapLump);
  void LoadTextMap (int, const VMapInfo &);
  // call this after loading things
  void SetupThingsFromMapinfo ();

  // call this after level is loaded or nodes are built
  void PostLoadSegs ();
  void PostLoadSubsectors ();

  // called by map loader, in postprocessing stage
  // sector `deepref`, `othersecFloor`, and `othersecCeiling` fields
  // are not set for the following two, and should not be read or written
  void DetectHiddenSectors ();
  void FixTransparentDoors ();

  void FixDeepWaters ();
  void FixSelfRefDeepWater (); // called from `FixDeepWaters`

  vuint32 IsFloodBugSectorPart (sector_t *sec);
  vuint32 IsFloodBugSector (sector_t *sec);
  sector_t *FindGoodFloodSector (sector_t *sec, bool wantFloor);

  void BuildDecalsVVList ();

  // cache
  static VStr getCacheDir ();
  static void doCacheCleanup ();

  // map loading helpers
  int FindGLNodes (VName) const;
  int TexNumForName (const char *name, int Type, bool CMap=false) const;
  int TexNumOrColor (const char *, int, bool &, vuint32 &) const;
  void CreateSides ();
  void FinaliseLines ();
  void CreateRepBase ();
  void CreateBlockMap ();

  enum { BSP_AJ, BSP_ZD };
  int GetNodesBuilder () const; // valid only after `LevelFlags` are set
  void BuildNodesAJ ();
  void BuildNodesZD ();
  void BuildNodes ();

  bool CreatePortals (void *pvsinfo);
  void SimpleFlood (/*portal_t*/void *srcportalp, int leafnum, void *pvsinfo);
  bool LeafFlow (int leafnum, void *pvsinfo);
  void BasePortalVis (void *pvsinfo);
  void HashSectors ();
  void HashLines ();
  void BuildSectorLists ();

  static inline void ClearBox2D (float box2d[4]) {
    box2d[BOX2D_TOP] = box2d[BOX2D_RIGHT] = -999999.0f;
    box2d[BOX2D_BOTTOM] = box2d[BOX2D_LEFT] = 999999.0f;
  }

  static inline void AddToBox2D (float box2d[4], const float x, const float y) {
         if (x < box2d[BOX2D_LEFT]) box2d[BOX2D_LEFT] = x;
    else if (x > box2d[BOX2D_RIGHT]) box2d[BOX2D_RIGHT] = x;
         if (y < box2d[BOX2D_BOTTOM]) box2d[BOX2D_BOTTOM] = y;
    else if (y > box2d[BOX2D_TOP]) box2d[BOX2D_TOP] = y;
  }

  // post-loading routines
  void GroupLines ();
  void LinkNode (int BSPNum, node_t *pParent);
  void FloodZones ();
  void FloodZone (sector_t *, int);

  // loader of the Strife conversations
  void LoadRogueConScript (VName, int, FRogueConSpeech *&, int &) const;

  // internal poly-object methods
  void IterFindPolySegs (const TVec &, seg_t **, int &, const TVec &);
  void TranslatePolyobjToStartSpot (float, float, int);
  void UpdatePolySegs (polyobj_t *);
  void InitPolyBlockMap ();
  void LinkPolyobj (polyobj_t *);
  void UnLinkPolyobj (polyobj_t *);
  bool PolyCheckMobjBlocking (seg_t *, polyobj_t *);

  // internal TraceLine methods
  //bool CheckPlane (linetrace_t &, const TSecPlaneRef &Plane) const;
  //bool CheckPlanes (linetrace_t &, sector_t *) const;
  bool CheckLine (linetrace_t &trace, seg_t *seg) const;
  bool CrossSubsector (linetrace_t &trace, int num) const;
  bool CrossBSPNode (linetrace_t &trace, int BspNum) const;

  int SetBodyQueueTrans (int, int);

  decal_t *AllocSegDecal (seg_t *seg, VDecalDef *dec);
  void AddAnimatedDecal (decal_t *dc);
  void RemoveDecalAnimator (decal_t *dc); // this will also delete animator; safe to call on decals without an animator

  void AddDecal (TVec org, VName dectype, int side, line_t *li, int level, int translation);
  void AddDecalById (TVec org, int id, int side, line_t *li, int level, int translation);
  // called by `AddDecal()`
  void AddOneDecal (int level, TVec org, VDecalDef *dec, int side, line_t *li, int translation);
  void PutDecalAtLine (int tex, float orgz, float lineofs, VDecalDef *dec, int side, line_t *li, vuint32 flips, int translation, bool skipMarkCheck);

  void PostProcessForDecals ();

  void processSoundSector (int validcount, TArray<VEntity *> &elist, sector_t *sec, int soundblocks, VEntity *soundtarget, float maxdist, const TVec sndorigin);
  void doRecursiveSound (TArray<VEntity *> &elist, sector_t *sec, VEntity *soundtarget, float maxdist, const TVec sndorigin);

  void eventAfterLevelLoaded () {
    static VMethodProxy method("AfterLevelLoaded");
    vobjPutParamSelf();
    VMT_RET_VOID(method);
  }

  void eventUpdateCachedCVars () {
    static VMethodProxy method("UpdateCachedCVars");
    vobjPutParamSelf();
    VMT_RET_VOID(method);
  }

  void eventBeforeWorldTick (float deltaTime) {
    if (deltaTime <= 0.0f) return;
    static VMethodProxy method("BeforeWorldTick");
    vobjPutParamSelf(deltaTime);
    VMT_RET_VOID(method);
  }

  void eventAfterWorldTick (float deltaTime) {
    if (deltaTime <= 0.0f) return;
    static VMethodProxy method("AfterWorldTick");
    vobjPutParamSelf(deltaTime);
    VMT_RET_VOID(method);
  }

  void eventEntitySpawned (VEntity *e) {
    if (e) {
      static VMethodProxy method("OnEntitySpawned");
      vobjPutParamSelf(e);
      VMT_RET_VOID(method);
    }
  }

  void eventEntityDying (VEntity *e) {
    if (e) {
      static VMethodProxy method("OnEntityDying");
      vobjPutParamSelf(e);
      VMT_RET_VOID(method);
    }
  }

  void eventKnownMapBugFixer () {
    static VMethodProxy method("KnownMapBugFixer");
    vobjPutParamSelf();
    VMT_RET_VOID(method);
  }

public:
  void eventAfterUnarchiveThinkers () {
    static VMethodProxy method("AfterUnarchiveThinkers");
    vobjPutParamSelf();
    VMT_RET_VOID(method);
  }

private:
  DECLARE_FUNCTION(GetLineIndex)

  DECLARE_FUNCTION(PointInSector)
  DECLARE_FUNCTION(PointInSubsector)
  DECLARE_FUNCTION(PointInSectorRender)
  DECLARE_FUNCTION(PointInSubsectorRender)
  DECLARE_FUNCTION(BSPTraceLine)
  DECLARE_FUNCTION(BSPTraceLineEx)
  DECLARE_FUNCTION(ChangeSector)
  DECLARE_FUNCTION(ChangeOneSectorInternal)
  DECLARE_FUNCTION(AddExtraFloor)
  DECLARE_FUNCTION(SwapPlanes)
  DECLARE_FUNCTION(CastLightRay)

  DECLARE_FUNCTION(SetFloorPic)
  DECLARE_FUNCTION(SetCeilPic)
  DECLARE_FUNCTION(SetLineTexture)
  DECLARE_FUNCTION(SetLineAlpha)
  DECLARE_FUNCTION(SetFloorLightSector)
  DECLARE_FUNCTION(SetCeilingLightSector)
  DECLARE_FUNCTION(SetHeightSector)

  DECLARE_FUNCTION(FindSectorFromTag)
  DECLARE_FUNCTION(FindLine)
  DECLARE_FUNCTION(SectorSetLink)

  //  Polyobj functions
  DECLARE_FUNCTION(SpawnPolyobj)
  DECLARE_FUNCTION(AddPolyAnchorPoint)
  DECLARE_FUNCTION(GetPolyobj)
  DECLARE_FUNCTION(GetPolyobjMirror)
  DECLARE_FUNCTION(MovePolyobj)
  DECLARE_FUNCTION(RotatePolyobj)

  //  ACS functions
  DECLARE_FUNCTION(StartACS)
  DECLARE_FUNCTION(SuspendACS)
  DECLARE_FUNCTION(TerminateACS)
  DECLARE_FUNCTION(StartTypedACScripts)

  DECLARE_FUNCTION(RunACS)
  DECLARE_FUNCTION(RunACSAlways)
  DECLARE_FUNCTION(RunACSWithResult)

  DECLARE_FUNCTION(RunNamedACS)
  DECLARE_FUNCTION(RunNamedACSAlways)
  DECLARE_FUNCTION(RunNamedACSWithResult)

  DECLARE_FUNCTION(SetBodyQueueTrans)

  DECLARE_FUNCTION(AddDecal)
  DECLARE_FUNCTION(AddDecalById)

  DECLARE_FUNCTION(doRecursiveSound)

  DECLARE_FUNCTION(AllThinkers)
  DECLARE_FUNCTION(AllActivePlayers)

  DECLARE_FUNCTION(LdrTexNumForName)

  DECLARE_FUNCTION(CD_SweepLinedefAABB)

  DECLARE_FUNCTION(GetNextVisitedCount)
};


void SV_LoadLevel (VName MapName);
void CL_LoadLevel (VName MapName);
void SwapPlanes (sector_t *);

// WARNING! sometimes `GClLevel` set go `GLevel`, and sometimes only one of those is not `nullptr`
// the basic rule is that if there is full-featured playsim, then `GLevel` is set
// and if we need to render something, then `GClLevel` is set
// for standalone game, `GClLevel` is equal to `GLevel` (this is one weird special case)
//FIXME: need to check if those vars are properly managed everywhere
extern VLevel *GLevel; // server/standalone level
extern VLevel *GClLevel; // level for client-only games


// ////////////////////////////////////////////////////////////////////////// //
#define MAPBLOCKUNITS  (128)


struct VBlockMapWalker {
public:
  DDALineWalker<MAPBLOCKUNITS, MAPBLOCKUNITS> dda;
  int maxTileX, maxTileY;

public:
  inline VBlockMapWalker () noexcept {}

  // returns `false` if coords are out of bounds
  // note that if this method returned `false`, calling `next()` method is UB
  inline bool start (const VLevel *level, float x1, float y1, float x2, float y2) noexcept {
    maxTileX = level->BlockMapWidth;
    maxTileY = level->BlockMapHeight;
    if (maxTileX < 1 || maxTileY < 1) return false; // oops

    x1 -= level->BlockMapOrgX;
    y1 -= level->BlockMapOrgY;
    x2 -= level->BlockMapOrgX;
    y2 -= level->BlockMapOrgY;

    const int bmWidth = maxTileX*MAPBLOCKUNITS;
    const int bmHeight = maxTileY*MAPBLOCKUNITS;

    // clip to blockmap rectanle
    //GLog.Logf("WKLINE000: x1=%g; y1=%g; x2=%g; y2=%g; w=%d; h=%d", x1, y1, x2, y2, level->BlockMapWidth*MAPBLOCKUNITS, level->BlockMapHeight*MAPBLOCKUNITS);
    if (!ClipLineToRect0(&x1, &y1, &x2, &y2, bmWidth, bmHeight)) {
      // fully clipped away, so don't do anything here
      //GLog.Logf(NAME_Debug, "***CLIPPED!");
      return false;
    }
    //GLog.Logf("WKLINE001: x1=%g; y1=%g; x2=%g; y2=%g; w=%d; h=%d", x1, y1, x2, y2, level->BlockMapWidth*MAPBLOCKUNITS, level->BlockMapHeight*MAPBLOCKUNITS);

    // values should be valid here, but hey, why don't double-ensure it?
    const int ix1 = clampval((int)x1, 0, bmWidth-1);
    const int iy1 = clampval((int)y1, 0, bmHeight-1);
    const int ix2 = clampval((int)x2, 0, bmWidth-1);
    const int iy2 = clampval((int)y2, 0, bmHeight-1);

    dda.start(ix1, iy1, ix2, iy2);

    return true;
  }

  // returns `false` if we should stop (and map coords are undefined in this case)
  inline bool next (int &tileX, int &tileY) noexcept {
    while (dda.next(tileX, tileY)) {
      if (tileX >= 0 && tileX < maxTileX && tileY >= 0 && tileY < maxTileY) return true;
    }
    tileX = tileY = 0; // just to make sure that we won't leave uninitialised values there
    return false;
  }
};


// ////////////////////////////////////////////////////////////////////////// //
TVec P_SectorClosestPoint (const sector_t *sec, const TVec in, line_t **resline=nullptr);

// considers the line to be infinite
// returns side 0 or 1, -1 if box crosses the line
int P_BoxOnLineSide (const float *tmbox, const line_t *ld);

// used only in debug code
bool P_GetMidTexturePosition (const line_t *line, int sideno, float *ptextop, float *ptexbot);


// ////////////////////////////////////////////////////////////////////////// //
extern int dbgEntityTickTotal;
extern int dbgEntityTickSimple;
extern int dbgEntityTickNoTick;
