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

#include "vc_local.h"


//==========================================================================
//
//  VArrayElement::VArrayElement
//
//==========================================================================
VArrayElement::VArrayElement (VExpression *AOp, VExpression *AInd, const TLocation &ALoc)
  : VExpression(ALoc)
  , genStringAssign(false)
  , sval(nullptr)
  , op(AOp)
  , ind(AInd)
  , AddressRequested(false)
  , IsAssign(false)
{
  if (!ind) {
    ParseError(Loc, "Expression expected");
    return;
  }
}


//==========================================================================
//
//  VArrayElement::~VArrayElement
//
//==========================================================================
VArrayElement::~VArrayElement () {
  if (op) { delete op; op = nullptr; }
  if (ind) { delete ind; ind = nullptr; }
  if (sval) { delete sval; sval = nullptr; }
}


//==========================================================================
//
//  VArrayElement::SyntaxCopy
//
//==========================================================================
VExpression *VArrayElement::SyntaxCopy () {
  auto res = new VArrayElement();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VArrayElement::DoSyntaxCopyTo
//
//==========================================================================
void VArrayElement::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VArrayElement *)e;
  res->genStringAssign = genStringAssign;
  res->sval = (sval ? sval->SyntaxCopy() : nullptr);
  res->op = (op ? op->SyntaxCopy() : nullptr);
  res->ind = (ind ? ind->SyntaxCopy() : nullptr);
  res->AddressRequested = AddressRequested;
  res->IsAssign = IsAssign;
}


//==========================================================================
//
//  VArrayElement::InternalResolve
//
//==========================================================================
VExpression *VArrayElement::InternalResolve (VEmitContext &ec, bool assTarget) {
  if (!op || !ind) {
    delete this;
    return nullptr;
  }

  // we need a copy for opDollar
  opcopy = op->SyntaxCopy();

  op = op->Resolve(ec);

  // hack: allow indexing of pointers to dynamic arrays without `(*arr)`
  /*
  if (op && op->Type.Type == TYPE_Pointer && (op->Type.InnerType == TYPE_DynamicArray || op->Type.InnerType == TYPE_String)) {
    delete op;
    op = new VPushPointed(opcopy);
    opcopy = op->SyntaxCopy();
    op = op->Resolve(ec);
  }
  */

  if (op) {
    // resolve index expression
    auto oldIndArray = ec.SetIndexArray(this);
    ind = ind->Resolve(ec);
    ec.SetIndexArray(oldIndArray);
  }

  // don't need it anymore
  delete opcopy;

  if (!op || !ind) {
    delete this;
    return nullptr;
  }

  if (ind->Type.Type != TYPE_Int) {
    ParseError(Loc, "Array index must be of integer type");
    delete this;
    return nullptr;
  }

  if (op->Type.IsAnyArray()) {
    Flags = op->Flags;
    Type = op->Type.GetArrayInnerType();
    op->Flags &= ~FIELD_ReadOnly;
    op->RequestAddressOf();
  } else if (op->Type.Type == TYPE_String) {
    if (assTarget) {
      ParseError(Loc, "Strings are immutable (yet)");
      delete this;
      return nullptr;
    } else {
      RealType = op->Type;
      Type = VFieldType(TYPE_Int);
    }
  } else if (op->Type.Type == TYPE_Pointer) {
    Flags = 0;
    Type = op->Type.GetPointerInnerType();
  } else {
    ParseError(Loc, "Bad operation with array");
    delete this;
    return nullptr;
  }

  // check for unsafe pointer access
  if (op->Type.Type == TYPE_Pointer) {
    if (ind->IsIntConst() && ind->GetIntConst() == 0) {
      // the only safe case, do nothing
    } else {
      if (!VMemberBase::unsafeCodeAllowed) {
        ParseError(Loc, "Unsafe pointer access is forbidden");
      } else if (VMemberBase::unsafeCodeWarning) {
        ParseWarning(Loc, "Unsafe pointer access");
      }
    }
  }

  RealType = Type;
  if (Type.Type == TYPE_Byte || Type.Type == TYPE_Bool) Type = VFieldType(TYPE_Int);
  return this;
}


