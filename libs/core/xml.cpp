//**************************************************************************
//**
//**    ##   ##    ##    ##   ##   ####     ####   ###     ###
//**    ##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**     ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**     ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**      ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**       #    ##    ##    #      ####     ####   ##       ##
//**
//**  Copyright (C) 1999-2010 Jānis Legzdiņš
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


//==========================================================================
//
//  VXmlNode::VXmlNode
//
//==========================================================================
VXmlNode::VXmlNode ()
  : Parent(nullptr)
  , FirstChild(nullptr)
  , LastChild(nullptr)
  , PrevSibling(nullptr)
  , NextSibling(nullptr)
{
}


//==========================================================================
//
//  VXmlNode::~VXmlNode
//
//==========================================================================
VXmlNode::~VXmlNode () {
  while (FirstChild) {
    VXmlNode *Temp = FirstChild;
    FirstChild = FirstChild->NextSibling;
    delete Temp;
  }
}


//==========================================================================
//
//  VXmlNode::FindChild
//
//==========================================================================
VXmlNode *VXmlNode::FindChild (const char *AName) const {
  for (VXmlNode *N = FirstChild; N; N = N->NextSibling) {
    if (N->Name == AName) return N;
  }
  return nullptr;
}


//==========================================================================
//
//  VXmlNode::FindChild
//
//==========================================================================
VXmlNode *VXmlNode::FindChild (VStr AName) const {
  for (VXmlNode *N = FirstChild; N; N = N->NextSibling) {
    if (N->Name == AName) return N;
  }
  return nullptr;
}


//==========================================================================
//
//  VXmlNode::GetChild
//
//==========================================================================
VXmlNode *VXmlNode::GetChild (const char *AName) const {
  VXmlNode *N = FindChild(AName);
  if (!N) Sys_Error("XML node '%s' not found", AName);
  return N;
}


//==========================================================================
//
//  VXmlNode::GetChild
//
//==========================================================================
VXmlNode *VXmlNode::GetChild (VStr AName) const {
  VXmlNode *N = FindChild(AName);
  if (!N) Sys_Error("XML node '%s' not found", *AName);
  return N;
}


//==========================================================================
//
//  VXmlNode::FindNext
//
//==========================================================================
VXmlNode *VXmlNode::FindNext (const char *AName) const {
  for (VXmlNode *N = NextSibling; N; N = N->NextSibling) {
    if (N->Name == AName) return N;
  }
  return nullptr;
}


//==========================================================================
//
//  VXmlNode::FindNext
//
//==========================================================================
VXmlNode *VXmlNode::FindNext (VStr AName) const {
  for (VXmlNode *N = NextSibling; N; N = N->NextSibling) {
    if (N->Name == AName) return N;
  }
  return nullptr;
}


//==========================================================================
//
//  VXmlNode::FindNext
//
//==========================================================================
VXmlNode *VXmlNode::FindNext () const {
  for (VXmlNode *N = NextSibling; N; N = N->NextSibling) {
    if (N->Name == Name) return N;
  }
  return nullptr;
}

//==========================================================================
//
//  VXmlNode::HasAttribute
//
//==========================================================================
bool VXmlNode::HasAttribute(const char *AttrName) const {
  for (int i = 0; i < Attributes.length(); ++i) {
    if (Attributes[i].Name == AttrName) return true;
  }
  return false;
}


//==========================================================================
//
//  VXmlNode::FindChild
//
//==========================================================================
bool VXmlNode::HasAttribute (VStr AttrName) const {
  for (int i = 0; i < Attributes.length(); ++i) {
    if (Attributes[i].Name == AttrName) return true;
  }
  return false;
}


//==========================================================================
//
//  VXmlNode::GetAttribute
//
//==========================================================================
VStr VXmlNode::GetAttribute (const char *AttrName, bool Required) const {
  for (int i = 0; i < Attributes.length(); ++i) {
    if (Attributes[i].Name == AttrName) return Attributes[i].Value;
  }
  if (Required) {
    if (Attributes.length()) {
      GLog.Logf("=== node \"%s\" attrs ===", *Name);
      for (auto &&a : Attributes) GLog.Logf("  \"%s\"=\"%s\"", *a.Name, *a.Value);
    }
    Sys_Error("XML attribute \"%s\" not found in node \"%s\"", AttrName, *Name);
  }
  return VStr::EmptyString;
}


