//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
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
//**************************************************************************
//**
//**	x86 assembly-language DOS-dependent routines.
//**	
//**************************************************************************

#include "asm_i386.h"

	.data

	Align4
ceil_cw:	.long	0
single_cw:	.long	0
full_cw:	.long	0
cw:			.long	0
pushed_cw:	.long	0

	.text

	Align16
.globl C(Sys_SetFPCW)
C(Sys_SetFPCW):
	fnstcw	cw
	movl	cw,%eax

    // round mode, 64-bit precision
	andb	$0xF0,%ah
	orb		$0x03,%ah
	movl	%eax,full_cw

    // chop mode, single precision
	andb	$0xF0,%ah
	orb		$0x0C,%ah
	movl	%eax,single_cw

    // ceil mode, single precision
	andb	$0xF0,%ah
	orb		$0x08,%ah
	movl	%eax,ceil_cw

	ret

	Align16
.globl C(Sys_LowFPPrecision)
C(Sys_LowFPPrecision):
	fldcw	single_cw
	ret

	Align16
.globl C(Sys_HighFPPrecision)
C(Sys_HighFPPrecision):
	fldcw	full_cw
	ret

	Align16
.globl C(Sys_PushFPCW_SetHigh)
C(Sys_PushFPCW_SetHigh):
	fnstcw	pushed_cw
	fldcw	full_cw
	ret

	Align16
.globl C(Sys_PopFPCW)
C(Sys_PopFPCW):
	fldcw	pushed_cw
	ret

