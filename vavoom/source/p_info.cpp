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
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "gamedefs.h"

// MACROS ------------------------------------------------------------------

#define MAPINFO_SCRIPT_NAME	"MAPINFO"
#define UNKNOWN_MAP_NAME	"DEVELOPMENT MAP"
#define DEFAULT_SKY_NAME	"SKY1"
#define DEFAULT_FADE_TABLE	"COLORMAP"

enum
{
	MCMD_SKY1,
	MCMD_SKY2,
	MCMD_DOUBLESKY,
	MCMD_LIGHTNING,
	MCMD_FADETABLE,
	MCMD_CLUSTER,
	MCMD_WARPTRANS,
	MCMD_NEXT,
	MCMD_MUSIC,
	MCMD_CDTRACK,
	MCMD_CD_STARTTRACK,
	MCMD_CD_END1TRACK,
	MCMD_CD_END2TRACK,
	MCMD_CD_END3TRACK,
	MCMD_CD_INTERTRACK,
	MCMD_CD_TITLETRACK,
};

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static mapInfo_t	MapInfo[99];
static int 			MapCount = 98;

static char *MapCmdNames[] =
{
	"SKY1",
	"SKY2",
	"DOUBLESKY",
	"LIGHTNING",
	"FADETABLE",
	"CLUSTER",
	"WARPTRANS",
	"NEXT",
    "MUSIC",
	"CDTRACK",
	"CD_START_TRACK",
	"CD_END1_TRACK",
	"CD_END2_TRACK",
	"CD_END3_TRACK",
	"CD_INTERMISSION_TRACK",
	"CD_TITLE_TRACK",
	NULL
};

static int cd_NonLevelTracks[6]; // Non-level specific song cd track numbers 

// CODE --------------------------------------------------------------------

//==========================================================================
//
// InitMapInfo
//
//==========================================================================

void InitMapInfo(void)
{
	int 		mcmdValue;
	mapInfo_t 	*info;
	char 		songMulch[10];

	MapCount = 1;

	// Put defaults into MapInfo[0]
	info = MapInfo;
	info->cluster = 0;
	info->warpTrans = 0;
	strcpy(info->nextMap, "MAP01"); // Always go to map 1 if not specified
	info->cdTrack = 1;
	info->sky1Texture = R_CheckTextureNumForName(DEFAULT_SKY_NAME);
	if (info->sky1Texture < 0)
		info->sky1Texture = R_CheckTextureNumForName("SKYMNT02");
	info->sky2Texture = info->sky1Texture;
	info->sky1ScrollDelta = 0.0;
	info->sky2ScrollDelta = 0.0;
	info->doubleSky = false;
	info->lightning = false;
	strcpy(info->fadetable, DEFAULT_FADE_TABLE);
	strcpy(info->name, UNKNOWN_MAP_NAME);

	SC_Open(MAPINFO_SCRIPT_NAME);
	while(SC_GetString())
	{
		if (SC_Compare("MAP") == false)
		{
			SC_ScriptError(NULL);
		}

		info = &MapInfo[MapCount];

		// Save song lump name
		strcpy(songMulch, info->songLump);

		// Copy defaults to current map definition
		memcpy(info, &MapInfo[0], sizeof(*info));

		// Restore song lump name
		strcpy(info->songLump, songMulch);

		// The warp translation defaults to the map	index
		info->warpTrans = MapCount;

		MapCount++;

		SC_MustGetString();
		if (sc_String[0] >= '0' && sc_String[0] <= '9')
		{
			//	Map number, for Hexen compatibility
			SC_UnGet();
			SC_MustGetNumber();
			if (sc_Number < 1 || sc_Number > 99)
			{
				SC_ScriptError(NULL);
			}
			sprintf(info->lumpname, "MAP%02d", sc_Number);
		}
		else
		{
			//	Map name
			if (strlen(sc_String) > 8)
			{
				SC_ScriptError(NULL);
			}
			W_CleanupName(sc_String, info->lumpname);
		}

		// Map name must follow the number
		SC_MustGetString();
		strcpy(info->name, sc_String);

		// Process optional tokens
		while (SC_GetString())
		{
			if (SC_Compare("MAP"))
			{ // Start next map definition
				SC_UnGet();
				break;
			}
			mcmdValue = SC_MustMatchString(MapCmdNames);
			switch(mcmdValue)
			{
				case MCMD_CLUSTER:
					SC_MustGetNumber();
					info->cluster = sc_Number;
					break;
				case MCMD_WARPTRANS:
					SC_MustGetNumber();
					info->warpTrans = sc_Number;
					break;
				case MCMD_NEXT:
					SC_MustGetString();
					strcpy(info->nextMap, sc_String);
					break;
				case MCMD_CDTRACK:
					SC_MustGetNumber();
					info->cdTrack = sc_Number;
					break;
				case MCMD_SKY1:
					SC_MustGetString();
					info->sky1Texture = R_CheckTextureNumForName(sc_String);
					SC_MustGetNumber();
					info->sky1ScrollDelta = (float)sc_Number * 35.0 / 256.0;
					break;
				case MCMD_SKY2:
					SC_MustGetString();
					info->sky2Texture = R_CheckTextureNumForName(sc_String);
					SC_MustGetNumber();
					info->sky2ScrollDelta = (float)sc_Number * 35.0 / 256.0;
					break;
				case MCMD_DOUBLESKY:
					info->doubleSky = true;
					break;
				case MCMD_LIGHTNING:
					info->lightning = true;
					break;
				case MCMD_FADETABLE:
					SC_MustGetString();
					strcpy(info->fadetable, sc_String);
					break;
				case MCMD_CD_STARTTRACK:
				case MCMD_CD_END1TRACK:
				case MCMD_CD_END2TRACK:
				case MCMD_CD_END3TRACK:
				case MCMD_CD_INTERTRACK:
				case MCMD_CD_TITLETRACK:
					SC_MustGetNumber();
					cd_NonLevelTracks[mcmdValue-MCMD_CD_STARTTRACK] = sc_Number;
					break;
				case MCMD_MUSIC:
					SC_MustGetString();
					strcpy(info->songLump, sc_String);
					break;
			}
		}
	}
	SC_Close();
}

