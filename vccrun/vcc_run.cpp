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

#include <signal.h>
#include <time.h>

#include "vcc_run.h"

#include "modules/mod_sound/sound.h"


// ////////////////////////////////////////////////////////////////////////// //
VObject *mainObject = nullptr;
VStr appName;


// ////////////////////////////////////////////////////////////////////////// //
static VStr buildConfigName (const VStr &optfile) {
  for (int f = 0; f < optfile.length(); ++f) {
    char ch = optfile[f];
    if (ch >= '0' && ch <= '9') continue;
    if (ch >= 'A' && ch <= 'Z') continue;
    if (ch >= 'a' && ch <= 'z') continue;
    if (ch == '_' || ch == ' ' || ch == '.') continue;
    return VStr();
  }
  if (optfile.length()) {
    return VStr(".")+optfile+".cfg";
  } else {
    return VStr(".options.cfg");
  }
}


// ////////////////////////////////////////////////////////////////////////// //
static bool cfgCanIOType (const VFieldType &type);
static bool cfgCanIOClass (VClass *cls, bool ignoreFlags=false);
static bool cfgCanIOStruct (VStruct *st);
static bool cfgCanIOField (VField *fld);
static bool cfgSkipClass (VClass *cls, bool checkName);


static bool cfgCanIOType (const VFieldType &type) {
  switch (type.Type) {
    case TYPE_Int:
    case TYPE_Byte:
    case TYPE_Bool:
    case TYPE_Float:
    case TYPE_Name:
    case TYPE_String:
    case TYPE_Vector:
    case TYPE_Class:
      return true;
    case TYPE_Reference:
      if (cfgSkipClass(type.Class, true)) return true;
      return cfgCanIOClass(type.Class);
    case TYPE_Struct:
      return cfgCanIOStruct(type.Struct);
    case TYPE_Array:
    case TYPE_DynamicArray:
      return cfgCanIOType(type.GetArrayInnerType());
  }
  return false;
}

static bool cfgSkipField (VField *fld) {
  if (!fld || fld->Name == NAME_None) return true;
  if (fld->Flags&(FIELD_Transient|FIELD_ReadOnly)) return true;
  return false;
}

static bool cfgCanIOField (VField *fld) {
  if (cfgSkipField(fld)) return false;
  return cfgCanIOType(fld->Type);
}

// saveable struct either has no fields, or all fields are saveable
static bool cfgCanIOStruct (VStruct *st) {
  if (!st || st->Name == NAME_None) return false;
  for (VStruct *cst = st; cst; cst = cst->ParentStruct) {
    for (VField *fld = cst->Fields; fld; fld = fld->Next) {
      if (cfgSkipField(fld)) continue;
      if (!cfgCanIOField(fld)) return false;
    }
  }
  return true;
}

// saveable class either has no fields, or all fields are saveable
static bool cfgSkipClass (VClass *cls, bool checkName) {
  if (!cls) return true;
  if (checkName && cls->Name == NAME_None) return true;
  if (cls->ClassFlags&(CLASS_Transient|CLASS_Abstract)) return true;
  return false;
}

// saveable class either has no fields, or all fields are saveable
static bool cfgCanIOClass (VClass *cls, bool ignoreFlags) {
  if (!cls || cls->Name == NAME_None) return false;
  if (!ignoreFlags && cfgSkipClass(cls, true)) return false;
  for (VClass *c = cls; c; c = c->ParentClass) {
    if (c != cls && cfgSkipClass(c, false)) break; // transient class breaks chain
    for (VField *fld = c->Fields; fld; fld = fld->Next) {
      if (cfgSkipField(fld)) continue;
      if (!cfgCanIOField(fld)) return false;
    }
  }
  return true;
}


// ////////////////////////////////////////////////////////////////////////// //
class ObjectSaveMap;

static bool cfgSaveObject (ObjectSaveMap &smap, VStream &strm, VObject *obj, bool ignoreFlags);

// build map of objects, strings and names
class ObjectSaveMap {
public:
  TMap<VObject *, vuint32> objmap;
  TArray<VObject *> objarr;
  TMap<VName, vuint32> namemap;
  TArray<VName> namearr;
  TMapDtor<VStr, vuint32> strmap;
  TArray<VStr> strarr;
  vuint32 objCount;
  vuint32 nameCount;
  vuint32 strCount;
  bool wasError;

public:
  ObjectSaveMap (VObject *ao)
    : objmap()
    , objarr()
    , namemap()
    , namearr()
    , strmap()
    , strarr()
    , objCount(0)
    , nameCount(0)
    , strCount(0)
    , wasError(false)
  {
    buildSaveMap(ao);
  }

  VObject *getByIndex (vuint32 idx) {
    if (idx < 1 || idx > objCount) return nullptr;
    return objarr[(vint32)idx-1];
  }

  vuint32 getObjectId (VObject *o) {
    if (!o) return 0;
    auto idp = objmap.find(o);
    if (!idp) FatalError("tried to save unregistered object");
    return *idp;
  }

  vuint32 getNameId (VName n) {
    if (n == NAME_None) return 0;
    auto idp = namemap.find(n);
    if (!idp) FatalError("tried to save unregistered name");
    return *idp;
  }

  vuint32 getStrId (const VStr &s) {
    if (s.isEmpty()) return 0;
    auto idp = strmap.find(s);
    if (!idp) FatalError("tried to save unregistered string");
    return *idp;
  }

  bool saveMap (VStream &strm) {
    if (wasError || strm.IsError() || strm.IsLoading()) return false;
    // signature
    static const char *sign = "VaVoom C Binary Data Storage v0"; // 32 bytes
    if (strlen(sign) != 31) return false;
    strm.Serialise(sign, 32);
    // counters
    strm << STRM_INDEX(objCount);
    strm << STRM_INDEX(nameCount);
    strm << STRM_INDEX(strCount);
    if (strm.IsError()) return false;
    // names
    for (int f = 0; f < namearr.length(); ++f) {
      VStr s = *namearr[f];
      if (s.isEmpty()) return false;
      strm << s;
      if (strm.IsError()) return false;
    }
    // strings
    for (int f = 0; f < strarr.length(); ++f) {
      VStr s = strarr[f];
      if (s.isEmpty()) return false;
      strm << s;
      if (strm.IsError()) return false;
    }
    // objects
    for (int f = 0; f < objarr.length(); ++f) {
      VObject *o = objarr[f];
      if (!o) return false;
      if (!cfgSaveObject(*this, strm, o, true)) return false;
    }
    return !strm.IsError();
  }

private:
  void buildSaveMap (VObject *o) {
    if (!o) return;
    processObject(o, true);
  }

  void putName (VName n) {
    if (n == NAME_None || namemap.has(n)) return;
    if ((vint32)nameCount != namearr.length()) FatalError("oops: putName");
    namemap.put(n, ++nameCount);
    namearr.append(n);
  }

  void putStr (const VStr &s) {
    if (s.isEmpty() || strmap.has(s)) return;
    if ((vint32)strCount != strarr.length()) FatalError("oops: putStr");
    strmap.put(s, ++strCount);
    strarr.append(s);
  }

