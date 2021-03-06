//**************************************************************************
//**
//**    ##   ##    ##    ##   ##   ####     ####   ###     ###
//**    ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**     ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**     ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**      ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**       #    ##    ##    #      ####     ####   ##       ##
//**
//**  Copyright (C) 1999-2010 Jānis Legzdiņš
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
#include "../core.h"
//#define VV_ANDROID_TEST

#ifdef ANDROID
#  include <SDL.h>
#  include <android/asset_manager.h>
#  include <android/asset_manager_jni.h>
#  include <jni.h>
#endif


#if defined(ANDROID) || defined(VV_ANDROID_TEST)

// note that some code copypasted from syslow.cpp

#ifndef VV_ANDROID_TEST
static AAssetManager *androidAssetManager;

static void getApkAssetManager () {
  if (androidAssetManager == nullptr) {
    JNIEnv *env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    assert(env != nullptr);
    jobject activity = (jobject)SDL_AndroidGetActivity();
    assert(activity != nullptr);
    jclass clazz(env->GetObjectClass(activity));
    //jclass clazz(env->FindClass("android/view/ContextThemeWrapper"));
    assert(clazz != nullptr);
    jmethodID method = env->GetMethodID(clazz, "getAssets", "()Landroid/content/res/AssetManager;");
    assert(method != nullptr);
    jobject assetManager = env->CallObjectMethod(activity, method);
    assert(assetManager != nullptr);
    androidAssetManager = AAssetManager_fromJava(env, assetManager);
    env->DeleteLocalRef(activity);
    env->DeleteLocalRef(clazz);
  }
}

static __attribute__((unused)) inline bool isApkPath (VStr s) noexcept {
  return (s.length() >= 1 && (s[0] == '.') && (s.length() == 1 || s[1] == '/'));
}

static __attribute__((unused)) inline VStr getApkPath (VStr s) noexcept {
  return (isApkPath(s) ? VStr(s, 2, s.length()-2) : VStr());
}

#else
#define AAsset  void
static __attribute__((unused)) inline bool isApkPath (VStr s) noexcept { return false; }
static __attribute__((unused)) inline VStr getApkPath (VStr s) noexcept { return s; }
#endif


class VAndroidFileStreamRO : public VStream {
private:
  AAsset *myAss;
  VStr myName;
  off_t myOff;

public:
  VV_DISABLE_COPY(VAndroidFileStreamRO)

  VAndroidFileStreamRO (AAsset *ass, VStr aname);

  virtual ~VAndroidFileStreamRO () override;

  virtual void SetError () override;
  virtual VStr GetName () const override;
  virtual void Seek (int pos) override;
  virtual int Tell () override;
  virtual int TotalSize () override;
  virtual bool Close () override;
  virtual void Serialise (void *buf, int len) override;
};


//==========================================================================
//
//  VAndroidFileStreamRO::VAndroidFileStreamRO
//
//==========================================================================
VAndroidFileStreamRO::VAndroidFileStreamRO (AAsset *ass, VStr aname)
  : myAss(ass)
  , myName(aname)
  , myOff(0)
{
  vassert(myAss != nullptr);
  #ifndef VV_ANDROID_TEST
  if (AAsset_seek(myAss, 0, SEEK_SET) != 0) Sys_Error("VAndroidFileStreamRO: cannot create stream for '%s'", *aname);
  #endif
  bLoading = true;
}


//==========================================================================
//
//  VAndroidFileStreamRO::~VAndroidFileStreamRO
//
//==========================================================================
VAndroidFileStreamRO::~VAndroidFileStreamRO () {
  Close();
}


//==========================================================================
//
//  VAndroidFileStreamRO::SetError
//
//==========================================================================
void VAndroidFileStreamRO::SetError () {
  Close();
  bError = true;
  VStream::SetError();
}


//==========================================================================
//
//  VAndroidFileStreamRO::GetName
//
//==========================================================================
VStr VAndroidFileStreamRO::GetName () const {
  return myName.cloneUnique();
}


//==========================================================================
//
//  VAndroidFileStreamRO::Seek
//
//==========================================================================
void VAndroidFileStreamRO::Seek (int pos) {
  #ifndef VV_ANDROID_TEST
  if (myAss == nullptr || AAsset_seek(myAss, pos, SEEK_SET) == -1) {
    SetError();
  } else {
    myOff = pos;
  }
  #else
  SetError();
  #endif
}


//==========================================================================
//
//  VAndroidFileStreamRO::Tell
//
//==========================================================================
int VAndroidFileStreamRO::Tell () {
  return (myAss ? myOff : 0);
}


