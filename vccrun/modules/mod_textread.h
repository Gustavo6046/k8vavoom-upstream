/**************************************************************************
 *
 * Coded by Ketmar Dark, 2018
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **************************************************************************/
#ifndef VCCMOD_TEXTREAD_HEADER_FILE
#define VCCMOD_TEXTREAD_HEADER_FILE

#include "../vcc_run.h"


// ////////////////////////////////////////////////////////////////////////// //
class VTextReader : public VObject {
  DECLARE_CLASS(VTextReader, VObject, 0)
  NO_DEFAULT_CONSTRUCTOR(VTextReader)

public:
  enum {
    SeekStart,
    SeekCur,
    SeekEnd,
  };

private:
  VStream *fstream;

public:
  virtual void Destroy () override;

public:
  DECLARE_FUNCTION(Open)
  DECLARE_FUNCTION(Destroy)
  DECLARE_FUNCTION(close)
  DECLARE_FUNCTION(seek)
  DECLARE_FUNCTION(getch)
  DECLARE_FUNCTION(readBuf)

  DECLARE_FUNCTION(get_fileName)
  DECLARE_FUNCTION(get_isOpen)
  DECLARE_FUNCTION(get_error)
  DECLARE_FUNCTION(get_size)
  DECLARE_FUNCTION(get_position)
};


#endif
