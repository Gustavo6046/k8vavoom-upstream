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
#include "fsys_local.h"


bool fsys_developer_debug = false;
int fsys_IgnoreZScript = 1;
bool fsys_DisableBDW = false;
bool fsys_report_added_paks = true;

bool fsys_skipSounds = false;
bool fsys_skipSprites = false;
bool fsys_skipDehacked = false;

// autodetected wad/pk3
int fsys_detected_mod = AD_NONE;
VStr fsys_detected_mod_wad;

// local
int fsys_dev_dump_paks = 0;


TArray<VSearchPath *> fsysSearchPaths;
TArray<VStr> fsysWadFileNames;
TArray<VStr> fsys_game_filters;

mythread_mutex fsys_glock;


//==========================================================================
//
//  FSysSavedState::save
//
//  this clears current archives
//
//==========================================================================
void FSysSavedState::save () {
  if (saved) Sys_Error("FSysSavedState: double save");
  saved = true;
  svSearchPaths = fsysSearchPaths;
  fsysSearchPaths.reset();
  svwadfiles = fsysWadFileNames;
  fsysWadFileNames.reset();
}


//==========================================================================
//
//  FSysSavedState::restore
//
//  this resets saved state
//
//==========================================================================
void FSysSavedState::restore () {
  if (!saved) Sys_Error("FSysSavedState: cannot restore empty save");
  saved = false;
  for (auto &&it : svSearchPaths) fsysSearchPaths.append(it);
  for (auto &&it : svwadfiles) fsysWadFileNames.append(it);
  svSearchPaths.clear();
  svwadfiles.clear();
}


// ////////////////////////////////////////////////////////////////////////// //
class FSys_Internal_Init_Class {
public:
  FSys_Internal_Init_Class (bool) {
    mythread_mutex_init(&fsys_glock);
  }
};

__attribute__((used)) FSys_Internal_Init_Class fsys_internal_init_class_variable_(true);


//==========================================================================
//
//  FSYS_Shutdown
//
//==========================================================================
void FSYS_InitOptions (VParsedArgs &pargs) {
  pargs.RegisterFlagSet("-ignore-zscript", "!", &fsys_IgnoreZScript);
  pargs.RegisterFlagSet("-fsys-dump-paks", "!dump loaded pak files", &fsys_dev_dump_paks);
}


//==========================================================================
//
//  FSYS_Shutdown
//
//==========================================================================
void FSYS_Shutdown () {
  MyThreadLocker glocker(&fsys_glock);
  for (int i = 0; i < fsysSearchPaths.length(); ++i) {
    delete fsysSearchPaths[i];
    fsysSearchPaths[i] = nullptr;
  }
  fsysSearchPaths.Clear();
  fsysWadFileNames.Clear();
}


//==========================================================================
//
//  FL_ClearGameFilters
//
//==========================================================================
void FL_ClearGameFilters () {
  fsys_game_filters.clear();
}


extern "C" {
  int cmpGameFilter (const void *aa, const void *bb, void *) {
    if (aa == bb) return 0;
    const VStr *a = (const VStr *)aa;
    const VStr *b = (const VStr *)bb;
    if (a->length() < b->length()) return -1;
    if (a->length() > b->length()) return 1;
    return a->ICmp(*b);
  }
}


//==========================================================================
//
//  FL_AddGameFilter
//
//  add new filter; it should start with "filter/"
//  duplicate filters will be ignored
//  returns 0 for "no error"
//
//==========================================================================
int FL_AddGameFilter (VStr path) {
  path = path.fixSlashes();
  while (path.length() && path[0] == '/') path.chopLeft(1);
  while (path.length() && path.endsWith("/")) path.chopRight(1);
  if (path.isEmpty()) return FL_ADDFILTER_INVALID;
  if (!path.startsWithCI("filter/")) return FL_ADDFILTER_INVALID;
  if (path.length() < 8) return FL_ADDFILTER_INVALID; // just in case
  // look for duplicates
  for (auto &&flt : fsys_game_filters) if (flt.strEquCI(path)) return FL_ADDFILTER_DUPLICATE;
  fsys_game_filters.append(path);
  // sort them, because why not?
  //GLog.Log(NAME_Debug, ":::: B: ===="); for (auto &&s : fsys_game_filters) GLog.Logf(NAME_Debug, "  <%s>", *s);
  timsort_r(fsys_game_filters.ptr(), fsys_game_filters.length(), sizeof(VStr), &cmpGameFilter, nullptr);
  //GLog.Log(NAME_Debug, ":::: A: ===="); for (auto &&s : fsys_game_filters) GLog.Logf(NAME_Debug, "  <%s>", *s);
  return FL_ADDFILTER_OK;
}