//==========================================================================
//
//  VArrayElement::DoResolve
//
//==========================================================================
VExpression *VArrayElement::DoResolve (VEmitContext &ec) {
  return InternalResolve(ec, false);
}


//==========================================================================
//
//  VArrayElement::ResolveAssignmentTarget
//
//==========================================================================
VExpression *VArrayElement::ResolveAssignmentTarget (VEmitContext &ec) {
  IsAssign = true;
  return InternalResolve(ec, true);
}


//==========================================================================
//
//  VArrayElement::ResolveCompleteAssign
//
//==========================================================================
VExpression *VArrayElement::ResolveCompleteAssign (VEmitContext &ec, VExpression *val, bool &resolved) {
  VExpression *rop = op->SyntaxCopy()->Resolve(ec);
  if (!rop) { delete val; delete this; return nullptr; }

  if (rop->Type.Type != TYPE_String && !(rop->Type.Type == TYPE_Pointer && rop->Type.InnerType == TYPE_String)) {
    delete rop;
    return this; // go on with the normal assignment
  }

  resolved = true; // anyway

  // hack: allow indexing of pointers to strings without `(*str)`
  /*if (rop->Type.Type == TYPE_Pointer) {
    delete rop;
    op = new VPushPointed(op);
    // we need a copy for opDollar
    opcopy = op->SyntaxCopy();
    op = op->Resolve(ec);
    if (!op) { delete opcopy; delete val; delete this; return nullptr; }
  } else*/ {
    // we need a copy for opDollar
    opcopy = op;
    op = rop;
  }

  if (op) {
    // resolve index expression
    auto oldIndArray = ec.SetIndexArray(this);
    ind = ind->Resolve(ec);
    ec.SetIndexArray(oldIndArray);
  }
  sval = (val ? val->Resolve(ec) : nullptr);

  // we don't need this anymore
  delete opcopy;

  if (!op || !ind || !sval) {
    delete this;
    return nullptr;
  }

  if (op->Type.Type != TYPE_String) {
    ParseError(Loc, "Something is *very* wrong with the compiler");
    delete this;
    return nullptr;
  }

  if (ind->Type.Type != TYPE_Int) {
    ParseError(Loc, "String index must be of integer type");
    delete this;
    return nullptr;
  }

  if (sval->Type.Type == TYPE_String && sval->IsStrConst() && sval->GetStrConst(ec.Package).length() == 1) {
    const char *s = *sval->GetStrConst(ec.Package);
    val = new VIntLiteral((vuint8)s[0], sval->Loc);
    delete sval;
    sval = val->Resolve(ec); // will never fail
  } else if (sval->Type.Type == TYPE_Name && sval->IsNameConst() && VStr::length(*sval->GetNameConst()) == 1) {
    const char *s = *sval->GetNameConst();
    val = new VIntLiteral((vuint8)s[0], sval->Loc);
    delete sval;
    sval = val->Resolve(ec); // will never fail
  }

  if (sval->Type.Type != TYPE_Int && sval->Type.Type != TYPE_Byte) {
    ParseError(Loc, "Cannot assign type '%s' to string element", *sval->Type.GetName());
    delete this;
    return nullptr;
  }

  op->RequestAddressOf();

  genStringAssign = true;
  Type = VFieldType(TYPE_Void);
  return this;
}


//==========================================================================
//
//  VArrayElement::RequestAddressOf
//
//==========================================================================
void VArrayElement::RequestAddressOf () {
  if (op->Type.Type == TYPE_String) {
    ParseError(Loc, "Cannot get string element address");
  } else {
    if (Flags&FIELD_ReadOnly) ParseError(op->Loc, "Tried to assign to a read-only field");
  }
  if (AddressRequested) ParseError(Loc, "Multiple address of");
  AddressRequested = true;
}


