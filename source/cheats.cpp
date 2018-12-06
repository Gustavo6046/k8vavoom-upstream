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
//**
//**  Self registering cheat commands.
//**
//**************************************************************************
#include "gamedefs.h"
#include "sv_local.h"


//==========================================================================
//
//  CheatAllowed
//
//==========================================================================
static bool CheatAllowed (VBasePlayer *Player) {
  if (!Player) return false;
  if (sv.intermission) {
    Player->Printf("You are not in game!");
    return false;
  }
  if (GGameInfo->NetMode >= NM_DedicatedServer) {
    Player->Printf("You cannot cheat in a network game!");
    return false;
  }
  if (GGameInfo->WorldInfo->Flags&VWorldInfo::WIF_SkillDisableCheats) {
    Player->Printf("You are too good to cheat!");
    return false;
  }
  if (Player->Health <= 0) {
    // dead players can't cheat
    Player->Printf("You must be alive to cheat");
    return false;
  }
  return true;
}


//==========================================================================
//
//  God_f
//
//  Cheat code GOD
//
//==========================================================================
COMMAND(God) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (!Player) return;
  if (CheatAllowed(Player)) Player->eventCheat_God();
}


//==========================================================================
//
//  Buddha_f
//
//  Cheat code Buddha
//
//==========================================================================
COMMAND(Buddha) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Buddha();
}


//==========================================================================
//
//  Summon_f
//
//  Cheat code Summon
//
//==========================================================================
COMMAND_WITH_AC(Summon) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Summon();
}

extern "C" {
  static int sortCmpVStrCI (const void *a, const void *b, void *udata) {
    if (a == b) return 0;
    VStr *sa = (VStr *)a;
    VStr *sb = (VStr *)b;
    return sa->ICmp(*sb);
  }
}

COMMAND_AC(Summon) {
  if (aidx != 1) return VStr::EmptyString;
  VClass *actor = VMemberBase::StaticFindClass("Actor");
  if (!actor) return VStr::EmptyString;
  VStr prefix = (aidx < args.length() ? args[aidx] : VStr());
  TArray<VStr> list;
  VMemberBase::StaticGetClassListNoCase(list, prefix, actor);
  // replace with replacements ;-)
  /*
  for (int f = 0; f < list.length(); ++f) {
    VClass *cls = VMemberBase::StaticFindClass(*list[f]);
    if (!cls) { list[f].clear(); continue; }
    VClass *c1 = cls->GetReplacement();
    if (!c1 || !c1->IsChildOf(actor)) { list[f].clear(); continue; }
    list[f] = VStr(c1->GetName());
  }
  */
  if (!list.length()) return VStr::EmptyString;
  // sort
  timsort_r(list.ptr(), list.length(), sizeof(VStr), &sortCmpVStrCI, nullptr);
  // drop unspawnable actors
  TArray<VStr> newlist;
  for (int f = 0; f < list.length(); ++f) {
    if (list.length() == 0 || (newlist.length() && newlist[newlist.length()-1] == list[f])) continue;
    VClass *cls = VMemberBase::StaticFindClass(*list[f]);
    if (!cls) continue;
    VClass *c1 = cls->GetReplacement();
    if (!c1 || !c1->IsChildOf(actor)) continue;
    // check state
    VStateLabel *lbl = cls->FindStateLabel("Spawn");
    if (!lbl || !lbl->State) continue; // no spawn state
    if (!R_AreSpritesPresent(lbl->State->SpriteIndex)) continue; // no sprite
    newlist.append(list[f]);
  }
  return AutoCompleteFromList(prefix, newlist, true); // return unchanged as empty
}


//==========================================================================
//
//  NoClip_f
//
//==========================================================================
COMMAND(NoClip) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_NoClip();
}


//==========================================================================
//
//  Gimme_f
//
//==========================================================================
COMMAND(Gimme) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Gimme();
}


//==========================================================================
//
//  KillAll_f
//
//==========================================================================
COMMAND(KillAll) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_KillAll();
}