//==========================================================================
//
//  VAndroidFileStreamRO::TotalSize
//
//==========================================================================
int VAndroidFileStreamRO::TotalSize () {
  #ifndef VV_ANDROID_TEST
  return (myAss ? (int)AAsset_getLength(myAss) : 0);
  #else
  return 0;
  #endif
}


//==========================================================================
//
//  VAndroidFileStreamRO::Close
//
//==========================================================================
bool VAndroidFileStreamRO::Close () {
  if (myAss) {
    #ifndef VV_ANDROID_TEST
    AAsset_close(myAss);
    #endif
    myAss = nullptr;
  }
  myName.clear();
  return !bError;
}


//==========================================================================
//
//  VAndroidFileStreamRO::Serialise
//
//==========================================================================
void VAndroidFileStreamRO::Serialise (void *buf, int len) {
  vassert(buf != nullptr);
  vassert(len >= 0);
  #ifndef VV_ANDROID_TEST
  if (AAsset_read(myAss, buf, len) != len) Sys_Error("VAndroidFileStreamRO: cannot read %d bytes from '%s'", len, *myName);
  #endif
  myOff += len;
}


//==========================================================================
//
//  openAndroidFileStreamRO
//
//==========================================================================
static __attribute__((unused)) VAndroidFileStreamRO *openAndroidFileStreamRO (VStr aname, VStr streamname) {
  #ifndef VV_ANDROID_TEST
  if (isApkPath(aname)) {
    getApkAssetManager();
    if (androidAssetManager) {
      VStr p = getApkPath(aname);
      AAsset *a = AAssetManager_open(androidAssetManager, *p, AASSET_MODE_UNKNOWN);
      if (a) return new VAndroidFileStreamRO(a, streamname);
    }
  }
  #endif
  return nullptr;
}
#endif // ANDROID


//==========================================================================
//
//  VMemoryStreamRO::VMemoryStreamRO
//
//==========================================================================
VMemoryStreamRO::VMemoryStreamRO ()
  : Data(nullptr)
  , Pos(0)
  , DataSize(0)
  , FreeData(false)
{
  bLoading = true;
}


//==========================================================================
//
//  VMemoryStreamRO::VMemoryStreamRO
//
//==========================================================================
VMemoryStreamRO::VMemoryStreamRO (VStr strmName, const void *adata, int adataSize, bool takeOwnership)
  : Data((const vuint8 *)adata)
  , Pos(0)
  , DataSize(adataSize)
  , FreeData(takeOwnership)
  , StreamName(strmName)
{
  vassert(adataSize >= 0);
  vassert(adataSize == 0 || (adataSize > 0 && adata));
  bLoading = true;
}


//==========================================================================
//
//  VMemoryStreamRO::VMemoryStreamRO
//
//==========================================================================
VMemoryStreamRO::VMemoryStreamRO (VStr strmName, VStream *strm)
  : Data(nullptr)
  , Pos(0)
  , DataSize(0)
  , FreeData(false)
  , StreamName()
{
  bLoading = true;
  Setup(strmName, strm);
}


//==========================================================================
//
//  VMemoryStreamRO::VMemoryStreamRO
//
//==========================================================================
VMemoryStreamRO::~VMemoryStreamRO () {
  Close();
}


//==========================================================================
//
//  VMemoryStreamRO::Clear
//
//==========================================================================
void VMemoryStreamRO::Clear () {
  if (FreeData && Data) Z_Free((void *)Data);
  Data = nullptr;
  Pos = 0;
  DataSize = 0;
  FreeData = false;
  bError = false;
  StreamName.clear();
}


//==========================================================================
//
//  VMemoryStreamRO::Setup
//
//==========================================================================
void VMemoryStreamRO::Setup (VStr strmName, const void *adata, int adataSize, bool takeOwnership) {
  vassert(!Data);
  vassert(Pos == 0);
  vassert(DataSize == 0);
  vassert(!bError);
  vassert(adataSize >= 0);
  vassert(adataSize == 0 || (adataSize > 0 && adata));
  vassert(StreamName.length() == 0);
  StreamName = strmName;
  Data = (const vuint8 *)adata;
  DataSize = adataSize;
  FreeData = takeOwnership;
}