//==========================================================================
//
//  VArrayElement::Emit
//
//==========================================================================
void VArrayElement::Emit (VEmitContext &ec) {
  op->Emit(ec);
  ind->Emit(ec);
  if (genStringAssign) {
    sval->Emit(ec);
    ec.AddStatement(OPC_StrSetChar, Loc);
  } else {
    if (op->Type.Type == TYPE_DynamicArray) {
      if (IsAssign) {
        ec.AddStatement(OPC_DynArrayElementGrow, RealType, Loc);
      } else {
        ec.AddStatement(OPC_DynArrayElement, RealType, Loc);
      }
    } else if (op->Type.Type == TYPE_String) {
      if (IsAssign) {
        ParseError(Loc, "Strings are immutable (yet) -- codegen");
      } else {
        ec.AddStatement(OPC_StrGetChar, Loc);
        return;
      }
    } else if (op->Type.Type == TYPE_SliceArray) {
      ec.AddStatement(OPC_SliceElement, RealType, Loc);
    } else {
      ec.AddStatement(OPC_ArrayElement, RealType, Loc);
    }
    if (!AddressRequested) EmitPushPointedCode(RealType, ec);
  }
}


//==========================================================================
//
//  VStringSlice::VStringSlice
//
//==========================================================================
VStringSlice::VStringSlice (VExpression *aop, VExpression *alo, VExpression *ahi, const TLocation &aloc)
  : VArrayElement(aop, alo, aloc)
  , hi(ahi)
{
  Flags = FIELD_ReadOnly;
}


//==========================================================================
//
//  VStringSlice::~VStringSlice
//
//==========================================================================
VStringSlice::~VStringSlice () {
  delete hi;
}


