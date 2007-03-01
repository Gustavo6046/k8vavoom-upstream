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

class VGameInfo : public VObject
{
	DECLARE_CLASS(VGameInfo, VObject, 0)

	VName			AcsHelper;
	VName			GenericConScript;

	vint32			netgame;
	vint32			deathmatch;
	vint32			gameskill;
	vint32			respawn;
	vint32			nomonsters;
	vint32			fastparm;

	vint32*			validcount;
	vint32			skyflatnum;

	VBasePlayer*	Players[MAXPLAYERS]; // Bookkeeping on players - state.

	level_t*		level;

	vint32			RebornPosition;

	float			frametime;

	float			FloatBobOffsets[64];
	vint32			PhaseTable[64];

	VGameInfo()
	{}

	void eventInit()
	{
		P_PASS_SELF;
		EV_RET_VOID("Init");
	}
	void eventInitNewGame(int skill)
	{
		P_PASS_SELF;
		P_PASS_INT(skill);
		EV_RET_VOID("InitNewGame");
	}
	VWorldInfo* eventCreateWorldInfo()
	{
		P_PASS_SELF;
		EV_RET_REF(VWorldInfo, "CreateWorldInfo");
	}
	VLevelInfo* eventCreateLevelInfo()
	{
		P_PASS_SELF;
		EV_RET_REF(VLevelInfo, "CreateLevelInfo");
	}
	void eventTranslateLevel(VLevel* InLevel)
	{
		P_PASS_SELF;
		P_PASS_REF(InLevel);
		EV_RET_VOID("TranslateLevel");
	}
	void eventSpawnWorld(VLevel* InLevel)
	{
		P_PASS_SELF;
		P_PASS_REF(InLevel);
		EV_RET_VOID("SpawnWorld");
	}
	VName eventGetConScriptName(VName LevelName)
	{
		P_PASS_SELF;
		P_PASS_NAME(LevelName);
		EV_RET_NAME("GetConScriptName");
	}
};
