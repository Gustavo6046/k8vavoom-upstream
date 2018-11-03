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
//**  EVENT HANDLING
//**
//**  Events are asynchronous inputs generally generated by the game user.
//**  Events can be discarded if no responder claims them
//**
//**************************************************************************
#include "gamedefs.h"
#include "cl_local.h"
#include "ui/ui.h"


static VCvarB allow_vanilla_cheats("allow_vanilla_cheats", true, "Allow vanilla keyboard cheat codes?", CVAR_Archive);


// input subsystem, handles all input events
class VInput : public VInputPublic {
public:
  VInput ();
  virtual ~VInput () override;

  // system device related functions
  virtual void Init () override;
  virtual void Shutdown () override;

  // input event handling
  virtual void PostEvent (event_t *) override;
  virtual void KeyEvent (int, int) override;
  virtual void ProcessEvents () override;
  virtual int ReadKey () override;

  // handling of key bindings
  virtual void GetBindingKeys (const VStr &, int &, int &) override;
  virtual void GetBinding (int, VStr &, VStr &) override;
  virtual void SetBinding (int, const VStr &, const VStr &, bool) override;
  virtual void WriteBindings (FILE *) override;

  virtual int TranslateKey (int) override;

  virtual int KeyNumForName (const VStr &Name) override;
  virtual VStr KeyNameForNum( int KeyNr) override;

  virtual void RegrabMouse () override; // called by UI when mouse cursor is turned off

private:
  enum { MAXEVENTS = 512 };

  VInputDevice *Device;

  event_t Events[MAXEVENTS];
  int EventHead;
  int EventTail;

  VStr KeyBindingsDown[256];
  VStr KeyBindingsUp[256];
  bool KeyBindingsSave[256];

  static const char ShiftXForm[];
};

VInputPublic *GInput;


// key shifting
const char VInput::ShiftXForm[] = {
  0,
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31,
  ' ', '!', '"', '#', '$', '%', '&',
  '"', // shift-'
  '(', ')', '*', '+',
  '<', // shift-,
  '_', // shift--
  '>', // shift-.
  '?', // shift-/
  ')', // shift-0
  '!', // shift-1
  '@', // shift-2
  '#', // shift-3
  '$', // shift-4
  '%', // shift-5
  '^', // shift-6
  '&', // shift-7
  '*', // shift-8
  '(', // shift-9
  ':',
  ':', // shift-;
  '<',
  '+', // shift-=
  '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', // shift-[
  '|', // shift-backslash - OH MY GOD DOES WATCOM SUCK
  '}', // shift-]
  '"', '_',
  '\'', // shift-`
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', '|', '}', '~', 127
};


TArray<VInputPublic::CheatCode> VInputPublic::kbcheats;
char VInputPublic::currkbcheat[VInputPublic::MAX_KBCHEAT_LENGTH+1];


//==========================================================================
//
//  VInputPublic::Create
//
//==========================================================================
VInputPublic *VInputPublic::Create () {
  return new VInput();
}


//==========================================================================
//
//  VInputPublic::KBCheatClearAll
//
//==========================================================================
void VInputPublic::KBCheatClearAll () {
  kbcheats.clear();
}


//==========================================================================
//
//  VInputPublic::KBCheatAppend
//
//==========================================================================
void VInputPublic::KBCheatAppend (VStr keys, VStr concmd) {
  if (keys.length() == 0) return;
  for (int f = 0; f < kbcheats.length(); ++f) {
    if (kbcheats[f].keys.ICmp(keys) == 0) {
      if (concmd.length() == 0) {
        kbcheats.removeAt(f);
      } else {
        kbcheats[f].concmd = concmd;
      }
      return;
    }
  }
  // reset cheat
  currkbcheat[0] = 0;
  CheatCode &cc = kbcheats.alloc();
  cc.keys = keys;
  cc.concmd = concmd;
  //GCon->Logf("added cheat code '%s' (command: '%s')", *keys, *concmd);
}


