//**************************************************************************
//**
//**    ##   ##    ##    ##   ##   ####     ####   ###     ###
//**    ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**     ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**     ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**      ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**       #    ##    ##    #      ####     ####   ##       ##
//**
//**  Copyright (C) 1999-2006 Jānis Legzdiņš
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
#include "core.h"

#if defined(WIN32)
# include <windows.h>
#endif

#ifdef ANDROID
# include <SDL.h>
#endif


#ifdef USE_GUARD_SIGNAL_CONTEXT
jmp_buf VSigContextHack::Env;
const char *VSigContextHack::ErrToThrow;
#endif

static char *host_error_string;

// call `abort()` or `exit()` there to stop standard processing
void (*SysErrorCB) (const char *msg) noexcept = nullptr;


//==========================================================================
//
//  VavoomError::VavoomError
//
//==========================================================================
VavoomError::VavoomError (const char *text) noexcept {
  VStr::NCpy(message, text, MAX_ERROR_TEXT_SIZE-1);
  message[MAX_ERROR_TEXT_SIZE-1] = 0;
}


//==========================================================================
//
//  VavoomError::What
//
//==========================================================================
const char *VavoomError::What () const noexcept {
  return message;
}


//==========================================================================
//
//  Host_CoreDump
//
//==========================================================================
void Host_CoreDump (const char *fmt, ...) noexcept {
  bool first = false;

  if (!host_error_string) {
    host_error_string = new char[32];
    VStr::Cpy(host_error_string, "Stack trace: ");
    first = true;
  }

  va_list argptr;
  static char string[1024]; //WARNING! not thread-safe!

  va_start(argptr, fmt);
  vsnprintf(string, sizeof(string), fmt, argptr);
  va_end(argptr);

  GLog.WriteLine("- %s", string);

  char *new_string = new char[VStr::Length(host_error_string)+VStr::Length(string)+6];
  VStr::Cpy(new_string, host_error_string);
  if (first) first = false; else strcat(new_string, " <- ");
  strcat(new_string, string);
  delete[] host_error_string;
  host_error_string = nullptr;
  host_error_string = new_string;
}


//==========================================================================
//
//  Host_GetCoreDump
//
//==========================================================================
const char *Host_GetCoreDump () noexcept {
  return (host_error_string ? host_error_string : "");
}


//==========================================================================
//
//  Sys_Error
//
//  Exits game and displays error message.
//
//==========================================================================
void Sys_Error (const char *error, ...) noexcept {
  va_list argptr;
  static char buf[16384]; //WARNING! not thread-safe!

  va_start(argptr,error);
  vsnprintf(buf, sizeof(buf), error, argptr);
  va_end(argptr);

  if (SysErrorCB) SysErrorCB(buf);

#if defined(WIN32)
  MessageBox(NULL, buf, "k8vavoom Fatal Error", MB_OK);
#elif defined(ANDROID)
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "k8vavoom Fatal Error", buf, NULL);
  _Exit(1);
/*
#else //if defined(VCC_STANDALONE_EXECUTOR)
  fputs("FATAL: ", stderr);
  fputs(buf, stderr);
  fputc('\n', stderr);
*/
#endif
  GLog.WriteLine(NAME_Error, "Sys_Error: %s", buf);
  //throw VavoomError(buf);
  abort(); // abort here, so we can drop back to gdb
}
