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
//**	Copyright (C) 1999-2001 J�nis Legzdi��
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
//**	$Log$
//**	Revision 1.2  2001/07/27 14:27:56  dj_jl
//**	Update with Id-s and Log-s, some fixes
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "vcc.h"

// MACROS ------------------------------------------------------------------

#define MAX_ARG_COUNT		16

// TYPES -------------------------------------------------------------------

class TOperator
{
 public:
	enum id_t 
	{
		ID_UNARYPLUS,
		ID_UNARYMINUS,
		ID_NEGATELOGICAL,
		ID_BITINVERSE,
		ID_PREINC,
		ID_PREDEC,
		ID_POSTINC,
		ID_POSTDEC,
		ID_MULTIPLY,
        ID_DIVIDE,
        ID_MODULUS,
    	ID_ADD,
        ID_SUBTRACT,
    	ID_LSHIFT,
        ID_RSHIFT,
    	ID_LT,
        ID_LE,
        ID_GT,
		ID_GE,
    	ID_EQ,
        ID_NE,
		ID_ANDBITWISE,
		ID_XORBITWISE,
		ID_ORBITWISE,

		ID_ASSIGN,
		ID_ADDVAR,
		ID_SUBVAR,
		ID_MULVAR,
		ID_DIVVAR,
		ID_MODVAR,
		ID_ANDVAR,
		ID_ORVAR,
		ID_XORVAR,
		ID_LSHIFTVAR,
		ID_RSHIFTVAR,

		NUM_OPERATORS
	};

	TOperator(id_t Aopid, TType* Atype, TType* Atype1, TType* Atype2, int Aopcode);

	TOperator	*next;
	id_t		opid;
	TType		*type;
	TType		*type1;
	TType		*type2;
	int			opcode;
};

class TOp
{
 public:
	TOp(void) : oper(NULL) {}
	TOp(TOperator *Aoper) : oper(Aoper) {}
	virtual ~TOp(void) {}
	virtual void Code(void){}
	virtual TOp *GetAddress(void)
	{
		ParseError("Invalid address operation");
		return this;
	}

	TType		*type;
	TOperator	*oper;
};

class TOp1 : public TOp
{
 public:
	TOp1(TOp *Aop, TOperator *Aoper) : TOp(Aoper), op(Aop)
	{
		if (op->type->type == ev_vector)
			type = op->type;
		else
			type = oper->type;
	}
	~TOp1(void)
	{
		if (op) delete op;
	}
	void Code(void)
	{
		if (op) op->Code();
		if (oper && oper->opcode != OPC_DONE) AddStatement(oper->opcode);
	}

	TOp			*op;
};

class TOp2 : public TOp
{
 public:
	TOp2(TOp *Aop1, TOp *Aop2, TOperator *Aoper) : TOp(Aoper), op1(Aop1), op2(Aop2)
	{
		if (oper->type == &type_void_ptr && op1->type->type == ev_pointer)
			type = op1->type;
		else
			type = oper->type;
	}
	~TOp2(void)
	{
		if (op1) delete op1;
		if (op2) delete op2;
	}
	void Code(void)
	{
		if (op1) op1->Code();
		if (op2) op2->Code();
		if (oper) AddStatement(oper->opcode);
	}

	TOp			*op1;
	TOp			*op2;
};

class TOpAnd : public TOp2
{
 public:
	TOpAnd(TOp *Aop1, TOp *Aop2) : TOp2(Aop1, Aop2, NULL)
	{
		TypeCheck1(op1->type);
		TypeCheck1(op2->type);
		type = &type_int;
	}
	void Code(void)
	{
	    int*		jmppos;

		if (op1) op1->Code();
		jmppos = AddStatement(OPC_IFNOTTOPGOTO, 0);
		if (op2) op2->Code();
		AddStatement(OPC_ANDLOGICAL);
    	*jmppos = CodeBufferSize;
	}
};

class TOpOr : public TOp2
{
 public:
	TOpOr(TOp *Aop1, TOp *Aop2) : TOp2(Aop1, Aop2, NULL)
	{
		TypeCheck1(op1->type);
		TypeCheck1(op2->type);
		type = &type_int;
	}
	void Code(void)
	{
	    int*		jmppos;

		if (op1) op1->Code();
		jmppos = AddStatement(OPC_IFTOPGOTO, 0);
		if (op2) op2->Code();
		AddStatement(OPC_ORLOGICAL);
    	*jmppos = CodeBufferSize;
	}
};

class TOpCond : public TOp
{
 public:
	TOpCond(TOp *Aexpr, TOp *Aop1, TOp *Aop2) : expr(Aexpr), op1(Aop1), op2(Aop2)
	{
		TypeCheck1(expr->type);
		TypeCheck3(op1->type, op2->type);
		if (op1->type == &type_void_ptr)
			type = op2->type;
		else
			type = op1->type;
	}
	~TOpCond(void)
	{
		if (expr) delete expr;
		if (op1) delete op1;
		if (op2) delete op2;
	}
	void Code(void)
	{
       	int*	jumppos1;
       	int*	jumppos2;

		if (expr) expr->Code();
		jumppos1 = AddStatement(OPC_IFNOTGOTO, 0);
		if (op1) op1->Code();
		jumppos2 = AddStatement(OPC_GOTO, 0);
		*jumppos1 = CodeBufferSize;
		if (op2) op2->Code();
		*jumppos2 = CodeBufferSize;
	}

	TOp	*expr;
	TOp *op1;
	TOp *op2;
};

class TOpPushPointed : public TOp
{
 public:
	TOpPushPointed(TOp *Aop) : op(Aop)
	{
		if (op->type->type != ev_pointer)
		{
			ParseError("Expression syntax error");
			type = op->type;
		}
		else
		{
			type = op->type->aux_type;
		}
	}
	TOpPushPointed(TOp *Aop, TType *Atype) : op(Aop)
	{
		type = Atype;
	}
	~TOpPushPointed(void)
	{
		if (op) delete op;
	}
	void Code(void)
	{
		if (op) op->Code();
		if (type->type == ev_vector)
		{
			AddStatement(OPC_VPUSHPOINTED);
		}
		else
		{
			AddStatement(OPC_PUSHPOINTED);
		}
	}
	TOp *GetAddress(void)
	{
		TOp *tmp;
		tmp = op;
		op = NULL;
		delete this;
		return tmp;
	}

	TOp			*op;
};

class TOpArrayIndex : public TOp
{
 public:
	TOpArrayIndex(TOp *Aop1, TOp *Aop2, TType *Atype) : op1(Aop1), op2(Aop2)
	{
		type = MakePointerType(Atype);
		type_size = TypeSize(Atype);
	}
	~TOpArrayIndex(void)
	{
		if (op1) delete op1;
		if (op2) delete op2;
	}
	void Code(void)
	{
		if (op1) op1->Code();
		if (op2) op2->Code();
		AddStatement(OPC_PUSHNUMBER, type_size);
		AddStatement(OPC_MULTIPLY);
		AddStatement(OPC_ADD);
	}