//==========================================================================
//
//  VMemoryStreamRO::Setup
//
//  from current position to stream end
//
//==========================================================================
void VMemoryStreamRO::Setup (VStr strmName, VStream *strm) {
  vassert(!Data);
  vassert(Pos == 0);
  vassert(DataSize == 0);
  vassert(!bError);
  vassert(StreamName.length() == 0);
  StreamName = strmName;
  if (strm) {
    vassert(strm->IsLoading());
    int tsz = strm->TotalSize();
    vassert(tsz >= 0);
    int cpos = strm->Tell();
    vassert(cpos >= 0);
    if (cpos < tsz) {
      int len = tsz-cpos;
      void *dta = Z_Malloc(len);
      strm->Serialize(dta, len);
      bError = strm->IsError();
      if (bError) {
        Z_Free(dta);
      } else {
        Data = (const vuint8 *)dta;
        DataSize = len;
        FreeData = true;
      }
    }
  }
}


//==========================================================================
//
//  VMemoryStreamRO::Serialise
//
//==========================================================================
void VMemoryStreamRO::Serialise (void *buf, int Len) {
  vassert(bLoading);
  vassert(Len >= 0);
  if (Len == 0) return;
  vassert(buf);
  if (Pos >= DataSize) {
    bError = true;
    return;
  }
  if (Len > DataSize-Pos) {
    // too much
    Len = DataSize-Pos;
    bError = true;
  }
  if (Len) {
    memcpy(buf, Data+Pos, Len);
    Pos += Len;
  }
}


//==========================================================================
//
//  VMemoryStreamRO::Seek
//
//==========================================================================
void VMemoryStreamRO::Seek (int InPos) {
  if (InPos < 0) {
    bError = true;
    Pos = 0;
  } else if (InPos > DataSize) {
    bError = true;
    Pos = DataSize;
  } else {
    Pos = InPos;
  }
}


//==========================================================================
//
//  VMemoryStreamRO::Tell
//
//==========================================================================
int VMemoryStreamRO::Tell () {
  return Pos;
}


//==========================================================================
//
//  VMemoryStreamRO::TotalSize
//
//==========================================================================
int VMemoryStreamRO::TotalSize () {
  return DataSize;
}


//==========================================================================
//
//  VMemoryStreamRO::GetName
//
//==========================================================================
VStr VMemoryStreamRO::GetName () const {
  return StreamName.cloneUnique();
}



//==========================================================================
//
//  VMemoryStream::VMemoryStream
//
//==========================================================================
VMemoryStream::VMemoryStream ()
  : Pos(0)
  , StreamName()
{
  bLoading = false;
}


//==========================================================================
//
//  VMemoryStream::VMemoryStream
//
//==========================================================================
VMemoryStream::VMemoryStream (VStr strmName)
  : Pos(0)
  , StreamName(strmName)
{
  bLoading = false;
}


//==========================================================================
//
//  VMemoryStream::VMemoryStream
//
//==========================================================================
VMemoryStream::VMemoryStream (VStr strmName, const void *InData, int InLen, bool takeOwnership)
  : Pos(0)
  , StreamName(strmName)
{
  if (InLen < 0) InLen = 0;
  bLoading = true;
  if (!takeOwnership) {
    Array.SetNum(InLen);
    if (InLen) memcpy(Array.Ptr(), InData, InLen);
  } else {
    Array.SetPointerData((void *)InData, InLen);
    vassert(Array.length() == InLen);
  }
}


//==========================================================================
//
//  VMemoryStream::VMemoryStream
//
//==========================================================================
VMemoryStream::VMemoryStream (VStr strmName, const TArray<vuint8> &InArray)
  : Pos(0)
  , StreamName(strmName)
{
  bLoading = true;
  Array = InArray;
}


//==========================================================================
//
//  VMemoryStream::VMemoryStream
//
//==========================================================================
VMemoryStream::VMemoryStream (VStr strmName, VStream *strm)
  : Pos(0)
  , StreamName(strmName)
{
  bLoading = true;
  if (strm) {
    vassert(strm->IsLoading());
    int tsz = strm->TotalSize();
    vassert(tsz >= 0);
    int cpos = strm->Tell();
    vassert(cpos >= 0);
    if (cpos < tsz) {
      int len = tsz-cpos;
      Array.setLength(len);
      strm->Serialize(Array.ptr(), len);
      bError = strm->IsError();
    }
  }
}


//==========================================================================
//
//  VMemoryStream::~VMemoryStream
//
//==========================================================================
VMemoryStream::~VMemoryStream () {
  Close();
}


//==========================================================================
//
//  VMemoryStream::Close
//
//==========================================================================
bool VMemoryStream::Close () {
  if (bLoading) {
    Array.setLength(0);
    Pos = 0;
  }
  return !bError;
}