//==========================================================================
//
//  VXmlNode::GetAttribute
//
//==========================================================================
VStr VXmlNode::GetAttribute (VStr AttrName, bool Required) const {
  for (int i = 0; i < Attributes.length(); ++i) {
    if (Attributes[i].Name == AttrName) return Attributes[i].Value;
  }
  if (Required) {
    if (Attributes.length()) {
      GLog.Logf("=== node \"%s\" attrs ===", *Name);
      for (auto &&a : Attributes) GLog.Logf("  \"%s\"=\"%s\"", *a.Name, *a.Value);
    }
    Sys_Error("XML attribute \"%s\" not found in node \"%s\"", *AttrName, *Name);
  }
  return VStr::EmptyString;
}


//==========================================================================
//
//  VXmlDocument::Parse
//
//==========================================================================
void VXmlDocument::Parse (VStream &Strm, VStr AName) {
  Name = AName;
  Encoding = UTF8;

  Buf = new char[Strm.TotalSize()+1];
  Strm.Seek(0);
  Strm.Serialise(Buf, Strm.TotalSize());
  Buf[Strm.TotalSize()] = 0;
  CurPos = 0;
  EndPos = Strm.TotalSize();

  // skip garbage some editors add in the begining of UTF-8 files
  if ((vuint8)Buf[0] == 0xef && (vuint8)Buf[1] == 0xbb && (vuint8)Buf[2] == 0xbf) CurPos += 3;

  do { SkipWhitespace(); } while (SkipComment());

  if (CurPos >= EndPos) Error("Empty document");

  if (!(Buf[CurPos] == '<' && Buf[CurPos+1] == '?' && Buf[CurPos+2] == 'x' &&
        Buf[CurPos+3] == 'm' && Buf[CurPos+4] == 'l' && Buf[CurPos+5] > 0 && Buf[CurPos+5] <= ' '))
  {
    Error("XML declaration expected");
  }
  CurPos += 5;
  SkipWhitespace();

  VStr AttrName;
  VStr AttrValue;
  if (!ParseAttribute(AttrName, AttrValue)) Error("XML version expected");
  if (AttrName != "version") Error("XML version expected");
  if (AttrValue != "1.0" && AttrValue != "1.1") Error("Bad XML version");

  SkipWhitespace();
  while (ParseAttribute(AttrName, AttrValue)) {
    if (AttrName == "encoding") {
      VStr ec = AttrValue;
           if (ec.ICmp("UTF-8") == 0) Encoding = UTF8;
      else if (ec.ICmp("WINDOWS-1251") == 0) Encoding = WIN1251;
      else if (ec.ICmp("WINDOWS1251") == 0) Encoding = WIN1251;
      else if (ec.ICmp("CP-1251") == 0) Encoding = WIN1251;
      else if (ec.ICmp("CP1251") == 0) Encoding = WIN1251;
      else if (ec.ICmp("KOI-8") == 0) Encoding = KOI8;
      else if (ec.ICmp("KOI-8U") == 0) Encoding = KOI8;
      else if (ec.ICmp("KOI-8R") == 0) Encoding = KOI8;
      else if (ec.ICmp("KOI8") == 0) Encoding = KOI8;
      else if (ec.ICmp("KOI8U") == 0) Encoding = KOI8;
      else if (ec.ICmp("KOI8R") == 0) Encoding = KOI8;
      else Error(va("Unknown XML encoding '%s'", *ec));
    } else if (AttrName == "standalone") {
      if (AttrValue.ICmp("yes") != 0) Error("Only standalone is supported");
    } else {
      Error(va("Unknown attribute '%s'", *AttrName));
    }
    SkipWhitespace();
  }

  if (Buf[CurPos] != '?' || Buf[CurPos+1] != '>') Error("Bad syntax");
  CurPos += 2;

  do { SkipWhitespace(); } while (SkipComment());

  if (Buf[CurPos] != '<') Error("Root node expected");
  ParseNode(&Root);

  do { SkipWhitespace(); } while (SkipComment());

  if (CurPos != EndPos) Error("Text after root node");

  delete[] Buf;
  Buf = nullptr;
}


//==========================================================================
//
//  VXmlDocument::SkipWhitespace
//
//==========================================================================
void VXmlDocument::SkipWhitespace () {
  while (Buf[CurPos] > 0 && (vuint8)Buf[CurPos] <= ' ') ++CurPos;
}