//==========================================================================
//
//  VInputPublic::KBCheatProcessor
//
//==========================================================================
bool VInputPublic::KBCheatProcessor (event_t *ev) {
  if (!allow_vanilla_cheats) { currkbcheat[0] = 0; return false; }
  if (ev->type != ev_keydown) return false;
  if (ev->data1 < ' ' || ev->data1 >= 127) { currkbcheat[0] = 0; return false; }
  int slen = (int)strlen(currkbcheat);
  if (slen >= MAX_KBCHEAT_LENGTH) { currkbcheat[0] = 0; return false; }
  currkbcheat[slen] = (char)ev->data1;
  currkbcheat[slen+1] = 0;
  ++slen;
  int clen = kbcheats.length();
  //GCon->Logf("C:<%s> (clen=%d)", currkbcheat, clen);
  char digits[MAX_KBCHEAT_LENGTH+1];
  for (int f = 0; f <= MAX_KBCHEAT_LENGTH; ++f) digits[f] = '0';
  int digcount = 0;
  bool wasCheat = false;
  for (int cnum = 0; cnum < clen; ++cnum) {
    CheatCode &cc = kbcheats[cnum];
    //GCon->Logf("  check00:<%s> (with <%s>)", *cc.keys, currkbcheat);
    if (cc.keys.length() < slen) continue; // too short
    bool ok = true;
    for (int f = 0; f < slen; ++f) {
      char c1 = currkbcheat[f];
      if (c1 >= 'A' && c1 <= 'Z') c1 = c1-'A'+'a';
      char c2 = cc.keys[f];
      if (c2 >= 'A' && c2 <= 'Z') c2 = c2-'A'+'a';
      if (c1 == c2) continue;
      if (c2 == '#' && (c1 >= '0' && c1 <= '9')) {
        digits[digcount++] = c1;
        continue;
      }
      ok = false;
      break;
    }
    //GCon->Logf("  check01:<%s> (with <%s>): ok=%d", *cc.keys, currkbcheat, (ok ? 1 : 0));
    if (!ok) continue;
    if (cc.keys.length() == slen) {
      // cheat complete
      VStr concmd;
      if (digcount > 0) {
        // preprocess console command
        int dignum = 0;
        for (int f = 0; f < cc.concmd.length(); ++f) {
          if (cc.concmd[f] == '#') {
            if (dignum < digcount) {
              concmd += digits[dignum++];
            } else {
              concmd += '0';
            }
          } else {
            concmd += cc.concmd[f];
          }
        }
      } else {
        concmd = cc.concmd;
      }
      if (concmd.length()) {
        if (concmd[concmd.length()-1] != '\n') concmd += "\n";
        GCmdBuf << concmd;
      }
      // reset cheat
      currkbcheat[0] = 0;
      return true;
    }
    wasCheat = true;
  }
  // nothing was found, reset
  if (!wasCheat) currkbcheat[0] = 0;
  return false;
}


//==========================================================================
//
//  VInput::VInput
//
//==========================================================================
VInput::VInput()
  : Device(0)
  , EventHead(0)
  , EventTail(0)
{
  memset(KeyBindingsSave, 0, sizeof(KeyBindingsSave));
}


//==========================================================================
//
//  VInput::~VInput
//
//==========================================================================
VInput::~VInput () {
  Shutdown();
}


//==========================================================================
//
//  VInput::Init
//
//==========================================================================
void VInput::Init () {
  guard(VInput::Init);
  Device = VInputDevice::CreateDevice();
  unguard;
}


//==========================================================================
//
//  VInput::Shutdown
//
//==========================================================================
void VInput::Shutdown () {
  guard(VInput::Shutdown);
  if (Device) {
    delete Device;
    Device = nullptr;
  }
  unguard;
}


//==========================================================================
//
//  VInput::PostEvent
//
//  Called by the I/O functions when input is detected
//
//==========================================================================
void VInput::PostEvent (event_t *ev) {
  guard(VInput::PostEvent);
  Events[EventHead] = *ev;
  EventHead = (EventHead+1)&(MAXEVENTS-1);
  unguard;
}


//==========================================================================
//
//  VInput::KeyEvent
//
//  Called by the I/O functions when a key or button is pressed or released
//
//==========================================================================
void VInput::KeyEvent (int key, int press) {
  guard(VInput::KeyEvent);
  if (!key) return;
  Events[EventHead].type = (press ? ev_keydown : ev_keyup);
  Events[EventHead].data1 = key;
  Events[EventHead].data2 = 0;
  Events[EventHead].data3 = 0;
  EventHead = (EventHead+1)&(MAXEVENTS-1);
  unguard;
}


