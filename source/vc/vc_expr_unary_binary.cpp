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
//  VUnary::VUnary
//
//==========================================================================
VUnary::VUnary (VUnary::EUnaryOp AOper, VExpression *AOp, const TLocation &ALoc)
  : VExpression(ALoc)
  , Oper(AOper)
  , op(AOp)
{
  if (!op) ParseError(Loc, "Expression expected");
}


//==========================================================================
//
//  VUnary::~VUnary
//
//==========================================================================
VUnary::~VUnary () {
  if (op) { delete op; op = nullptr; }
}


//==========================================================================
//
//  VUnary::SyntaxCopy
//
//==========================================================================
VExpression *VUnary::SyntaxCopy () {
  auto res = new VUnary();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VUnary::DoSyntaxCopyTo
//
//==========================================================================
void VUnary::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VUnary *)e;
  res->Oper = Oper;
  res->op = (op ? op->SyntaxCopy() : nullptr);
}


//==========================================================================
//
//  VUnary::DoResolve
//
//==========================================================================
VExpression *VUnary::DoResolve (VEmitContext &ec) {
  if (op) {
    if (Oper == Not) op = op->ResolveBoolean(ec); else op = op->Resolve(ec);
  }
  if (!op) {
    delete this;
    return nullptr;
  }

  switch (Oper) {
    case Plus:
      Type = op->Type;
      if (op->Type.Type != TYPE_Int && op->Type.Type != TYPE_Float) {
        ParseError(Loc, "Expression type mismatch");
        delete this;
        return nullptr;
      } else {
        VExpression *e = op;
        op = nullptr;
        delete this;
        return e;
      }
    case Minus:
           if (op->Type.Type == TYPE_Int) Type = TYPE_Int;
      else if (op->Type.Type == TYPE_Float) Type = TYPE_Float;
      else if (op->Type.Type == TYPE_Vector) Type = op->Type;
      else {
        ParseError(Loc, "Expression type mismatch");
        delete this;
        return nullptr;
      }
      break;
    case Not:
      Type = TYPE_Int;
      break;
    case BitInvert:
      if (op->Type.Type != TYPE_Int) {
        ParseError(Loc, "Expression type mismatch");
        delete this;
        return nullptr;
      }
      Type = TYPE_Int;
      break;
    case TakeAddress:
      if (op->Type.Type == TYPE_Reference) {
        ParseError(Loc, "Tried to take address of reference");
        delete this;
        return nullptr;
      } else {
        op->RequestAddressOf();
        Type = op->RealType.MakePointerType();
      }
      break;
  }

  // optimise integer constants
  if (op->IsIntConst()) {
    vint32 Value = op->GetIntConst();
    VExpression *e = nullptr;
    switch (Oper) {
      case Minus: e = new VIntLiteral(-Value, Loc); break;
      case Not: e = new VIntLiteral(!Value, Loc); break;
      case BitInvert: e = new VIntLiteral(~Value, Loc); break;
      default: break;
    }
    if (e) {
      delete this;
      return e;
    }
  }

  // optimise float constants
  if (op->IsFloatConst() && Oper == Minus) {
    float Value = op->GetFloatConst();
    VExpression *e = new VFloatLiteral(-Value, Loc);
    delete this;
    return e;
  }

  return this;
}


//==========================================================================
//
//  VUnary::Emit
//
//==========================================================================
void VUnary::Emit (VEmitContext &ec) {
  op->Emit(ec);

  switch (Oper) {
    case Plus:
      break;
    case Minus:
           if (op->Type.Type == TYPE_Int) ec.AddStatement(OPC_UnaryMinus);
      else if (op->Type.Type == TYPE_Float) ec.AddStatement(OPC_FUnaryMinus);
      else if (op->Type.Type == TYPE_Vector) ec.AddStatement(OPC_VUnaryMinus);
      break;
    case Not:
      ec.AddStatement(OPC_NegateLogical);
      break;
    case BitInvert:
      ec.AddStatement(OPC_BitInverse);
      break;
    case TakeAddress:
      break;
  }
}


//==========================================================================
//
//  VUnary::EmitBranchable
//
//==========================================================================
void VUnary::EmitBranchable (VEmitContext &ec, VLabel Lbl, bool OnTrue) {
  if (Oper == Not) {
    op->EmitBranchable(ec, Lbl, !OnTrue);
  } else {
    VExpression::EmitBranchable(ec, Lbl, OnTrue);
  }
}


