//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
//**
//**	Copyright (C) 1999-2000 J�nis Legzdi��
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
// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log$
// Revision 1.1  2001/07/25 18:13:59  dj_jl
// Initial revision
//
//
// DESCRIPTION:
//	Created by a sound utility.
//	Kept as a sample, DOOM2 sounds.
//
//-----------------------------------------------------------------------------

//**************************************************************************
//**
//** sounds.c : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile$
//** $Revision$
//** $Date$
//** $Author$
//**
//**************************************************************************

#include "../../makeinfo.h"
#include "info.h"

//==========================================================================
//
//	Hexen sounds
//
//==========================================================================

sfxinfo_t sfx[] =
{
	// tagname, lumpname, priority, numchannels, pitchshift,
    // usefulness, snd_ptr, lumpnum
	{ "", "", 0, 0, 0 },
	{ "PlayerFighterNormalDeath", "", 256, 2, 1 },
	{ "PlayerFighterCrazyDeath", "", 256, 2, 1 },
	{ "PlayerFighterExtreme1Death", "", 256, 2, 1 },
	{ "PlayerFighterExtreme2Death", "", 256, 2, 1 },
	{ "PlayerFighterExtreme3Death", "", 256, 2, 1 },
	{ "PlayerFighterBurnDeath", "", 256, 2, 1 },
	{ "PlayerClericNormalDeath", "", 256, 2, 1 },
	{ "PlayerClericCrazyDeath", "", 256, 2, 1 },
	{ "PlayerClericExtreme1Death", "", 256, 2, 1 },
	{ "PlayerClericExtreme2Death", "", 256, 2, 1 },
	{ "PlayerClericExtreme3Death", "", 256, 2, 1 },
	{ "PlayerClericBurnDeath", "", 256, 2, 1 },
	{ "PlayerMageNormalDeath", "", 256, 2, 0 },
	{ "PlayerMageCrazyDeath", "", 256, 2, 0 },
	{ "PlayerMageExtreme1Death", "", 256, 2, 0 },
	{ "PlayerMageExtreme2Death", "", 256, 2, 0 },
	{ "PlayerMageExtreme3Death", "", 256, 2, 0 },
	{ "PlayerMageBurnDeath", "", 256, 2, 0 },
	{ "PlayerFighterPain", "", 256, 2, 1 },
	{ "PlayerClericPain", "", 256, 2, 1 },
	{ "PlayerMagePain", "", 256, 2, 0 },
	{ "PlayerFighterGrunt", "", 256, 2, 1 },
	{ "PlayerClericGrunt", "", 256, 2, 1 },
	{ "PlayerMageGrunt", "", 256, 2, 0 },
	{ "PlayerLand", "", 32, 2, 1 },
	{ "PlayerPoisonCough", "", 256, 2, 1 },
	{ "PlayerFighterFallingScream", "", 256, 2, 1 },
	{ "PlayerClericFallingScream", "", 256, 2, 1 },
	{ "PlayerMageFallingScream", "", 256, 2, 0 },
	{ "PlayerFallingSplat", "", 256, 2, 1 },
	{ "PlayerFighterFailedUse", "", 256, 1, 1 },
	{ "PlayerClericFailedUse", "", 256, 1, 1 },
	{ "PlayerMageFailedUse", "", 256, 1, 0 },
	{ "PlatformStart", "", 36, 2, 1 },
	{ "PlatformStartMetal", "", 36, 2, 1 },
	{ "PlatformStop", "", 40, 2, 1 },
	{ "StoneMove", "", 32, 2, 1 },
	{ "MetalMove", "", 32, 2, 1 },
	{ "DoorOpen", "", 36, 2, 1 },
	{ "DoorLocked", "", 36, 2, 1 },
	{ "DoorOpenMetal", "", 36, 2, 1 },
	{ "DoorCloseMetal", "", 36, 2, 1 },
	{ "DoorCloseLight", "", 36, 2, 1 },
	{ "DoorCloseHeavy", "", 36, 2, 1 },
	{ "DoorCreak", "", 36, 2, 1 },
	{ "PickupWeapon", "", 36, 2, 0 },
	{ "PickupArtifact", "", 36, 2, 1 },
	{ "PickupKey", "", 36, 2, 1 },
	{ "PickupItem", "", 36, 2, 1 },
	{ "PickupPiece", "", 36, 2, 0 },
	{ "WeaponBuild", "", 36, 2, 0 },
	{ "UseArtifact", "", 36, 2, 1 },
	{ "BlastRadius", "", 36, 2, 1 },
	{ "Teleport", "", 256, 2, 1 },
	{ "ThunderCrash", "", 30, 2, 1 },
	{ "FighterPunchMiss", "", 80, 2, 1 },
	{ "FighterPunchHitThing", "", 80, 2, 1 },
	{ "FighterPunchHitWall", "", 80, 2, 1 },
	{ "FighterGrunt", "", 80, 2, 1 },
	{ "FighterAxeHitThing", "", 80, 2, 1 },
	{ "FighterHammerMiss", "", 80, 2, 1 },
	{ "FighterHammerHitThing", "", 80, 2, 1 },
	{ "FighterHammerHitWall", "", 80, 2, 1 },
	{ "FighterHammerContinuous", "", 32, 2, 1 },
	{ "FighterHammerExplode", "", 80, 2, 1 },
	{ "FighterSwordFire", "", 80, 2, 1 },
	{ "FighterSwordExplode", "", 80, 2, 1 },
	{ "ClericCStaffFire", "", 80, 2, 1 },
	{ "ClericCStaffExplode", "", 40, 2, 1 },
	{ "ClericCStaffHitThing", "", 80, 2, 1 },
	{ "ClericFlameFire", "", 80, 2, 1 },
	{ "ClericFlameExplode", "", 80, 2, 1 },
	{ "ClericFlameCircle", "", 80, 2, 1 },
	{ "MageWandFire", "", 80, 2, 1 },
	{ "MageLightningFire", "", 80, 2, 1 },
	{ "MageLightningZap", "", 32, 2, 1 },
	{ "MageLightningContinuous", "", 32, 2, 1 },
	{ "MageLightningReady", "", 30, 2, 1 },
	{ "MageShardsFire","", 80, 2, 1 },
	{ "MageShardsExplode","", 36, 2, 1 },
	{ "MageStaffFire","", 80, 2, 1 },
	{ "MageStaffExplode","", 40, 2, 1 },
	{ "Switch1", "", 32, 2, 1 },
	{ "Switch2", "", 32, 2, 1 },
	{ "SerpentSight", "", 32, 2, 1 },
	{ "SerpentActive", "", 32, 2, 1 },
	{ "SerpentPain", "", 32, 2, 1 },
	{ "SerpentAttack", "", 32, 2, 1 },
	{ "SerpentMeleeHit", "", 32, 2, 1 },
	{ "SerpentDeath", "", 40, 2, 1 },
	{ "SerpentBirth", "", 32, 2, 1 },
	{ "SerpentFXContinuous", "", 32, 2, 1 },
	{ "SerpentFXHit", "", 32, 2, 1 },
	{ "PotteryExplode", "", 32, 2, 1 },
	{ "Drip", "", 32, 2, 1 },
	{ "CentaurSight", "", 32, 2, 1 },
	{ "CentaurActive", "", 32, 2, 1 },
	{ "CentaurPain", "", 32, 2, 1 },
	{ "CentaurAttack", "", 32, 2, 1 },
	{ "CentaurDeath", "", 40, 2, 1 },
	{ "CentaurLeaderAttack", "", 32, 2, 1 },
	{ "CentaurMissileExplode", "", 32, 2, 1 },
	{ "Wind", "", 1, 2, 1 },
	{ "BishopSight", "", 32, 2, 1 },
	{ "BishopActive", "", 32, 2, 1 },
	{ "BishopPain", "", 32, 2, 1 },
	{ "BishopAttack", "", 32, 2, 1 },
	{ "BishopDeath", "", 40, 2, 1 },
	{ "BishopMissileExplode", "", 32, 2, 1 },
	{ "BishopBlur", "", 32, 2, 1 },
	{ "DemonSight", "", 32, 2, 1 },
	{ "DemonActive", "", 32, 2, 1 },
	{ "DemonPain", "", 32, 2, 1 },
	{ "DemonAttack", "", 32, 2, 1 },
	{ "DemonMissileFire", "", 32, 2, 1 },
	{ "DemonMissileExplode", "", 32, 2, 1 },
	{ "DemonDeath", "", 40, 2, 1 },
	{ "WraithSight", "", 32, 2, 1 },
	{ "WraithActive", "", 32, 2, 1 },
	{ "WraithPain", "", 32, 2, 1 },
	{ "WraithAttack", "", 32, 2, 1 },
	{ "WraithMissileFire", "", 32, 2, 1 },
	{ "WraithMissileExplode", "", 32, 2, 1 },
	{ "WraithDeath", "", 40, 2, 1 },
	{ "PigActive1", "", 32, 2, 1 },
	{ "PigActive2", "", 32, 2, 1 },
	{ "PigPain", "", 32, 2, 1 },
	{ "PigAttack", "", 32, 2, 1 },
	{ "PigDeath", "", 40, 2, 1 },
	{ "MaulatorSight", "", 32, 2, 1 },
	{ "MaulatorActive", "", 32, 2, 1 },
	{ "MaulatorPain", "", 32, 2, 1 },
	{ "MaulatorHamSwing", "", 32, 2, 1 },
	{ "MaulatorHamHit", "", 32, 2, 1 },
	{ "MaulatorMissileHit", "", 32, 2, 1 },
	{ "MaulatorDeath", "", 40, 2, 1 },
	{ "FreezeDeath", "", 40, 2, 1 },
	{ "FreezeShatter", "", 40, 2, 1 },
	{ "EttinSight", "", 32, 2, 1 },
	{ "EttinActive", "", 32, 2, 1 },
	{ "EttinPain", "", 32, 2, 1 },
	{ "EttinAttack", "", 32, 2, 1 },
	{ "EttinDeath", "", 40, 2, 1 },
	{ "FireDemonSpawn", "", 32, 2, 1 },
	{ "FireDemonActive", "", 32, 2, 1 },
	{ "FireDemonPain", "", 32, 2, 1 },
	{ "FireDemonAttack", "", 32, 2, 1 },
	{ "FireDemonMissileHit", "", 32, 2, 1 },
	{ "FireDemonDeath", "", 40, 2, 1 },
	{ "IceGuySight", "", 32, 2, 1 },
	{ "IceGuyActive", "", 32, 2, 1 },
	{ "IceGuyAttack", "", 32, 2, 1 },
	{ "IceGuyMissileExplode", "", 32, 2, 1 },
	{ "SorcererSight", "", 256, 2, 1 },
	{ "SorcererActive", "", 256, 2, 1 },
	{ "SorcererPain", "", 256, 2, 1 },
	{ "SorcererSpellCast", "", 256, 2, 1 },
	{ "SorcererBallWoosh", "", 256, 4, 1 },
	{ "SorcererDeathScream", "", 256, 2, 1 },
	{ "SorcererBishopSpawn", "", 80, 2, 1 },
	{ "SorcererBallPop", "", 80, 2, 1 },
	{ "SorcererBallBounce", "", 80, 3, 1 },
	{ "SorcererBallExplode", "", 80, 3, 1 },
	{ "SorcererBigBallExplode", "", 80, 3, 1 },
	{ "SorcererHeadScream", "", 256, 2, 1 },
	{ "DragonSight", "", 64, 2, 1 },
	{ "DragonActive", "", 64, 2, 1 },
	{ "DragonWingflap", "", 64, 2, 1 },
	{ "DragonAttack", "", 64, 2, 1 },
	{ "DragonPain", "", 64, 2, 1 },
	{ "DragonDeath", "", 64, 2, 1 },
	{ "DragonFireballExplode", "", 32, 2, 1 },
	{ "KoraxSight", "", 256, 2, 1 },
	{ "KoraxActive", "", 256, 2, 1 },
	{ "KoraxPain", "", 256, 2, 1 },
	{ "KoraxAttack", "", 256, 2, 1 },
	{ "KoraxCommand", "", 256, 2, 1 },
	{ "KoraxDeath", "", 256, 2, 1 },
	{ "KoraxStep", "", 128, 2, 1 },
	{ "ThrustSpikeRaise", "", 32, 2, 1 },
	{ "ThrustSpikeLower", "", 32, 2, 1 },
	{ "GlassShatter", "", 32, 2, 1 },
	{ "FlechetteBounce", "", 32, 2, 1 },
	{ "FlechetteExplode", "", 32, 2, 1 },
	{ "LavaMove", "", 36, 2, 1 },
	{ "WaterMove", "", 36, 2, 1 },
	{ "IceStartMove", "", 36, 2, 1 },
	{ "EarthStartMove", "", 36, 2, 1 },
	{ "WaterSplash", "", 32, 2, 1 },
	{ "LavaSizzle", "", 32, 2, 1 },
	{ "SludgeGloop", "", 32, 2, 1 },
	{ "HolySymbolFire", "", 64, 2, 1 },
	{ "SpiritActive", "", 32, 2, 1 },
	{ "SpiritAttack", "", 32, 2, 1 },
	{ "SpiritDie", "", 32, 2, 1 },
	{ "ValveTurn", "", 36, 2, 1 },
	{ "RopePull", "", 36, 2, 1 },
	{ "FlyBuzz", "", 20, 2, 1 },
	{ "Ignite", "", 32, 2, 1 },
	{ "PuzzleSuccess", "", 256, 2, 1 },
	{ "PuzzleFailFighter", "", 256, 2, 1 },
	{ "PuzzleFailCleric", "", 256, 2, 1 },
	{ "PuzzleFailMage", "", 256, 2, 1 },
	{ "Earthquake", "", 32, 2, 1 },
	{ "BellRing", "", 32, 2, 0 },
	{ "TreeBreak", "", 32, 2, 1 },
	{ "TreeExplode", "", 32, 2, 1 },
	{ "SuitofArmorBreak", "", 32, 2, 1 },
	{ "PoisonShroomPain", "", 20, 2, 1 },
	{ "PoisonShroomDeath", "", 32, 2, 1 },
	{ "Ambient1", "", 1, 1, 1 },
	{ "Ambient2", "", 1, 1, 1 },
	{ "Ambient3", "", 1, 1, 1 },
	{ "Ambient4", "", 1, 1, 1 },
	{ "Ambient5", "", 1, 1, 1 },
	{ "Ambient6", "", 1, 1, 1 },
	{ "Ambient7", "", 1, 1, 1 },
	{ "Ambient8", "", 1, 1, 1 },
	{ "Ambient9", "", 1, 1, 1 },
	{ "Ambient10", "", 1, 1, 1 },
	{ "Ambient11", "", 1, 1, 1 },
	{ "Ambient12", "", 1, 1, 1 },
	{ "Ambient13", "", 1, 1, 1 },
	{ "Ambient14", "", 1, 1, 1 },
	{ "Ambient15", "", 1, 1, 1 },
	{ "StartupTick", "", 32, 2, 1 },
	{ "SwitchOtherLevel", "", 32, 2, 1 },
	{ "Respawn", "", 32, 2, 1 },
	{ "KoraxVoiceGreetings", "", 512, 2, 1 },
	{ "KoraxVoiceReady", "", 512, 2, 1 },
	{ "KoraxVoiceBlood", "", 512, 2, 1 },
	{ "KoraxVoiceGame", "", 512, 2, 1 },
	{ "KoraxVoiceBoard", "", 512, 2, 1 },
	{ "KoraxVoiceWorship", "", 512, 2, 1 },
	{ "KoraxVoiceMaybe", "", 512, 2, 1 },
	{ "KoraxVoiceStrong", "", 512, 2, 1 },
	{ "KoraxVoiceFace", "", 512, 2, 1 },
	{ "BatScream", "", 32, 2, 1 },
	{ "Chat", "", 512, 2, 1 },
	{ "MenuMove", "hamwal1", 32, 2, 1 },
	{ "ClockTick", "", 32, 2, 1 },
	{ "Fireball", "", 32, 2, 1 },
	{ "PuppyBeat", "", 30, 2, 1 },
	{ "MysticIncant", "", 32, 4, 1 },
	{ "MenuActivate", "dorstp1b", 32, 2, 1 },
	{ "MenuSelect", "dorstp4", 32, 2, 1 },
	{ "MenuSwitch", "keys2a", 32, 2, 1 },
	{ "MenuPrevious", "keys2a", 32, 2, 1 },
	{ "MenuClose", "dorstp4", 32, 2, 1 },
};

int			num_sfx = sizeof(sfx) / sizeof(sfx[0]);
