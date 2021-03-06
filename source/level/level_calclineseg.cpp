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
#include "../gamedefs.h"


//==========================================================================
//
//  VLevel::CalcLine
//
//==========================================================================
void VLevel::CalcLine (line_t *line) {
  // calc line's slopetype
  line->dir = (*line->v2)-(*line->v1);
  line->dir.z = 0.0f;

  if (line->dir.x == 0.0f) {
    line->slopetype = ST_VERTICAL;
  } else if (line->dir.y == 0.0f) {
    line->slopetype = ST_HORIZONTAL;
  } else {
    line->slopetype = (line->dir.y*line->dir.x >= 0.0f ? ST_POSITIVE : ST_NEGATIVE);
  }

  line->SetPointDirXY(*line->v1, line->dir);
  line->ndir = line->dir.normalised2D();

  // calc line's bounding box
  line->bbox2d[BOX2D_LEFT] = min2(line->v1->x, line->v2->x);
  line->bbox2d[BOX2D_RIGHT] = max2(line->v1->x, line->v2->x);
  line->bbox2d[BOX2D_BOTTOM] = min2(line->v1->y, line->v2->y);
  line->bbox2d[BOX2D_TOP] = max2(line->v1->y, line->v2->y);

  CalcLineCDPlanes(line);
}


//==========================================================================
//
//  VLevel::CalcSegLenOfs
//
//  only length and offset
//
//==========================================================================
void VLevel::CalcSegLenOfs (seg_t *seg) {
  const line_t *ldef = seg->linedef;
  if (ldef) {
    const TVec *vv = (seg->side ? ldef->v2 : ldef->v1);
    seg->offset = seg->v1->DistanceTo2D(*vv);
  }
  seg->length = seg->v2->DistanceTo2D(*seg->v1);
  if (!isFiniteF(seg->length)) seg->length = 0.0f; // just in case
}


//==========================================================================
//
//  VLevel::CalcSeg
//
//==========================================================================
void VLevel::CalcSeg (seg_t *seg) {
  seg->Set2Points(*seg->v1, *seg->v2);
  bool valid = (isFiniteF(seg->length) && seg->length >= 0.0001f);
  if (valid) {
    if (seg->v1->x == seg->v2->x) {
      // vertical
      if (seg->v1->y == seg->v2->y) {
        valid = false;
      } else {
        seg->dir = TVec(0, (seg->v1->y < seg->v2->y ? 1 : -1), 0);
      }
    } else if (seg->v1->y == seg->v2->y) {
      // horizontal
      seg->dir = TVec((seg->v1->x < seg->v2->x ? 1 : -1), 0, 0);
    } else {
      seg->dir = ((*seg->v2)-(*seg->v1)).normalised2D();
    }
    if (!seg->dir.isValid() || seg->dir.isZero2D()) valid = false;
  }
  if (!valid) {
    GCon->Logf(NAME_Warning, "ZERO-LENGTH %sseg #%d (flags: 0x%04x)!", (seg->linedef ? "" : "mini"), (int)(ptrdiff_t)(seg-Segs), (unsigned)seg->flags);
    GCon->Logf(NAME_Warning, "  verts: (%g,%g,%g)-(%g,%g,%g)", seg->v1->x, seg->v1->y, seg->v1->z, seg->v2->x, seg->v2->y, seg->v2->z);
    GCon->Logf(NAME_Warning, "  offset: %g", seg->offset);
    GCon->Logf(NAME_Warning, "  length: %g", seg->length);
    if (seg->linedef) {
      GCon->Logf(NAME_Warning, "  linedef: %d", (int)(ptrdiff_t)(seg->linedef-Lines));
      GCon->Logf(NAME_Warning, "  sidedef: %d (side #%d)", (int)(ptrdiff_t)(seg->sidedef-Sides), seg->side);
      GCon->Logf(NAME_Warning, "  front sector: %d", (int)(ptrdiff_t)(seg->frontsector-Sectors));
      if (seg->backsector) GCon->Logf(NAME_Warning, "  back sector: %d", (int)(ptrdiff_t)(seg->backsector-Sectors));
    }
    if (seg->partner) GCon->Logf(NAME_Warning, "  partner: %d", (int)(ptrdiff_t)(seg->partner-Segs));
    if (seg->frontsub) GCon->Logf(NAME_Warning, "  frontsub: %d", (int)(ptrdiff_t)(seg->frontsub-Subsectors));

    seg->dir = TVec(1.0f, 0.0f, 0.0f); // arbitrary
    seg->flags |= SF_ZEROLEN;
    if (!isFiniteF(seg->offset)) seg->offset = 0.0f;
    seg->length = 0.0001f;
    // setup fake seg's plane params
    seg->normal = TVec(1.0f, 0.0f, 0.0f);
    seg->dist = 0.0f;
    seg->dir = TVec(1.0f, 0.0f, 0.0f); // arbitrary
  } else {
    seg->flags &= ~SF_ZEROLEN;
  }
}