//==========================================================================
//
//  FL_CheckFilterName
//
//  returns `false` if file was filtered out (and clears name)
//  returns `true` if file should be kept (and modifies name if necessary)
//
//==========================================================================
bool FL_CheckFilterName (VStr &fname) {
  if (fname.isEmpty()) return false; // empty names should not be kept ;-)
  if (fsys_game_filters.length() == 0) return true; // keep it
  if (!fname.startsWithNoCase("filter/")) return true; // keep it
  if (fname.endsWith("/")) { fname.clear(); return false; } // drop it (it is a directory)
  // latest filter is always the best one
  for (int f = fsys_game_filters.length()-1; f >= 0; --f) {
    VStr fs = fsys_game_filters[f];
    if (fname.length() > fs.length()+1 && fname[fs.length()] == '/' && fname.startsWithNoCase(fs)) {
      fname.chopLeft(fs.length()+1);
      while (fname.length() && fname[0] == '/') fname.chopLeft(1);
      return !fname.isEmpty(); // just in case
    }
  }
  // drop it
  fname.clear();
  return false;
}


//==========================================================================
//
//  FL_FileExists
//
//==========================================================================
bool FL_FileExists (VStr fname, int *lump) {
  if (!fname.isEmpty()) {
    MyThreadLocker glocker(&fsys_glock);
    for (int i = fsysSearchPaths.length()-1; i >= 0; --i) {
      if (fsysSearchPaths[i]->FileExists(fname, lump)) return true;
    }
  }
  if (lump) *lump = -1;
  return false;
}


//==========================================================================
//
//  FL_OpenFileReadBaseOnly_NoLock
//
//  if `lump` is not `nullptr`, sets it to file lump or to -1
//
//==========================================================================
VStream *FL_OpenFileReadBaseOnly_NoLock (VStr Name, int *lump) {
  if (!Name.isEmpty()) {
    for (int i = fsysSearchPaths.length()-1; i >= 0; --i) {
      if (!fsysSearchPaths[i]->basepak) continue;
      VStream *Strm = fsysSearchPaths[i]->OpenFileRead(Name, lump);
      if (Strm) return Strm;
    }
  }
  if (lump) *lump = -1;
  return nullptr;
}


//==========================================================================
//
//  FL_OpenFileRead_NoLock
//
//  if `lump` is not `nullptr`, sets it to file lump or to -1
//
//==========================================================================
VStream *FL_OpenFileRead_NoLock (VStr Name, int *lump) {
  if (!Name.isEmpty()) {
    if (Name.length() >= 2 && Name[0] == '/' && Name[1] == '/') {
      return FL_OpenFileReadBaseOnly_NoLock(Name.mid(2, Name.length()), lump);
    } else {
      for (int i = fsysSearchPaths.length()-1; i >= 0; --i) {
        VStream *Strm = fsysSearchPaths[i]->OpenFileRead(Name, lump);
        if (Strm) return Strm;
      }
    }
  }
  if (lump) *lump = -1;
  return nullptr;
}


//==========================================================================
//
//  FL_OpenFileReadBaseOnly
//
//  if `lump` is not `nullptr`, sets it to file lump or to -1
//
//==========================================================================
VStream *FL_OpenFileReadBaseOnly (VStr Name, int *lump) {
  if (Name.isEmpty()) {
    if (lump) *lump = -1;
    return nullptr;
  }
  MyThreadLocker glocker(&fsys_glock);
  return FL_OpenFileReadBaseOnly_NoLock(Name, lump);
}


//==========================================================================
//
//  FL_OpenFileRead
//
//  if `lump` is not `nullptr`, sets it to file lump or to -1
//
//==========================================================================
VStream *FL_OpenFileRead (VStr Name, int *lump) {
  if (Name.isEmpty()) {
    if (lump) *lump = -1;
    return nullptr;
  }
  MyThreadLocker glocker(&fsys_glock);
  return FL_OpenFileRead_NoLock(Name, lump);
}


//==========================================================================
//
//  FL_CreatePath
//
//==========================================================================
void FL_CreatePath (VStr Path) {
  if (Path.isEmpty() || Path == ".") return;
  TArray<VStr> spp;
  Path.SplitPath(spp);
  if (spp.length() == 0 || (spp.length() == 1 && spp[0] == "/")) return;
  if (spp.length() > 0) {
    VStr newpath;
    for (int pos = 0; pos < spp.length(); ++pos) {
      if (newpath.length() && newpath[newpath.length()-1] != '/') newpath += "/";
      newpath += spp[pos];
      if (!Sys_DirExists(newpath)) Sys_CreateDirectory(newpath);
    }
  }
}


//==========================================================================
//
//  FL_OpenSysFileRead
//
//==========================================================================
VStream *FL_OpenSysFileRead (VStr Name) {
  if (Name.isEmpty()) return nullptr;
  return CreateDiskStreamRead(Name);
}


//==========================================================================
//
//  FL_OpenSysFileWrite
//
//==========================================================================
VStream *FL_OpenSysFileWrite (VStr Name) {
  if (Name.isEmpty()) return nullptr;
  FL_CreatePath(Name.ExtractFilePath());
  return CreateDiskStreamWrite(Name);
}


//==========================================================================
//
//  FL_IsSafeDiskFileName
//
//==========================================================================
bool FL_IsSafeDiskFileName (VStr fname) {
  if (fname.isEmpty()) return false;
  return fname.isSafeDiskFileName();
}


// ////////////////////////////////////////////////////////////////////////// //
// i have to do this, otherwise the linker will optimise openers away
#include "fsys_register.cpp"