//==========================================================================
//
//  VXmlDocument::SkipComment
//
//==========================================================================
bool VXmlDocument::SkipComment () {
  if (Buf[CurPos] == '<' && Buf[CurPos+1] == '!' &&
      Buf[CurPos+2] == '-' && Buf[CurPos+3] == '-')
  {
    // skip comment
    CurPos += 4;
    while (CurPos < EndPos-2 && (Buf[CurPos] != '-' || Buf[CurPos+1] != '-' || Buf[CurPos+2] != '>')) ++CurPos;
    if (CurPos >= EndPos-2) Error("Unterminated comment");
    CurPos += 3;
    return true;
  }
  return false;
}


//==========================================================================
//
//  VXmlDocument::Error
//
//==========================================================================
void VXmlDocument::Error (const char *Msg) {
  Sys_Error("%s: %s", *Name, Msg);
}


//==========================================================================
//
//  VXmlDocument::ParseName
//
//==========================================================================
VStr VXmlDocument::ParseName () {
  VStr Ret;
  char c = Buf[CurPos];
  if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-' || c == ':')) return VStr();

  do {
    Ret += c;
    CurPos++;
    c = Buf[CurPos];
  } while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ':' || c == '.');

  return Ret;
}


//==========================================================================
//
//  VXmlDocument::GetChar
//
//  with correct encoding
//
//==========================================================================
vuint32 VXmlDocument::GetChar () {
  if (CurPos >= EndPos) Error("unexpected EOF");
  char ch = Buf[CurPos++];
  if (ch == '\r' && Buf[CurPos] == '\n') { ch = '\n'; ++CurPos; }
  if (!ch) ch = ' ';
  if ((vuint8)ch < 128) return (vuint32)(ch&0xff);
  switch (Encoding) {
    case WIN1251: return VStr::cp1251[((vuint8)ch)-128];
    case KOI8: return VStr::cpKOI[((vuint8)ch)-128];
    case UTF8:
      {
        VUtf8DecoderFast dc;
        if (!dc.put(ch)) {
          while (CurPos < EndPos) {
            if (dc.put(Buf[CurPos++])) {
              if (dc.invalid()) Error("invalid UTF-8 char");
              return dc.codepoint;
            }
          }
        }
        Error("invalid UTF-8 char");
      }
  }
  Error("WUT?!");
  return 0;
}


//==========================================================================
//
//  VXmlDocument::ParseAttrValue
//
//==========================================================================
VStr VXmlDocument::ParseAttrValue (char EndChar) {
  VStr Ret;
  while (CurPos < EndPos && Buf[CurPos] != EndChar) Ret.utf8Append(GetChar());
  if (CurPos >= EndPos) Error("Unterminated attribute value");
  ++CurPos;
  return HandleReferences(Ret);
}


//==========================================================================
//
//  VXmlDocument::ParseAttribute
//
//==========================================================================
bool VXmlDocument::ParseAttribute (VStr &AttrName, VStr &AttrValue) {
  AttrName = ParseName();
  if (AttrName.IsEmpty()) return false;
  SkipWhitespace();
  if (Buf[CurPos] != '=') Error("Attribute value expected");
  ++CurPos;
  SkipWhitespace();
  if (CurPos >= EndPos) Error("unexpected end of document");
  if (Buf[CurPos] == '\"' || Buf[CurPos] == '\'') {
    ++CurPos;
    AttrValue = ParseAttrValue(Buf[CurPos-1]);
  } else {
    Error("Unquoted attribute value");
  }
  return true;
}