//==========================================================================
//
//  Morph_f
//
//==========================================================================
COMMAND(Morph) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Morph();
}


//==========================================================================
//
//  NoWeapons_f
//
//==========================================================================
COMMAND(NoWeapons) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_NoWeapons();
}


//==========================================================================
//
//  Class_f
//
//==========================================================================
COMMAND(ChangeClass) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Class();
}


//==========================================================================
//
//  Script_f
//
//==========================================================================
COMMAND(Script) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }

  if (CheatAllowed(Player)) {
    if (Args.Num() != 2) return;
    int script = atoi(*Args[1]);
    if (script < 1) return;
    if (script > 9999) return;
    if (Player->Level->XLevel->Acs->Start(script, 0, 0, 0, 0, 0, Player->MO, nullptr, 0, false, false)) {
      GCon->Logf("Running script %d", script);
    }
  }
}


//==========================================================================
//
//  MyPos_f
//
//==========================================================================
COMMAND(MyPos) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) {
    Player->Printf("MAP %s  X:%f  Y:%f  Z:%f  Yaw:%f Pitch:%f",
      *GLevel->MapName, Player->MO->Origin.x,
      Player->MO->Origin.y, Player->MO->Origin.z,
      Player->MO->Angles.yaw, Player->MO->Angles.pitch);
  }
}


//==========================================================================
//
//  Fly_f
//
//==========================================================================
COMMAND(Fly) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Fly();
}


//==========================================================================
//
//  NoTarget_f
//
//==========================================================================
COMMAND(NoTarget) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_NoTarget();
}


//==========================================================================
//
//  Anubis_f
//
//==========================================================================
COMMAND(Anubis) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Anubis();
}


//==========================================================================
//
//  Freeze_f
//
//==========================================================================
COMMAND(Freeze) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Freeze();
}


//==========================================================================
//
//  my_sector_info
//
//==========================================================================
COMMAND(my_sector_info) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (!Player) { GCon->Log("NO PLAYER!"); return; }
  if (!Player->MO) { GCon->Log("NO PLAYER MOBJ!"); return; }
  if (!Player->MO->Sector) { GCon->Log("PLAYER MOBJ SECTOR IS UNKNOWN!"); return; }
  //if (!Player || !Player->MO || !Player->MO->Sector) return;
  sector_t *sec = Player->MO->Sector;
  GCon->Logf("Sector #%d; tag=%d; special=%d; damage=%d; seqtype=%d; sndtrav=%d; sky=%d",
    (int)(intptr_t)(sec-Player->Level->XLevel->Sectors),
    sec->tag, sec->special, sec->seqType, sec->soundtraversed, sec->Damage, sec->Sky
  );
  GCon->Logf("  floor texture  : %s", *GTextureManager.GetTextureName(sec->floor.pic));
  GCon->Logf("  ceiling texture: %s", *GTextureManager.GetTextureName(sec->ceiling.pic));
}


//==========================================================================
//
//  Jumper
//
//==========================================================================
COMMAND(Jumper) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Jumper();
}


//==========================================================================
//
//  ShooterKing
//
//==========================================================================
COMMAND(ShooterKing) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_ShooterKing();
}


//==========================================================================
//
//  Regeneration
//
//==========================================================================
COMMAND(Regeneration) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  if (CheatAllowed(Player)) Player->eventCheat_Regeneration();
}


//==========================================================================
//
//  Dump_Inventory
//
//==========================================================================
COMMAND(Dump_Inventory) {
  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }
  Player->eventCheat_DumpInventory();
}


//==========================================================================
//
//  COMMAND VScript_Command
//
//==========================================================================
COMMAND(VScript_Command) {
  if (Args.Num() < 2) return;

  if (Source == SRC_Command) {
    ForwardToServer();
    return;
  }

  if (!Player) return;
  if (CheatAllowed(Player)) {
    Args.removeAt(0); // remove command name
    Player->eventCheat_VScriptCommand(Args);
  }
}