//==========================================================================
//
// QualifyMap
//
//==========================================================================

static int QualifyMap(int map)
{
	return (map < 1 || map > MapCount) ? 0 : map;
}

//==========================================================================
//
//	P_GetMapInfo
//
//==========================================================================

void P_GetMapInfo(int map, mapInfo_t &info)
{
	info = MapInfo[QualifyMap(map)];
}

//==========================================================================
//
//	P_GetMapInfo
//
//==========================================================================

void P_GetMapInfo(const char *map, mapInfo_t &info)
{
	for (int i = 1; i < 99; i++)
	{
		if (!stricmp(map, MapInfo[i].lumpname))
		{
			info = MapInfo[i];
			return;
		}
	}
	info = MapInfo[0];
}

//==========================================================================
//
//	P_GetMapName
//
//==========================================================================

char* P_GetMapName(int map)
{
	return MapInfo[QualifyMap(map)].name;
}

//==========================================================================
//
// P_GetMapCluster
//
//==========================================================================

int P_GetMapCluster(int map)
{
	return MapInfo[QualifyMap(map)].cluster;
}

//==========================================================================
//
// P_GetMapWarpTrans
//
//==========================================================================

int P_GetMapWarpTrans(int map)
{
	return MapInfo[QualifyMap(map)].warpTrans;
}

//==========================================================================
//
// P_TranslateMap
//
// Returns the actual map number given a warp map number.
//
//==========================================================================

int P_TranslateMap(int map)
{
	int i;

	for(i = 1; i < 99; i++) // Make this a macro
	{
		if(MapInfo[i].warpTrans == map)
		{
			return i;
		}
	}
	// Not found
	return -1;
}

//==========================================================================
//
// P_PutMapSongLump
//
//==========================================================================

void P_PutMapSongLump(int map, char *lumpName)
{
	if (map >= 1 && map <= MapCount)
	{
		strcpy(MapInfo[map].songLump, lumpName);
	}
}

//==========================================================================
//
// P_GetCDStartTrack
//
//==========================================================================

int P_GetCDStartTrack(void)
{
	return cd_NonLevelTracks[MCMD_CD_STARTTRACK-MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDEnd1Track
//
//==========================================================================

int P_GetCDEnd1Track(void)
{
	return cd_NonLevelTracks[MCMD_CD_END1TRACK-MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDEnd2Track
//
//==========================================================================

int P_GetCDEnd2Track(void)
{
	return cd_NonLevelTracks[MCMD_CD_END2TRACK-MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDEnd3Track
//
//==========================================================================

int P_GetCDEnd3Track(void)
{
	return cd_NonLevelTracks[MCMD_CD_END3TRACK-MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDIntermissionTrack
//
//==========================================================================

int P_GetCDIntermissionTrack(void)
{
	return cd_NonLevelTracks[MCMD_CD_INTERTRACK-MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDTitleTrack
//
//==========================================================================

int P_GetCDTitleTrack(void)
{
	return cd_NonLevelTracks[MCMD_CD_TITLETRACK-MCMD_CD_STARTTRACK];
}

/*
	int		cluster;    // Defines what cluster level belongs to
	int		warpTrans;  // Actual map number in case maps are not sequential
	int		nextMap;    // Map to teleport to upon exit of timed deathmatch
	int		cdTrack;    // CD track to play during level
	char	fadetable[12];// Fade table {fogmap}
	char	songLump[12];// Background music (MUS or MIDI)
*/

//**************************************************************************
//
//	$Log$
//	Revision 1.3  2001/07/31 17:16:31  dj_jl
//	Just moved Log to the end of file
//
//	Revision 1.2  2001/07/27 14:27:54  dj_jl
//	Update with Id-s and Log-s, some fixes
//
//**************************************************************************