	TOp			*op1;
	TOp			*op2;
	int			type_size;
};

class TOpField : public TOp
{
 public:
	TOpField(TOp *Aop, int Aoffs, TType *Atype) : op(Aop), offs(Aoffs)
	{
		type = MakePointerType(Atype);
	}
	~TOpField(void)
	{
		if (op) delete op;
	}
	void Code(void)
	{
		if (op) op->Code();
		AddStatement(OPC_PUSHNUMBER, offs);
		AddStatement(OPC_ADD);
	}

	TOp			*op;
	int			offs;
};

class TOpConst : public TOp
{
 public:
	TOpConst(int Aval, TType *Atype) : val(Aval)
	{
		type = Atype;
	}
	~TOpConst(void)
	{
	}
	void Code(void)
	{
		AddStatement(OPC_PUSHNUMBER, val);
	}

	int			val;
};

class TOpLocal : public TOp
{
 public:
	TOpLocal(int Aoffs, TType *Atype) : offs(Aoffs)
	{
		type = Atype;
	}
	~TOpLocal(void)
	{
	}
	void Code(void)
	{
		AddStatement(OPC_LOCALADDRESS, offs);
	}

	int			offs;
};

class TOpGlobal : public TOp
{
 public:
	TOpGlobal(int Aoffs, TType *Atype) : offs(Aoffs)
	{
		type = Atype;
	}
	~TOpGlobal(void)
	{
	}
	void Code(void)
	{
		AddStatement(OPC_GLOBALADDRESS, offs);
	}

	int			offs;
};

class TOpFuncCall : public TOp
{
 public:
	TOpFuncCall(int Afnum) : fnum(Afnum), ftype(functions[fnum].type)
	{
		type = ftype->aux_type;
		for (int i = 0; i < MAX_ARG_COUNT; i++)
			parms[i] = NULL;
	}
	~TOpFuncCall(void)
	{
		for (int i = 0; i < MAX_ARG_COUNT; i++)
			if (parms[i])
				delete parms[i];
	}
	void Code(void)
	{
		for (int i = 0; i < MAX_ARG_COUNT; i++)
		{
			if (parms[i])
			{
				parms[i]->Code();
			}
		}
		AddStatement(OPC_CALL, fnum);
	}

	int			fnum;
	TType		*ftype;
	TOp			*parms[MAX_ARG_COUNT];
};

class TOpIndirectFuncCall : public TOp
{
 public:
	TOpIndirectFuncCall(TOp *Afnumop, TType *Aftype) : fnumop(Afnumop), ftype(Aftype)
	{
		type = ftype->aux_type;
		for (int i = 0; i < MAX_PARAMS; i++)
			parms[i] = NULL;
	}
	~TOpIndirectFuncCall(void)
	{
		for (int i = 0; i < MAX_PARAMS; i++)
			if (parms[i])
				delete parms[i];
	}
	void Code(void)
	{
		if (fnumop) fnumop->Code();
		for (int i = 0; i < ftype->num_params; i++)
		{
			if (parms[i])
			{
				parms[i]->Code();
				AddStatement(OPC_SWAP);
			}
		}
		AddStatement(OPC_ICALL);
	}

	TOp			*fnumop;
	TType		*ftype;
	TOp			*parms[MAX_PARAMS];
};

class TOpVector : public TOp
{
 public:
	TOpVector(TOp *Aop1, TOp *Aop2, TOp *Aop3) : op1(Aop1), op2(Aop2), op3(Aop3)
	{
		if (op1->type != &type_float)
			ParseError("Expression type mistmatch, vector param 1 is not a float");
		if (op2->type != &type_float)
			ParseError("Expression type mistmatch, vector param 2 is not a float");
		if (op3->type != &type_float)
			ParseError("Expression type mistmatch, vector param 3 is not a float");
		type = &type_vector;
	}
	~TOpVector(void)
	{
		if (op1) delete op1;
		if (op2) delete op2;
		if (op3) delete op3;
	}
	void Code(void)
	{
		if (op1) op1->Code();
		if (op2) op2->Code();
		if (op3) op3->Code();
	}

