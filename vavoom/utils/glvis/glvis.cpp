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
//**
//**	Potentially Visible Set
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <time.h>
//	When compiling with -ansi isatty() is not declared
#if defined __unix__ && !defined __STRICT_ANSI__
#include <unistd.h>
#endif
#include "glvis.h"

// MACROS ------------------------------------------------------------------

#define TEMP_FILE	"$glvis$$.$$$"

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

TIWadFile		inwad;
TIWadFile		gwa;
TOWadFile		outwad;

TIWadFile		*mainwad;
TIWadFile		*glwad;

bool			silent_mode = false;
bool			show_progress = true;
bool			fastvis = false;
bool			verbose = false;

int				testlevel = 2;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int		num_specified_maps = 0;
static char		specified_maps[100][16];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	IsLevelName
//
//==========================================================================

static bool	IsLevelName(const char *name)
{
	if (name[0] != 'G' || name[1] != 'L' || name[2] != '_')
	{
		return false;
	}

	if (num_specified_maps)
	{
		for (int i = 0; i < num_specified_maps; i++)
		{
			if (!strcmp(specified_maps[i], name + 3))
			{
				return true;
			}
		}
	}
	else
	{
		if (name[3] == 'E' && name[4] >= '0' && name[4] <= '9' &&
			name[5] == 'M' && name[6] >= '0' && name[6] <= '9')
		{
			return true;
		}
		if (name[3] == 'M' && name[4] == 'A' && name[5] == 'P' &&
			name[6] >= '0' && name[6] <= '9' &&
			name[7] >= '0' && name[7] <= '9')
		{
			return true;
		}
	}
	return false;
}

//==========================================================================
//
//	ShowUsage
//
//==========================================================================

static void ShowUsage(void)
{
	cerr << "\nGLVIS version 1.2, Copyright (c)2000 J�nis Legzdi��      ("__DATE__" "__TIME__")\n";
	cerr << "Usage: glvis [options] file[.wad]\n";
	cerr << "    -s            silent mode\n";
	cerr << "    -f            fast mode\n";
	cerr << "    -v            verbose mode\n";
	cerr << "    -t#           specify test level\n";
	cerr << "    -m<LEVELNAME> specifies a level to process, can be used multiple times\n";
	exit(1);
}

//==========================================================================
//
//	main
//
//==========================================================================

int main(int argc, char *argv[])
{
	char filename[1024];
	char destfile[1024];
	char bakext[8];
	char *srcfile = NULL;
	int i;

	int starttime = time(0);

	for (i = 1; i < argc; i++)
	{
		char *arg = argv[i];
		if (*arg == '-')
		{
			switch (arg[1])
			{
			 case 's':
				silent_mode = true;
				break;

			 case 'f':
				fastvis = true;
				break;

			 case 'v':
				verbose = true;
				break;

			 case 't':
				testlevel = arg[2] - '0';
				break;

			 case 'm':
				CleanupName(arg + 2, specified_maps[num_specified_maps++]);
				break;

			 default:
				ShowUsage();
			}
		}
		else
		{
			if (srcfile)
			{
				ShowUsage();
			}
			srcfile = arg;
		}
	}

	if (!srcfile)
	{
		ShowUsage();
	}

	show_progress = !silent_mode;
#if defined __unix__ && !defined __STRICT_ANSI__
	// Unix: no whirling baton if stderr is redirected
	if (!isatty(2))
		show_progress = false;
#endif

	strcpy(filename, srcfile);
	DefaultExtension(filename, ".wad");
	strcpy(destfile, filename);
	inwad.Open(filename);
	mainwad = &inwad;

	StripExtension(filename);
	strcat(filename, ".gwa");
	FILE *ff = fopen(filename, "rb");
	if (ff)
	{
		fclose(ff);
		gwa.Open(filename);
		glwad = &gwa;
		strcpy(destfile, filename);
		strcpy(bakext, ".~gw");
	}
	else
	{
		glwad = &inwad;
		strcpy(bakext, ".~wa");
	}

	outwad.Open(TEMP_FILE, glwad->wadid);

	//	Process lumps
	i = 0;
	while (i < glwad->numlumps)
	{
		void *ptr =	glwad->GetLump(i);
		const char *name = glwad->LumpName(i);
		outwad.AddLump(name, ptr, glwad->LumpSize(i));
		Free(ptr);
		if (IsLevelName(name))
		{
			LoadLevel(mainwad->LumpNumForName(name + 3), i);
			i += 5;
			if (!strcmp("GL_PVS", glwad->LumpName(i)))
			{
				i++;
			}
			BuildPVS();
			FreeLevel();
		}
		else
		{
			i++;
		}
	}

	inwad.Close();
	if (gwa.handle)
	{
		gwa.Close();
	}
	outwad.Close();

	strcpy(filename, destfile);
	StripExtension(filename);
	strcat(filename, bakext);
	remove(filename);
	rename(destfile, filename);
	rename(TEMP_FILE, destfile);

	if (!silent_mode)
	{
		int worktime = time(0) - starttime;
		fprintf(stderr, "Time elapsed: %d:%02d:%02d\n", worktime / 3600, (worktime / 60) % 60, worktime % 60);
	}

	return 0;
}