  void processValue (vuint8 *data, const VFieldType &type) {
    if (!cfgCanIOType(type)) {
      //fprintf(stderr, "FUCKED type '%s'\n", *type.GetName());
      wasError = true;
      return;
    }
    switch (type.Type) {
      case TYPE_Name:
        putName(*(*(VName *)data));
        break;
      case TYPE_String:
        putStr(*(VStr *)data);
        break;
      case TYPE_Reference:
        {
          VObject *o = *(VObject **)data;
          if (o) processObject(o);
        }
        break;
      case TYPE_Class:
        {
          VClass *c = *(VClass **)data;
          if (c) putName(c->Name);
        }
        break;
      case TYPE_Struct:
        if (cfgCanIOStruct(type.Struct)) {
          processStruct(data, type.Struct);
        } else {
          wasError = true;
        }
        break;
      case TYPE_Array:
        if (cfgCanIOType(type.GetArrayInnerType())) {
          VFieldType intType = type;
          intType.Type = type.ArrayInnerType;
          vint32 innerSize = intType.GetSize();
          vint32 dim = type.GetArrayDim();
          for (int f = 0; f < dim; ++f) {
            processValue(data+f*innerSize, intType);
            if (wasError) break;
          }
        } else {
          wasError = true;
        }
        break;
      case TYPE_DynamicArray:
        if (cfgCanIOType(type.GetArrayInnerType())) {
          VScriptArray *a = (VScriptArray *)data;
          VFieldType intType = type;
          intType.Type = type.ArrayInnerType;
          vint32 innerSize = intType.GetSize();
          for (int f = 0; f < a->length(); ++f) {
            processValue(a->Ptr()+f*innerSize, intType);
            if (wasError) break;
          }
        } else {
          wasError = true;
        }
        break;
    }
  }

  void processFields (vuint8 *data, VField *fields) {
    for (VField *fld = fields; fld; fld = fld->Next) {
      if (cfgSkipField(fld)) continue;
      if (cfgCanIOField(fld)) {
        putName(fld->Name);
        processValue(data+fld->Ofs, fld->Type);
      } else {
        wasError = false;
        break;
      }
    }
  }

  void processStruct (vuint8 *data, VStruct *st) {
    if (!cfgCanIOStruct(st)) { wasError = true; return; }
    putName(st->Name);
    for (VStruct *cst = st; cst; cst = cst->ParentStruct) {
      processFields(data, cst->Fields);
      if (wasError) break;
    }
  }

  void processObject (VObject *obj, bool ignoreFlags=false) {
    if (!obj) { wasError = true; return; }
    VClass *cls = obj->GetClass();
    //if (!obj->GetClass()->IsChildOf(cls)) { wasError = true; return; }
    if (!obj || !cfgCanIOClass(cls, ignoreFlags)) {
      //fprintf(stderr, "CANNOT save class '%s'\n", *cls->Name);
      wasError = true;
      return;
    }
    // check for cycles
    if (objmap.has(obj)) {
      //fprintf(stderr, "DUP object '%s' (%p)\n", *cls->Name, obj);
      return;
    }
    //fprintf(stderr, "DOING object '%s' (%p)\n", *cls->Name, obj);
    // mark as processed
    if ((vint32)objCount != objarr.length()) FatalError("oops: processObject");
    objmap.put(obj, ++objCount);
    objarr.append(obj);
    putName(cls->Name);
    // do this class and superclasses
    for (VClass *c = cls; c; c = c->ParentClass) {
      if (c != cls && cfgSkipClass(c, false)) break; // transient class breaks chain
      processFields((vuint8 *)obj, c->Fields);
      if (wasError) break;
    }
  }
};


// ////////////////////////////////////////////////////////////////////////// //
static bool cfgSaveFields (ObjectSaveMap &smap, VStream &strm, vuint8 *data, VField *fields);
static bool cfgSaveValue (ObjectSaveMap &smap, VStream &strm, vuint8 *data, const VFieldType &type);
static bool cfgSaveStruct (ObjectSaveMap &smap, VStream &strm, vuint8 *data, VStruct *st);
static bool cfgSaveObject (ObjectSaveMap &smap, VStream &strm, VObject *obj, bool ignoreFlags);


// ////////////////////////////////////////////////////////////////////////// //
static bool cfgCountSaveableFields (vuint32 &fcount, VField *fields) {
  for (VField *fld = fields; fld; fld = fld->Next) {
    if (cfgSkipField(fld)) continue;
    if (!cfgCanIOField(fld)) return false;
    ++fcount;
  }
  return true;
}

static bool cfgSaveFields (ObjectSaveMap &smap, VStream &strm, vuint8 *data, VField *fields) {
  // save fields
  for (VField *fld = fields; fld; fld = fld->Next) {
    if (cfgSkipField(fld)) continue;
    if (!cfgCanIOField(fld)) return false;
    vuint32 n = smap.getNameId(fld->Name);
    strm << STRM_INDEX(n);
    if (strm.IsError()) return false;
    if (!cfgSaveValue(smap, strm, data+fld->Ofs, fld->Type)) return false;
  }
  return !strm.IsError();
}

static bool cfgSaveValue (ObjectSaveMap &smap, VStream &strm, vuint8 *data, const VFieldType &type) {
  vuint8 typetag = (vuint8)type.Type;
  strm << typetag;
  switch (type.Type) {
    case TYPE_Int:
      strm << STRM_INDEX(*(vint32 *)data);
      break;
    case TYPE_Byte:
      strm << *(vuint8 *)data;
      break;
    case TYPE_Bool:
      {
        vuint8 b;
        if (type.BitMask == 0) {
          b = ((*(vuint32 *)data) != 0 ? 1 : 0);
        } else {
          b = (((*(vuint32 *)data)&type.BitMask) != 0 ? 1 : 0);
        }
        strm << b;
      }
      break;
    case TYPE_Float:
      strm << *(float *)data;
      break;
    case TYPE_Name:
      {
        vuint32 n = smap.getNameId(*(VName *)data);
        strm << STRM_INDEX(n);
      }
      break;
    case TYPE_String:
      {
        vuint32 s = smap.getStrId(*(VStr *)data);
        strm << STRM_INDEX(s);
      }
      break;
    case TYPE_Reference:
      {
        VObject *o = *(VObject **)data;
        if (o) {
          // class to cast
          vuint32 n = smap.getNameId(type.Class->Name);
          strm << STRM_INDEX(n);
          if (strm.IsError()) return false;
          n = smap.getObjectId(o);
          strm << STRM_INDEX(n);
        } else {
          vuint32 n = smap.getNameId(NAME_None);
          strm << STRM_INDEX(n);
        }
      }
      break;
    case TYPE_Class:
      {
        // saving
        VClass *c = *(VClass **)data;
        vuint32 n = smap.getNameId(c ? c->Name : NAME_None);
        strm << STRM_INDEX(n);
      }
      break;
    case TYPE_Struct:
      if (!cfgCanIOStruct(type.Struct)) return false;
      return cfgSaveStruct(smap, strm, data, type.Struct);
    case TYPE_Vector:
      strm << *(float *)data;
      strm << *(float *)(data+sizeof(float));
      strm << *(float *)(data+sizeof(float)*2);
      break;
    case TYPE_Array:
      if (cfgCanIOType(type.GetArrayInnerType())) {
        VFieldType intType = type;
        intType.Type = type.ArrayInnerType;
        vint32 innerSize = intType.GetSize();
        vint32 dim = type.GetArrayDim();
        strm << STRM_INDEX(dim);
        strm << STRM_INDEX(innerSize);
        if (strm.IsError()) return false;
        for (int f = 0; f < dim; ++f) {
          if (!cfgSaveValue(smap, strm, data+f*innerSize, intType)) return false;
        }
      } else {
        return false;
      }
      break;
    case TYPE_DynamicArray:
      if (cfgCanIOType(type.GetArrayInnerType())) {
        VScriptArray *a = (VScriptArray *)data;
        VFieldType intType = type;
        intType.Type = type.ArrayInnerType;
        vint32 innerSize = intType.GetSize();
        vint32 d = a->length();
        strm << STRM_INDEX(d);
        strm << STRM_INDEX(innerSize);
        for (int f = 0; f < a->length(); ++f) {
          if (!cfgSaveValue(smap, strm, a->Ptr()+f*innerSize, intType)) return false;
        }
      } else {
        return false;
      }
      break;
    default:
      return false;
  }
  return !strm.IsError();
}