//==========================================================================
//
//  VStringSlice::SyntaxCopy
//
//==========================================================================
VExpression *VStringSlice::SyntaxCopy () {
  auto res = new VStringSlice();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VStringSlice::DoSyntaxCopyTo
//
//==========================================================================
void VStringSlice::DoSyntaxCopyTo (VExpression *e) {
  VArrayElement::DoSyntaxCopyTo(e);
  auto res = (VStringSlice *)e;
  res->hi = (hi ? hi->SyntaxCopy() : nullptr);
}


//==========================================================================
//
//  VStringSlice::DoResolve
//
//==========================================================================
VExpression *VStringSlice::DoResolve (VEmitContext &ec) {
  if (!op || !ind || !hi) {
    delete this;
    return nullptr;
  }

  // we need a copy for opDollar
  opcopy = op->SyntaxCopy();

  op = op->Resolve(ec);

  // hack: allow indexing of pointers to strings without `(*str)`
  /*
  if (op && op->Type.Type == TYPE_Pointer && op->Type.InnerType == TYPE_String) {
    delete op;
    op = new VPushPointed(opcopy);
    opcopy = op->SyntaxCopy();
    op = op->Resolve(ec);
    if (!op) { delete opcopy; delete this; return nullptr; }
  }
  */

  if (op) {
    // resolve index expressions
    auto oldIndArray = ec.SetIndexArray(this);
    ind = ind->Resolve(ec);
    hi = hi->Resolve(ec);
    ec.SetIndexArray(oldIndArray);
  }

  // we don't need this anymore
  delete opcopy;

  if (!op || !ind || !hi) {
    delete this;
    return nullptr;
  }

  if (op->Type.Type != TYPE_String) {
    ParseError(Loc, "Only string slices are supported (for now)");
    delete this;
    return nullptr;
  }

  if (ind->Type.Type != TYPE_Int || hi->Type.Type != TYPE_Int) {
    ParseError(Loc, "String slice indicies must be of integer type");
    delete this;
    return nullptr;
  }

  Type = VFieldType(TYPE_String);
  return this;
}


//==========================================================================
//
//  VStringSlice::ResolveAssignmentTarget
//
//==========================================================================
VExpression *VStringSlice::ResolveAssignmentTarget (VEmitContext &ec) {
  ParseError(Loc, "Cannot assign to string slice (yet)");
  return nullptr;
}


//==========================================================================
//
//  VArrayElement::ResolveCompleteAssign
//
//==========================================================================
VExpression *VStringSlice::ResolveCompleteAssign (VEmitContext &ec, VExpression *val, bool &resolved) {
  resolved = true; // anyway

  VExpression *rop = op->SyntaxCopy()->Resolve(ec);
  if (!rop) { delete val; delete this; return nullptr; }

  if (rop->Type.Type != TYPE_String && !(rop->Type.Type == TYPE_Pointer && rop->Type.InnerType == TYPE_String)) {
    ParseError(Loc, "Only string slices are supported (for now)");
    delete rop;
    delete val;
    delete this;
    return nullptr;
  }

  // hack: allow indexing of pointers to strings without `(*str)`
  /*
  if (rop->Type.Type == TYPE_Pointer) {
    delete rop;
    op = new VPushPointed(op);
    // we need a copy for opDollar
    opcopy = op->SyntaxCopy();
    op = op->Resolve(ec);
    if (!op) { delete opcopy; delete val; delete this; return nullptr; }
  } else*/ {
    // we need a copy for opDollar
    opcopy = op;
    op = rop;
  }

  if (op) {
    // resolve index expression
    auto oldIndArray = ec.SetIndexArray(this);
    ind = ind->Resolve(ec);
    hi = hi->Resolve(ec);
    ec.SetIndexArray(oldIndArray);
  }
  sval = (val ? val->Resolve(ec) : nullptr);

  // we don't need this anymore
  delete opcopy;

  if (!op || !ind || !hi || !sval) {
    delete this;
    return nullptr;
  }

  if (ind->Type.Type != TYPE_Int || hi->Type.Type != TYPE_Int) {
    ParseError(Loc, "String slice index must be of integer type");
    delete this;
    return nullptr;
  }

  if (sval->Type.Type != TYPE_String) {
    ParseError(Loc, "String slice new value must be of string type");
    delete this;
    return nullptr;
  }

  op->RequestAddressOf();

  genStringAssign = true;
  Type = VFieldType(TYPE_Void);
  return this;
}


//==========================================================================
//
//  VStringSlice::RequestAddressOf
//
//==========================================================================
void VStringSlice::RequestAddressOf () {
  ParseError(Loc, "Cannot get string slice address");
}


//==========================================================================
//
//  VStringSlice::Emit
//
//==========================================================================
void VStringSlice::Emit (VEmitContext &ec) {
  if (!op || !ind || !hi) return;
  op->Emit(ec);
  ind->Emit(ec);
  hi->Emit(ec);
  if (genStringAssign) {
    sval->Emit(ec);
    ec.AddStatement(OPC_StrSliceAssign, Loc);
  } else {
    ec.AddStatement(OPC_StrSlice, Loc);
  }
}


//==========================================================================
//
//  VDynArrayGetNum::VDynArrayGetNum
//
//==========================================================================
VDynArrayGetNum::VDynArrayGetNum (VExpression *AArrayExpr, const TLocation &ALoc)
  : VExpression(ALoc)
  , ArrayExpr(AArrayExpr)
{
  Flags = FIELD_ReadOnly;
}


//==========================================================================
//
//  VDynArrayGetNum::~VDynArrayGetNum
//
//==========================================================================
VDynArrayGetNum::~VDynArrayGetNum () {
  if (ArrayExpr) { delete ArrayExpr; ArrayExpr = nullptr; }
}


//==========================================================================
//
//  VDynArrayGetNum::SyntaxCopy
//
//==========================================================================
VExpression *VDynArrayGetNum::SyntaxCopy () {
  auto res = new VDynArrayGetNum();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VDynArrayGetNum::DoRestSyntaxCopyTo
//
//==========================================================================
void VDynArrayGetNum::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VDynArrayGetNum *)e;
  res->ArrayExpr = (ArrayExpr ? ArrayExpr->SyntaxCopy() : nullptr);
}


//==========================================================================
//
//  VDynArrayGetNum::DoResolve
//
//==========================================================================
VExpression *VDynArrayGetNum::DoResolve (VEmitContext &) {
  Type = VFieldType(TYPE_Int);
  return this;
}


//==========================================================================
//
//  VDynArrayGetNum::Emit
//
//==========================================================================
void VDynArrayGetNum::Emit (VEmitContext &ec) {
  ArrayExpr->Emit(ec);
  ec.AddStatement(OPC_DynArrayGetNum, Loc);
}