//==========================================================================
//
//  VInput::ProcessEvents
//
//  Send all the Events of the given timestamp down the responder chain
//
//==========================================================================
void VInput::ProcessEvents () {
  guard(VInput::ProcessEvents);
  Device->ReadInput();
  for (; EventTail != EventHead; EventTail = (EventTail+1)&(MAXEVENTS-1)) {
    event_t *ev = &Events[EventTail];
    // shift key state
    if (ev->data1 == K_RSHIFT) {
      ShiftDown &= ~1;
      if (ev->type == ev_keydown) ShiftDown |= 1;
    }
    if (ev->data1 == K_LSHIFT) {
      ShiftDown &= ~2;
      if (ev->type == ev_keydown) ShiftDown |= 2;
    }
    // ctrl key state
    if (ev->data1 == K_RCTRL) {
      CtrlDown &= ~1;
      if (ev->type == ev_keydown) CtrlDown |= 1;
    }
    if (ev->data1 == K_LCTRL) {
      CtrlDown &= ~2;
      if (ev->type == ev_keydown) CtrlDown |= 2;;
    }
    // alt key state
    if (ev->data1 == K_RALT) {
      AltDown &= ~1;
      if (ev->type == ev_keydown) AltDown |= 1;
    }
    if (ev->data1 == K_LALT) {
      AltDown &= ~2;
      if (ev->type == ev_keydown) AltDown |= 2;;
    }

    if (C_Responder(ev)) continue; // console
    if (CT_Responder(ev)) continue; // chat
    if (MN_Responder(ev)) continue; // menu
    if (GRoot->Responder(ev)) continue; // root widget

    if (cl && !GClGame->intermission) {
      if (KBCheatProcessor(ev)) continue; // cheatcode typed
      if (SB_Responder(ev)) continue; // status window ate it
      if (AM_Responder(ev)) continue; // automap ate it
    }

    if (F_Responder(ev)) continue; // finale

    // key bindings
    if (ev->type == ev_keydown) {
      VStr kb = KeyBindingsDown[ev->data1];
      if (kb.IsNotEmpty()) {
        if (kb[0] == '+' || kb[0] == '-') {
          // button commands add keynum as a parm
          GCmdBuf << kb << " " << VStr(ev->data1) << "\n";
        } else {
          GCmdBuf << kb << "\n";
        }
        continue;
      }
    }
    if (ev->type == ev_keyup) {
      VStr kb = KeyBindingsUp[ev->data1];
      if (kb.IsNotEmpty()) {
        if (kb[0] == '+' || kb[0] == '-') {
          // button commands add keynum as a parm
          GCmdBuf << kb << " " << VStr(ev->data1) << "\n";
        } else {
          GCmdBuf << kb << "\n";
        }
        continue;
      }
    }
    if (CL_Responder(ev)) continue;
  }
  unguard;
}


//==========================================================================
//
//  VInput::ReadKey
//
//==========================================================================
int VInput::ReadKey () {
  guard(VInput::ReadKey);
  int ret = 0;
  do {
    Device->ReadInput();
    while (EventTail != EventHead && !ret) {
      if (Events[EventTail].type == ev_keydown) ret = Events[EventTail].data1;
      EventTail = (EventTail+1)&(MAXEVENTS-1);
    }
  } while (!ret);
  return ret;
  unguard;
}


//==========================================================================
//
//  VInput::GetBindingKeys
//
//==========================================================================
void VInput::GetBindingKeys (const VStr &Binding, int &Key1, int &Key2) {
  guard(VInput::GetBindingKeys);
  Key1 = -1;
  Key2 = -1;
  for (int i = 0; i < 256; ++i) {
    if (!Binding.ICmp(KeyBindingsDown[i])) {
      if (Key1 != -1) {
        Key2 = i;
        return;
      }
      Key1 = i;
    }
  }
  unguard;
}


//==========================================================================
//
//  VInput::GetBinding
//
//==========================================================================
void VInput::GetBinding (int KeyNum, VStr &Down, VStr &Up) {
  guard(VInput::GetBinding);
  Down = KeyBindingsDown[KeyNum];
  Up = KeyBindingsUp[KeyNum];
  unguard;
}


//==========================================================================
//
//  VInput::SetBinding
//
//==========================================================================
void VInput::SetBinding (int KeyNum, const VStr &Down, const VStr &Up, bool Save) {
  guard(VInput::SetBinding);
  if (KeyNum == -1) return;
  if (Down.IsEmpty() && Up.IsEmpty() && !KeyBindingsSave[KeyNum]) return;
  KeyBindingsDown[KeyNum] = Down;
  KeyBindingsUp[KeyNum] = Up;
  KeyBindingsSave[KeyNum] = Save;
  unguard;
}


//==========================================================================
//
//  VInput::WriteBindings
//
//  Writes lines containing "bind key value"
//
//==========================================================================
void VInput::WriteBindings (FILE *f) {
  guard(VInput::WriteBindings);
  fprintf(f, "UnbindAll\n");
  for (int i = 0; i < 256; ++i) {
    if ((KeyBindingsDown[i].IsNotEmpty() || KeyBindingsUp[i].IsNotEmpty()) && KeyBindingsSave[i]) {
      fprintf(f, "bind \"%s\" \"%s\" \"%s\"\n", *KeyNameForNum(i).quote(), *KeyBindingsDown[i].quote(), *KeyBindingsUp[i].quote());
    }
  }
  unguard;
}