static bool cfgSaveStruct (ObjectSaveMap &smap, VStream &strm, vuint8 *data, VStruct *st) {
  if (!cfgCanIOStruct(st)) return false;
  // struct name
  vuint32 nn = smap.getNameId(st->Name);
  strm << STRM_INDEX(nn);
  if (strm.IsError()) return false;
  // field count
  vuint32 fcount = 0;
  for (VStruct *cst = st; cst; cst = cst->ParentStruct) {
    if (!cfgCountSaveableFields(fcount, cst->Fields)) return false;
  }
  strm << STRM_INDEX(fcount);
  if (strm.IsError()) return false;
  // fields
  for (VStruct *cst = st; cst; cst = cst->ParentStruct) {
    if (!cfgSaveFields(smap, strm, data, cst->Fields)) return false;
  }
  return !strm.IsError();
}

static bool cfgSaveObject (ObjectSaveMap &smap, VStream &strm, VObject *obj, bool ignoreFlags) {
  if (!obj || !cfgCanIOClass(obj->GetClass(), ignoreFlags)) return false;
  // object class
  VClass *cls = obj->GetClass();
  vuint32 nn = smap.getNameId(cls->Name);
  strm << STRM_INDEX(nn);
  if (strm.IsError()) return false;
  // field count
  vuint32 fcount = 0;
  for (VClass *c = cls; c; c = c->ParentClass) {
    if (c != cls && cfgSkipClass(c, false)) break; // transient class breaks chain
    if (!cfgCountSaveableFields(fcount, c->Fields)) return false;
  }
  strm << STRM_INDEX(fcount);
  // fields
  for (VClass *c = obj->GetClass(); c; c = c->ParentClass) {
    if (c != cls && cfgSkipClass(c, false)) break; // transient class breaks chain
    if (!cfgSaveFields(smap, strm, (vuint8 *)obj, c->Fields)) continue;
  }
  return !strm.IsError();
}