//==========================================================================
//
//  VDynArraySetNum::VDynArraySetNum
//
//==========================================================================
VDynArraySetNum::VDynArraySetNum (VExpression *AArrayExpr, VExpression *ANumExpr, const TLocation &ALoc)
  : VExpression(ALoc)
  , ArrayExpr(AArrayExpr)
  , NumExpr(ANumExpr)
  , opsign(0)
{
  Type = VFieldType(TYPE_Void);
}


//==========================================================================
//
//  VDynArraySetNum::~VDynArraySetNum
//
//==========================================================================
VDynArraySetNum::~VDynArraySetNum () {
  if (ArrayExpr) { delete ArrayExpr; ArrayExpr = nullptr; }
  if (NumExpr) { delete NumExpr; NumExpr = nullptr; }
}


//==========================================================================
//
//  VDynArraySetNum::SyntaxCopy
//
//==========================================================================
VExpression *VDynArraySetNum::SyntaxCopy () {
  auto res = new VDynArraySetNum();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VDynArraySetNum::DoRestSyntaxCopyTo
//
//==========================================================================
void VDynArraySetNum::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VDynArraySetNum *)e;
  res->ArrayExpr = (ArrayExpr ? ArrayExpr->SyntaxCopy() : nullptr);
  res->NumExpr = (NumExpr ? NumExpr->SyntaxCopy() : nullptr);
  res->opsign = opsign;
}


//==========================================================================
//
//  VDynArraySetNum::DoResolve
//
//==========================================================================
VExpression *VDynArraySetNum::DoResolve (VEmitContext &) {
  return this;
}


//==========================================================================
//
//  VDynArraySetNum::Emit
//
//==========================================================================
void VDynArraySetNum::Emit (VEmitContext &ec) {
  ArrayExpr->Emit(ec);
  NumExpr->Emit(ec);
  if (opsign == 0) {
    // normal assign
    ec.AddStatement(OPC_DynArraySetNum, ArrayExpr->Type.GetArrayInnerType(), Loc);
  } else if (opsign < 0) {
    // -=
    ec.AddStatement(OPC_DynArraySetNumMinus, ArrayExpr->Type.GetArrayInnerType(), Loc);
  } else {
    // +=
    ec.AddStatement(OPC_DynArraySetNumPlus, ArrayExpr->Type.GetArrayInnerType(), Loc);
  }
}


//==========================================================================
//
//  VDynArraySetNum::IsDynArraySetNum
//
//==========================================================================
bool VDynArraySetNum::IsDynArraySetNum () const {
  return true;
}


//==========================================================================
//
//  VDynArrayInsert::VDynArrayInsert
//
//==========================================================================
VDynArrayInsert::VDynArrayInsert (VExpression *AArrayExpr, VExpression *AIndexExpr, VExpression *ACountExpr, const TLocation &ALoc)
  : VExpression(ALoc)
  , ArrayExpr(AArrayExpr)
  , IndexExpr(AIndexExpr)
  , CountExpr(ACountExpr)
{
}


//==========================================================================
//
//  VDynArrayInsert::~VDynArrayInsert
//
//==========================================================================
VDynArrayInsert::~VDynArrayInsert () {
  if (ArrayExpr) { delete ArrayExpr; ArrayExpr = nullptr; }
  if (IndexExpr) { delete IndexExpr; IndexExpr = nullptr; }
  if (CountExpr) { delete CountExpr; CountExpr = nullptr; }
}


//==========================================================================
//
//  VDynArrayInsert::SyntaxCopy
//
//==========================================================================
VExpression *VDynArrayInsert::SyntaxCopy () {
  auto res = new VDynArrayInsert();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VDynArrayInsert::DoRestSyntaxCopyTo
//
//==========================================================================
void VDynArrayInsert::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VDynArrayInsert *)e;
  res->ArrayExpr = (ArrayExpr ? ArrayExpr->SyntaxCopy() : nullptr);
  res->IndexExpr = (IndexExpr ? IndexExpr->SyntaxCopy() : nullptr);
  res->CountExpr = (CountExpr ? CountExpr->SyntaxCopy() : nullptr);
}