//==========================================================================
//
//  VUnaryMutator::VUnaryMutator
//
//==========================================================================
VUnaryMutator::VUnaryMutator (EIncDec AOper, VExpression *AOp, const TLocation &ALoc)
  : VExpression(ALoc)
  , Oper(AOper)
  , op(AOp)
{
  if (!op) ParseError(Loc, "Expression expected");
}


//==========================================================================
//
//  VUnaryMutator::~VUnaryMutator
//
//==========================================================================
VUnaryMutator::~VUnaryMutator () {
  if (op) { delete op; op = nullptr; }
}


//==========================================================================
//
//  VUnaryMutator::SyntaxCopy
//
//==========================================================================
VExpression *VUnaryMutator::SyntaxCopy () {
  auto res = new VUnaryMutator();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VUnaryMutator::DoSyntaxCopyTo
//
//==========================================================================
void VUnaryMutator::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VUnaryMutator *)e;
  res->Oper = Oper;
  res->op = (op ? op->SyntaxCopy() : nullptr);
}


//==========================================================================
//
//  VUnaryMutator::DoResolve
//
//==========================================================================
VExpression *VUnaryMutator::DoResolve (VEmitContext &ec) {
  if (op) op = op->Resolve(ec);
  if (!op) {
    delete this;
    return nullptr;
  }

  if (op->Type.Type != TYPE_Int) {
    ParseError(Loc, "Expression type mismatch");
    delete this;
    return nullptr;
  }
  Type = TYPE_Int;
  op->RequestAddressOf();

  return this;
}


//==========================================================================
//
//  VUnaryMutator::Emit
//
//==========================================================================
void VUnaryMutator::Emit (VEmitContext &ec) {
  op->Emit(ec);
  switch (Oper) {
    case PreInc: ec.AddStatement(OPC_PreInc); break;
    case PreDec: ec.AddStatement(OPC_PreDec); break;
    case PostInc: ec.AddStatement(OPC_PostInc); break;
    case PostDec: ec.AddStatement(OPC_PostDec); break;
    case Inc: ec.AddStatement(OPC_IncDrop); break;
    case Dec: ec.AddStatement(OPC_DecDrop); break;
  }
}


//==========================================================================
//
//  VUnaryMutator::AddDropResult
//
//==========================================================================
bool VUnaryMutator::AddDropResult () {
  switch (Oper) {
    case PreInc: case PostInc: Oper = Inc; break;
    case PreDec: case PostDec: Oper = Dec; break;
    case Inc: case Dec: FatalError("Should not happen (inc/dec)");
  }
  Type = TYPE_Void;
  return true;
}


//==========================================================================
//
//  VBinary::VBinary
//
//==========================================================================
VBinary::VBinary (EBinOp AOper, VExpression *AOp1, VExpression *AOp2, const TLocation &ALoc)
  : VExpression(ALoc)
  , Oper(AOper)
  , op1(AOp1)
  , op2(AOp2)
{
  if (!op2) ParseError(Loc, "Expression expected");
}


//==========================================================================
//
//  VBinary::~VBinary
//
//==========================================================================
VBinary::~VBinary () {
  if (op1) { delete op1; op1 = nullptr; }
  if (op2) { delete op2; op2 = nullptr; }
}


//==========================================================================
//
//  VBinary::SyntaxCopy
//
//==========================================================================
VExpression *VBinary::SyntaxCopy () {
  auto res = new VBinary();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VBinary::DoSyntaxCopyTo
//
//==========================================================================
void VBinary::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VBinary *)e;
  res->Oper = Oper;
  res->op1 = (op1 ? op1->SyntaxCopy() : nullptr);
  res->op2 = (op2 ? op2->SyntaxCopy() : nullptr);
}


