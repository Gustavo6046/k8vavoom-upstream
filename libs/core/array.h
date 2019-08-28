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
//**  Copyright (C) 2018-2019 Ketmar Dark
//**
//**  This program is free software: you can redistribute it and/or modify
//**  it under the terms of the GNU General Public License as published by
//**  the Free Software Foundation, either version 3 of the License, or
//**  (at your option) any later version.
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
//**
//**  Dynamic array template.
//**
//**************************************************************************
//#define VAVOOM_CORELIB_ARRAY_MINIMAL_RESIZE

enum EArrayNew { E_ArrayNew };

inline void *operator new (size_t, void *ptr, EArrayNew, EArrayNew) { return ptr; }


template<class T> class TArray {
private:
  // 2d info is from `VScriptArray`
  int ArrNum; // if bit 31 is set, this is 1st dim of 2d array
  int ArrSize; // if bit 31 is set in `ArrNum`, this is 2nd dim of 2d array
  T *ArrData;

public:
  TArray () : ArrNum(0), ArrSize(0), ArrData(nullptr) {}
  TArray (ENoInit) {}
  TArray (const TArray<T> &other) : ArrNum(0), ArrSize(0), ArrData(nullptr) { *this = other; }

  ~TArray () { clear(); }

  inline int length1D () const { return (ArrNum >= 0 ? ArrNum : (ArrNum&0x7fffffff)*(ArrSize&0x7fffffff)); }

  // this is from `VScriptArray`
  inline bool Is2D () const { return (ArrNum < 0); }
  inline void Flatten () { if (Is2D()) { int oldlen = length1D(); ArrSize = ArrNum = oldlen; } }

  inline void Clear () { clear(); }

  inline void clear () {
    if (ArrData) {
      Flatten(); // just in case
      for (int i = 0; i < ArrNum; ++i) ArrData[i].~T();
      Z_Free(ArrData);
    }
    ArrData = nullptr;
    ArrNum = ArrSize = 0;
  }

  // don't free array itself
  inline void reset () {
    Flatten(); // just in case
    for (int f = 0; f < ArrNum; ++f) ArrData[f].~T();
    ArrNum = 0;
  }

  // don't free array itself
  inline void resetNoDtor () {
    Flatten(); // just in case
    ArrNum = 0;
  }

  inline int Num () const { return ArrNum; }
  inline int Length () const { return ArrNum; }
  inline int length () const { return ArrNum; }

  inline int NumAllocated () const { return ArrSize; }
  inline int numAllocated () const { return ArrSize; }
  inline int capacity () const { return ArrSize; }

  // don't do any sanity checks here, `ptr()` can be used even on empty arrays
  inline T *Ptr () { return ArrData; }
  inline const T *Ptr () const { return ArrData; }

  inline T *ptr () { return ArrData; }
  inline const T *ptr () const { return ArrData; }

  inline void SetPointerData (void *adata, int datalen) {
    vassert(datalen >= 0);
    if (datalen == 0) {
      if (ArrData && adata) {
        Flatten();
        vassert((uintptr_t)adata < (uintptr_t)ArrData || (uintptr_t)adata >= (uintptr_t)(ArrData+ArrSize));
      }
      clear();
      if (adata) Z_Free(adata);
    } else {
      vassert(adata);
      vassert(datalen%(int)sizeof(T) == 0);
      if (ArrData) {
        Flatten();
        vassert((uintptr_t)adata < (uintptr_t)ArrData || (uintptr_t)adata >= (uintptr_t)(ArrData+ArrSize));
        clear();
      }
      ArrData = (T *)adata;
      ArrNum = ArrSize = datalen/(int)sizeof(T);
    }
  }

  // this changes only capacity, length will not be increased (but can be decreased)
  void Resize (int NewSize) {
    vassert(NewSize >= 0);

    if (NewSize <= 0) { clear(); return; }

    Flatten(); // just in case
    if (NewSize == ArrSize) return;

    // free unused elements
    for (int i = NewSize; i < ArrNum; ++i) ArrData[i].~T();

    // realloc buffer
    ArrData = (T *)Z_Realloc(ArrData, NewSize*sizeof(T));

    // no need to init new data, allocator will do it later
    ArrSize = NewSize;
    if (ArrNum > NewSize) ArrNum = NewSize;
  }
  inline void resize (int NewSize) { Resize(NewSize); }

  void SetNum (int NewNum, bool bResize=true) {
    vassert(NewNum >= 0);
    Flatten(); // just in case
    if (bResize || NewNum > ArrSize) Resize(NewNum);
    vassert(ArrSize >= NewNum);
    if (ArrNum > NewNum) {
      // destroy freed elements
      for (int i = NewNum; i < ArrNum; ++i) ArrData[i].~T();
    } else if (ArrNum < NewNum) {
      // initialize new elements
      memset((void *)(ArrData+ArrNum), 0, (NewNum-ArrNum)*sizeof(T));
      for (int i = ArrNum; i < NewNum; ++i) new(&ArrData[i], E_ArrayNew, E_ArrayNew)T;
    }
    ArrNum = NewNum;
  }
  inline void setNum (int NewNum, bool bResize=true) { SetNum(NewNum, bResize); }
  inline void setLength (int NewNum, bool bResize=true) { SetNum(NewNum, bResize); }
  inline void SetLength (int NewNum, bool bResize=true) { SetNum(NewNum, bResize); }

  inline void SetNumWithReserve (int NewNum) {
    vassert(NewNum >= 0);
    if (NewNum > ArrSize) {
#ifdef VAVOOM_CORELIB_ARRAY_MINIMAL_RESIZE
      Resize(NewNum+64);
#else
      Resize(NewNum+NewNum*3/8+32);
#endif
    }
    SetNum(NewNum, false); // don't resize
  }
  inline void setLengthReserve (int NewNum) { SetNumWithReserve(NewNum); }

  inline void Condense () { Resize(ArrNum); }
  inline void condense () { Resize(ArrNum); }

  // this won't copy capacity (there is no reason to do it)
  TArray<T> &operator = (const TArray<T> &other) {
    if (&other == this) return *this; // oops
    vassert(!other.Is2D());
    clear();
    int newsz = other.ArrNum;
    if (newsz) {
      ArrNum = ArrSize = newsz;
      ArrData = (T *)Z_Malloc(newsz*sizeof(T));
      memset((void *)ArrData, 0, newsz*sizeof(T));
      for (int i = 0; i < newsz; ++i) {
        new(&ArrData[i], E_ArrayNew, E_ArrayNew)T;
        ArrData[i] = other.ArrData[i];
      }
    }
    return *this;
  }

  inline T &operator [] (int index) {
    vassert(index >= 0);
    vassert(index < ArrNum);
    return ArrData[index];
  }

  inline const T &operator [] (int index) const {
    vassert(index >= 0);
    vassert(index < ArrNum);
    return ArrData[index];
  }

  inline T &last () {
    vassert(!Is2D());
    vassert(ArrNum > 0);
    return ArrData[ArrNum-1];
  }

  inline const T &last () const {
    vassert(!Is2D());
    vassert(ArrNum > 0);
    return ArrData[ArrNum-1];
  }

  inline void drop () {
    vassert(!Is2D());
    if (ArrNum > 0) {
      --ArrNum;
      ArrData[ArrNum].~T();
    }
  }

  inline void Insert (int index, const T &item) {
    vassert(!Is2D());
    int oldlen = ArrNum;
    setLengthReserve(oldlen+1);
    for (int i = oldlen; i > index; --i) ArrData[i] = ArrData[i-1];
    ArrData[index] = item;
  }
  inline void insert (int index, const T &item) { Insert(index, item); }

  inline int Append (const T &item) {
    vassert(!Is2D());
    int oldlen = ArrNum;
    setLengthReserve(oldlen+1);
    ArrData[oldlen] = item;
    return oldlen;
  }
  inline int append (const T &item) { return Append(item); }

  inline T &Alloc () {
    vassert(!Is2D());
    int oldlen = ArrNum;
    setLengthReserve(oldlen+1);
    return ArrData[oldlen];
  }
  inline T &alloc () { return Alloc(); }

  inline void RemoveIndex (int index) {
    vassert(ArrData != nullptr);
    vassert(index >= 0);
    vassert(index < ArrNum);
    Flatten(); // just in case
    --ArrNum;
    for (int i = index; i < ArrNum; ++i) ArrData[i] = ArrData[i+1];
    ArrData[ArrNum].~T();
  }
  inline void removeAt (int index) { return RemoveIndex(index); }

  inline int Remove (const T &item) {
    Flatten(); // just in case
    int count = 0;
    for (int i = 0; i < ArrNum; ++i) {
      if (ArrData[i] == item) { ++count; RemoveIndex(i--); }
    }
    return count;
  }
  inline int remove (const T &item) { return Remove(item); }

  friend VStream &operator << (VStream &Strm, TArray<T> &Array) {
    vassert(!Array.Is2D());
    int NumElem = Array.Num();
    Strm << STRM_INDEX(NumElem);
    if (Strm.IsLoading()) Array.SetNum(NumElem);
    for (int i = 0; i < Array.Num(); ++i) Strm << Array[i];
    return Strm;
  }

public:
  // range iteration
  // WARNING! don't add/remove array elements in iterator loop!

  inline T *begin () { return (length1D() > 0 ? ArrData : nullptr); }
  inline const T *begin () const { return (length1D() > 0 ? ArrData : nullptr); }
  inline T *end () { return (length1D() > 0 ? ArrData+length1D() : nullptr); }
  inline const T *end () const { return (length1D() > 0 ? ArrData+length1D() : nullptr); }

  #define VARR_DEFINE_ITEMS_ITERATOR(xconst_)  \
    class xconst_##IndexIterator { \
    public: \
      xconst_ T *currvalue; \
      xconst_ T *endvalue; \
      int currindex; \
    public: \
      xconst_##IndexIterator (xconst_ TArray<T> *arr) { \
        if (arr->length1D() > 0) { \
          currvalue = arr->ArrData; \
          endvalue = currvalue+arr->length1D(); \
        } else { \
          currvalue = endvalue = nullptr; \
        } \
        currindex = 0; \
      } \
      xconst_##IndexIterator (const xconst_##IndexIterator &it) : currvalue(it.currvalue), endvalue(it.endvalue), currindex(it.currindex) {} \
      xconst_##IndexIterator (const xconst_##IndexIterator &it, bool asEnd) : currvalue(it.endvalue), endvalue(it.endvalue), currindex(it.currindex) {} \
      inline xconst_##IndexIterator begin () { return xconst_##IndexIterator(*this); } \
      inline xconst_##IndexIterator end () { return xconst_##IndexIterator(*this, true); } \
      inline bool operator == (const xconst_##IndexIterator &b) const { return (currvalue == b.currvalue); } \
      inline bool operator != (const xconst_##IndexIterator &b) const { return (currvalue != b.currvalue); } \
      inline xconst_##IndexIterator operator * () const { return xconst_##IndexIterator(*this); } /* required for iterator */ \
      inline void operator ++ () { ++currvalue; ++currindex; } /* this is enough for iterator */ \
      inline xconst_ T &value () { return *currvalue; } \
      /*inline const T &value () const { return *currvalue; }*/ \
      inline int index () const { return currindex; } \
    }; \
    inline xconst_##IndexIterator itemsIdx () xconst_ { return xconst_##IndexIterator(this); }

  VARR_DEFINE_ITEMS_ITERATOR()
  VARR_DEFINE_ITEMS_ITERATOR(const)
  #undef VARR_DEFINE_ITEMS_ITERATOR

  #define VARR_DEFINE_ITEMS_ITERATOR(xconst_)  \
    class xconst_##IndexIteratorRev { \
    public: \
      xconst_ TArray<T> *arr; \
      int currindex; \
    public: \
      xconst_##IndexIteratorRev (xconst_ TArray<T> *aarr) : arr(aarr) { currindex = (arr->length1D() > 0 ? arr->length1D()-1 : -1); } \
      xconst_##IndexIteratorRev (const xconst_##IndexIteratorRev &it) : arr(it.arr), currindex(it.currindex) {} \
      xconst_##IndexIteratorRev (const xconst_##IndexIteratorRev &it, bool asEnd) : arr(it.arr), currindex(-1) {} \
      inline xconst_##IndexIteratorRev begin () { return xconst_##IndexIteratorRev(*this); } \
      inline xconst_##IndexIteratorRev end () { return xconst_##IndexIteratorRev(*this, true); } \
      inline bool operator == (const xconst_##IndexIteratorRev &b) const { return (arr == b.arr && currindex == b.currindex); } \
      inline bool operator != (const xconst_##IndexIteratorRev &b) const { return (arr != b.arr || currindex != b.currindex); } \
      inline xconst_##IndexIteratorRev operator * () const { return xconst_##IndexIteratorRev(*this); } /* required for iterator */ \
      inline void operator ++ () { --currindex; } /* this is enough for iterator */ \
      inline xconst_ T &value () { return arr->ArrData[currindex]; } \
      /*inline const T &value () const { return arr->ArrData[currindex]; }*/ \
      inline int index () const { return currindex; } \
    }; \
    inline xconst_##IndexIteratorRev itemsIdxRev () xconst_ { return xconst_##IndexIteratorRev(this); }

  VARR_DEFINE_ITEMS_ITERATOR()
  VARR_DEFINE_ITEMS_ITERATOR(const)
  #undef VARR_DEFINE_ITEMS_ITERATOR
};