//==========================================================================
//
//  VDynArrayInsert::DoResolve
//
//==========================================================================
VExpression *VDynArrayInsert::DoResolve (VEmitContext &ec) {
  if (ArrayExpr) ArrayExpr->RequestAddressOf();
  // resolve arguments
  if (IndexExpr) IndexExpr = IndexExpr->Resolve(ec);
  if (CountExpr) CountExpr = CountExpr->Resolve(ec);
  if (!IndexExpr || !CountExpr) {
    delete this;
    return nullptr;
  }

  // check argument types
  if (IndexExpr->Type.Type != TYPE_Int) {
    ParseError(Loc, "Index must be integer expression");
    delete this;
    return nullptr;
  }

  if (CountExpr->Type.Type != TYPE_Int) {
    ParseError(Loc, "Count must be integer expression");
    delete this;
    return nullptr;
  }

  Type = VFieldType(TYPE_Void);
  return this;
}


//==========================================================================
//
//  VDynArrayInsert::Emit
//
//==========================================================================
void VDynArrayInsert::Emit (VEmitContext &ec) {
  ArrayExpr->Emit(ec);
  IndexExpr->Emit(ec);
  CountExpr->Emit(ec);
  ec.AddStatement(OPC_DynArrayInsert, ArrayExpr->Type.GetArrayInnerType(), Loc);
}


//==========================================================================
//
//  VDynArrayRemove::VDynArrayRemove
//
//==========================================================================
VDynArrayRemove::VDynArrayRemove (VExpression *AArrayExpr, VExpression *AIndexExpr, VExpression *ACountExpr, const TLocation &ALoc)
  : VExpression(ALoc)
  , ArrayExpr(AArrayExpr)
  , IndexExpr(AIndexExpr)
  , CountExpr(ACountExpr)
{
}


//==========================================================================
//
//  VDynArrayRemove::~VDynArrayRemove
//
//==========================================================================
VDynArrayRemove::~VDynArrayRemove () {
  if (ArrayExpr) { delete ArrayExpr; ArrayExpr = nullptr; }
  if (IndexExpr) { delete IndexExpr; IndexExpr = nullptr; }
  if (CountExpr) { delete CountExpr; CountExpr = nullptr; }
}


//==========================================================================
//
//  VDynArrayRemove::SyntaxCopy
//
//==========================================================================
VExpression *VDynArrayRemove::SyntaxCopy () {
  auto res = new VDynArrayRemove();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VDynArrayRemove::DoRestSyntaxCopyTo
//
//==========================================================================
void VDynArrayRemove::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VDynArrayRemove *)e;
  res->ArrayExpr = (ArrayExpr ? ArrayExpr->SyntaxCopy() : nullptr);
  res->IndexExpr = (IndexExpr ? IndexExpr->SyntaxCopy() : nullptr);
  res->CountExpr = (CountExpr ? CountExpr->SyntaxCopy() : nullptr);
}


//==========================================================================
//
//  VDynArrayRemove::DoResolve
//
//==========================================================================
VExpression *VDynArrayRemove::DoResolve (VEmitContext &ec) {
  if (ArrayExpr) ArrayExpr->RequestAddressOf();
  // resolve arguments
  if (IndexExpr) IndexExpr = IndexExpr->Resolve(ec);
  if (CountExpr) CountExpr = CountExpr->Resolve(ec);
  if (!IndexExpr || !CountExpr) {
    delete this;
    return nullptr;
  }

  // check argument types
  if (IndexExpr->Type.Type != TYPE_Int) {
    ParseError(Loc, "Index must be integer expression");
    delete this;
    return nullptr;
  }

  if (CountExpr->Type.Type != TYPE_Int) {
    ParseError(Loc, "Count must be integer expression");
    delete this;
    return nullptr;
  }

  Type = VFieldType(TYPE_Void);
  return this;
}


//==========================================================================
//
//  VDynArrayRemove::Emit
//
//==========================================================================
void VDynArrayRemove::Emit (VEmitContext &ec) {
  ArrayExpr->Emit(ec);
  IndexExpr->Emit(ec);
  CountExpr->Emit(ec);
  ec.AddStatement(OPC_DynArrayRemove, ArrayExpr->Type.GetArrayInnerType(), Loc);
}