//==========================================================================
//
//  VXmlDocument::ParseNode
//
//==========================================================================
void VXmlDocument::ParseNode (VXmlNode *Node) {
  if (Buf[CurPos] != '<') Error("Bad tag start");
  ++CurPos;
  Node->Name = ParseName();
  if (Node->Name.IsEmpty()) Error("Bad or missing tag name");

  SkipWhitespace();
  VStr AttrName;
  VStr AttrValue;
  while (ParseAttribute(AttrName, AttrValue)) {
    VXmlAttribute &A = Node->Attributes.Alloc();
    A.Name = AttrName;
    A.Value = AttrValue;
    SkipWhitespace();
  }

  if (Buf[CurPos] == '/' && Buf[CurPos+1] == '>') {
    CurPos += 2;
  } else if (Buf[CurPos] == '>') {
    ++CurPos;
    while (CurPos < EndPos && (Buf[CurPos] != '<' || Buf[CurPos+1] != '/')) {
      if (Buf[CurPos] == '<') {
        if (Buf[CurPos+1] == '!' && Buf[CurPos+2] == '-' && Buf[CurPos+3] == '-') {
          SkipComment();
        } else if (Buf[CurPos+1] == '!' && Buf[CurPos+2] == '[' &&
                   Buf[CurPos+3] == 'C' && Buf[CurPos+4] == 'D' &&
                   Buf[CurPos+5] == 'A' && Buf[CurPos+6] == 'T' &&
                   Buf[CurPos+7] == 'A' && Buf[CurPos+8] == '[')
        {
          CurPos += 9;
          VStr TextVal;
          while (CurPos < EndPos && (Buf[CurPos] != ']' || Buf[CurPos+1] != ']' || Buf[CurPos+2] != '>')) {
            TextVal.utf8Append(GetChar());
          }
          if (CurPos >= EndPos) Error("Unexpected end of file in CDATA");
          Node->Value += TextVal;
          CurPos += 3;
        } else {
          VXmlNode *NewChild = new VXmlNode();
          NewChild->PrevSibling = Node->LastChild;
          if (Node->LastChild) {
            Node->LastChild->NextSibling = NewChild;
          } else {
            Node->FirstChild = NewChild;
          }
          Node->LastChild = NewChild;
          NewChild->Parent = Node;
          ParseNode(NewChild);
        }
      } else {
        VStr TextVal;
        bool HasNonWhitespace = false;
        while (CurPos < EndPos && Buf[CurPos] != '<') {
          if (!HasNonWhitespace && (vuint8)Buf[CurPos] > ' ') HasNonWhitespace = true;
          TextVal.utf8Append(GetChar());
        }
        if (HasNonWhitespace) Node->Value += HandleReferences(TextVal);
      }
    }
    if (CurPos >= EndPos) Error("Unexpected end of file");
    CurPos += 2;
    VStr Test = ParseName();
    if (Node->Name != Test) Error("Wrong end tag");
    if (Buf[CurPos] != '>') Error("Tag not closed");
    ++CurPos;
  } else {
    Error("Tag is not closed");
  }
}


//==========================================================================
//
//  VXmlDocument::HandleReferences
//
//==========================================================================
VStr VXmlDocument::HandleReferences (VStr AStr) {
  VStr Ret = AStr;
  for (int i = 0; i < Ret.length(); ++i) {
    if (Ret[i] == '&') {
      int EndPos = i+1;
      while (EndPos < Ret.length() && Ret[EndPos] != ';') ++EndPos;
      if (EndPos >= Ret.length()) Error("Unterminated character or entity reference");
      ++EndPos;
      VStr Seq = VStr(Ret, i, EndPos-i);
      VStr NewVal;
      if (Seq.length() > 4 && Seq[1] == '#' && Seq[2] == 'x') {
        int Val = 0;
        for (int j = 3; j < Seq.length()-1; ++j) {
          int dg = VStr::digitInBase(Seq[j], 16);
          if (dg < 0) Error("Bad character reference");
          Val = (Val<<4)|dg;
        }
        NewVal = VStr::FromUtf8Char(Val);
      } else if (Seq.length() > 3 && Seq[1] == '#') {
        int Val = 0;
        for (int j = 2; j < Seq.length()-1; ++j) {
          int dg = VStr::digitInBase(Seq[j], 10);
          if (dg < 0) Error("Bad character reference");
          Val = (Val*10)+dg;
        }
        NewVal = VStr::FromUtf8Char(Val);
      } else if (Seq == "&amp;") NewVal = "&";
        else if (Seq == "&quot;") NewVal = "\"";
        else if (Seq == "&apos;") NewVal = "\'";
        else if (Seq == "&lt;") NewVal = "<";
        else if (Seq == "&gt;") NewVal = ">";
        else Error(va("Unknown entity reference '%s'", *Seq));
      Ret = VStr(Ret, 0, i)+NewVal+VStr(Ret, EndPos, Ret.length()-EndPos);
      i += NewVal.length()-1;
    }
  }
  return Ret;
}