	TOp *op1;
	TOp *op2;
	TOp *op3;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static TOp *ParseExpressionPriority2(void);
static TOp* ParseExpressionPriority14(void);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static TOperator	*operators[TOperator::NUM_OPERATORS];

static TOperator	UnaryPlus_int(TOperator::ID_UNARYPLUS, &type_int, &type_int, &type_void, OPC_DONE);
static TOperator	UnaryPlus_uint(TOperator::ID_UNARYPLUS, &type_uint, &type_uint, &type_void, OPC_DONE);
static TOperator	UnaryPlus_float(TOperator::ID_UNARYPLUS, &type_float, &type_float, &type_void, OPC_DONE);

static TOperator	UnaryMinus_int(TOperator::ID_UNARYMINUS, &type_int, &type_int, &type_void, OPC_UNARYMINUS);
static TOperator	UnaryMinus_uint(TOperator::ID_UNARYMINUS, &type_uint, &type_uint, &type_void, OPC_UNARYMINUS);
static TOperator	UnaryMinus_float(TOperator::ID_UNARYMINUS, &type_float, &type_float, &type_void, OPC_FUNARYMINUS);
static TOperator	UnaryMinus_vector(TOperator::ID_UNARYMINUS, &type_vector, &type_vector, &type_void, OPC_VUNARYMINUS);

static TOperator	NotLogical_int(TOperator::ID_NEGATELOGICAL, &type_int, &type_int, &type_void, OPC_NEGATELOGICAL);
static TOperator	NotLogical_uint(TOperator::ID_NEGATELOGICAL, &type_int, &type_uint, &type_void, OPC_NEGATELOGICAL);
static TOperator	NotLogical_float(TOperator::ID_NEGATELOGICAL, &type_int, &type_float, &type_void, OPC_NEGATELOGICAL);
static TOperator	NotLogical_str(TOperator::ID_NEGATELOGICAL, &type_int, &type_string, &type_void, OPC_NEGATELOGICAL);
static TOperator	NotLogical_func(TOperator::ID_NEGATELOGICAL, &type_int, &type_function, &type_void, OPC_NEGATELOGICAL);
static TOperator	NotLogical_ptr(TOperator::ID_NEGATELOGICAL, &type_int, &type_void_ptr, &type_void, OPC_NEGATELOGICAL);

static TOperator	BitInverse_int(TOperator::ID_BITINVERSE, &type_int, &type_int, &type_void, OPC_BITINVERSE);
static TOperator	BitInverse_uint(TOperator::ID_BITINVERSE, &type_uint, &type_uint, &type_void, OPC_BITINVERSE);

static TOperator	PreInc_int(TOperator::ID_PREINC, &type_int, &type_int, &type_void, OPC_PREINC);
static TOperator	PreInc_uint(TOperator::ID_PREINC, &type_uint, &type_uint, &type_void, OPC_PREINC);

static TOperator	PreDec_int(TOperator::ID_PREDEC, &type_int, &type_int, &type_void, OPC_PREDEC);
static TOperator	PreDec_uint(TOperator::ID_PREDEC, &type_uint, &type_uint, &type_void, OPC_PREDEC);

static TOperator	PostInc_int(TOperator::ID_POSTINC, &type_int, &type_int, &type_void, OPC_POSTINC);
static TOperator	PostInc_uint(TOperator::ID_POSTINC, &type_uint, &type_uint, &type_void, OPC_POSTINC);

static TOperator	PostDec_int(TOperator::ID_POSTDEC, &type_int, &type_int, &type_void, OPC_POSTDEC);
static TOperator	PostDec_uint(TOperator::ID_POSTDEC, &type_uint, &type_uint, &type_void, OPC_POSTDEC);

static TOperator	Mul_int_int(TOperator::ID_MULTIPLY, &type_int, &type_int, &type_int, OPC_MULTIPLY);
static TOperator	Mul_int_uint(TOperator::ID_MULTIPLY, &type_uint, &type_int, &type_uint, OPC_MULTIPLY);
static TOperator	Mul_uint_int(TOperator::ID_MULTIPLY, &type_uint, &type_uint, &type_int, OPC_MULTIPLY);
static TOperator	Mul_uint_uint(TOperator::ID_MULTIPLY, &type_uint, &type_uint, &type_uint, OPC_MULTIPLY);
static TOperator	Mul_float_float(TOperator::ID_MULTIPLY, &type_float, &type_float, &type_float, OPC_FMULTIPLY);
static TOperator	Mul_vec_float(TOperator::ID_MULTIPLY, &type_vector, &type_vector, &type_float, OPC_VPOSTSCALE);
static TOperator	Mul_float_vec(TOperator::ID_MULTIPLY, &type_vector, &type_float, &type_vector, OPC_VPRESCALE);

static TOperator	Div_int_int(TOperator::ID_DIVIDE, &type_int, &type_int, &type_int, OPC_DIVIDE);
static TOperator	Div_int_uint(TOperator::ID_DIVIDE, &type_uint, &type_int, &type_uint, OPC_UDIVIDE);
static TOperator	Div_uint_int(TOperator::ID_DIVIDE, &type_uint, &type_uint, &type_int, OPC_UDIVIDE);
static TOperator	Div_uint_uint(TOperator::ID_DIVIDE, &type_uint, &type_uint, &type_uint, OPC_UDIVIDE);
static TOperator	Div_float_float(TOperator::ID_DIVIDE, &type_float, &type_float, &type_float, OPC_FDIVIDE);
static TOperator	Div_vec_float(TOperator::ID_DIVIDE, &type_vector, &type_vector, &type_float, OPC_VISCALE);

static TOperator	Mod_int_int(TOperator::ID_MODULUS, &type_int, &type_int, &type_int, OPC_MODULUS);
static TOperator	Mod_int_uint(TOperator::ID_MODULUS, &type_uint, &type_int, &type_uint, OPC_UMODULUS);
static TOperator	Mod_uint_int(TOperator::ID_MODULUS, &type_uint, &type_uint, &type_int, OPC_UMODULUS);
static TOperator	Mod_uint_uint(TOperator::ID_MODULUS, &type_uint, &type_uint, &type_uint, OPC_UMODULUS);

static TOperator	Add_int_int(TOperator::ID_ADD, &type_int, &type_int, &type_int, OPC_ADD);
static TOperator	Add_int_uint(TOperator::ID_ADD, &type_uint, &type_int, &type_uint, OPC_ADD);
static TOperator	Add_uint_int(TOperator::ID_ADD, &type_uint, &type_uint, &type_int, OPC_ADD);
static TOperator	Add_uint_uint(TOperator::ID_ADD, &type_uint, &type_uint, &type_uint, OPC_ADD);
static TOperator	Add_float_float(TOperator::ID_ADD, &type_float, &type_float, &type_float, OPC_FADD);
static TOperator	Add_vec_vec(TOperator::ID_ADD, &type_vector, &type_vector, &type_vector, OPC_VADD);

static TOperator	Sub_int_int(TOperator::ID_SUBTRACT, &type_int, &type_int, &type_int, OPC_SUBTRACT);
static TOperator	Sub_int_uint(TOperator::ID_SUBTRACT, &type_uint, &type_int, &type_uint, OPC_SUBTRACT);
static TOperator	Sub_uint_int(TOperator::ID_SUBTRACT, &type_uint, &type_uint, &type_int, OPC_SUBTRACT);
static TOperator	Sub_uint_uint(TOperator::ID_SUBTRACT, &type_uint, &type_uint, &type_uint, OPC_SUBTRACT);
static TOperator	Sub_float_float(TOperator::ID_SUBTRACT, &type_float, &type_float, &type_float, OPC_FSUBTRACT);
static TOperator	Sub_vec_vec(TOperator::ID_SUBTRACT, &type_vector, &type_vector, &type_vector, OPC_VSUBTRACT);

static TOperator	LShift_int_int(TOperator::ID_LSHIFT, &type_int, &type_int, &type_int, OPC_LSHIFT);
static TOperator	LShift_int_uint(TOperator::ID_LSHIFT, &type_int, &type_int, &type_uint, OPC_LSHIFT);
static TOperator	LShift_uint_int(TOperator::ID_LSHIFT, &type_uint, &type_uint, &type_int, OPC_LSHIFT);
static TOperator	LShift_uint_uint(TOperator::ID_LSHIFT, &type_uint, &type_uint, &type_uint, OPC_LSHIFT);

static TOperator	RShift_int_int(TOperator::ID_RSHIFT, &type_int, &type_int, &type_int, OPC_RSHIFT);
static TOperator	RShift_int_uint(TOperator::ID_RSHIFT, &type_int, &type_int, &type_uint, OPC_RSHIFT);
static TOperator	RShift_uint_int(TOperator::ID_RSHIFT, &type_uint, &type_uint, &type_int, OPC_URSHIFT);
static TOperator	RShift_uint_uint(TOperator::ID_RSHIFT, &type_uint, &type_uint, &type_uint, OPC_URSHIFT);

static TOperator	Lt_int_int(TOperator::ID_LT, &type_int, &type_int, &type_int, OPC_LT);
static TOperator	Lt_int_uint(TOperator::ID_LT, &type_int, &type_int, &type_uint, OPC_ULT);
static TOperator	Lt_uint_int(TOperator::ID_LT, &type_int, &type_uint, &type_int, OPC_ULT);
static TOperator	Lt_uint_uint(TOperator::ID_LT, &type_int, &type_uint, &type_uint, OPC_ULT);
static TOperator	Lt_float_float(TOperator::ID_LT, &type_int, &type_float, &type_float, OPC_FLT);

static TOperator	Le_int_int(TOperator::ID_LE, &type_int, &type_int, &type_int, OPC_LE);
static TOperator	Le_int_uint(TOperator::ID_LE, &type_int, &type_int, &type_uint, OPC_ULE);
static TOperator	Le_uint_int(TOperator::ID_LE, &type_int, &type_uint, &type_int, OPC_ULE);
static TOperator	Le_uint_uint(TOperator::ID_LE, &type_int, &type_uint, &type_uint, OPC_ULE);
static TOperator	Le_float_float(TOperator::ID_LE, &type_int, &type_float, &type_float, OPC_FLE);

static TOperator	Gt_int_int(TOperator::ID_GT, &type_int, &type_int, &type_int, OPC_GT);
static TOperator	Gt_int_uint(TOperator::ID_GT, &type_int, &type_int, &type_uint, OPC_UGT);
static TOperator	Gt_uint_int(TOperator::ID_GT, &type_int, &type_uint, &type_int, OPC_UGT);
static TOperator	Gt_uint_uint(TOperator::ID_GT, &type_int, &type_uint, &type_uint, OPC_UGT);
static TOperator	Gt_float_float(TOperator::ID_GT, &type_int, &type_float, &type_float, OPC_FGT);

static TOperator	Ge_int_int(TOperator::ID_GE, &type_int, &type_int, &type_int, OPC_GE);
static TOperator	Ge_int_uint(TOperator::ID_GE, &type_int, &type_int, &type_uint, OPC_UGE);
static TOperator	Ge_uint_int(TOperator::ID_GE, &type_int, &type_uint, &type_int, OPC_UGE);
static TOperator	Ge_uint_uint(TOperator::ID_GE, &type_int, &type_uint, &type_uint, OPC_UGE);
static TOperator	Ge_float_float(TOperator::ID_GE, &type_int, &type_float, &type_float, OPC_FGE);

static TOperator	Eq_int_int(TOperator::ID_EQ, &type_int, &type_int, &type_int, OPC_EQ);
static TOperator	Eq_int_uint(TOperator::ID_EQ, &type_int, &type_int, &type_uint, OPC_EQ);
static TOperator	Eq_uint_int(TOperator::ID_EQ, &type_int, &type_uint, &type_int, OPC_EQ);
static TOperator	Eq_uint_uint(TOperator::ID_EQ, &type_int, &type_uint, &type_uint, OPC_EQ);
static TOperator	Eq_float_float(TOperator::ID_EQ, &type_int, &type_float, &type_float, OPC_FEQ);
static TOperator	Eq_str_str(TOperator::ID_EQ, &type_int, &type_string, &type_string, OPC_EQ);
static TOperator	Eq_func_func(TOperator::ID_EQ, &type_int, &type_function, &type_function, OPC_EQ);
static TOperator	Eq_ptr_ptr(TOperator::ID_EQ, &type_int, &type_void_ptr, &type_void_ptr, OPC_EQ);
static TOperator	Eq_vec_vec(TOperator::ID_EQ, &type_int, &type_vector, &type_vector, OPC_VEQ);

static TOperator	Ne_int_int(TOperator::ID_NE, &type_int, &type_int, &type_int, OPC_NE);
static TOperator	Ne_int_uint(TOperator::ID_NE, &type_int, &type_int, &type_uint, OPC_NE);
static TOperator	Ne_uint_int(TOperator::ID_NE, &type_int, &type_uint, &type_int, OPC_NE);
static TOperator	Ne_uint_uint(TOperator::ID_NE, &type_int, &type_uint, &type_uint, OPC_NE);
static TOperator	Ne_float_float(TOperator::ID_NE, &type_int, &type_float, &type_float, OPC_FNE);
static TOperator	Ne_str_str(TOperator::ID_NE, &type_int, &type_string, &type_string, OPC_NE);
static TOperator	Ne_func_func(TOperator::ID_NE, &type_int, &type_function, &type_function, OPC_NE);
static TOperator	Ne_ptr_ptr(TOperator::ID_NE, &type_int, &type_void_ptr, &type_void_ptr, OPC_NE);
static TOperator	Ne_vec_vec(TOperator::ID_NE, &type_int, &type_vector, &type_vector, OPC_VNE);

static TOperator	And_int_int(TOperator::ID_ANDBITWISE, &type_int, &type_int, &type_int, OPC_ANDBITWISE);
static TOperator	And_int_uint(TOperator::ID_ANDBITWISE, &type_uint, &type_int, &type_uint, OPC_ANDBITWISE);
static TOperator	And_uint_int(TOperator::ID_ANDBITWISE, &type_uint, &type_uint, &type_int, OPC_ANDBITWISE);
static TOperator	And_uint_uint(TOperator::ID_ANDBITWISE, &type_uint, &type_uint, &type_uint, OPC_ANDBITWISE);

static TOperator	Xor_int_int(TOperator::ID_XORBITWISE, &type_int, &type_int, &type_int, OPC_XORBITWISE);
static TOperator	Xor_int_uint(TOperator::ID_XORBITWISE, &type_uint, &type_int, &type_uint, OPC_XORBITWISE);
static TOperator	Xor_uint_int(TOperator::ID_XORBITWISE, &type_uint, &type_uint, &type_int, OPC_XORBITWISE);
static TOperator	Xor_uint_uint(TOperator::ID_XORBITWISE, &type_uint, &type_uint, &type_uint, OPC_XORBITWISE);

static TOperator	Or_int_int(TOperator::ID_ORBITWISE, &type_int, &type_int, &type_int, OPC_ORBITWISE);
static TOperator	Or_int_uint(TOperator::ID_ORBITWISE, &type_uint, &type_int, &type_uint, OPC_ORBITWISE);
static TOperator	Or_uint_int(TOperator::ID_ORBITWISE, &type_uint, &type_uint, &type_int, OPC_ORBITWISE);
static TOperator	Or_uint_uint(TOperator::ID_ORBITWISE, &type_uint, &type_uint, &type_uint, OPC_ORBITWISE);

static TOperator	Assign_int_int(TOperator::ID_ASSIGN, &type_int, &type_int, &type_int, OPC_ASSIGN);
static TOperator	Assign_int_uint(TOperator::ID_ASSIGN, &type_uint, &type_int, &type_uint, OPC_ASSIGN);
static TOperator	Assign_uint_int(TOperator::ID_ASSIGN, &type_uint, &type_uint, &type_int, OPC_ASSIGN);
static TOperator	Assign_uint_uint(TOperator::ID_ASSIGN, &type_uint, &type_uint, &type_uint, OPC_ASSIGN);
static TOperator	Assign_float_float(TOperator::ID_ASSIGN, &type_float, &type_float, &type_float, OPC_ASSIGN);
static TOperator	Assign_str_str(TOperator::ID_ASSIGN, &type_string, &type_string, &type_string, OPC_ASSIGN);
static TOperator	Assign_func_func(TOperator::ID_ASSIGN, &type_function, &type_function, &type_function, OPC_ASSIGN);
static TOperator	Assign_ptr_ptr(TOperator::ID_ASSIGN, &type_void_ptr, &type_void_ptr, &type_void_ptr, OPC_ASSIGN);
static TOperator	Assign_vec_vec(TOperator::ID_ASSIGN, &type_vector, &type_vector, &type_vector, OPC_VASSIGN);

static TOperator	AddVar_int_int(TOperator::ID_ADDVAR, &type_int, &type_int, &type_int, OPC_ADDVAR);
static TOperator	AddVar_int_uint(TOperator::ID_ADDVAR, &type_uint, &type_int, &type_uint, OPC_ADDVAR);
static TOperator	AddVar_uint_int(TOperator::ID_ADDVAR, &type_uint, &type_uint, &type_int, OPC_ADDVAR);
static TOperator	AddVar_uint_uint(TOperator::ID_ADDVAR, &type_uint, &type_uint, &type_uint, OPC_ADDVAR);
static TOperator	AddVar_float_float(TOperator::ID_ADDVAR, &type_float, &type_float, &type_float, OPC_FADDVAR);
static TOperator	AddVar_vec_vec(TOperator::ID_ADDVAR, &type_vector, &type_vector, &type_vector, OPC_VADDVAR);

static TOperator	SubVar_int_int(TOperator::ID_SUBVAR, &type_int, &type_int, &type_int, OPC_SUBVAR);
static TOperator	SubVar_int_uint(TOperator::ID_SUBVAR, &type_uint, &type_int, &type_uint, OPC_SUBVAR);
static TOperator	SubVar_uint_int(TOperator::ID_SUBVAR, &type_uint, &type_uint, &type_int, OPC_SUBVAR);
static TOperator	SubVar_uint_uint(TOperator::ID_SUBVAR, &type_uint, &type_uint, &type_uint, OPC_SUBVAR);
static TOperator	SubVar_float_float(TOperator::ID_SUBVAR, &type_float, &type_float, &type_float, OPC_FSUBVAR);
static TOperator	SubVar_vec_vec(TOperator::ID_SUBVAR, &type_vector, &type_vector, &type_vector, OPC_VSUBVAR);

static TOperator	MulVar_int_int(TOperator::ID_MULVAR, &type_int, &type_int, &type_int, OPC_MULVAR);
static TOperator	MulVar_int_uint(TOperator::ID_MULVAR, &type_uint, &type_int, &type_uint, OPC_MULVAR);
static TOperator	MulVar_uint_int(TOperator::ID_MULVAR, &type_uint, &type_uint, &type_int, OPC_MULVAR);
static TOperator	MulVar_uint_uint(TOperator::ID_MULVAR, &type_uint, &type_uint, &type_uint, OPC_MULVAR);
static TOperator	MulVar_float_float(TOperator::ID_MULVAR, &type_float, &type_float, &type_float, OPC_FMULVAR);
static TOperator	MulVar_vec_float(TOperator::ID_MULVAR, &type_vector, &type_vector, &type_float, OPC_VSCALEVAR);

static TOperator	DivVar_int_int(TOperator::ID_DIVVAR, &type_int, &type_int, &type_int, OPC_DIVVAR);
static TOperator	DivVar_int_uint(TOperator::ID_DIVVAR, &type_uint, &type_int, &type_uint, OPC_UDIVVAR);
static TOperator	DivVar_uint_int(TOperator::ID_DIVVAR, &type_uint, &type_uint, &type_int, OPC_UDIVVAR);
static TOperator	DivVar_uint_uint(TOperator::ID_DIVVAR, &type_uint, &type_uint, &type_uint, OPC_UDIVVAR);
static TOperator	DivVar_float_float(TOperator::ID_DIVVAR, &type_float, &type_float, &type_float, OPC_FDIVVAR);
static TOperator	DivVar_vec_float(TOperator::ID_DIVVAR, &type_vector, &type_vector, &type_float, OPC_VISCALEVAR);

static TOperator	ModVar_int_int(TOperator::ID_MODVAR, &type_int, &type_int, &type_int, OPC_MODVAR);
static TOperator	ModVar_int_uint(TOperator::ID_MODVAR, &type_uint, &type_int, &type_uint, OPC_UMODVAR);
static TOperator	ModVar_uint_int(TOperator::ID_MODVAR, &type_uint, &type_uint, &type_int, OPC_UMODVAR);
static TOperator	ModVar_uint_uint(TOperator::ID_MODVAR, &type_uint, &type_uint, &type_uint, OPC_UMODVAR);

static TOperator	AndVar_int_int(TOperator::ID_ANDVAR, &type_int, &type_int, &type_int, OPC_ANDVAR);
static TOperator	AndVar_int_uint(TOperator::ID_ANDVAR, &type_uint, &type_int, &type_uint, OPC_ANDVAR);
static TOperator	AndVar_uint_int(TOperator::ID_ANDVAR, &type_uint, &type_uint, &type_int, OPC_ANDVAR);
static TOperator	AndVar_uint_uint(TOperator::ID_ANDVAR, &type_uint, &type_uint, &type_uint, OPC_ANDVAR);

static TOperator	OrVar_int_int(TOperator::ID_ORVAR, &type_int, &type_int, &type_int, OPC_ORVAR);
static TOperator	OrVar_int_uint(TOperator::ID_ORVAR, &type_uint, &type_int, &type_uint, OPC_ORVAR);
static TOperator	OrVar_uint_int(TOperator::ID_ORVAR, &type_uint, &type_uint, &type_int, OPC_ORVAR);
static TOperator	OrVar_uint_uint(TOperator::ID_ORVAR, &type_uint, &type_uint, &type_uint, OPC_ORVAR);

static TOperator	XorVar_int_int(TOperator::ID_XORVAR, &type_int, &type_int, &type_int, OPC_XORVAR);
static TOperator	XorVar_int_uint(TOperator::ID_XORVAR, &type_uint, &type_int, &type_uint, OPC_XORVAR);
static TOperator	XorVar_uint_int(TOperator::ID_XORVAR, &type_uint, &type_uint, &type_int, OPC_XORVAR);
static TOperator	XorVar_uint_uint(TOperator::ID_XORVAR, &type_uint, &type_uint, &type_uint, OPC_XORVAR);

static TOperator	LShiftVar_int_int(TOperator::ID_LSHIFTVAR, &type_int, &type_int, &type_int, OPC_LSHIFTVAR);
static TOperator	LShiftVar_int_uint(TOperator::ID_LSHIFTVAR, &type_uint, &type_int, &type_uint, OPC_LSHIFTVAR);
static TOperator	LShiftVar_uint_int(TOperator::ID_LSHIFTVAR, &type_uint, &type_uint, &type_int, OPC_LSHIFTVAR);
static TOperator	LShiftVar_uint_uint(TOperator::ID_LSHIFTVAR, &type_uint, &type_uint, &type_uint, OPC_LSHIFTVAR);

static TOperator	RShiftVar_int_int(TOperator::ID_RSHIFTVAR, &type_int, &type_int, &type_int, OPC_RSHIFTVAR);
static TOperator	RShiftVar_int_uint(TOperator::ID_RSHIFTVAR, &type_uint, &type_int, &type_uint, OPC_URSHIFTVAR);
static TOperator	RShiftVar_uint_int(TOperator::ID_RSHIFTVAR, &type_uint, &type_uint, &type_int, OPC_URSHIFTVAR);
static TOperator	RShiftVar_uint_uint(TOperator::ID_RSHIFTVAR, &type_uint, &type_uint, &type_uint, OPC_URSHIFTVAR);

static TOperator	NullOp(TOperator::NUM_OPERATORS, &type_void, &type_void, &type_void, OPC_DONE);

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	TOperator::TOperator
//
//==========================================================================

TOperator::TOperator(id_t Aopid, TType* Atype, TType* Atype1, TType* Atype2, int Aopcode) :
	opid(Aopid), type(Atype), type1(Atype1), type2(Atype2), opcode(Aopcode)
{
	next = operators[opid];
	operators[opid] = this;
}

//==========================================================================
//
//	TypeCmp
//
//==========================================================================

bool TypeCmp(TType *type1, TType *type2)
{
	if (type1 == type2)
    {
    	return true;
	}
    if ((type1->type == ev_vector) && (type2->type == ev_vector))
    {
		return true;
	}
    if ((type1->type == ev_function) && (type2->type == ev_function))
    {
		ParseWarning("Different function types");
		return true;
	}
    if ((type1->type == ev_pointer) && (type2->type == ev_pointer))
    {
		if (type1 == &type_void_ptr || type2 == &type_void_ptr)
        {
        	return true;
        }
    }
	return false;
}

//==========================================================================
//
//	FindOperator
//
//==========================================================================

TOperator *FindOperator(TOperator::id_t opid, TType *type1, TType *type2)
{
	TOperator	*oper;

	for (oper = operators[opid]; oper; oper = oper->next)
	{
		if (oper->type1->type == ev_function &&
			oper->type2->type == ev_function &&
			type1->type == ev_function && type1 == type2)
		{
			return oper;
		}
		if (TypeCmp(oper->type1, type1) && TypeCmp(oper->type2, type2))
		{
			return oper;
		}
	}
	ParseError("Expression type mistmatch - can't find matching operator");
	return &NullOp;
}

//==========================================================================
//
//	ParseExpressionPriority0
//
//==========================================================================

static TOp *ParseExpressionPriority0(void)
{
	TOp			*op;
	TType		*type;
	int			num;
	int			arg;

   	switch (tk_Token)
	{
	 case TK_INTEGER:
		op = new TOpConst(tk_Number, &type_int);
		TK_NextToken();
		return op;

	 case TK_FLOAT:
		op = new TOpConst(PassFloat(tk_Float), &type_float);
		TK_NextToken();
		return op;

	 case TK_STRING:
		op = new TOpConst(FindString(tk_String), &type_string);
		TK_NextToken();
		return op;

	 case TK_PUNCT:
       	if (TK_Check("("))
		{
			type = CheckForType();
			if (type)
			{
				while (TK_Check("*"))
				{
					type = MakePointerType(type);
				}
				TK_Expect(")", ERR_BAD_EXPR);
				op = ParseExpressionPriority2();
				op->type = type;
				return op;
			}

			op = ParseExpressionPriority14();
			TK_Expect(")", ERR_BAD_EXPR);
			return op;
		}
		break;

	 case TK_KEYWORD:
		if (TK_Check("vector"))
		{
			TK_Expect("(", ERR_MISSING_LPAREN);
			TOp *op1 = ParseExpressionPriority14();
			TK_Expect(",", ERR_BAD_EXPR);
			TOp *op2 = ParseExpressionPriority14();
			TK_Expect(",", ERR_BAD_EXPR);
			TOp *op3 = ParseExpressionPriority14();
			TK_Expect(")", ERR_MISSING_RPAREN);
			return new TOpVector(op1, op2, op3);
		}
		break;

	 case TK_IDENTIFIER:
		if (TK_Check("NULL"))
		{
           	return new TOpConst(0, &type_void_ptr);
		}

		num = CheckForLocalVar(tk_String);
		if (num)
		{
			TK_NextToken();
			op = new TOpLocal(localdefs[num].ofs, MakePointerType(localdefs[num].type));
			op = new TOpPushPointed(op,	localdefs[num].type);
			return op;
		}

		num = CheckForConstant(tk_String);
		if (num != -1)
		{
			TK_NextToken();
           	return new TOpConst(Constants[num].value, &type_int);
    	}

		num = CheckForGlobalVar(tk_String);
		if (num)
		{
			TK_NextToken();
			op = new TOpGlobal(num, MakePointerType(globaldefs[num].type));
			op = new TOpPushPointed(op,	globaldefs[num].type);
			return op;
		}

		num = CheckForFunction(tk_String);
		if (num)
		{
			TOpFuncCall *fop;
			int			argsize;

			TK_NextToken();
			if (!TK_Check("("))
			{
				op = new TOpConst(num, functions[num].type);
				return op;
			}
			fop = new TOpFuncCall(num);
			arg = 0;
			argsize = 0;
			int max_params;
			int num_needed_params = functions[num].type->num_params & PF_COUNT_MASK;
			if (functions[num].type->num_params & PF_VARARGS)
			{
				max_params = MAX_ARG_COUNT - 1;
			}
			else
			{
				max_params = functions[num].type->num_params;
			}
			if (!TK_Check(")"))
			{
				do
				{
					op = ParseExpressionPriority14();
					if (arg >= max_params)
					{
						ParseError("Incorrect number of arguments.");
					}
					else
					{
						if (arg < num_needed_params)
						{
		    	            TypeCheck3(op->type, functions[num].type->param_types[arg]);
						}
						fop->parms[arg] = op;
					}
					arg++;
					argsize += TypeSize(op->type);
				} while (TK_Check(","));
				TK_Expect(")", ERR_MISSING_RPAREN);
			}
			if (arg < num_needed_params)
			{
				ParseError("Incorrect argument count %d, should be %d", arg, num_needed_params);
			}
			if (functions[num].type->num_params & PF_VARARGS)
			{
				fop->parms[arg] = new TOpConst(argsize / 4 - num_needed_params, &type_int);
			}
			return fop;
		}
		ERR_Exit(ERR_ILLEGAL_EXPR_IDENT, true, "Identifier: %s", tk_String);
		break;

	 default:
       	break;
	}

	op = new TOp;
	op->type = &type_void;
	return op;
}

//==========================================================================
//
//	ParseExpressionPriority1
//
//==========================================================================

static TOp *ParseExpressionPriority1(void)
{
    bool		done;
	TOp			*op;
	TType		*type;
	field_t		*field;

   	op = ParseExpressionPriority0();
	done = false;
	do
	{
   	    if (TK_Check("->"))
   	   	{
           	TypeCheck1(op->type);
        	if (op->type->type != ev_pointer)
	        {
   		    	ERR_Exit(ERR_BAD_EXPR, true, NULL);
           	}
			type = op->type->aux_type;
			field = ParseField(type);
			if (field)
			{
				op = new TOpField(op, field->ofs, field->type);
				op = new TOpPushPointed(op,	field->type);
			}
   	    }
   	    else if (TK_Check("."))
   	   	{
			type = op->type;
           	if (op->type->type == ev_array || op->type->type == ev_pointer)
			{
               	ERR_Exit(ERR_BAD_EXPR, true, NULL);
			}
           	op = op->GetAddress();
			field = ParseField(type);
			if (field)
			{
				op = new TOpField(op, field->ofs, field->type);
				op = new TOpPushPointed(op,	field->type);
			}
   	    }
		else if (TK_Check("["))
		{
			TOp *ind;
        	if (op->type->type == ev_array)
			{
        	    type = op->type->aux_type;
    	    	op = op->GetAddress();
			}
			else if (op->type->type == ev_pointer)
			{
               	type = op->type->aux_type;
			}
        	else
			{
				ERR_Exit(ERR_BAD_ARRAY, true, NULL);
           	}
			ind = ParseExpressionPriority14();
			TK_Expect("]", ERR_BAD_ARRAY);
			op = new TOpArrayIndex(op, ind, type);
			op = new TOpPushPointed(op, type);
		}
		else if (TK_Check("("))
		{
			TOpIndirectFuncCall *fop;
			TType *ftype = op->type;

			if (ftype->type != ev_function)
			{
				ParseError("Not a function");
				return op;
			}
			fop = new TOpIndirectFuncCall(op, ftype);
			int arg = 0;
			if (!TK_Check(")"))
			{
				do
				{
					op = ParseExpressionPriority14();
					if (arg >= ftype->num_params)
					{
						ParseError("Incorrect number of arguments.");
					}
					else
					{
	    	            TypeCheck3(op->type, ftype->param_types[arg]);
						fop->parms[arg] = op;
					}
					arg++;
				} while (TK_Check(","));
				TK_Expect(")", ERR_MISSING_RPAREN);
			}
			if (arg != ftype->num_params)
			{
				ParseError("Incorrect argument count %d, should be %d", arg, ftype->num_params);
			}
			return fop;
		}
		else
		{
			done = true;
		}
	} while (!done);

	return op;
}

//==========================================================================
//
//	ParseExpressionPriority2
//
//==========================================================================

static TOp *ParseExpressionPriority2(void)
{
	TOp			*op;
	TOperator	*oper;
	TType		*type;

   	if (tk_Token == TK_PUNCT)
	{
		if (TK_Check("+"))
		{
           	op = ParseExpressionPriority2();
			oper = FindOperator(TOperator::ID_UNARYPLUS, op->type, &type_void);
			return new TOp1(op, oper);
		}

       	if (TK_Check("-"))
		{
           	op = ParseExpressionPriority2();
			oper = FindOperator(TOperator::ID_UNARYMINUS, op->type, &type_void);
			return new TOp1(op, oper);
		}

       	if (TK_Check("!"))
		{
           	op = ParseExpressionPriority2();
			oper = FindOperator(TOperator::ID_NEGATELOGICAL, op->type, &type_void);
			return new TOp1(op, oper);
		}

       	if (TK_Check("~"))
		{
           	op = ParseExpressionPriority2();
			oper = FindOperator(TOperator::ID_BITINVERSE, op->type, &type_void);
			return new TOp1(op, oper);
		}

		if (TK_Check("&"))
		{
           	op = ParseExpressionPriority1();
			type = MakePointerType(op->type);
			op = op->GetAddress();
			op->type = type;
			return op;
		}
		if (TK_Check("*"))
		{
           	op = ParseExpressionPriority2();
			return new TOpPushPointed(op);
		}

       	if (TK_Check("++"))
		{
           	op = ParseExpressionPriority2();
			type = op->type;
			op = op->GetAddress();
			oper = FindOperator(TOperator::ID_PREINC, type, &type_void);
			return new TOp1(op, oper);
		}

       	if (TK_Check("--"))
		{
           	op = ParseExpressionPriority2();
			type = op->type;
			op = op->GetAddress();
			oper = FindOperator(TOperator::ID_PREDEC, type, &type_void);
			return new TOp1(op, oper);
		}
	}

	op = ParseExpressionPriority1();

	if (TK_Check("++"))
	{
		type = op->type;
		op = op->GetAddress();
		oper = FindOperator(TOperator::ID_POSTINC, type, &type_void);
		return new TOp1(op, oper);
	}

	if (TK_Check("--"))
	{
		type = op->type;
		op = op->GetAddress();
		oper = FindOperator(TOperator::ID_POSTDEC, type, &type_void);
		return new TOp1(op, oper);
	}

	return op;
}

//==========================================================================
//
//	ParseExpressionPriority3
//
//==========================================================================

static TOp *ParseExpressionPriority3(void)
{
	TOp			*op1;
	TOp			*op2;
	TOperator	*oper;
	bool		done;

	op1 = ParseExpressionPriority2();
	done = false;
    do
    {
   		if (TK_Check("*"))
		{
   	    	op2 = ParseExpressionPriority2();
			oper = FindOperator(TOperator::ID_MULTIPLY, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
   		else if (TK_Check("/"))
		{
   	    	op2 = ParseExpressionPriority2();
			oper = FindOperator(TOperator::ID_DIVIDE, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
   		else if (TK_Check("%"))
		{
   	    	op2 = ParseExpressionPriority2();
			oper = FindOperator(TOperator::ID_MODULUS, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
		else
		{
			done = true;
		}
	}
    while (!done);
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority4
//
//==========================================================================

static TOp *ParseExpressionPriority4(void)
{
	TOp			*op1;
	TOp			*op2;
	TOperator	*oper;
	bool		done;

	op1 = ParseExpressionPriority3();
	done = false;
    do
    {
   		if (TK_Check("+"))
		{
   	    	op2 = ParseExpressionPriority3();
			oper = FindOperator(TOperator::ID_ADD, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
   		else if (TK_Check("-"))
		{
   	    	op2 = ParseExpressionPriority3();
			oper = FindOperator(TOperator::ID_SUBTRACT, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
		else
		{
			done = true;
		}
	}
    while (!done);
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority5
//
//==========================================================================

static TOp *ParseExpressionPriority5(void)
{
	TOp			*op1;
	TOp			*op2;
	TOperator	*oper;
	bool		done;

	op1 = ParseExpressionPriority4();
	done = false;
    do
    {
   		if (TK_Check("<<"))
		{
   	    	op2 = ParseExpressionPriority4();
			oper = FindOperator(TOperator::ID_LSHIFT, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
   		else if (TK_Check(">>"))
		{
   	    	op2 = ParseExpressionPriority4();
			oper = FindOperator(TOperator::ID_RSHIFT, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
		else
		{
			done = true;
		}
	}
    while (!done);
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority6
//
//==========================================================================

static TOp* ParseExpressionPriority6(void)
{
	TOp			*op1;
	TOp			*op2;
	TOperator	*oper;
	bool		done;

	op1 = ParseExpressionPriority5();
	done = false;
    do
    {
   		if (TK_Check("<"))
		{
   	    	op2 = ParseExpressionPriority5();
			oper = FindOperator(TOperator::ID_LT, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
   		else if (TK_Check("<="))
		{
   	    	op2 = ParseExpressionPriority5();
			oper = FindOperator(TOperator::ID_LE, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
   		else if (TK_Check(">"))
		{
   	    	op2 = ParseExpressionPriority5();
			oper = FindOperator(TOperator::ID_GT, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
   		else if (TK_Check(">="))
		{
   	    	op2 = ParseExpressionPriority5();
			oper = FindOperator(TOperator::ID_GE, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
		else
		{
			done = true;
		}
	}
    while (!done);
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority7
//
//==========================================================================

static TOp* ParseExpressionPriority7(void)
{
	TOp			*op1;
	TOp			*op2;
	TOperator	*oper;
	bool		done;

	op1 = ParseExpressionPriority6();
	done = false;
    do
    {
   		if (TK_Check("=="))
		{
   	    	op2 = ParseExpressionPriority6();
			oper = FindOperator(TOperator::ID_EQ, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
   		else if (TK_Check("!="))
		{
   	    	op2 = ParseExpressionPriority6();
			oper = FindOperator(TOperator::ID_NE, op1->type, op2->type);
			op1 = new TOp2(op1, op2, oper);
	    }
		else
		{
			done = true;
		}
	} while (!done);
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority8
//
//==========================================================================

static TOp* ParseExpressionPriority8(void)
{
	TOp			*op1;
	TOp			*op2;
	TOperator	*oper;

	op1 = ParseExpressionPriority7();
	while (TK_Check("&"))
	{
		op2 = ParseExpressionPriority7();
		oper = FindOperator(TOperator::ID_ANDBITWISE, op1->type, op2->type);
		op1 = new TOp2(op1, op2, oper);
	}
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority9
//
//==========================================================================

static TOp* ParseExpressionPriority9(void)
{
	TOp			*op1;
	TOp			*op2;
	TOperator	*oper;

	op1 = ParseExpressionPriority8();
	while (TK_Check("^"))
	{
		op2 = ParseExpressionPriority8();
		oper = FindOperator(TOperator::ID_XORBITWISE, op1->type, op2->type);
		op1 = new TOp2(op1, op2, oper);
	}
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority10
//
//==========================================================================

static TOp* ParseExpressionPriority10(void)
{
	TOp			*op1;
	TOp			*op2;
	TOperator	*oper;

	op1 = ParseExpressionPriority9();
	while (TK_Check("|"))
	{
		op2 = ParseExpressionPriority9();
		oper = FindOperator(TOperator::ID_ORBITWISE, op1->type, op2->type);
		op1 = new TOp2(op1, op2, oper);
	}
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority11
//
//==========================================================================

static TOp* ParseExpressionPriority11(void)
{
	TOp			*op1;
	TOp			*op2;

	op1 = ParseExpressionPriority10();
	while (TK_Check("&&"))
	{
		op2 = ParseExpressionPriority10();
		op1 = new TOpAnd(op1, op2);
	}
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority12
//
//==========================================================================

static TOp* ParseExpressionPriority12(void)
{
	TOp			*op1;
	TOp			*op2;

	op1 = ParseExpressionPriority11();
	while (TK_Check("||"))
	{
		op2 = ParseExpressionPriority11();
		op1 = new TOpOr(op1, op2);
	}
	return op1;
}

//==========================================================================
//
//	ParseExpressionPriority13
//
//==========================================================================

static TOp* ParseExpressionPriority13(void)
{
	TOp			*op;
	TOp			*op1;
	TOp			*op2;

	op = ParseExpressionPriority12();
   	if (TK_Check("?"))
	{
		op1 = ParseExpressionPriority13();
		TK_Expect(":", ERR_MISSING_COLON);
		op2 = ParseExpressionPriority13();
		op = new TOpCond(op, op1, op2);
	}
	return op;
}

//==========================================================================
//
//	ParseExpressionPriority14
//
//==========================================================================

static TOp* ParseExpressionPriority14(void)
{
	int			i;
	struct
	{
		char*				name;
		TOperator::id_t		opid;
	} AssignOps[] =
	{
		{"=",	TOperator::ID_ASSIGN},
		{"+=",	TOperator::ID_ADDVAR},
		{"-=",	TOperator::ID_SUBVAR},
		{"*=",	TOperator::ID_MULVAR},
		{"/=",	TOperator::ID_DIVVAR},
		{"%=",	TOperator::ID_MODVAR},
		{"&=",	TOperator::ID_ANDVAR},
		{"|=",	TOperator::ID_ORVAR},
		{"^=",	TOperator::ID_XORVAR},
		{"<<=",	TOperator::ID_LSHIFTVAR},
		{">>=",	TOperator::ID_RSHIFTVAR},
        {NULL,	TOperator::NUM_OPERATORS}
	};
	TOperator	*oper;
	TOp			*op1;
	TOp			*op2;
	TType		*type;

	op1 = ParseExpressionPriority13();
	for (i = 0; AssignOps[i].name; i++)
   	{
		if (TK_Check(AssignOps[i].name))
		{
			type = op1->type;
			op1 = op1->GetAddress();
   	    	op2 = ParseExpressionPriority14();
			oper = FindOperator(AssignOps[i].opid, type, op2->type);
			TypeCheck3(type, op2->type);
           	op1 = new TOp2(op1, op2, oper);
			op1->type = type;
			return op1;
   	    }
	}
	return op1;
}

//==========================================================================
//
//	ParseExpression
//
//==========================================================================

TType *ParseExpression(void)
{
	TOp *op = ParseExpressionPriority14();
	op->Code();
	TType *t = op->type;
	delete op;
	return t;
}