//==========================================================================
//
//  VMemoryStream::Serialise
//
//==========================================================================
void VMemoryStream::Serialise (void *Data, int Len) {
  vassert(Len >= 0);
  if (Len == 0) return;
  const int alen = Array.length();
  if (bLoading) {
    if (Pos >= alen) {
      bError = true;
      return;
    }
    if (Len > alen-Pos) {
      // too much
      Len = alen-Pos;
      bError = true;
    }
    if (Len) {
      memcpy(Data, &Array[Pos], Len);
      Pos += Len;
    }
  } else {
    if (Pos+Len > alen) Array.SetNumWithReserve(Pos+Len);
    memcpy(&Array[Pos], Data, Len);
    Pos += Len;
  }
}


//==========================================================================
//
//  VMemoryStream::Seek
//
//==========================================================================
void VMemoryStream::Seek (int InPos) {
  if (InPos < 0) {
    bError = true;
    Pos = 0;
  } else if (InPos > Array.length()) {
    bError = true;
    Pos = Array.length();
  } else {
    Pos = InPos;
  }
}


//==========================================================================
//
//  VMemoryStream::Tell
//
//==========================================================================
int VMemoryStream::Tell () {
  return Pos;
}


//==========================================================================
//
//  VMemoryStream::TotalSize
//
//==========================================================================
int VMemoryStream::TotalSize () {
  return Array.length();
}


//==========================================================================
//
//  VMemoryStream::GetName
//
//==========================================================================
VStr VMemoryStream::GetName () const {
  return StreamName.cloneUnique();
}



//==========================================================================
//
//  VArrayStream::VArrayStream
//
//==========================================================================
VArrayStream::VArrayStream (VStr strmName, TArray<vuint8>& InArray)
  : Array(InArray)
  , Pos(0)
  , StreamName(strmName)
{
  bLoading = true;
}


//==========================================================================
//
//  VArrayStream::~VArrayStream
//
//==========================================================================
VArrayStream::~VArrayStream () {
  Close();
}


//==========================================================================
//
//  VArrayStream::Serialise
//
//==========================================================================
void VArrayStream::Serialise (void *Data, int Len) {
  vassert(Len >= 0);
  if (Len == 0) return;
  const int alen = Array.length();
  if (bLoading) {
    if (Pos >= alen) {
      bError = true;
      return;
    }
    if (Len > alen-Pos) {
      // too much
      Len = alen-Pos;
      bError = true;
    }
    if (Len) {
      memcpy(Data, &Array[Pos], Len);
      Pos += Len;
    }
  } else {
    if (Pos+Len > alen) Array.SetNumWithReserve(Pos+Len);
    memcpy(&Array[Pos], Data, Len);
    Pos += Len;
  }
}


//==========================================================================
//
//  VArrayStream::Seek
//
//==========================================================================
void VArrayStream::Seek (int InPos) {
  if (InPos < 0) {
    bError = true;
    Pos = 0;
  } else if (InPos > Array.length()) {
    bError = true;
    Pos = Array.length();
  } else {
    Pos = InPos;
  }
}


//==========================================================================
//
//  VArrayStream::Tell
//
//==========================================================================
int VArrayStream::Tell () {
  return Pos;
}


//==========================================================================
//
//  VArrayStream::TotalSize
//
//==========================================================================
int VArrayStream::TotalSize () {
  return Array.length();
}


//==========================================================================
//
//  VArrayStream::GetName
//
//==========================================================================
VStr VArrayStream::GetName () const {
  return StreamName.cloneUnique();
}



//==========================================================================
//
//  VPagedMemoryStream::VPagedMemoryStream
//
//  initialise empty writing stream
//
//==========================================================================
VPagedMemoryStream::VPagedMemoryStream (VStr strmName)
  : first(nullptr)
  , curr(nullptr)
  , pos(0)
  , size(0)
  , StreamName(strmName)
{
  bLoading = false;
}


//==========================================================================
//
//  VPagedMemoryStream::~VPagedMemoryStream
//
//==========================================================================
VPagedMemoryStream::~VPagedMemoryStream () {
  Close();
}


//==========================================================================
//
//  VPagedMemoryStream::Close
//
//==========================================================================
bool VPagedMemoryStream::Close () {
  while (first) {
    vuint8 *next = *(vuint8 **)first;
    Z_Free(first);
    first = next;
  }
  first = curr = nullptr;
  pos = size = 0;
  return !bError;
}


//==========================================================================
//
//  VPagedMemoryStream::GetName
//
//==========================================================================
VStr VPagedMemoryStream::GetName () const {
  return StreamName.cloneUnique();
}