//==========================================================================
//
//  VBinary::DoResolve
//
//==========================================================================
VExpression *VBinary::DoResolve (VEmitContext &ec) {
  if (op1) op1 = op1->Resolve(ec);
  if (op2) op2 = op2->Resolve(ec);
  if (!op1 || !op2) { delete this; return nullptr; }

  // coerce both to floats
  //k8:FIXME: simplify this!

  // if op1 is `float` or `vector`, and op2 is integral -> coerce op2
  if ((op1->Type.Type == TYPE_Float || op1->Type.Type == TYPE_Vector) &&
      (op2->Type.Type == TYPE_Int || op2->Type.Type == TYPE_Byte))
  {
    //printf("*** OP1 require float, and OP2 is integral: COERCING OP2\n");
    op2 = op2->CoerceToFloat();
    if (!op2) { delete this; return nullptr; }
  }

  // if op2 is `float` or `vector`, and op1 is integral -> coerce op1
  if ((op2->Type.Type == TYPE_Float || op2->Type.Type == TYPE_Vector) &&
      (op1->Type.Type == TYPE_Int || op1->Type.Type == TYPE_Byte))
  {
    //printf("*** OP2 require float, and OP1 is integral: COERCING OP1\n");
    op1 = op1->CoerceToFloat();
    if (!op1) { delete this; return nullptr; }
  }

  if (ec.Package->Name == NAME_decorate) {
    if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Float) {
      /*
      VExpression *TmpArgs[1];
      TmpArgs[0] = op1;
      op1 = new VInvocation(nullptr, ec.SelfClass->FindMethodChecked(
        "itof"), nullptr, false, false, op1->Loc, 1, TmpArgs);
      op1 = op1->Resolve(ec);
      */
      op1 = (new VScalarToFloat(op1))->Resolve(ec);
      if (!op1) { delete this; return nullptr; } // oops
    } else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Int) {
      /*
      VExpression *TmpArgs[1];
      TmpArgs[0] = op2;
      op2 = new VInvocation(nullptr, ec.SelfClass->FindMethodChecked(
        "itof"), nullptr, false, false, op2->Loc, 1, TmpArgs);
      op2 = op2->Resolve(ec);
      */
      op2 = (new VScalarToFloat(op2))->Resolve(ec);
      if (!op2) { delete this; return nullptr; } // oops
    }
  }

  // determine resulting type (and check operand types)
  switch (Oper) {
    case Add:
    case Subtract:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) Type = TYPE_Int;
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) Type = TYPE_Float;
      else if (op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Vector) Type = TYPE_Vector;
      else {
        ParseError(Loc, "Expression type mismatch");
        delete this;
        return nullptr;
      }
      break;
    case Multiply:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) Type = TYPE_Int;
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) Type = TYPE_Float;
      else if (op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Float) Type = TYPE_Vector;
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Vector) Type = TYPE_Vector;
      else {
        ParseError(Loc, "Expression type mismatch");
        delete this;
        return nullptr;
      }
      break;
    case Divide:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) Type = TYPE_Int;
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) Type = TYPE_Float;
      else if (op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Float) Type = TYPE_Vector;
      else {
        ParseError(Loc, "Expression type mismatch");
        delete this;
        return nullptr;
      }
      break;
    case Modulus:
    case LShift:
    case RShift:
    case And:
    case XOr:
    case Or:
      if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) Type = TYPE_Int;
      else {
        ParseError(Loc, "Expression type mismatch");
        delete this;
        return nullptr;
      }
      break;
    case Less:
    case LessEquals:
    case Greater:
    case GreaterEquals:
      if (!(op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) &&
          !(op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) &&
          !(op1->Type.Type == TYPE_String && op2->Type.Type == TYPE_String))
      {
        ParseError(Loc, "Expression type mismatch");
        delete this;
        return nullptr;
      }
      Type = TYPE_Int;
      break;
    case Equals:
    case NotEquals:
      if (!(op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) &&
          !(op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) &&
          !(op1->Type.Type == TYPE_Name && op2->Type.Type == TYPE_Name) &&
          !(op1->Type.Type == TYPE_Pointer && op2->Type.Type == TYPE_Pointer) &&
          !(op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Vector) &&
          !(op1->Type.Type == TYPE_Class && op2->Type.Type == TYPE_Class) &&
          !(op1->Type.Type == TYPE_State && op2->Type.Type == TYPE_State) &&
          !(op1->Type.Type == TYPE_Reference && op2->Type.Type == TYPE_Reference) &&
          !(op1->Type.Type == TYPE_String && op2->Type.Type == TYPE_String))
      {
        ParseError(Loc, "Expression type mismatch");
        delete this;
        return nullptr;
      }
      Type = TYPE_Int;
      break;
  }

  // optimise integer constants
  if (op1->IsIntConst() && op2->IsIntConst()) {
    vint32 Value1 = op1->GetIntConst();
    vint32 Value2 = op2->GetIntConst();
    VExpression *e = nullptr;
    switch (Oper) {
      case Add: e = new VIntLiteral(Value1+Value2, Loc); break;
      case Subtract: e = new VIntLiteral(Value1-Value2, Loc); break;
      case Multiply: e = new VIntLiteral(Value1*Value2, Loc); break;
      case Divide:
        if (!Value2) {
          ParseError(Loc, "Division by 0");
          delete this;
          return nullptr;
        }
        e = new VIntLiteral(Value1/Value2, Loc);
        break;
      case Modulus:
        if (!Value2) {
          ParseError(Loc, "Division by 0");
          delete this;
          return nullptr;
        }
        e = new VIntLiteral(Value1%Value2, Loc);
        break;
      case LShift: e = new VIntLiteral(Value1<<Value2, Loc); break;
      case RShift: e = new VIntLiteral(Value1>>Value2, Loc); break;
      case Less: e = new VIntLiteral(Value1 < Value2, Loc); break;
      case LessEquals: e = new VIntLiteral(Value1 <= Value2, Loc); break;
      case Greater: e = new VIntLiteral(Value1 > Value2, Loc); break;
      case GreaterEquals: e = new VIntLiteral(Value1 >= Value2, Loc); break;
      case Equals: e = new VIntLiteral(Value1 == Value2, Loc); break;
      case NotEquals: e = new VIntLiteral(Value1 != Value2, Loc); break;
      case And: e = new VIntLiteral(Value1 & Value2, Loc); break;
      case XOr: e = new VIntLiteral(Value1 ^ Value2, Loc); break;
      case Or: e = new VIntLiteral(Value1 | Value2, Loc); break;
      default: break;
    }
    if (e) { delete this; return e; }
  }

  // optimise float constants
  if (op1->IsFloatConst() && op2->IsFloatConst()) {
    float Value1 = op1->GetFloatConst();
    float Value2 = op2->GetFloatConst();
    VExpression *e = nullptr;
    switch (Oper) {
      case Add: e = new VFloatLiteral(Value1+Value2, Loc); break;
      case Subtract: e = new VFloatLiteral(Value1-Value2, Loc); break;
      case Multiply: e = new VFloatLiteral(Value1*Value2, Loc); break;
      case Divide:
        if (!Value2) {
          ParseError(Loc, "Division by 0");
          delete this;
          return nullptr;
        }
        e = new VFloatLiteral(Value1/Value2, Loc);
        break;
      default: break;
    }
    if (e) { delete this; return e; }
  }

  bool isOp1Zero = ((op1->IsIntConst() && op1->GetIntConst() == 0) || (op1->IsFloatConst() && op1->GetFloatConst() == 0));
  bool isOp2Zero = ((op2->IsIntConst() && op2->GetIntConst() == 0) || (op2->IsFloatConst() && op2->GetFloatConst() == 0));

  bool isOp1One = ((op1->IsIntConst() && op1->GetIntConst() == 1) || (op1->IsFloatConst() && op1->GetFloatConst() == 1));
  bool isOp2One = ((op2->IsIntConst() && op2->GetIntConst() == 1) || (op2->IsFloatConst() && op2->GetFloatConst() == 1));

  // division by zero check
  if (isOp2Zero && (Oper == Divide || Oper == Modulus)) {
    ParseError(Loc, "Division by 0");
    delete this;
    return nullptr;
  }

  // optimize 0+n
  if (Oper == Add && isOp1Zero) { VExpression *e = op2; op2 = nullptr; delete this; return e; }
  // optimize n+0
  if (Oper == Add && isOp2Zero) { VExpression *e = op1; op1 = nullptr; delete this; return e; }

  // optimize n-0
  if (Oper == Subtract && isOp2Zero) { VExpression *e = op1; op1 = nullptr; delete this; return e; }

  // optimize 0*n
  if (Oper == Multiply && isOp1Zero) { VExpression *e = op1; op1 = nullptr; delete this; return e; }
  // optimize n*0
  if (Oper == Multiply && isOp2Zero) { VExpression *e = op2; op2 = nullptr; delete this; return e; }
  // optimize 1*n
  if (Oper == Multiply && isOp1One) { VExpression *e = op2; op2 = nullptr; delete this; return e; }
  // optimize n*1
  if (Oper == Multiply && isOp2One) { VExpression *e = op1; op1 = nullptr; delete this; return e; }

  // optimize n/1
  if (Oper == Divide && isOp2One) { VExpression *e = op1; op1 = nullptr; delete this; return e; }

  return this;
}