// ////////////////////////////////////////////////////////////////////////// //
/*
static bool cfgLoadFields (VStream &strm, vuint8 *data, VField *fields) {
  vuint16 fcount;
  if (strm.IsLoading()) {
    strm << fcount;
    if (strm.IsError()) return false;
    if (fcount == 0) return false;
    if (!fields) return false;
    // load fields
    while (fcount-- > 0) {
      VStr fldName;
      strm << fldName;
      if (strm.IsError()) return false;
      // find field
      VField *fld = fields;
      while (fld) {
        if (fldName == *fld->Name) break;
        fld = fld->Next;
      }
      if (!fld) {
        // field not found
        fprintf(stderr, "CFG LOADER: field '%s' not found!\n", *fldName);
        //return false;
        // ignore unknown fields
        continue;
      }
      if (!cfgCanIOField(fld)) {
        fprintf(stderr, "CFG LOADER: field '%s' is not suitable for i/o!\n", *fldName);
        return false;
      }
      if (!cfgLoadValue(strm, data+fld->Ofs, fld->Type)) return false;
    }
  } else {
    // count saveable fields
    fcount = 0;
    for (VField *fld = fields; fld; fld = fld->Next) {
      if (cfgCanIOField(fld)) {
        if (fcount == 0xffff) return false; // too many
        ++fcount;
      }
    }
    strm << fcount;
    if (strm.IsError()) return false;
    // save fields
    for (VField *fld = fields; fld; fld = fld->Next) {
      if (cfgCanIOField(fld)) {
        VStr fldName = *fld->Name;
        strm << fldName;
        if (strm.IsError()) return false;
        if (!cfgLoadValue(strm, data+fld->Ofs, fld->Type)) return false;
      }
    }
  }
  return !strm.IsError();
}

static bool cfgLoadValue (VStream &strm, vuint8 *data, const VFieldType &type) {
  vuint8 typetag = (vuint8)type.Type;
  strm << typetag;
  if (strm.IsLoading() && type.Type != typetag) return false;
  switch (type.Type) {
    case TYPE_Int:
      strm << *(vint32 *)data;
      break;
    case TYPE_Byte:
      strm << *(vuint8 *)data;
      break;
    case TYPE_Bool:
      if (strm.IsLoading()) {
        vuint8 b = 0;
        strm << b;
        if (type.BitMask == 0) {
          *(vuint32 *)data = (b ? 1 : 0);
        } else {
          if (b) *(vuint32 *)data |= type.BitMask; else *(vuint32 *)data &= ~type.BitMask;
        }
      } else {
        vuint8 b;
        if (type.BitMask == 0) {
          b = ((*(vuint32 *)data) != 0 ? 1 : 0);
        } else {
          b = (((*(vuint32 *)data)&type.BitMask) != 0 ? 1 : 0);
        }
        strm << b;
      }
      break;
    case TYPE_Float:
      strm << *(float *)data;
      break;
    case TYPE_Name:
      if (strm.IsLoading()) {
        VStr s;
        strm << s;
        *(VName *)data = VName(*s);
      } else {
        VStr s = *(*(VName *)data);
        strm << s;
      }
      break;
    case TYPE_String:
      strm << *(VStr *)data;
      return true;
    case TYPE_Reference:
      if (cfgCanIOClass(type.Class)) {
        VObject **o = (VObject **)data;
        vuint8 b = (*o != nullptr);
        strm << b;
        if (strm.IsError()) return false;
        if (strm.IsLoading()) {
          // loading
          if (*o) (*o)->ConditionalDestroy();
          if (b) {
            VStr clsName;
            strm << clsName;
            if (clsName != *type.Class->Name) return false;
            *o = VObject::StaticSpawnObject(type.Class);
            return cfgLoadObject(strm, *o, type.Class);
          } else {
            *o = nullptr;
          }
        } else {
          // saving
          if (*o) {
            VStr clsName = *type.Class->Name;
            strm << clsName;
            if (strm.IsError()) return false;
            return cfgLoadObject(strm, *o, type.Class);
          }
        }
      } else {
        return false;
      }
      break;
    case TYPE_Class:
      if (strm.IsLoading()) {
        // loading
        VClass **c = (VClass **)data;
        VStr clsName;
        strm << clsName;
        if (strm.IsError()) return false;
        if (clsName.isEmpty()) {
          *c = nullptr;
        } else {
          *c = VClass::FindClass(*clsName);
          if (*c == nullptr) return false;
        }
      } else {
        // saving
        VClass *c = *(VClass **)data;
        VStr clsName = (c ? *c->Name : "");
        strm << clsName;
      }
      break;
    case TYPE_Struct:
      return cfgLoadStruct(strm, data, type.Struct);
    case TYPE_Vector:
      strm << *(float *)data;
      strm << *(float *)(data+sizeof(float));
      strm << *(float *)(data+sizeof(float)*2);
      break;
    case TYPE_Array:
      if (cfgCanIOType(type.GetArrayInnerType())) {
        VFieldType intType = type;
        intType.Type = type.ArrayInnerType;
        vint32 innerSize = intType.GetSize();
        vint32 dim = type.ArrayDim;
        if (strm.IsLoading()) {
          vint32 d;
          strm << d;
          if (strm.IsError()) return false;
          if (d != dim) return false;
          strm << d;
          if (strm.IsError()) return false;
          if (d != innerSize) return false;
        } else {
          strm << dim;
          strm << innerSize;
          if (strm.IsError()) return false;
        }
        for (int f = 0; f < dim; ++f) {
          if (!cfgLoadValue(strm, data+f*innerSize, intType)) return false;
        }
      } else {
        return false;
      }
      break;
    case TYPE_DynamicArray:
      if (cfgCanIOType(type.GetArrayInnerType())) {
        VScriptArray *a = (VScriptArray *)data;
        VFieldType intType = type;
        intType.Type = type.ArrayInnerType;
        vint32 innerSize = intType.GetSize();
        if (strm.IsLoading()) {
          vint32 d, s;
          strm << d;
          strm << s;
          if (strm.IsError()) return false;
          if (d < 0 || d > 1024*1024*512 || s != innerSize) return false;
          a->SetNum(d, intType);
        } else {
          vint32 d = a->length();
          strm << d;
          strm << innerSize;
        }
        for (int f = 0; f < a->length(); ++f) {
          if (!cfgLoadValue(strm, a->Ptr()+f*innerSize, intType)) return false;
        }
      } else {
        return false;
      }
      break;
    default:
      return false;
  }
  return !strm.IsError();
}

static bool cfgLoadStruct (VStream &strm, vuint8 *data, VStruct *st) {
  if (!st) return false;
  if (!cfgCanIOStruct(st)) return false;
  VStr stName;
  if (strm.IsLoading()) {
    // loading
    strm << stName;
    if (strm.IsError()) return false;
    if (stName != *st->Name) return false;
    if (!cfgLoadFields(strm, data, st->Fields)) return false;
    // parent
    vuint8 b;
    strm << b;
    if (strm.IsError()) return false;
    if (b) return cfgLoadStruct(strm, data, st->ParentStruct);
  } else {
    // saving
    stName = *st->Name;
    strm << stName;
    if (strm.IsError()) return false;
    if (!cfgLoadFields(strm, data, st->Fields)) return false;
    if (cfgCanIOStruct(st->ParentStruct)) {
      // parent
      vuint8 b = 1;
      strm << b;
      if (strm.IsError()) return false;
      return cfgLoadStruct(strm, data, st->ParentStruct);
    } else {
      vuint8 b = 0;
      strm << b;
    }
  }
  return !strm.IsError();
}

static bool cfgLoadObject (VStream &strm, VObject *obj, VClass *cls, bool ignoreFlags) {
  if (!obj || !cls) return false;
  if (!obj->GetClass()->IsChildOf(cls)) return false;
  if (!cfgCanIOClass(cls, ignoreFlags)) return false;
  VStr stName;
  if (strm.IsLoading()) {
    // loading
    strm << stName;
    if (strm.IsError()) return false;
    if (stName != *cls->Name) return false;
    if (!cfgLoadFields(strm, (vuint8 *)obj, cls->Fields)) return false;
    // parent
    vuint8 b;
    strm << b;
    if (strm.IsError()) return false;
    if (b) return cfgLoadObject(strm, obj, cls->ParentClass, ignoreFlags);
  } else {
    // saving
    stName = *obj->GetClass()->Name;
    strm << stName;
    if (strm.IsError()) return false;
    if (!cfgLoadFields(strm, (vuint8 *)obj, cls->Fields)) return false;
    if (cfgCanIOClass(cls->ParentClass, ignoreFlags)) {
      // parent
      vuint8 b = 1;
      strm << b;
      if (strm.IsError()) return false;
      return cfgLoadObject(strm, obj, cls->ParentClass, ignoreFlags);
    } else {
      vuint8 b = 0;
      strm << b;
    }
  }
  return !strm.IsError();
}
*/


// ////////////////////////////////////////////////////////////////////////// //
#if 0
// replicator
static void evalCondValues (VObject *obj, VClass *Class, vuint8 *values) {
  if (Class->GetSuperClass()) evalCondValues(obj, Class->GetSuperClass(), values);
  int len = Class->RepInfos.length();
  for (int i = 0; i < len; ++i) {
    P_PASS_REF(obj);
    vuint8 val = (VObject::ExecuteFunction(Class->RepInfos[i].Cond).i ? 1 : 0);
    int rflen = Class->RepInfos[i].RepFields.length();
    for (int j = 0; j < rflen; ++j) {
      if (Class->RepInfos[i].RepFields[j].Member->MemberType != MEMBER_Field) continue;
      values[((VField *)Class->RepInfos[i].RepFields[j].Member)->NetIndex] = val;
    }
  }
}


static vuint8 *createEvalConds (VObject *obj) {
  return new vuint8[obj->NumNetFields];
}


static vuint8 *createOldData (VClass *Class) {
  vuint8 *oldData = new vuint8[Class->ClassSize];
  memset(oldData, 0, Class->ClassSize);
  vuint8 *def = (vuint8 *)Class->Defaults;
  for (VField *F = Class->NetFields; F; F = F->NextNetField) {
    VField::CopyFieldValue(def+F->Ofs, oldData+F->Ofs, F->Type);
  }
}