//==========================================================================
//
//  VPagedMemoryStream::Serialise
//
//==========================================================================
void VPagedMemoryStream::Serialise (void *bufp, int count) {
  vassert(count >= 0);
  if (count == 0 || bError) return;
  int leftInPage = DataPerPage-pos%DataPerPage;
  vuint8 *buf = (vuint8 *)bufp;
  if (bLoading) {
    // loading
    if (pos >= size) { bError = true; return; }
    if (count > size-pos) { count = size-pos; bError = true; }
    while (count > 0) {
      if (count <= leftInPage) {
        memcpy(buf, curr+sizeof(vuint8 *)+pos%DataPerPage, count);
        pos += count;
        break;
      }
      memcpy(buf, curr+sizeof(vuint8 *)+pos%DataPerPage, leftInPage);
      pos += leftInPage;
      count -= leftInPage;
      buf += leftInPage;
      vassert(pos%DataPerPage == 0);
      curr = *(vuint8 **)curr; // next page
      leftInPage = DataPerPage; // it is safe to overestimate here
    }
  } else {
    // writing
    while (count > 0) {
      if (leftInPage == DataPerPage) {
        // need new page
        if (first) {
          if (pos != 0) {
            vuint8 *next = *(vuint8 **)curr;
            if (!next) {
              // allocate next page
              next = (vuint8 *)Z_Malloc(FullPageSize);
              *(vuint8 **)next = nullptr; // no next page
              *(vuint8 **)curr = next; // pointer to next page
            }
            curr = next;
          } else {
            curr = first;
          }
        } else {
          // allocate first page
          first = curr = (vuint8 *)Z_Malloc(FullPageSize);
          *(vuint8 **)first = nullptr; // no next page
        }
      }
      if (count <= leftInPage) {
        memcpy(curr+sizeof(vuint8 *)+pos%DataPerPage, buf, count);
        pos += count;
        break;
      }
      memcpy(curr+sizeof(vuint8 *)+pos%DataPerPage, buf, leftInPage);
      pos += leftInPage;
      count -= leftInPage;
      buf += leftInPage;
      vassert(pos%DataPerPage == 0);
      leftInPage = DataPerPage;
    }
    if (size < pos) size = pos;
  }
}


//==========================================================================
//
//  VPagedMemoryStream::Seek
//
//==========================================================================
void VPagedMemoryStream::Seek (int newpos) {
  if (bError) return;
  if (newpos < 0 || newpos > size) { bError = true; return; }
  if (bLoading) {
    // loading
  } else {
    // writing is somewhat special:
    // we don't have to go to next page if pos is at its first byte
    if (newpos <= DataPerPage) {
      // in the first page
      curr = first;
    } else {
      // walk pages
      int pgcount = newpos/DataPerPage;
      // if we are at page boundary, do one step less
      if (newpos%DataPerPage == 0) {
        --pgcount;
        vassert(pgcount > 0);
      }
      curr = first;
      while (pgcount--) curr = *(vuint8 **)curr;
    }
  }
  pos = newpos;
}


//==========================================================================
//
//  VPagedMemoryStream::Tell
//
//==========================================================================
int VPagedMemoryStream::Tell () {
  return pos;
}


//==========================================================================
//
//  VPagedMemoryStream::TotalSize
//
//==========================================================================
int VPagedMemoryStream::TotalSize () {
  return size;
}


//==========================================================================
//
//  VPagedMemoryStream::CopyTo
//
//==========================================================================
void VPagedMemoryStream::CopyTo (VStream *strm) {
  if (!strm) return;
  int left = size;
  vuint8 *cpg = first;
  while (left > 0) {
    int wrt = (left > DataPerPage ? DataPerPage : left);
    strm->Serialise(cpg+sizeof(vuint8 *), wrt);
    if (strm->IsError()) return;
    left -= wrt;
    cpg = *(vuint8 **)cpg; // next page
  }
}



//==========================================================================
//
//  VStdFileStreamBase::VStdFileStreamBase
//
//==========================================================================
VStdFileStreamBase::VStdFileStreamBase (FILE* afl, VStr aname, bool asWriter)
  : mFl(afl)
  , mName(aname)
  , size(-1)
{
  bLoading = !asWriter;
  if (afl && !asWriter) {
    if (fseek(afl, 0, SEEK_SET)) SetError();
  }
}


//==========================================================================
//
//  VStdFileStreamBase::~VStdFileStreamBase
//
//==========================================================================
VStdFileStreamBase::~VStdFileStreamBase () {
  Close();
}


//==========================================================================
//
//  VStdFileStreamBase::Close
//
//==========================================================================
bool VStdFileStreamBase::Close () {
  if (mFl) {
    //if (!bLoading) fflush(mFl);
    fclose(mFl);
    mFl = nullptr;
  }
  //fflush(stderr);
  mName.clear();
  return !bError;
}