//==========================================================================
//
//  VInput::TranslateKey
//
//==========================================================================
int VInput::TranslateKey (int ch) {
  guard(VInput::TranslateKey);
  int Tmp = ch;
  return (ShiftDown ? ShiftXForm[Tmp] : Tmp);
  unguard;
}


//==========================================================================
//
//  VInput::KeyNumForName
//
//  Searches in key names for given name
// return key code
//
//==========================================================================
int VInput::KeyNumForName (const VStr &Name) {
  guard(VInput::KeyNumForName);
  if (Name.IsEmpty()) return -1;
  int res = VObject::VKeyFromName(Name);
  if (res <= 0) res = -1;
  return res;
  unguard;
}


//==========================================================================
//
//  VInput::KeyNameForNum
//
//  Writes into given string key name
//
//==========================================================================
VStr VInput::KeyNameForNum (int KeyNr) {
  guard(VInput::KeyNameForNum);
       if (KeyNr == ' ') return "SPACE";
  else if (KeyNr == K_ESCAPE) return VStr("ESCAPE");
  else if (KeyNr == K_ENTER) return VStr("ENTER");
  else if (KeyNr == K_TAB) return VStr("TAB");
  else if (KeyNr == K_BACKSPACE) return VStr("BACKSPACE");
  else return VObject::NameFromVKey(KeyNr);
  unguard;
}


//==========================================================================
//
//  VInput::RegrabMouse
//
//  Called by UI when mouse cursor is turned off
//
//==========================================================================
void VInput::RegrabMouse () {
  if (Device) Device->RegrabMouse();
}


//==========================================================================
//
//  COMMAND Unbind
//
//==========================================================================
COMMAND(Unbind) {
  guard(COMMAND Unbind);
  if (Args.Num() != 2) {
    GCon->Log("unbind <key> : remove commands from a key");
    return;
  }

  int b = GInput->KeyNumForName(Args[1]);
  if (b == -1) {
    GCon->Log(VStr("\"")+Args[1]+"\" isn't a valid key");
    return;
  }

  GInput->SetBinding(b, VStr(), VStr());
  unguard;
}


//==========================================================================
//
//  COMMAND UnbindAll
//
//==========================================================================
COMMAND(UnbindAll) {
  guard(COMMAND UnbindAll);
  for (int i = 0; i < 256; ++i) GInput->SetBinding(i, VStr(), VStr());
  unguard;
}


//==========================================================================
//
//  COMMAND Bind
//
//==========================================================================
COMMAND(Bind) {
  guard(COMMAND Bind);
  int c = Args.length();

  if (c != 2 && c != 3 && c != 4) {
    GCon->Log("bind <key> [down_command] [up_command]: attach a command to a key");
    return;
  }

  if (Args[1].length() == 0) return;
  int b = GInput->KeyNumForName(Args[1]);
  if (b == -1) {
    GCon->Log(VStr("\"")+Args[1]+"\" isn't a valid key");
    return;
  }

  if (c == 2) {
    VStr Down, Up;
    GInput->GetBinding(b, Down, Up);
    if (Down.IsNotEmpty() || Up.IsNotEmpty()) {
      GCon->Log(Args[1] + " = \"" + Down + "\" / \"" + Up + "\"");
    } else {
      GCon->Logf("'%s' is not bound", *Args[1]);
    }
    return;
  }
  GInput->SetBinding(b, Args[2], c > 3 ? Args[3] : VStr(), !ParsingKeyConf);
  unguard;
}


//==========================================================================
//
//  COMMAND DefaultBind
//
//==========================================================================
COMMAND(DefaultBind) {
  guard(COMMAND DefaultBind);
  int c = Args.length();

  if (c != 2 && c != 3 && c != 4) {
    GCon->Log("defaultbind <key> [down_command] [up_command]: attach a command to a key");
    return;
  }

  if (Args[1].length() == 0) return;
  int b = GInput->KeyNumForName(Args[1]);
  if (b == -1) {
    GCon->Log(VStr("\"")+Args[1]+"\" isn't a valid key");
    return;
  }

  if (c == 2) {
    VStr Down, Up;
    GInput->GetBinding(b, Down, Up);
    if (Down.IsNotEmpty() || Up.IsNotEmpty()) {
      GCon->Log(Args[1] + " = \"" + Down + "\" / \"" + Up + "\"");
    } else {
      GCon->Logf("%s is not bound", *Args[1]);
    }
    return;
  }
  GInput->SetBinding(b, Args[2], c > 3 ? Args[3] : VStr(), !ParsingKeyConf);
  unguard;
}