//==========================================================================
//
//  VBinary::Emit
//
//==========================================================================
void VBinary::Emit (VEmitContext &ec) {
  op1->Emit(ec);
  op2->Emit(ec);

  switch (Oper) {
    case Add:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_Add);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FAdd);
      else if (op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Vector) ec.AddStatement(OPC_VAdd);
      break;
    case Subtract:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_Subtract);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FSubtract);
      else if (op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Vector) ec.AddStatement(OPC_VSubtract);
      break;
    case Multiply:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_Multiply);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FMultiply);
      else if (op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_VPostScale);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Vector) ec.AddStatement(OPC_VPreScale);
      break;
    case Divide:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_Divide);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FDivide);
      else if (op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_VIScale);
      break;
    case Modulus:
      if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_Modulus);
      break;
    case LShift:
      if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_LShift);
      break;
    case RShift:
      if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_RShift);
      break;
    case Less:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_Less);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FLess);
      else if (op1->Type.Type == TYPE_String && op2->Type.Type == TYPE_String) ec.AddStatement(OPC_StrLess);
      break;
    case LessEquals:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_LessEquals);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FLessEquals);
      else if (op1->Type.Type == TYPE_String && op2->Type.Type == TYPE_String) ec.AddStatement(OPC_StrLessEqu);
      break;
    case Greater:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_Greater);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FGreater);
      else if (op1->Type.Type == TYPE_String && op2->Type.Type == TYPE_String) ec.AddStatement(OPC_StrGreat);
      break;
    case GreaterEquals:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_GreaterEquals);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FGreaterEquals);
      else if (op1->Type.Type == TYPE_String && op2->Type.Type == TYPE_String) ec.AddStatement(OPC_StrGreatEqu);
      break;
    case Equals:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_Equals);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FEquals);
      else if (op1->Type.Type == TYPE_Name && op2->Type.Type == TYPE_Name) ec.AddStatement(OPC_Equals);
      else if (op1->Type.Type == TYPE_Pointer && op2->Type.Type == TYPE_Pointer) ec.AddStatement(OPC_PtrEquals);
      else if (op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Vector) ec.AddStatement(OPC_VEquals);
      else if (op1->Type.Type == TYPE_Class && op2->Type.Type == TYPE_Class) ec.AddStatement(OPC_PtrEquals);
      else if (op1->Type.Type == TYPE_State && op2->Type.Type == TYPE_State) ec.AddStatement(OPC_PtrEquals);
      else if (op1->Type.Type == TYPE_Reference && op2->Type.Type == TYPE_Reference) ec.AddStatement(OPC_PtrEquals);
      else if (op1->Type.Type == TYPE_String && op2->Type.Type == TYPE_String) ec.AddStatement(OPC_StrEquals);
      break;
    case NotEquals:
           if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_NotEquals);
      else if (op1->Type.Type == TYPE_Float && op2->Type.Type == TYPE_Float) ec.AddStatement(OPC_FNotEquals);
      else if (op1->Type.Type == TYPE_Name && op2->Type.Type == TYPE_Name) ec.AddStatement(OPC_NotEquals);
      else if (op1->Type.Type == TYPE_Pointer && op2->Type.Type == TYPE_Pointer) ec.AddStatement(OPC_PtrNotEquals);
      else if (op1->Type.Type == TYPE_Vector && op2->Type.Type == TYPE_Vector) ec.AddStatement(OPC_VNotEquals);
      else if (op1->Type.Type == TYPE_Class && op2->Type.Type == TYPE_Class) ec.AddStatement(OPC_PtrNotEquals);
      else if (op1->Type.Type == TYPE_State && op2->Type.Type == TYPE_State) ec.AddStatement(OPC_PtrNotEquals);
      else if (op1->Type.Type == TYPE_Reference && op2->Type.Type == TYPE_Reference) ec.AddStatement(OPC_PtrNotEquals);
      else if (op1->Type.Type == TYPE_String && op2->Type.Type == TYPE_String) ec.AddStatement(OPC_StrNotEquals);
      break;
    case And:
      if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_AndBitwise);
      break;
    case XOr:
      if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_XOrBitwise);
      break;
    case Or:
      if (op1->Type.Type == TYPE_Int && op2->Type.Type == TYPE_Int) ec.AddStatement(OPC_OrBitwise);
      break;
  }
}