//==========================================================================
//
//  VStdFileStreamBase::SetError
//
//==========================================================================
void VStdFileStreamBase::SetError () {
  if (mFl) {
    //fflush(stderr);
    fclose(mFl);
    mFl = nullptr;
  }
  mName.clear();
  bError = true;
  VStream::SetError(); // just in case
}


//==========================================================================
//
//  VStdFileStreamBase::GetName
//
//==========================================================================
VStr VStdFileStreamBase::GetName () const {
  return mName.cloneUnique();
}


//==========================================================================
//
//  VStdFileStreamBase::Seek
//
//==========================================================================
void VStdFileStreamBase::Seek (int pos) {
  if (!mFl) { SetError(); return; }
  if (fseek(mFl, pos, SEEK_SET)) SetError();
}


//==========================================================================
//
//  VStdFileStreamBase::Tell
//
//==========================================================================
int VStdFileStreamBase::Tell () {
  if (mFl && !bError) {
    int res = (int)ftell(mFl);
    if (res < 0) { SetError(); return 0; }
    return res;
  } else {
    SetError();
    return 0;
  }
}


//==========================================================================
//
//  VStdFileStreamBase::TotalSize
//
//==========================================================================
int VStdFileStreamBase::TotalSize () {
  if (!mFl || bError) { SetError(); return 0; }
  if (!IsLoading()) size = -1;
  if (size < 0) {
    auto opos = ftell(mFl);
    fseek(mFl, 0, SEEK_END);
    size = (int)ftell(mFl);
    fseek(mFl, opos, SEEK_SET);
  }
  return size;
}


//==========================================================================
//
//  VStdFileStreamBase::AtEnd
//
//==========================================================================
bool VStdFileStreamBase::AtEnd () {
  return (bError || !mFl || Tell() >= TotalSize());
}


//==========================================================================
//
//  VStdFileStreamBase::Serialise
//
//==========================================================================
void VStdFileStreamBase::Serialise (void *buf, int len) {
  if (bError || !mFl || len < 0) { SetError(); return; }
  if (len == 0) return;
  if (bLoading) {
    if (fread(buf, len, 1, mFl) != 1) SetError();
  } else {
    if (fwrite(buf, len, 1, mFl) != 1) SetError();
  }
}



// ////////////////////////////////////////////////////////////////////////// //
//  VPartialStreamRO::VPartialStreamRO
// ////////////////////////////////////////////////////////////////////////// //

//==========================================================================
//
//  VPartialStreamRO::VPartialStreamRO
//
//==========================================================================
VPartialStreamRO::VPartialStreamRO (VStream *ASrcStream, int astpos, int apartlen, bool aOwnSrc)
  : lockptr(nullptr)
  , srcStream(ASrcStream)
  , stpos(astpos)
  , srccurpos(astpos)
  , partlen(apartlen)
  , srcOwned(aOwnSrc)
  , closed(false)
  , myname()
{
  lockptr = &lock;
  mythread_mutex_init(lockptr);
  bLoading = true;
  if (!srcStream) { srcOwned = false; bError = true; return; }
  {
    MyThreadLocker locker(lockptr);
    if (!srcStream->IsLoading()) {
      bError = true;
      partlen = 0;
    } else {
      if (partlen < 0) {
        partlen = srcStream->TotalSize()-stpos;
             if (partlen < 0) { partlen = 0; bError = true; }
        else if (srcStream->IsError()) bError = true;
      }
    }
  }
}


//==========================================================================
//
//  VPartialStreamRO::VPartialStreamRO
//
//==========================================================================
VPartialStreamRO::VPartialStreamRO (VStr aname, VStream *ASrcStream, int astpos, int apartlen, mythread_mutex *alockptr)
  : lockptr(alockptr)
  , srcStream(ASrcStream)
  , stpos(astpos)
  , srccurpos(astpos)
  , partlen(apartlen)
  , srcOwned(false)
  , closed(false)
  , myname(aname)
{
  if (!lockptr) {
    lockptr = &lock;
    mythread_mutex_init(lockptr);
  }
  bLoading = true;
  if (partlen < 0 || !srcStream) { bError = true; return; }
  {
    MyThreadLocker locker(lockptr);
    if (srcStream->IsError() || !srcStream->IsLoading()) { bError = true; return; }
  }
}


