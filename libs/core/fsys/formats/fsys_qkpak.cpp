//**************************************************************************
//**
//**    ##   ##    ##    ##   ##   ####     ####   ###     ###
//**    ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**     ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**     ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**      ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**       #    ##    ##    #      ####     ####   ##       ##
//**
//**  Copyright (C) 2018-2019 Ketmar Dark
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
#include "../fsys_local.h"


//==========================================================================
//
//  VQuakePakFile::VQuakePakFile
//
//  takes ownership
//
//==========================================================================
VQuakePakFile::VQuakePakFile (VStream *fstream)
  : VPakFileBase("<memory>", true)
{
  mythread_mutex_init(&rdlock);
  if (fstream->GetName().length()) PakFileName = fstream->GetName();
  OpenArchive(fstream);
}


//==========================================================================
//
//  VQuakePakFile::VQuakePakFile
//
//  takes ownership
//
//==========================================================================
VQuakePakFile::VQuakePakFile (VStream *fstream, VStr name, int signtype)
  : VPakFileBase(name, true)
{
  mythread_mutex_init(&rdlock);
  OpenArchive(fstream, signtype);
}


//==========================================================================
//
//  VQuakePakFile::VQuakePakFile
//
//==========================================================================
VQuakePakFile::VQuakePakFile (VStr zipfile)
  : VPakFileBase(zipfile, true)
{
  mythread_mutex_init(&rdlock);
  if (fsys_report_added_paks) GLog.Logf(NAME_Init, "Adding \"%s\"...", *PakFileName);
  auto fstream = FL_OpenSysFileRead(PakFileName);
  vassert(fstream);
  OpenArchive(fstream);
}


//==========================================================================
//
//  VQuakePakFile::~VQuakePakFile
//
//==========================================================================
VQuakePakFile::~VQuakePakFile () {
  Close();
  mythread_mutex_destroy(&rdlock);
}


//==========================================================================
//
//  VQuakePakFile::VQuakePakFile
//
//==========================================================================
void VQuakePakFile::OpenArchive (VStream *fstream, int signtype) {
  Stream = fstream;
  vassert(Stream);

  bool isSinPack = false;

  //Stream->Seek(0);
  if (!signtype) {
    char sign[4];
    memset(sign, 0, 4);
    Stream->Serialise(sign, 4);
         if (memcmp(sign, "PACK", 4) == 0) isSinPack = false;
    else if (memcmp(sign, "SPAK", 4) == 0) isSinPack = true;
    else Sys_Error("not a quake pak file \"%s\"", *PakFileName);
  } else {
    isSinPack = (signtype > 1);
  }

  vuint32 dirofs;
  vuint32 dirsize;

  *Stream << dirofs << dirsize;

  if (!isSinPack) dirsize /= 64;

  char namebuf[121];
  vuint32 ofs, size;

  Stream->Seek(dirofs);
  if (Stream->IsError()) Sys_Error("cannot read quake pak file \"%s\"", *PakFileName);

  while (dirsize > 0) {
    --dirsize;
    memset(namebuf, 0, sizeof(namebuf));
    if (!isSinPack) {
      Stream->Serialise(namebuf, 56);
    } else {
      Stream->Serialise(namebuf, 120);
    }
    *Stream << ofs << size;
    if (Stream->IsError()) Sys_Error("cannot read quake pak file \"%s\"", *PakFileName);

    VStr zfname = VStr(namebuf).ToLower().FixFileSlashes();

    // fix some idiocity introduced by some shitdoze doom tools
    for (;;) {
           if (zfname.startsWith("./")) zfname.chopLeft(2);
      else if (zfname.startsWith("../")) zfname.chopLeft(3);
      else if (zfname.startsWith("/")) zfname.chopLeft(1);
      else break;
    }
    if (zfname.length() == 0 || zfname.endsWith("/")) continue; // something strange

    VPakFileInfo fi;
    fi.fileName = zfname;
    fi.pakdataofs = ofs;
    fi.filesize = size;
    pakdir.append(fi);
  }

  pakdir.buildLumpNames();
  pakdir.buildNameMaps();
}


//==========================================================================
//
//  VQuakePakFile::Close
//
//==========================================================================
void VQuakePakFile::Close () {
  VPakFileBase::Close();
  if (Stream) { delete Stream; Stream = nullptr; }
}


//==========================================================================
//
//  VQuakePakFile::CreateLumpReaderNum
//
//==========================================================================
VStream *VQuakePakFile::CreateLumpReaderNum (int Lump) {
  vassert(Lump >= 0);
  vassert(Lump < pakdir.files.length());
  const VPakFileInfo &fi = pakdir.files[Lump];
  // this is mt-protected
  VStream *S = new VPartialStreamRO(GetPrefix()+":"+fi.fileName, Stream, fi.pakdataofs, fi.filesize, &rdlock);
  return S;
}