//==========================================================================
//
// VBinary::IsBinaryMath
//
//==========================================================================
bool VBinary::IsBinaryMath () const {
  return true;
}


//==========================================================================
//
//  VBinaryLogical::VBinaryLogical
//
//==========================================================================
VBinaryLogical::VBinaryLogical (ELogOp AOper, VExpression *AOp1, VExpression *AOp2, const TLocation &ALoc)
  : VExpression(ALoc)
  , Oper(AOper)
  , op1(AOp1)
  , op2(AOp2)
{
  if (!op2) ParseError(Loc, "Expression expected");
}


//==========================================================================
//
//  VBinaryLogical::~VBinaryLogical
//
//==========================================================================
VBinaryLogical::~VBinaryLogical () {
  if (op1) { delete op1; op1 = nullptr; }
  if (op2) { delete op2; op2 = nullptr; }
}


//==========================================================================
//
//  VBinaryLogical::SyntaxCopy
//
//==========================================================================
VExpression *VBinaryLogical::SyntaxCopy () {
  auto res = new VBinaryLogical();
  DoSyntaxCopyTo(res);
  return res;
}


//==========================================================================
//
//  VBinaryLogical::DoSyntaxCopyTo
//
//==========================================================================
void VBinaryLogical::DoSyntaxCopyTo (VExpression *e) {
  VExpression::DoSyntaxCopyTo(e);
  auto res = (VBinaryLogical *)e;
  res->Oper = Oper;
  res->op1 = (op1 ? op1->SyntaxCopy() : nullptr);
  res->op2 = (op2 ? op2->SyntaxCopy() : nullptr);
}