//==========================================================================
//
//  VPartialStreamRO::~VPartialStreamRO
//
//==========================================================================
VPartialStreamRO::~VPartialStreamRO () {
  Close();
  if (srcOwned && srcStream) {
    MyThreadLocker locker(lockptr);
    delete srcStream;
  }
  srcOwned = false;
  srcStream = nullptr;
  if (lockptr == &lock) mythread_mutex_destroy(lockptr);
  lockptr = nullptr;
}


//==========================================================================
//
//  VPartialStreamRO::Close
//
//==========================================================================
bool VPartialStreamRO::Close () {
  {
    MyThreadLocker locker(lockptr);
    if (!closed) {
      closed = true;
      myname.clear();
    }
  }
  return !bError;
}


//==========================================================================
//
//  VPartialStreamRO::GetName
//
//==========================================================================
VStr VPartialStreamRO::GetName () const {
  if (!closed) {
    if (!myname.isEmpty()) return myname.cloneUnique();
    if (srcStream) {
      MyThreadLocker locker(lockptr);
      return srcStream->GetName();
    }
  }
  return VStr();
}


//==========================================================================
//
//  VPartialStreamRO::IsError
//
//==========================================================================
bool VPartialStreamRO::IsError () const {
  if (lockptr) {
    MyThreadLocker locker(lockptr);
    return bError;
  } else {
    return bError;
  }
}


//==========================================================================
//
//  VPartialStreamRO::checkValidityCond
//
//==========================================================================
bool VPartialStreamRO::checkValidityCond (bool mustBeTrue) noexcept {
  if (!lockptr) { bError = true; return false; }
  {
    MyThreadLocker locker(lockptr);
    if (!bError) {
      if (!mustBeTrue || closed || !srcStream || srcStream->IsError()) bError = true;
    }
    if (!bError) {
      if (srccurpos < stpos || srccurpos > stpos+partlen) bError = true;
    }
    if (bError) return false;
  }
  return true;
}


//==========================================================================
//
//  VPartialStreamRO::Serialise
//
//==========================================================================
void VPartialStreamRO::Serialise (void *buf, int len) {
  if (!checkValidityCond(len >= 0)) return;
  if (len == 0) return;
  {
    MyThreadLocker locker(lockptr);
    if (stpos+partlen-srccurpos < len) { bError = true; return; }
    if (srcStream->Tell() != srccurpos) srcStream->Seek(srccurpos);
    if (srcStream->IsError()) { bError = true; return; }
    srcStream->Serialise(buf, len);
    if (srcStream->IsError()) { bError = true; return; }
    srccurpos += len;
  }
}


//==========================================================================
//
//  VPartialStreamRO::SerialiseBits
//
//==========================================================================
void VPartialStreamRO::SerialiseBits (void *Data, int Length) {
  if (!checkValidityCond(Length >= 0)) return;
  {
    MyThreadLocker locker(lockptr);
    if (srcStream->Tell() != srccurpos) srcStream->Seek(srccurpos);
    if (srcStream->IsError()) { bError = true; return; }
    srcStream->SerialiseBits(Data, Length);
    int cpos = srcStream->Tell();
    if (srcStream->IsError()) { bError = true; return; }
    if (cpos < stpos) { srccurpos = stpos; bError = true; return; }
    if (cpos > stpos+partlen) { srccurpos = stpos+partlen; bError = true; return; }
    srccurpos = cpos-stpos;
  }
}


void VPartialStreamRO::SerialiseInt (vuint32 &Value/*, vuint32 Max*/) {
  if (!checkValidity()) return;
  {
    MyThreadLocker locker(lockptr);
    if (srcStream->Tell() != srccurpos) srcStream->Seek(srccurpos);
    if (srcStream->IsError()) { bError = true; return; }
    srcStream->SerialiseInt(Value/*, Max*/);
    int cpos = srcStream->Tell();
    if (srcStream->IsError()) { bError = true; return; }
    if (cpos < stpos) { srccurpos = stpos; bError = true; return; }
    if (cpos > stpos+partlen) { srccurpos = stpos+partlen; bError = true; return; }
    srccurpos = cpos-stpos;
  }
}


//==========================================================================
//
//  VPartialStreamRO::Seek
//
//==========================================================================
void VPartialStreamRO::Seek (int pos) {
  if (!checkValidityCond(pos >= 0 && pos <= partlen)) return;
  {
    MyThreadLocker locker(lockptr);
    srccurpos = stpos+pos;
  }
}


//==========================================================================
//
//  VPartialStreamRO::Tell
//
//==========================================================================
int VPartialStreamRO::Tell () {
  if (!checkValidity()) return 0;
  {
    MyThreadLocker locker(lockptr);
    return (srccurpos-stpos);
  }
}