static void deleteOldData (VClass *Class, vuint8 *oldData) {
  if (oldData) {
    for (VField *F = Class->NetFields; F; F = F->NextNetField) {
      VField::DestructField(oldData+F->Ofs, F->Type);
    }
    delete[] oldData;
}


static void replicateObj (VObject *obj, vuint8 *oldData) {
  // set up thinker flags that can be used by field condition
  //if (NewObj) Thinker->ThinkerFlags |= VThinker::TF_NetInitial;
  //if (Ent != nullptr && Ent->GetTopOwner() == Connection->Owner->MO) Thinker->ThinkerFlags |= VThinker::TF_NetOwner;

  auto condv = createEvalConds(obj);
  memset(condv, 0, obj->NumNetFields);
  evalCondValues(obj, obj->GetClass(), condv);

  vuint8 *data = (vuint8 *)obj;
  VObject *nullObj = nullptr;

  /*
  if (NewObj) {
    Msg.bOpen = true;
    VClass *TmpClass = Thinker->GetClass();
    Connection->ObjMap->SerialiseClass(Msg, TmpClass);
    NewObj = false;
  }
  */

  /*
  TAVec SavedAngles;
  if (Ent) {
    SavedAngles = Ent->Angles;
    if (Ent->EntityFlags & VEntity::EF_IsPlayer) {
      // clear look angles, because they must not affect model orientation
      Ent->Angles.pitch = 0;
      Ent->Angles.roll = 0;
    }
  } else {
    // shut up compiler warnings
    SavedAngles.yaw = 0;
    SavedAngles.pitch = 0;
    SavedAngles.roll = 0;
  }
  */

  for (VField *F = obj->GetClass()->NetFields; F; F = F->NextNetField) {
    if (!condv[F->NetIndex]) continue;

    // set up pointer to the value and do swapping for the role fields
    vuint8 *fieldData = data+F->Ofs;
    /*
         if (F == Connection->Context->RoleField) fieldData = data+Connection->Context->RemoteRoleField->Ofs;
    else if (F == Connection->Context->RemoteRoleField) fieldData = data+Connection->Context->RoleField->Ofs;
    */

    if (VField::IdenticalValue(fieldData, oldData+F->Ofs, F->Type)) continue;

    if (F->Type.Type == TYPE_Array) {
      VFieldType intrType = F->Type;
      intrType.Type = F->Type.ArrayInnerType;
      int innerSize = intrType.GetSize();
      for (int i = 0; i < F->Type.GetArrayDim(); ++i) {
        vuint8 *val = fieldData+i*innerSize;
        vuint8 *oldval = oldData+F->Ofs+i*innerSize;
        if (VField::IdenticalValue(val, oldval, intrType)) continue;
        // if it's an object reference that cannot be serialised, send it as nullptr reference
        if (intrType.Type == TYPE_Reference && !Connection->ObjMap->CanSerialiseObject(*(VObject **)val)) {
          if (!*(VObject **)oldval) continue; // already sent as nullptr
          val = (vuint8 *)&nullObj;
        }

        Msg.WriteInt(F->NetIndex, obj->GetClass()->NumNetFields);
        Msg.WriteInt(i, F->Type.GetArrayDim());
        if (VField::NetSerialiseValue(Msg, Connection->ObjMap, val, intrType)) {
          VField::CopyFieldValue(val, oldval, intrType);
        }
      }
    } else {
      // if it's an object reference that cannot be serialised, send it as nullptr reference
      if (F->Type.Type == TYPE_Reference && !Connection->ObjMap->CanSerialiseObject(*(VObject**)fieldData)) {
        if (!*(VObject **)(oldData+F->Ofs)) continue; // already sent as nullptr
        fieldData = (vuint8 *)&nullObj;
      }

      Msg.WriteInt(F->NetIndex, obj->GetClass()->NumNetFields);
      if (VField::NetSerialiseValue(Msg, Connection->ObjMap, fieldData, F->Type)) {
        VField::CopyFieldValue(fieldData, oldData + F->Ofs, F->Type);
      }
    }
  }

  if (Ent && (Ent->EntityFlags & VEntity::EF_IsPlayer)) Ent->Angles = SavedAngles;
  UpdatedThisFrame = true;

  if (Msg.GetNumBits()) SendMessage(&Msg);

  // clear temporary networking flags
  obj->ThinkerFlags &= ~VThinker::TF_NetInitial;
  obj->ThinkerFlags &= ~VThinker::TF_NetOwner;

  unguard;
}
#endif



// ////////////////////////////////////////////////////////////////////////// //
static bool onExecuteNetMethod (VObject *aself, VMethod *func) {
  /*
  if (GDemoRecordingContext) {
    // find initial version of the method
    VMethod *Base = func;
    while (Base->SuperMethod) Base = Base->SuperMethod;
    // execute it's replication condition method
    check(Base->ReplCond);
    P_PASS_REF(this);
    vuint32 SavedFlags = PlayerFlags;
    PlayerFlags &= ~VBasePlayer::PF_IsClient;
    bool ShouldSend = false;
    if (VObject::ExecuteFunction(Base->ReplCond).i) ShouldSend = true;
    PlayerFlags = SavedFlags;
    if (ShouldSend) {
      // replication condition is true, the method must be replicated
      GDemoRecordingContext->ClientConnections[0]->Channels[CHANIDX_Player]->SendRpc(func, this);
    }
  }
  */

  /*
#ifdef CLIENT
  if (GGameInfo->NetMode == NM_TitleMap ||
    GGameInfo->NetMode == NM_Standalone ||
    (GGameInfo->NetMode == NM_ListenServer && this == cl))
  {
    return false;
  }
#endif
  */

  // find initial version of the method
  VMethod *Base = func;
  while (Base->SuperMethod) Base = Base->SuperMethod;
  // execute it's replication condition method
  check(Base->ReplCond);
  P_PASS_REF(aself);
  if (!VObject::ExecuteFunction(Base->ReplCond).i) {
    //fprintf(stderr, "rpc call to `%s` (%s) is not done\n", aself->GetClass()->GetName(), *func->GetFullName());
    return false;
  }

  /*
  if (Net) {
    // replication condition is true, the method must be replicated
    Net->Channels[CHANIDX_Player]->SendRpc(func, this);
  }
  */

  // clean up parameters
  func->CleanupParams();

  fprintf(stderr, "rpc call to `%s` (%s) is DONE!\n", aself->GetClass()->GetName(), *func->GetFullName());

  // it's been handled here
  return true;
}


// ////////////////////////////////////////////////////////////////////////// //
__attribute__((noreturn, format(printf, 1, 2))) void Host_Error (const char *error, ...) {
  fprintf(stderr, "FATAL: ");
  va_list argPtr;
  va_start(argPtr, error);
  vfprintf(stderr, error, argPtr);
  va_end(argPtr);
  fprintf(stderr, "\n");
  exit(1);
}


static const char *comatoze (vuint32 n) {
  static char buf[128];
  int bpos = (int)sizeof(buf);
  buf[--bpos] = 0;
  int xcount = 0;
  do {
    if (xcount == 3) { buf[--bpos] = ','; xcount = 0; }
    buf[--bpos] = '0'+n%10;
    ++xcount;
  } while ((n /= 10) != 0);
  return &buf[bpos];
}


// ////////////////////////////////////////////////////////////////////////// //
void PR_WriteOne (const VFieldType &type) {
  switch (type.Type) {
    case TYPE_Int: case TYPE_Byte: printf("%d", PR_Pop()); break;
    case TYPE_Bool: printf("%s", (PR_Pop() ? "true" : "false")); break;
    case TYPE_Float: printf("%f", PR_Popf()); break;
    case TYPE_Name: printf("%s", *PR_PopName()); break;
    case TYPE_String: printf("%s", *PR_PopStr()); break;
    case TYPE_Vector: { TVec v = PR_Popv(); printf("(%f,%f,%f)", v.x, v.y, v.z); } break;
    case TYPE_Pointer: printf("<%s>(%p)", *type.GetName(), PR_PopPtr()); break;
    case TYPE_Class: if (PR_PopPtr()) printf("<%s>", *type.GetName()); else printf("<none>"); break;
    case TYPE_State:
      {
        VState *st = (VState *)PR_PopPtr();
        if (st) {
          printf("<state:%s %d %f>", *st->SpriteName, st->Frame, st->Time);
        } else {
          printf("<state>");
        }
      }
      break;
    case TYPE_Reference: printf("<%s>", (type.Class ? *type.Class->Name : "none")); break;
    case TYPE_Delegate: printf("<%s:%p:%p>", *type.GetName(), PR_PopPtr(), PR_PopPtr()); break;
    case TYPE_Struct: PR_PopPtr(); printf("<%s>", *type.Struct->Name); break;
    case TYPE_Array: PR_PopPtr(); printf("<%s>", *type.GetName()); break;
    case TYPE_SliceArray: printf("<%s:%d>", *type.GetName(), PR_Pop()); PR_PopPtr(); break;
    case TYPE_DynamicArray:
      {
        VScriptArray *a = (VScriptArray *)PR_PopPtr();
        printf("%s(%d)", *type.GetName(), a->Num());
      }
      break;
    default: Sys_Error(va("Tried to print something strange: `%s`", *type.GetName()));
  }
}

void PR_WriteFlush () {
  printf("\n");
}


// ////////////////////////////////////////////////////////////////////////// //
class VVccLog : public VLogListener {
public:
  virtual void Serialise (const char* text, EName event) override {
    dprintf("%s", text);
  }
};


// ////////////////////////////////////////////////////////////////////////// //
static VStr SourceFileName;
static TArray<VStr> scriptArgs;

static int num_dump_asm;
static const char *dump_asm_names[1024];
static bool DebugMode = false;
static FILE *DebugFile;

static VLexer Lex;
static VVccLog VccLog;


//==========================================================================
//
//  dprintf
//
//==========================================================================
__attribute__((format(printf, 1, 2))) int dprintf (const char *text, ...) {
  if (!DebugMode) return 0;

  va_list argPtr;
  FILE* fp = stderr; //(DebugFile ? DebugFile : stdout);
  va_start(argPtr, text);
  int ret = vfprintf(fp, text, argPtr);
  va_end(argPtr);
  fflush(fp);
  return ret;
}


//==========================================================================
//
//  Malloc
//
//==========================================================================
void* Malloc (size_t size) {
  if (!size) return nullptr;
  void *ptr = Z_Malloc(size);
  if (!ptr) FatalError("FATAL: couldn't alloc %d bytes", (int)size);
  memset(ptr, 0, size);
  return ptr;
}


//==========================================================================
//
//  Free
//
//==========================================================================
void Free (void* ptr) {
  if (ptr) Z_Free(ptr);
}


//==========================================================================
//
//  OpenFile
//
//==========================================================================
VStream* OpenFile (const VStr& Name) {
  return fsysOpenFile(Name);
  /*
  FILE* file = fopen(*Name, "rb");
  return (file ? new VFileReader(file) : nullptr);
  */
}


//==========================================================================
//
//  OpenDebugFile
//
//==========================================================================
static void OpenDebugFile (const VStr& name) {
  DebugFile = fopen(*name, "w");
  if (!DebugFile) FatalError("FATAL: can\'t open debug file \"%s\".", *name);
}


//==========================================================================
//
//  PC_DumpAsm
//
//==========================================================================
static void PC_DumpAsm (const char* name) {
  char buf[1024];
  char *cname;
  char *fname;

  snprintf(buf, sizeof(buf), "%s", name);

  //FIXME! PATH WITH DOTS!
  if (strstr(buf, ".")) {
    cname = buf;
    fname = strstr(buf, ".")+1;
    fname[-1] = 0;
  } else {
    dprintf("Dump ASM: Bad name %s\n", name);
    return;
  }

  //printf("<%s>.<%s>\n", cname, fname);

  for (int i = 0; i < VMemberBase::GMembers.Num(); ++i) {
    //if (VMemberBase::GMembers[i]->MemberType == MEMBER_Method) printf("O:<%s>; N:<%s>\n", *VMemberBase::GMembers[i]->Outer->Name, *VMemberBase::GMembers[i]->Name);
    if (VMemberBase::GMembers[i]->MemberType == MEMBER_Method &&
        !VStr::Cmp(cname, *VMemberBase::GMembers[i]->Outer->Name) &&
        !VStr::Cmp(fname, *VMemberBase::GMembers[i]->Name))
    {
      ((VMethod*)VMemberBase::GMembers[i])->DumpAsm();
      return;
    }
  }

  dprintf("Dump ASM: %s not found!\n", name);
}


//==========================================================================
//
//  DumpAsm
//
//==========================================================================
static void DumpAsm () {
  for (int i = 0; i < num_dump_asm; ++i) PC_DumpAsm(dump_asm_names[i]);
}


//==========================================================================
//
//  DisplayUsage
//
//==========================================================================
static void DisplayUsage () {
  printf("\n");
  printf("VCC Version 1.%d. Copyright (c) 2000-2001 by JL, 2018 by Ketmar Dark. (" __DATE__ " " __TIME__ "; opcodes: %d)\n", PROG_VERSION, NUM_OPCODES);
  printf("Usage: vcc [options] source[.c] [object[.dat]]\n");
  printf("    -d<file>     Output debugging information into specified file\n");
  printf("    -a<function> Output function's ASM statements into debug file\n");
  printf("    -D<name>           Define macro\n");
  printf("    -I<directory>      Include files directory\n");
  printf("    -P<directory>      Package import files directory\n");
  printf("    -base <directory>  Set base directory\n");
  printf("    -file <name>       Add pak file\n");
  exit(1);
}


//==========================================================================
//
//  ProcessArgs
//
//==========================================================================
static void ProcessArgs (int ArgCount, char **ArgVector) {
  int count = 0; // number of file arguments
  bool nomore = false;

  TArray<VStr> paklist;

  for (int i = 1; i < ArgCount; ++i) {
    const char *text = ArgVector[i];
    if (count == 0 && !nomore && *text == '-') {
      ++text;
      if (*text == 0) DisplayUsage();
      if (text[0] == '-' && text[1] == 0) { nomore = true; continue; }
      const char option = *text++;
      switch (option) {
        case 'd': DebugMode = true; if (*text) OpenDebugFile(text); break;
        case 'a': /*if (!*text) DisplayUsage(); dump_asm_names[num_dump_asm++] = text;*/ VMemberBase::doAsmDump = true; break;
        case 'I': VMemberBase::StaticAddIncludePath(text); break;
        case 'D': VMemberBase::StaticAddDefine(text); break;
        case 'P': VMemberBase::StaticAddPackagePath(text); break;
        default:
          --text;
          if (VStr::Cmp(text, "base") == 0) {
            ++i;
            if (i >= ArgCount) DisplayUsage();
            fsysBaseDir = VStr(ArgVector[i]);
          } else if (VStr::Cmp(text, "file") == 0) {
            ++i;
            if (i >= ArgCount) DisplayUsage();
            paklist.append(VStr(":")+VStr(ArgVector[i]));
            //fprintf(stderr, "<%s>\n", ArgVector[i]);
          } else if (VStr::Cmp(text, "pakdir") == 0) {
            ++i;
            if (i >= ArgCount) DisplayUsage();
            paklist.append(VStr("/")+VStr(ArgVector[i]));
            //fprintf(stderr, "<%s>\n", ArgVector[i]);
          } else {
            //fprintf(stderr, "*<%s>\n", text);
            DisplayUsage();
          }
          break;
      }
      continue;
    }
    ++count;
    switch (count) {
      case 1: SourceFileName = VStr(text).DefaultExtension(".vc"); break;
      default: scriptArgs.Append(VStr(text)); break;
    }
  }

  if (count == 0) DisplayUsage();

  /*
  if (!DebugFile) {
    VStr DbgFileName;
    DbgFileName = ObjectFileName.StripExtension()+".txt";
    OpenDebugFile(DbgFileName);
    DebugMode = true;
  }
  */

  fsysInit();
  for (int f = 0; f < paklist.length(); ++f) {
    VStr pname = paklist[f];
    if (pname.length() < 2) continue;
    char type = pname[0];
    pname.chopLeft(1);
    if (type == ':') {
      if (fsysAppendPak(pname)) {
        dprintf("added pak file '%s'...\n", *pname);
      } else {
        fprintf(stderr, "CAN'T add pak file '%s'!\n", *pname);
      }
    } else if (type == '/') {
      if (fsysAppendDir(pname)) {
        dprintf("added pak directory '%s'...\n", *pname);
      } else {
        fprintf(stderr, "CAN'T add pak directory '%s'!\n", *pname);
      }
    }
  }

  SourceFileName = SourceFileName.fixSlashes();
  dprintf("Main source file: %s\n", *SourceFileName);
}


//==========================================================================
//
//  initialize
//
//==========================================================================
static void initialize () {
  DebugMode = false;
  DebugFile = nullptr;
  num_dump_asm = 0;
  VName::StaticInit();
  //VMemberBase::StaticInit();
  VObject::StaticInit();
  VMemberBase::StaticAddDefine("VCC_STANDALONE_EXECUTOR");
#ifdef VCCRUN_HAS_SDL
  VMemberBase::StaticAddDefine("VCCRUN_HAS_SDL");
#endif
#ifdef VCCRUN_HAS_OPENGL
  VMemberBase::StaticAddDefine("VCCRUN_HAS_OPENGL");
#endif
#ifdef VCCRUN_HAS_OPENAL
  VMemberBase::StaticAddDefine("VCCRUN_HAS_OPENAL");
#endif
  VMemberBase::StaticAddDefine("VCCRUN_HAS_IMAGO");
  VObject::onExecuteNetMethodCB = &onExecuteNetMethod;
}


// ////////////////////////////////////////////////////////////////////////// //
// <0: error; bit 0: has arg; bit 1: returns int
static int checkArg (VMethod *mmain) {
  if (!mmain) return -1;
  if ((mmain->Flags&FUNC_VarArgs) != 0) return -1;
  //if (mmain->NumParams > 0 && mmain->ParamFlags[0] != 0) return -1;
  int res = 0;
  if (mmain->ReturnType.Type != TYPE_Void && mmain->ReturnType.Type != TYPE_Int) return -1;
  if (mmain->ReturnType.Type == TYPE_Int) res |= 0x02;
  if (mmain->NumParams != 0) {
    if (mmain->NumParams != 1) return -1;
    if (mmain->ParamFlags[0] == 0) {
      VFieldType atp = mmain->ParamTypes[0];
      dprintf("  ptype0: %s\n", *atp.GetName());
      if (atp.Type != TYPE_Pointer) return -1;
      atp = atp.GetPointerInnerType();
      if (atp.Type != TYPE_DynamicArray) return -1;
      atp = atp.GetArrayInnerType();
      if (atp.Type != TYPE_String) return -1;
      res |= 0x01;
    } else if ((mmain->ParamFlags[0]&(FPARM_Out|FPARM_Ref)) != 0) {
      VFieldType atp = mmain->ParamTypes[0];
      dprintf("  ptype1: %s\n", *atp.GetName());
      if (atp.Type != TYPE_DynamicArray) return -1;
      atp = atp.GetArrayInnerType();
      if (atp.Type != TYPE_String) return -1;
      res |= 0x01;
    }
  }
  return res;
}


// ////////////////////////////////////////////////////////////////////////// //
int main (int argc, char **argv) {
  VStack ret;
  ret.i = 0;

  srand(time(nullptr));

#if 0
  {
    TArray<VStr> pts;
    VStr s = "/this/is/path/";
    s.SplitPath(pts);
    for (int f = 0; f < pts.length(); ++f) printf("  %d: <%s>:%d\n", f, *pts[f], pts[f].length());
    printf("<%s>\n", *(s.ToUpper()));
    printf("<%s>\n", *(s.ToLower()));
    printf("<%s>\n", *(s.ToUpper().ToLower()));
    printf("<%s>\n", *s);
  }
#endif

  try {
    GLog.AddListener(&VccLog);

    int starttime;
    int endtime;

    starttime = time(0);

    initialize();

    ProcessArgs(argc, argv);

    /*{
      auto fl = fsysOpenFile("text/model");
      auto sz = fl->TotalSize();
      printf("SIZE: %d (%d)\n", sz, fl->Tell());
      auto buf = new char[sz];
      fl->Serialize(buf, sz);
      delete fl;
      auto fo = fsysOpenDiskFileWrite("_000.ini");
      fo->Serialize(buf, sz);
      delete fo;
    }*/

    /*{
      VStr s = fsysFileFindAnyExt("a.boo");
      fprintf(stderr, "%s\n", *s);
    }*/

    PR_Init();

    VMemberBase::StaticLoadPackage(VName("engine"), TLocation());
    //VMemberBase::StaticLoadPackage(VName("ui"), TLocation());

    VPackage *CurrentPackage = new VPackage(VName("vccrun"));

    dprintf("Compiling '%s'...\n", *SourceFileName);

    VStream *strm = OpenFile(SourceFileName);
    if (!strm) {
      FatalError("FATAL: cannot open file '%s'", *SourceFileName);
    }

    CurrentPackage->LoadSourceObject(strm, SourceFileName, TLocation());
    dprintf("Total memory used: %u\n", VExpression::TotalMemoryUsed);
    DumpAsm();
    endtime = time(0);
    dprintf("Time elapsed: %02d:%02d\n", (endtime-starttime)/60, (endtime-starttime)%60);
    // free compiler memory
    VMemberBase::StaticCompilerShutdown();
    dprintf("Peak compiler memory usage: %s bytes.\n", comatoze(VExpression::PeakMemoryUsed));
    dprintf("Released compiler memory  : %s bytes.\n", comatoze(VExpression::TotalMemoryFreed));
    if (VExpression::CurrMemoryUsed != 0) {
      dprintf("Compiler leaks %s bytes (this is harmless).\n", comatoze(VExpression::CurrMemoryUsed));
    }

    VScriptArray scargs(scriptArgs);
    VClass *mklass = VClass::FindClass("Main");
    if (mklass) {
      dprintf("Found class 'Main'\n");
      VMethod *mmain = mklass->FindAccessibleMethod("main");
      if (mmain) {
        dprintf(" Found method 'main()' (return type: %u:%s)\n", mmain->ReturnType.Type, *mmain->ReturnType.GetName());
        int atp = checkArg(mmain);
        if (atp < 0) FatalError("Main::main() should be either arg-less, or have one `array!string*` argument, and should be either `void`, or return `int`!");
        auto sss = pr_stackPtr;
        mainObject = VObject::StaticSpawnObject(mklass);
        if ((mmain->Flags&FUNC_Static) == 0) {
          //auto imain = Spawn<VLevel>();
          P_PASS_REF((VObject *)mainObject);
        }
        if (atp&0x01) P_PASS_REF(&scargs);
        ret = VObject::ExecuteFunction(mmain);
        if ((atp&0x02) == 0) ret.i = 0;
        if (sss != pr_stackPtr) FatalError("FATAL: stack imbalance!");
      }
    }

    VSoundManager::StaticShutdown();
    VObject::StaticExit();
    VName::StaticExit();
  } catch (VException& e) {
    ret.i = -1;
    FatalError("FATAL: %s", e.What());
  }

  return ret.i;
}


// ////////////////////////////////////////////////////////////////////////// //
//native static final int fsysAppendDir (string path, optional string pfx);
IMPLEMENT_FUNCTION(VObject, fsysAppendDir) {
  P_GET_STR_OPT(pfx, VStr());
  P_GET_STR(fname);
  //fprintf(stderr, "pakid(%d)=%d; fname=<%s>\n", (int)specified_pakid, pakid, *fname);
  if (specified_pfx) {
    RET_INT(fsysAppendDir(fname, pfx));
  } else {
    RET_INT(fsysAppendDir(fname));
  }
}


// append archive to the list of archives
// it will be searched in the current dir, and then in `fsysBaseDir`
// returns pack id or 0
//native static final int fsysAppendPak (string fname, optional int pakid);
IMPLEMENT_FUNCTION(VObject, fsysAppendPak) {
  P_GET_INT_OPT(pakid, -1);
  P_GET_STR(fname);
  //fprintf(stderr, "pakid(%d)=%d; fname=<%s>\n", (int)specified_pakid, pakid, *fname);
  if (specified_pakid) {
    RET_INT(fsysAppendPak(fname, pakid));
  } else {
    RET_INT(fsysAppendPak(fname));
  }
}

// remove given pack from pack list
//native static final void fsysRemovePak (int pakid);
IMPLEMENT_FUNCTION(VObject, fsysRemovePak) {
  P_GET_INT(pakid);
  fsysRemovePak(pakid);
}

// remove all packs from pakid and later
//native static final void fsysRemovePaksFrom (int pakid);
IMPLEMENT_FUNCTION(VObject, fsysRemovePaksFrom) {
  P_GET_INT(pakid);
  fsysRemovePaksFrom(pakid);
}

// 0: no such pack
//native static final int fsysFindPakByPrefix (string pfx);
IMPLEMENT_FUNCTION(VObject, fsysFindPakByPrefix) {
  P_GET_STR(pfx);
  RET_BOOL(fsysFindPakByPrefix(pfx));
}

//native static final bool fsysFileExists (string fname, optional int pakid);
IMPLEMENT_FUNCTION(VObject, fsysFileExists) {
  P_GET_INT_OPT(pakid, -1);
  P_GET_STR(fname);
  if (specified_pakid) {
    RET_BOOL(fsysFileExists(fname, pakid));
  } else {
    RET_BOOL(fsysFileExists(fname));
  }
}

// find file with any extension
//native static final string fsysFileFindAnyExt (string fname, optional int pakid);
IMPLEMENT_FUNCTION(VObject, fsysFileFindAnyExt) {
  P_GET_INT_OPT(pakid, -1);
  P_GET_STR(fname);
  if (specified_pakid) {
    RET_STR(fsysFileFindAnyExt(fname, pakid));
  } else {
    RET_STR(fsysFileFindAnyExt(fname));
  }
}


// return pack file path for the given pack id (or empty string)
//native static final string fsysGetPakPath (int pakid);
IMPLEMENT_FUNCTION(VObject, fsysGetPakPath) {
  P_GET_INT(pakid);
  RET_STR(fsysGetPakPath(pakid));
}

// return pack prefix for the given pack id (or empty string)
//native static final string fsysGetPakPrefix (int pakid);
IMPLEMENT_FUNCTION(VObject, fsysGetPakPrefix) {
  P_GET_INT(pakid);
  RET_STR(fsysGetPakPrefix(pakid));
}


//native static final int fsysGetLastPakId ();
IMPLEMENT_FUNCTION(VObject, fsysGetLastPakId) {
  RET_INT(fsysGetLastPakId());
}


IMPLEMENT_FUNCTION(VObject, get_fsysKillCommonZipPrefix) {
  RET_BOOL(fsysKillCommonZipPrefix);
}

IMPLEMENT_FUNCTION(VObject, set_fsysKillCommonZipPrefix) {
  P_GET_BOOL(v);
  fsysKillCommonZipPrefix = v;
}


// native final void appSetName (string appname);
IMPLEMENT_FUNCTION(VObject, appSetName) {
  P_GET_STR(aname);
  appName = aname;
}


//native final bool appSaveOptions (Object optobj, optional string optfile);
IMPLEMENT_FUNCTION(VObject, appSaveOptions) {
  P_GET_STR_OPT(optfile, VStr());
  P_GET_REF(VObject, optobj);
  if (appName.isEmpty() || !optobj) { RET_BOOL(false); return; }
  if (!cfgCanIOClass(optobj->GetClass(), true)) { RET_BOOL(false); return; }
  //fprintf(stderr, "000\n");
  ObjectSaveMap smap(optobj);
  if (smap.wasError) { RET_BOOL(false); return; }
  //fprintf(stderr, "001\n");
  auto fname = buildConfigName(optfile);
  if (fname.isEmpty()) { RET_BOOL(false); return; }
  auto strm = fsysOpenDiskFileWrite(fname);
  if (!strm) { RET_BOOL(false); return; }
  //static const char *sign = "BCF0";
  //strm->Serialise(sign, 4);
  //if (strm->IsError()) { delete strm; RET_BOOL(false); return; }
  //bool res = cfgSaveObject(smap, *strm, optobj, true);
  bool res = smap.saveMap(*strm);
  if (res && strm->IsError()) res = false;
  delete strm;
  RET_BOOL(res);
}


//native final bool appLoadOptions (Object optobj, optional string optfile);
IMPLEMENT_FUNCTION(VObject, appLoadOptions) {
  P_GET_STR_OPT(optfile, VStr());
  P_GET_REF(VObject, optobj);
  if (appName.isEmpty() || !optobj) { RET_BOOL(false); return; }
  if (!cfgCanIOClass(optobj->GetClass(), true)) { RET_BOOL(false); return; }
  auto fname = buildConfigName(optfile);
  if (fname.isEmpty()) { RET_BOOL(false); return; }
  auto strm = fsysOpenDiskFile(fname);
  if (!strm) { RET_BOOL(false); return; }
  char sign[4];
  strm->Serialise(sign, 4);
  if (strm->IsError()) { delete strm; RET_BOOL(false); return; }
  if (memcmp(sign, "BCF0", 4) != 0) { delete strm; RET_BOOL(false); return; }
  bool res = false;
  //bool res = cfgIOObject(*strm, optobj, optobj->GetClass(), true);
  //if (res && strm->IsError()) res = false;
  delete strm;
  RET_BOOL(res);
}