//==========================================================================
//
//  VBinaryLogical::DoResolve
//
//==========================================================================
VExpression *VBinaryLogical::DoResolve (VEmitContext &ec) {
  if (op1) op1 = op1->ResolveBoolean(ec);
  if (op2) op2 = op2->ResolveBoolean(ec);
  if (!op1 || !op2) {
    delete this;
    return nullptr;
  }

  Type = TYPE_Int;

  // optimise constant cases
  if (op1->IsIntConst() && op2->IsIntConst()) {
    vint32 Value1 = op1->GetIntConst();
    vint32 Value2 = op2->GetIntConst();
    VExpression *e = nullptr;
    switch (Oper) {
      case And: e = new VIntLiteral(Value1 && Value2, Loc); break;
      case Or: e = new VIntLiteral(Value1 || Value2, Loc); break;
    }
    if (e) {
      delete this;
      return e;
    }
  }

  return this;
}


//==========================================================================
//
//  VBinaryLogical::Emit
//
//==========================================================================
void VBinaryLogical::Emit (VEmitContext &ec) {
  VLabel Push01 = ec.DefineLabel();
  VLabel End = ec.DefineLabel();

  op1->EmitBranchable(ec, Push01, Oper == Or);

  op2->Emit(ec);
  ec.AddStatement(OPC_Goto, End);

  ec.MarkLabel(Push01);
  ec.AddStatement(Oper == And ? OPC_PushNumber0 : OPC_PushNumber1);

  ec.MarkLabel(End);
}


//==========================================================================
//
//  VBinaryLogical::EmitBranchable
//
//==========================================================================
void VBinaryLogical::EmitBranchable (VEmitContext &ec, VLabel Lbl, bool OnTrue) {
  switch (Oper) {
    case And:
      if (OnTrue) {
        VLabel End = ec.DefineLabel();
        op1->EmitBranchable(ec, End, false);
        op2->EmitBranchable(ec, Lbl, true);
        ec.MarkLabel(End);
      } else {
        op1->EmitBranchable(ec, Lbl, false);
        op2->EmitBranchable(ec, Lbl, false);
      }
      break;
    case Or:
      if (OnTrue) {
        op1->EmitBranchable(ec, Lbl, true);
        op2->EmitBranchable(ec, Lbl, true);
      } else {
        VLabel End = ec.DefineLabel();
        op1->EmitBranchable(ec, End, true);
        op2->EmitBranchable(ec, Lbl, false);
        ec.MarkLabel(End);
      }
      break;
  }
}