//==========================================================================
//
//  VStringGetLength::VStringGetLength
//
//==========================================================================
VStringGetLength::VStringGetLength(VExpression *AStrExpr, const TLocation &ALoc)
  : VExpression(ALoc)
  , StrExpr(AStrExpr)
{
  Flags = FIELD_ReadOnly;
}


//==========================================================================
//
//  VStringGetLength::~VStringGetLength
//
//==========================================================================
VStringGetLength::~VStringGetLength () {
  if (StrExpr) {
    delete StrExpr;
    StrExpr = nullptr;
  }
}


//==========================================================================
//
//  VStringGetLength::SyntaxCopy
//
//==========================================================================
VExpression *VStringGetLength::SyntaxCopy () {
  auto res = new VStringGetLength();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VStringGetLength::DoRestSyntaxCopyTo
//
//==========================================================================
void VStringGetLength::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VStringGetLength *)e;
  res->StrExpr = (StrExpr ? StrExpr->SyntaxCopy() : nullptr);
}


//==========================================================================
//
//  VStringGetLength::DoResolve
//
//==========================================================================
VExpression *VStringGetLength::DoResolve (VEmitContext &ec) {
  // optimize it for string literals
  if (StrExpr->IsStrConst()) {
    VStr val = StrExpr->GetStrConst(ec.Package);
    VExpression *e = new VIntLiteral(val.Length(), Loc);
    e = e->Resolve(ec);
    delete this;
    return e;
  }
  Type = VFieldType(TYPE_Int);
  return this;
}


//==========================================================================
//
//  VStringGetLength::Emit
//
//==========================================================================
void VStringGetLength::Emit (VEmitContext &ec) {
  StrExpr->Emit(ec);
  ec.AddStatement(OPC_StrLength, Loc);
}


//==========================================================================
//
//  VSliceGetLength
//
//==========================================================================
VSliceGetLength::VSliceGetLength (VExpression *asexpr, const TLocation &aloc)
  : VExpression(aloc)
  , sexpr(asexpr)
{
  Flags = FIELD_ReadOnly;
}

VSliceGetLength::~VSliceGetLength () {
  delete sexpr;
}

VExpression *VSliceGetLength::SyntaxCopy () {
  auto res = new VSliceGetLength();
  DoSyntaxCopyTo(res);
  return res;
}

void VSliceGetLength::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VSliceGetLength *)e;
  res->sexpr = (sexpr ? sexpr->SyntaxCopy() : nullptr);
}

VExpression *VSliceGetLength::DoResolve (VEmitContext &ec) {
  Type = VFieldType(TYPE_Int);
  return this;
}

void VSliceGetLength::Emit (VEmitContext &ec) {
  sexpr->Emit(ec);
  /*
  ec.AddStatement(OPC_OffsetPtr, (int)sizeof(void *), Loc);
  ec.AddStatement(OPC_PushPointed, Loc);
  */
  // use special instruction, so we can check for null ptr
  ec.AddStatement(OPC_PushPointedSliceLen, Loc);
}


//==========================================================================
//
//  VSliceGetPtr
//
//==========================================================================
VSliceGetPtr::VSliceGetPtr (VExpression *asexpr, const TLocation &aloc)
  : VExpression(aloc)
  , sexpr(asexpr)
{
  Flags = FIELD_ReadOnly;
}

VSliceGetPtr::~VSliceGetPtr () {
  delete sexpr;
}

VExpression *VSliceGetPtr::SyntaxCopy () {
  auto res = new VSliceGetPtr();
  DoSyntaxCopyTo(res);
  return res;
}

void VSliceGetPtr::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VSliceGetPtr *)e;
  res->sexpr = (sexpr ? sexpr->SyntaxCopy() : nullptr);
}

VExpression *VSliceGetPtr::DoResolve (VEmitContext &ec) {
  Type = VFieldType(TYPE_Int);
  Type = Type.MakePointerType();
  return this;
}

void VSliceGetPtr::Emit (VEmitContext &ec) {
  sexpr->Emit(ec);
  ec.AddStatement(OPC_PushPointedPtr, Loc);
}