//==========================================================================
//
//  VPartialStreamRO::TotalSize
//
//==========================================================================
int VPartialStreamRO::TotalSize () {
  if (!checkValidity()) return 0;
  {
    MyThreadLocker locker(lockptr);
    return partlen;
  }
}


//==========================================================================
//
//  VPartialStreamRO::AtEnd
//
//==========================================================================
bool VPartialStreamRO::AtEnd () {
  if (!checkValidity()) return true;
  {
    MyThreadLocker locker(lockptr);
    return (srccurpos >= stpos+partlen);
  }
}


//==========================================================================
//
//  VPartialStreamRO::Flush
//
//==========================================================================
void VPartialStreamRO::Flush () {
  if (!checkValidity()) return;
  {
    MyThreadLocker locker(lockptr);
    srcStream->Flush();
    if (srcStream->IsError()) bError = true;
  }
}


#define PARTIAL_DO_IO()  do { \
  if (!checkValidity()) return; \
  { \
    MyThreadLocker locker(lockptr); \
    if (srcStream->Tell() != srccurpos) srcStream->Seek(srccurpos); \
    if (srcStream->IsError()) { bError = true; return; } \
    srcStream->io(v); \
    int cpos = srcStream->Tell(); \
    if (srcStream->IsError()) { bError = true; return; } \
    if (cpos < stpos) { srccurpos = stpos; bError = true; return; } \
    if (cpos > stpos+partlen) { srccurpos = stpos+partlen; bError = true; return; } \
    srccurpos = cpos-stpos; \
  } \
} while (0)


//==========================================================================
//
//  VPartialStreamRO::io
//
//==========================================================================
void VPartialStreamRO::io (VName &v) {
  PARTIAL_DO_IO();
}


//==========================================================================
//
//  VPartialStreamRO::io
//
//==========================================================================
void VPartialStreamRO::io (VStr &v) {
  PARTIAL_DO_IO();
}


//==========================================================================
//
//  VPartialStreamRO::io
//
//==========================================================================
void VPartialStreamRO::io (VObject *&v) {
  PARTIAL_DO_IO();
}


//==========================================================================
//
//  VPartialStreamRO::io
//
//==========================================================================
void VPartialStreamRO::io (VMemberBase *&v) {
  PARTIAL_DO_IO();
}


//==========================================================================
//
//  VPartialStreamRO::io
//
//==========================================================================
void VPartialStreamRO::io (VSerialisable *&v) {
  PARTIAL_DO_IO();
}


//==========================================================================
//
//  VPartialStreamRO::SerialiseStructPointer
//
//==========================================================================
void VPartialStreamRO::SerialiseStructPointer (void *&Ptr, VStruct *Struct) {
  if (!checkValidityCond(!!Ptr && !!Struct)) return;
  {
    MyThreadLocker locker(lockptr);
    if (srcStream->Tell() != srccurpos) srcStream->Seek(srccurpos);
    if (srcStream->IsError()) { bError = true; return; }
    srcStream->SerialiseStructPointer(Ptr, Struct);
    int cpos = srcStream->Tell();
    if (srcStream->IsError()) { bError = true; return; }
    if (cpos < stpos) { srccurpos = stpos; bError = true; return; }
    if (cpos > stpos+partlen) { srccurpos = stpos+partlen; bError = true; return; }
    srccurpos = cpos-stpos;
  }
}


//==========================================================================
//
//  CreateDiskStreamRead
//
//  this function is called by the engine to open disk files
//  return any reading stream or `nullptr` for "file not found"
//
//==========================================================================
VStream *CreateDiskStreamRead (VStr fname, VStr streamname) {
  if (fname.isEmpty()) return nullptr;
  if (streamname.isEmpty()) streamname = fname;
  // here you can put various custom handlers
  //GLog.Logf("CreateDiskStreamRead(%s)", *fname);
  #ifdef ANDROID
  if (isApkPath(fname)) return openAndroidFileStreamRO(fname, streamname);
  #endif
  FILE *fl = fopen(*fname, "rb");
  if (!fl) return nullptr;
  return new VStdFileStreamRead(fl, streamname);
}


//==========================================================================
//
//  CreateDiskStreamWrite
//
//  this function is called by the engine to create disk files
//  return any writing stream or `nullptr` for "cannot create"
//
//==========================================================================
VStream *CreateDiskStreamWrite (VStr fname, VStr streamname) {
  if (fname.isEmpty()) return nullptr;
  if (streamname.isEmpty()) streamname = fname;
  // here you can put various custom handlers
  FILE *fl = fopen(*fname, "wb");
  if (!fl) return nullptr;
  return new VStdFileStreamWrite(fl, streamname);
}
