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
//**  k8vavoom object base class.
//**
//**************************************************************************

// Define private default constructor.
#define NO_DEFAULT_CONSTRUCTOR(cls) \
  protected: cls() {} public:

// Declare the base VObject class.
#define DECLARE_BASE_CLASS(TClass, TSuperClass, TStaticFlags) \
public: \
  /* Identification */ \
  enum {StaticClassFlags = TStaticFlags|CLASS_Native}; \
  private: static VClass PrivateStaticClass; public: \
  typedef TSuperClass Super; \
  typedef TClass ThisClass; \
  static VClass *StaticClass() { return &PrivateStaticClass; }

// Declare a concrete class.
#define DECLARE_CLASS(TClass, TSuperClass, TStaticFlags) \
  DECLARE_BASE_CLASS(TClass, TSuperClass, TStaticFlags|CLASS_Native) \
  virtual ~TClass() { ConditionalDestroy(); } \
  friend inline VStream &operator<<(VStream &Strm, TClass *&Obj) { return Strm << *(VObject**)&Obj; } \
  static void InternalConstructor() { new TClass(); }

// Declare an abstract class.
#define DECLARE_ABSTRACT_CLASS(TClass, TSuperClass, TStaticFlags) \
  DECLARE_BASE_CLASS(TClass, TSuperClass, TStaticFlags|CLASS_Abstract) \
  virtual ~TClass() { ConditionalDestroy(); } \
  friend inline VStream &operator<<(VStream &Strm, TClass *&Obj) { return Strm << *(VObject**)&Obj; }

// Register a class at startup time.
#define IMPLEMENT_CLASS(Pre, TClass) \
  VClass Pre##TClass::PrivateStaticClass \
  ( \
    EC_NativeConstructor, \
    sizeof(Pre##TClass), \
    Pre##TClass::StaticClassFlags, \
    Pre##TClass::Super::StaticClass(), \
    NAME_##TClass, \
    Pre##TClass::InternalConstructor \
  ); \
  VClass *autoclass##Pre##TClass = Pre##TClass::StaticClass();

// use this to declare VavoomC native function in some object
#define DECLARE_FUNCTION(func) \
  static FBuiltinInfo funcinfo##func; \
  static void exec##func();

// use this to implement VavoomC native function in some object
#define IMPLEMENT_FUNCTION(TClass, Func) \
  FBuiltinInfo TClass::funcinfo##Func(#Func, TClass::StaticClass(), TClass::exec##Func); \
  void TClass::exec##Func()

// use this to implement VavoomC native function as free function (it will still belong to the given VC object)
#define IMPLEMENT_FREE_FUNCTION(TClass, Func) \
  static void vc_free_exec##Func(); \
  /*static*/ FBuiltinInfo vc_free_funcinfo##Func(#Func, TClass::StaticClass(), &vc_free_exec##Func); \
  static void vc_free_exec##Func()


// ////////////////////////////////////////////////////////////////////////// //
// keys and buttons
enum {
  K_ESCAPE = 27,
  K_ENTER = 13,
  K_TAB = 9,
  K_BACKSPACE = 8,

  K_SPACE = 32,

  K_N0 = 48, K_N1, K_N2, K_N3, K_N4, K_N5, K_N6, K_N7, K_N8, K_N9,

  K_a = 97, K_b, K_c, K_d, K_e, K_f,  K_g, K_h, K_i, K_j, K_k, K_l,
  K_m, K_n, K_o, K_p, K_q, K_r, K_s, K_t, K_u, K_v, K_w, K_x, K_y, K_z,

  K_FIRST_CONTROL_KEYCODE = 0x80,

  K_UPARROW = 0x80, K_LEFTARROW, K_RIGHTARROW, K_DOWNARROW,
  K_INSERT, K_DELETE, K_HOME, K_END, K_PAGEUP, K_PAGEDOWN,

  K_PAD0, K_PAD1, K_PAD2, K_PAD3, K_PAD4, K_PAD5, K_PAD6, K_PAD7, K_PAD8, K_PAD9,

  K_NUMLOCK,
  K_PADDIVIDE, K_PADMULTIPLE,
  K_PADMINUS, K_PADPLUS,
  K_PADENTER, K_PADDOT,

  K_CAPSLOCK,
  K_BACKQUOTE,

  K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12,

  K_LSHIFT, K_RSHIFT,
  K_LCTRL, K_RCTRL,
  K_LALT, K_RALT,

  K_LWIN, K_RWIN,
  K_MENU,

  K_PRINTSCRN,
  K_SCROLLLOCK,
  K_PAUSE,

  K_MOUSE1, K_MOUSE2, K_MOUSE3, K_MOUSE4, K_MOUSE5,
  K_MOUSE6, K_MOUSE7, K_MOUSE8, K_MOUSE9,
  K_MWHEELUP, K_MWHEELDOWN,

  K_JOY1, K_JOY2, K_JOY3, K_JOY4, K_JOY5, K_JOY6, K_JOY7, K_JOY8, K_JOY9,
  K_JOY10, K_JOY11, K_JOY12, K_JOY13, K_JOY14, K_JOY15, K_JOY16,

  K_MOUSE_FIRST = K_MOUSE1,
  K_MOUSE_LAST = K_MWHEELDOWN,

  K_MOUSE_BUTTON_FIRST = K_MOUSE1,
  K_MOUSE_BUTTON_LAST = K_MOUSE9,

  K_JOY_FIRST = K_JOY1,
  K_JOY_LAST = K_JOY16,

  K_KEYCODE_MAX,
};

static_assert(K_KEYCODE_MAX < 256, "too many keycodes");


// input event types
enum {
  ev_keydown,
  ev_keyup,
  ev_mouse,
  ev_joystick,
  // extended events for vcc_run
  ev_uimouse,
  ev_winfocus, // data1: focused
  // only for vccrun
  ev_timer, // data1: timer id
  ev_closequery,
  // socket library
  ev_socket,
  // for neoUI library
  ev_neoui = 69,
  ev_user = 666,
};


enum {
  EFlag_Eaten = 1U<<0,
  EFlag_Cancelled = 1U<<1,
  EFlag_Bubbling = 1U<<2, // this event is "bubbling up"
};


enum {
  bCtrl = 1U<<0,
  bAlt = 1U<<1,
  bShift = 1U<<2,
  bHyper = 1U<<3,
  bLMB = 1U<<4,
  bMMB = 1U<<5,
  bRMB = 1U<<6,
  bCtrlLeft = 1U<<7,
  bAltLeft = 1U<<8,
  bShiftLeft = 1U<<9,
  bCtrlRight = 1U<<10,
  bAltRight = 1U<<11,
  bShiftRight = 1U<<12,
};


// event structure
struct event_t {
  vint32 type; // event type
  // unions are actually aliases
  union {
    vint32 data1; // keys / mouse / joystick buttons
    vint32 keycode; // ev_keyXXX
    vint32 focused; // ev_winfocus
    vint32 timerid; // ev_timer
    vint32 sockev; // ev_socket
  };
  union {
    vint32 data2; // mouse / joystick x move
    vint32 dx; // ev_mouse
    vint32 x; // ev_uimouse
    vint32 sockid; // ev_socket
  };
  union {
    vint32 data3; // mouse / joystick y move
    vint32 dy; // ev_mouse
    vint32 y; // ev_uimouse
    vint32 sockdata; // ev_socket
  };
  vuint32 flags; // EFlag_XXX
  VObject *obj;
  VObject *dest;
  vuint32 modflags;

  inline bool isEaten () const { return !!(flags&EFlag_Eaten); }
  inline void setEaten () { flags|= EFlag_Eaten; }

  inline bool isCancelled () const { return !!(flags&EFlag_Cancelled); }
  inline void setCancelled () { flags |= EFlag_Cancelled; }

  inline bool isBubbling () const { return !!(flags&EFlag_Bubbling); }
  inline void setBubbling () { flags |= EFlag_Bubbling; }

  inline bool isCtrlDown () const { return !!(modflags&bCtrl); }
  inline bool isAltDown () const { return !!(modflags&bAlt); }
  inline bool isShiftDown () const { return !!(modflags&bShift); }
  inline bool isHyperDown () const { return !!(modflags&bHyper); }

  inline bool isLMBDown () const { return !!(modflags&bLMB); }
  inline bool isMMBDown () const { return !!(modflags&bMMB); }
  inline bool isRMBDown () const { return !!(modflags&bRMB); }

  inline bool isMBDown (int index) const { return (index >= 0 && index <= 2 ? !!(modflags&(bLMB<<(index&0x0f))) : false); }

  inline bool isLCtrlDown () const { return !!(modflags&bCtrlLeft); }
  inline bool isLAltDown () const { return !!(modflags&bAltLeft); }
  inline bool isLShiftDown () const { return !!(modflags&bShiftLeft); }

  inline bool isRCtrlDown () const { return !!(modflags&bCtrlRight); }
  inline bool isRAltDown () const { return !!(modflags&bAltRight); }
  inline bool isRShiftDown () const { return !!(modflags&bShiftRight); }
};


#if !defined(IN_VCC)
// ////////////////////////////////////////////////////////////////////////// //
// `ExecuteFunction()` returns this
// we cannot return arrays
// returning pointers is not implemented yet (except VClass and VObject)
struct VFuncRes {
private:
  EType type;
  bool pointer;
  /*const*/ void *ptr;
  int ival; // and bool
  float fval;
  TVec vval;
  VStr sval;
  VName nval;

public:
  VFuncRes () : type(TYPE_Void), pointer(false), ptr(nullptr), ival(0), fval(0), vval(0, 0, 0), sval(), nval(NAME_None) {}
  VFuncRes (const VFuncRes &b) : type(TYPE_Void), pointer(false), ptr(nullptr), ival(0), fval(0), vval(0, 0, 0), sval(), nval(NAME_None) { *this = b; }

  VFuncRes (bool v) : type(TYPE_Int), pointer(false), ptr(nullptr), ival(v ? 1 : 0), fval(v ? 1 : 0), vval(0, 0, 0), sval(), nval(NAME_None) {}

  VFuncRes (int v) : type(TYPE_Int), pointer(false), ptr(nullptr), ival(v), fval(v), vval(0, 0, 0), sval(), nval(NAME_None) {}
  //VFuncRes (const int *v) : type(TYPE_Int), pointer(true), ptr(v), ival(v ? *v : 0), fval(v ? float(*v) : 0.0f), vval(0, 0, 0), sval(), nval(NAME_None) {}

  VFuncRes (float v) : type(TYPE_Float), pointer(false), ptr(nullptr), ival(v), fval(v), vval(0, 0, 0), sval(), nval(NAME_None) {}
  //VFuncRes (const float *v) : type(TYPE_Float), pointer(true), ptr(v), ival(*v ? int(v) : 0), fval(v ? *v : 0.0f), vval(0, 0, 0), sval(), nval(NAME_None) {}

  VFuncRes (const TVec &v) : type(TYPE_Vector), pointer(false), ptr(nullptr), ival(0), fval(0), vval(v.x, v.y, v.z), sval(), nval(NAME_None) {}
  //VFuncRes (const TVec *&v) : type(TYPE_Vector), pointer(true), ptr(v), ival(0), fval(0), vval(v ? v->x : 0.0f, v ? v->y : 0.0f, v ? v->z : 0.0f), sval(), nval(NAME_None) {}
  VFuncRes (const float x, const float y, const float z) : type(TYPE_Vector), pointer(false), ptr(nullptr), ival(0), fval(0), vval(x, y, z), sval(), nval(NAME_None) {}

  VFuncRes (VStr v) : type(TYPE_String), pointer(false), ptr(nullptr), ival(0), fval(0), vval(0, 0, 0), sval(v), nval(NAME_None) {}
  //VFuncRes (const VStr *&v) : type(TYPE_String), pointer(true), ptr(v), ival(0), fval(0), vval(0, 0, 0), sval(v ? *v : VStr::EmptyString), nval(NAME_None) {}

  VFuncRes (VName v) : type(TYPE_Name), pointer(false), ptr(nullptr), ival(0), fval(0), vval(0, 0, 0), sval(), nval(v) {}
  //VFuncRes (const VName *&v) : type(TYPE_Name), pointer(true), ptr(v), ival(0), fval(0), vval(0, 0, 0), sval(), nval() { if (v) nval = VName(*v); else nval = NAME_None; }

  VFuncRes (VClass *v) : type(TYPE_Class), pointer(false), ptr(v), ival(0), fval(0), vval(0, 0, 0), sval(), nval(NAME_None) {}
  VFuncRes (VObject *v) : type(TYPE_Reference), pointer(false), ptr(v), ival(0), fval(0), vval(0, 0, 0), sval(), nval(NAME_None) {}
  VFuncRes (VState *v) : type(TYPE_State), pointer(false), ptr(v), ival(0), fval(0), vval(0, 0, 0), sval(), nval(NAME_None) {}

  inline VFuncRes &operator = (const VFuncRes &b) {
    if (&b == this) return *this;
    type = b.type;
    pointer = b.pointer;
    ptr = b.ptr;
    ival = b.ival;
    fval = b.fval;
    vval = b.vval;
    sval = b.sval;
    nval = b.nval;
    return *this;
  }

  inline EType getType () const { return type; }
  inline bool isPointer () const { return pointer; }

  inline bool isVoid () const { return (type == TYPE_Void); }

  inline bool isNumber () const { return (type == TYPE_Int || type == TYPE_Float); }
  inline bool isInt () const { return (type == TYPE_Int); }
  inline bool isFloat () const { return (type == TYPE_Float); }
  inline bool isVector () const { return (type == TYPE_Vector); }
  inline bool isStr () const { return (type == TYPE_String); }
  inline bool isName () const { return (type == TYPE_Name); }
  inline bool isClass () const { return (type == TYPE_Class); }
  inline bool isObject () const { return (type == TYPE_Reference); }
  inline bool isState () const { return (type == TYPE_State); }

  inline const void *getPtr () const { return ptr; }
  inline VClass *getClass () const { return (VClass *)ptr; }
  inline VObject *getObject () const { return (VObject *)ptr; }
  inline VState *getState () const { return (VState *)ptr; }

  inline int getInt () const { return ival; }
  inline float getFloat () const { return fval; }
  inline const TVec &getVector () const { return vval; }
  inline VStr getStr () const { return sval; }
  inline VName getName () const { return nval; }

  inline bool getBool () const {
    switch (type) {
      case TYPE_Int: return (ival != 0);
      case TYPE_Float: return (fval != 0);
      case TYPE_Vector: return (vval.x != 0 && vval.y != 0 && vval.z != 0);
      case TYPE_String: return (sval.length() != 0);
      case TYPE_Name: return (nval != NAME_None);
      default: break;
    }
    return false;
  }

  inline operator bool () const { return getBool(); }
};
#endif


// flags describing an object instance
enum EObjectFlags {
  // object Destroy has already been called
  _OF_Destroyed      = 0x00000001,
  // for k8vavoom: this thinker is marked for deletion on a next tick
  //               tick processor will take care of calling `Destroy()` on it
  // for VccRun: you can call `CollectGarbage(true)` to destroy that objects
  _OF_DelayedDestroy = 0x00000002,
  // this object is going to be destroyed; only GC will set this flag, and
  // you have to check it in your `ClearReferences()`
  _OF_CleanupRef     = 0x00000004, // this object is goind to be destroyed
};


//==========================================================================
//
//  VObject
//
//==========================================================================

// the base class of all objects
class VObject : public VInterface {
  // declarations
  DECLARE_BASE_CLASS(VObject, VObject, CLASS_Abstract|CLASS_Native)

  // friends
  friend class FObjectIterator;
  friend class VMethodProxy;

public:
  static int cliShowReplacementMessages; // default is false
  static int cliShowLoadingMessages; // default is false
  static int cliShowGCMessages; // default is false
  static int cliShowIODebugMessages; // default is false
  static int cliDumpNameTables; // default is false
  static int cliAllErrorsAreFatal; // default is false

  static TMap<VStrCI, bool> cliAsmDumpMethods;

#if defined(VCC_STANDALONE_EXECUTOR)
# define VCC_OBJECT_DEFAULT_SKIP_REPLACE_ON_SPAWN  false
#else
# define VCC_OBJECT_DEFAULT_SKIP_REPLACE_ON_SPAWN  true
#endif

public:
  struct GCStats {
    int alive; // number of currently alive objects
    int markedDead; // number of objects currently marked as dead
    int lastCollected; // number of objects collected on last non-empty cycle
    int poolSize; // total number of used (including free) slots in object pool
    int poolAllocated; // total number of allocated slots in object pool
    int firstFree; // first free slot in pool
    double lastCollectDuration; // in seconds
    double lastCollectTime;
  };

private:
  // internal per-object variables
  VMethod **vtable; // Vavoom C VMT
  vint32 Index; // index of object into table
  vuint32 UniqueId; // monotonically increasing
  vuint32 ObjectFlags; // private EObjectFlags used by object manager
  VClass *Class; // class the object belongs to

  // private systemwide variables
  //static bool GObjInitialised;
  static TArray<VObject *> GObjObjects; // list of all objects
  static int GNumDeleted;
  static bool GInGarbageCollection;

  static GCStats gcLastStats;

public:
#ifdef VCC_STANDALONE_EXECUTOR
  static bool GImmediadeDelete;
#endif
  static bool GGCMessagesAllowed;
  static int GCDebugMessagesAllowed;
#if !defined(IN_VCC)
  static bool (*onExecuteNetMethodCB) (VObject *obj, VMethod *func); // return `false` to do normal execution
#endif
  static bool DumpBacktraceToStdErr;

public:
  // constructors
  VObject ();
  static void InternalConstructor () { new VObject(); }
  virtual void PostCtor (); // this is called after defaults were blit

  // destructors
  virtual ~VObject () override;

  void *operator new (size_t);
  void *operator new (size_t, const char *, int);
  void operator delete (void *);
  void operator delete (void *, const char *, int);

  // VObject interface
  virtual void Register ();
  virtual void Destroy ();
  virtual void SerialiseFields (VStream &); // this serialises object fields
  virtual void SerialiseOther (VStream &); // this serialises other object internal data
  void Serialise (VStream &); // this calls field serialisation, then other serialisation (and writes metadata)
  virtual void ClearReferences ();
#if !defined(IN_VCC)
  virtual bool ExecuteNetMethod (VMethod *);
#endif

  // system-wide functions
  static void StaticInit ();
  // call this before exiting from `main()`, so VC can skip some destructor duties
  static void StaticExit ();
  // call to register options in `GParsedArgs`
  static void StaticInitOptions (VParsedArgs &pargs);

  static VObject *StaticSpawnObject (VClass *AClass, bool skipReplacement=VCC_OBJECT_DEFAULT_SKIP_REPLACE_ON_SPAWN);

  inline static VObject *StaticSpawnWithReplace (VClass *AClass) { return StaticSpawnObject(AClass, false); }
  inline static VObject *StaticSpawnNoReplace (VClass *AClass) { return StaticSpawnObject(AClass, true); }

  // note that you CANNOT use `delete` on VObjects, you have to call
  // either `Destroy()` or (even better) `ConditionalDestroy()`
  // the difference is that `ConditionalDestroy()` will call `Destroy()`
  // by itself, and will do it only once
#if defined(VCC_STANDALONE_EXECUTOR)
  static void CollectGarbage (bool destroyDelayed=false);
#else
  static void CollectGarbage ();
#endif
  static VObject *GetIndexObject (int);

  static int GetObjectsCount ();

#if !defined(IN_VCC)
  static VFuncRes ExecuteFunction (VMethod *func); // all arguments should be on the stack
  static VFuncRes ExecuteFunctionNoArgs (VObject *Self, VMethod *func, bool allowVMTLookups=true);
#endif
  static void VMDumpCallStack ();
#if !defined(IN_VCC)
  static void VMDumpCallStackToStdErr ();
  static void ClearProfiles ();
  static void DumpProfile ();
  static void DumpProfileInternal (int type); // <0: only native; >0: only script; 0: everything
#endif

  // functions
  void ConditionalDestroy ();

  inline bool IsA (VClass *SomeBaseClass) const {
    for (const VClass *c = Class; c; c = c->GetSuperClass()) if (SomeBaseClass == c) return true;
    return false;
  }

  // accessors
  inline VClass *GetClass () const { return Class; }
  inline vuint32 GetFlags () const { return ObjectFlags; }
  //inline void SetFlags (vuint32 NewFlags) { ObjectFlags |= NewFlags; }
  void SetFlags (vuint32 NewFlags);
  inline void ClearFlags (vuint32 NewFlags) { ObjectFlags &= ~NewFlags; }
  //inline vuint32 GetObjectIndex () const { return Index; }
  inline vuint32 GetUniqueId () const { return UniqueId; } // never 0

  inline VMethod *GetVFunctionIdx (int InIndex) const { return vtable[InIndex]; }
  inline VMethod *GetVFunction (VName FuncName) const { return vtable[Class->GetMethodIndex(FuncName)]; }

  static VStr NameFromVKey (int vkey);
  static int VKeyFromName (VStr kn);

  inline static const GCStats &GetGCStats () { return gcLastStats; }
  inline static void ResetGCStatsLastCollected () { gcLastStats.lastCollected = 0; }

  #include "vc_object_common.h"

public:
  // event queue API; as it is used both in k8vavoom and in vccrun, and with common `event_t` struct,
  // there is no reason to not have it here

  // returns `false` if queue is full
  // add event to the bottom of the current queue
  // it is unspecified if posted event will be processed in the current
  // frame, or in the next one
  static bool PostEvent (const event_t &ev);

  // returns `false` if queue is full
  // add event to the top of the current queue
  // it is unspecified if posted event will be processed in the current
  // frame, or in the next one
  static bool InsertEvent (const event_t &ev);

  // check if event queue has any unprocessed events
  // returns number of events in queue or 0
  // it is unspecified if unprocessed events will be processed in the current
  // frame, or in the next one
  static int CountQueuedEvents ();

  // peek event from queue
  // event with index 0 is the top one
  // it is safe to call this with `nullptr`
  static bool PeekEvent (int idx, event_t *ev);

  // get top event from queue
  // returns `false` if there are no more events
  // it is safe to call this with `nullptr` (in this case event will be removed)
  static bool GetEvent (event_t *ev);

  // returns maximum size of event queue
  // note that event queue may be longer that the returned value
  static int GetEventQueueSize ();

  // invalid newsize values will be ignored
  // if event queue currently contanis more than `newsize` events, the API is noop
  // returns success flag
  static bool SetEventQueueSize (int newsize);

  // unconditionally clears event queue
  static void ClearEventQueue ();
};


// dynamically cast an object type-safely
template<class T> T *Cast (VObject *Src) { return (Src && Src->IsA(T::StaticClass()) ? (T *)Src : nullptr); }
template<class T, class U> T *CastChecked (U *Src) {
  if (!Src || !Src->IsA(T::StaticClass())) Sys_Error("Cast `%s` to `%s` failed", (Src ? Src->GetClass()->GetName() : "none"), T::StaticClass()->GetName());
  return (T *)Src;
}


#if !defined(IN_VCC)
//==========================================================================
//
//  VMethodProxy
//
//==========================================================================
class VMethodProxy {
private:
  const char *MethodName;
  VMethod *Method;
  VClass *Class;

public:
  VMethodProxy ();
  VMethodProxy (const char *AMethod);

  // returns `false` if method not found
  bool Resolve (VObject *Self);
  void ResolveChecked (VObject *Self);

  VFuncRes Execute (VObject *Self);
  // this doesn't check is `Self` isa `Class`
  VFuncRes ExecuteNoCheck (VObject *Self);
};
#endif


/*----------------------------------------------------------------------------
  Object iterators.
----------------------------------------------------------------------------*/

// class for iterating through all objects
class FObjectIterator {
protected:
  VClass *Class;
  int Index;

public:
  FObjectIterator (VClass *InClass=VObject::StaticClass()) : Class(InClass), Index(-1) { ++*this; }

  void operator ++ () { while (++Index < VObject::GObjObjects.Num() && (!VObject::GObjObjects[Index] || !VObject::GObjObjects[Index]->IsA(Class))) {} }
  VObject *operator * () { return VObject::GObjObjects[Index]; }
  VObject *operator -> () { return VObject::GObjObjects[Index]; }
  operator bool () { return (Index < VObject::GObjObjects.Num()); }
};


// class for iterating through all objects which inherit from a specified base class
template<class T> class TObjectIterator : public FObjectIterator {
public:
  TObjectIterator () : FObjectIterator(T::StaticClass()) {}

  T *operator * () { return (T*)FObjectIterator::operator*(); }
  T *operator -> () { return (T*)FObjectIterator::operator->(); }
};


// object creation template
template<class T> T *Spawn () { return (T*)VObject::StaticSpawnObject(T::StaticClass()); }
template<class T> T *SpawnWithReplace () { return (T*)VObject::StaticSpawnObject(T::StaticClass(), false); } // don't skip replacement
template<class T> T *SpawnNoReplace () { return (T*)VObject::StaticSpawnObject(T::StaticClass(), true); } // skip replacement

//inline vuint32 GetTypeHash (const VObject *Obj) { return (Obj ? Obj->GetIndex() : 0); }
inline vuint32 GetTypeHash (const VObject *Obj) { return (Obj ? hashU32(Obj->GetUniqueId()) : 0); }

// helper macros for implementing native VavoomC functions and calls to the VavoomC methods:
// this will make it simpler to port it to 64 bit platforms

// macros for passign arguments to VavoomC methods
#define P_PASS_INT(v)    PR_Push(v)
#define P_PASS_BYTE(v)   PR_Push(v)
#define P_PASS_FLOAT(v)  PR_Pushf(v)
#define P_PASS_BOOL(v)   PR_Push(v)
#define P_PASS_NAME(v)   PR_PushName(v)
#define P_PASS_STR(v)    PR_PushStr(v)
#define P_PASS_VEC(v)    PR_Pushv(v)
#define P_PASS_AVEC(v)   PR_Pushav(v)
#define P_PASS_REF(v)    PR_PushPtr(v)
#define P_PASS_PTR(v)    PR_PushPtr(v)
#define P_PASS_SELF      PR_PushPtr(this)

/*
// macros for calling VavoomC methods with different return types
#define EV_RET_VOID(v)    (void)ExecuteFunction(GetVFunction(v))
#define EV_RET_INT(v)     return ExecuteFunction(GetVFunction(v)).getInt()
#define EV_RET_BYTE(v)    return ExecuteFunction(GetVFunction(v)).getInt()
#define EV_RET_FLOAT(v)   return ExecuteFunction(GetVFunction(v)).getFloat()
#define EV_RET_BOOL(v)    return !!ExecuteFunction(GetVFunction(v)).getInt()
#define EV_RET_NAME(v)    return ExecuteFunction(GetVFunction(v)).getName()
#define EV_RET_STR(v)     return ExecuteFunction(GetVFunction(v)).getStr()
#define EV_RET_VEC(v)     return ExecuteFunction(GetVFunction(v)).getVector()
//#define EV_RET_AVEC(v)    Sys_Error("Not implemented") / *ExecuteFunction(GetVFunction(v))* /
#define EV_RET_REF(t, v)  return (t *)ExecuteFunction(GetVFunction(v)).getObject()
#define EV_RET_PTR(t, v)  return (t *)ExecuteFunction(GetVFunction(v)).getClass()

#define EV_RET_VOID_IDX(v)    (void)ExecuteFunction(GetVFunctionIdx(v))
#define EV_RET_INT_IDX(v)     return ExecuteFunction(GetVFunctionIdx(v)).getInt()
#define EV_RET_BYTE_IDX(v)    return ExecuteFunction(GetVFunctionIdx(v)).getInt()
#define EV_RET_FLOAT_IDX(v)   return ExecuteFunction(GetVFunctionIdx(v)).getFloat()
#define EV_RET_BOOL_IDX(v)    return !!ExecuteFunction(GetVFunctionIdx(v)).getInt()
#define EV_RET_NAME_IDX(v)    return ExecuteFunction(GetVFunctionIdx(v)).getName()
#define EV_RET_STR_IDX(v)     return ExecuteFunction(GetVFunctionIdx(v)).getStr()
#define EV_RET_VEC_IDX(v)     return ExecuteFunction(GetVFunctionIdx(v)).getVector()
//#define EV_RET_AVEC_IDX(v)    return ExecuteFunction(GetVFunctionIdx(v)).getVector()
#define EV_RET_REF_IDX(t, v)  return (t *)ExecuteFunction(GetVFunctionIdx(v)).getObject()
#define EV_RET_PTR_IDX(t, v)  return (t *)ExecuteFunction(GetVFunctionIdx(v)).getClass()
*/

// macros for calling VavoomC methods with different return types
// this is for `VMethodProxy`
#define VMT_RET_VOID(v)    (void)(v).Execute(this)
#define VMT_RET_INT(v)     return (v).Execute(this).getInt()
#define VMT_RET_BYTE(v)    return (v).Execute(this).getInt()
#define VMT_RET_FLOAT(v)   return (v).Execute(this).getFloat()
#define VMT_RET_BOOL(v)    return !!(v).Execute(this).getInt()
#define VMT_RET_NAME(v)    return (v).Execute(this).getName()
#define VMT_RET_STR(v)     return (v).Execute(this).getStr()
#define VMT_RET_VEC(v)     return (v).Execute(this).getVector()
//#define VMT_RET_AVEC(v)    Sys_Error("Not implemented") /*(v).ExecuteFunction(this)*/
#define VMT_RET_REF(t, v)  return (t *)(v).Execute(this).getObject()
#define VMT_RET_PTR(t, v)  return (t *)(v).Execute(this).getClass()

// parameter get macros; parameters must be retrieved in backwards order
#define P_GET_INT(v)     vint32 v = PR_Pop()
#define P_GET_BYTE(v)    vuint8 v = PR_Pop()
#define P_GET_FLOAT(v)   float v = PR_Popf()
#define P_GET_BOOL(v)    bool v = !!PR_Pop()
#define P_GET_NAME(v)    VName v = PR_PopName()
#define P_GET_STR(v)     VStr v = PR_PopStr()
#define P_GET_VEC(v)     TVec v = PR_Popv()
#define P_GET_AVEC(v)    TAVec v = PR_Popav()
#define P_GET_REF(c, v)  c *v = (c*)PR_PopPtr()
#define P_GET_PTR(t, v)  t *v = (t*)PR_PopPtr()
#define P_GET_SELF       ThisClass *Self = (ThisClass*)PR_PopPtr()

#define P_GET_INT_OPT(v, d)     bool specified_##v = !!PR_Pop(); vint32 v = PR_Pop(); if (!specified_##v) v = d
#define P_GET_BYTE_OPT(v, d)    bool specified_##v = !!PR_Pop(); vuint8 v = PR_Pop(); if (!specified_##v) v = d
#define P_GET_FLOAT_OPT(v, d)   bool specified_##v = !!PR_Pop(); float v = PR_Popf(); if (!specified_##v) v = d
#define P_GET_BOOL_OPT(v, d)    bool specified_##v = !!PR_Pop(); bool v = !!PR_Pop(); if (!specified_##v) v = d
#define P_GET_NAME_OPT(v, d)    bool specified_##v = !!PR_Pop(); VName v = PR_PopName(); if (!specified_##v) v = d
#define P_GET_STR_OPT(v, d)     bool specified_##v = !!PR_Pop(); VStr v = PR_PopStr(); if (!specified_##v) v = d
#define P_GET_VEC_OPT(v, d)     bool specified_##v = !!PR_Pop(); TVec v = PR_Popv(); if (!specified_##v) v = d
#define P_GET_AVEC_OPT(v, d)    bool specified_##v = !!PR_Pop(); TAVec v = PR_Popav(); if (!specified_##v) v = d
#define P_GET_REF_OPT(c, v, d)  bool specified_##v = !!PR_Pop(); c *v = (c *)PR_PopPtr(); if (!specified_##v) v = d
#define P_GET_PTR_OPT(t, v, d)  bool specified_##v = !!PR_Pop(); t *v = (t *)PR_PopPtr(); if (!specified_##v) v = d
#define P_GET_OUT_OPT(t, v)     bool specified_##v = !!PR_Pop(); t *v = (t *)PR_PopPtr()

#define P_GET_PTR_OPT_NOSP(t, v)  PR_Pop(); t *v = (t *)PR_PopPtr()
#define P_GET_OUT_OPT_NOSP(t, v)  PR_Pop(); t *v = (t *)PR_PopPtr()

// method return macros
#define RET_INT(v)    PR_Push(v)
#define RET_BYTE(v)   PR_Push(v)
#define RET_FLOAT(v)  PR_Pushf(v)
#define RET_BOOL(v)   PR_Push(v)
#define RET_NAME(v)   PR_PushName(v)
#define RET_STR(v)    PR_PushStr(v)
#define RET_VEC(v)    PR_Pushv(v)
#define RET_AVEC(v)   PR_Pushav(v)
#define RET_REF(v)    PR_PushPtr(v)
#define RET_PTR(v)    PR_PushPtr(v)


// ////////////////////////////////////////////////////////////////////////// //
class VScriptIterator : public VInterface {
public:
  virtual bool GetNext() = 0;
  // by default, the following does `delete this;`
  virtual void Finished ();
};


// ////////////////////////////////////////////////////////////////////////// //
static __attribute__((unused)) inline void vobj_get_param (int &n) { n = PR_Pop(); }
static __attribute__((unused)) inline void vobj_get_param (float &n) { n = PR_Popf(); }
static __attribute__((unused)) inline void vobj_get_param (double &n) { n = PR_Popf(); }
static __attribute__((unused)) inline void vobj_get_param (bool &n) { n = !!PR_Pop(); }
static __attribute__((unused)) inline void vobj_get_param (VStr &n) { n = PR_PopStr(); }
static __attribute__((unused)) inline void vobj_get_param (VName &n) { n = PR_PopName(); }
static __attribute__((unused)) inline void vobj_get_param (int *&n) { n = (int *)PR_PopPtr(); }
static __attribute__((unused)) inline void vobj_get_param (TVec &n) { n = PR_Popv(); }
static __attribute__((unused)) inline void vobj_get_param (TAVec &n) { n = PR_Popav(); }
template<class C> static __attribute__((unused)) inline void vobj_get_param (C *&n) { n = (C *)PR_PopPtr(); }

#define VC_DEFINE_OPTPARAM(tname_,type_,defval_,pop_) \
struct VOptParam##tname_ { \
  bool specified; \
  type_ value; \
  VOptParam##tname_ (type_ adefault=defval_) : specified(false), value(adefault) {} \
  inline operator type_ () { return value; } \
}; \
static __attribute__((unused)) inline void vobj_get_param (VOptParam##tname_ &n) { \
  n.specified = !!PR_Pop(); \
  if (n.specified) n.value = pop_(); else (void)pop_(); \
}

VC_DEFINE_OPTPARAM(Int, int, 0, PR_Pop) // VOptParamInt
VC_DEFINE_OPTPARAM(Float, float, 0.0f, PR_Popf) // VOptParamFloat
VC_DEFINE_OPTPARAM(Double, double, 0.0, PR_Popf) // VOptParamDouble
VC_DEFINE_OPTPARAM(Bool, bool, false, !!PR_Pop) // VOptParamBool
VC_DEFINE_OPTPARAM(Str, VStr, VStr::EmptyString, PR_PopStr) // VOptParamStr
VC_DEFINE_OPTPARAM(Name, VName, NAME_None, PR_PopName) // VOptParamName
VC_DEFINE_OPTPARAM(Vec, TVec, TVec(0.0f, 0.0f, 0.0f), PR_Popv) // VOptParamVec
VC_DEFINE_OPTPARAM(AVec, TAVec, TAVec(0.0f, 0.0f, 0.0f), PR_Popav) // VOptParamAVec

template<class C> struct VOptParamPtr {
  bool specified;
  bool popDummy;
  C *value;
  VOptParamPtr () : specified(false), popDummy(true), value(nullptr) {}
  VOptParamPtr (bool aPopDummy, C *adefault=nullptr) : specified(false), popDummy(aPopDummy), value(adefault) {}
  VOptParamPtr (C *adefault) : specified(false), popDummy(false), value(adefault) {}
  inline operator C* () { return value; }
};
template<class C> static __attribute__((unused)) inline void vobj_get_param (VOptParamPtr<C> &n) {
  n.specified = !!PR_Pop();
  // optional ref pushes pointer to dummy object, so why not?
  //if (n.specified || !n.value) n.value = (C *)PR_PopPtr(); else (void)PR_PopPtr();
  if (n.specified || n.popDummy) n.value = (C *)PR_PopPtr(); else (void)PR_PopPtr();
}


template<typename T, typename... Args> static __attribute__((unused)) inline void vobj_get_param (T &n, Args&... args) {
  vobj_get_param(args...);
  vobj_get_param(n);
}


template<typename... Args> static __attribute__((unused)) inline void vobjGetParam (Args&... args) {
  vobj_get_param(args...);
}

#define vobjGetParamSelf(...)  ThisClass *Self; vobjGetParam(Self, ##__VA_ARGS__)


// ////////////////////////////////////////////////////////////////////////// //
static __attribute__((unused)) inline void vobj_put_param (const int v) { PR_Push(v); }
static __attribute__((unused)) inline void vobj_put_param (const float v) { PR_Pushf(v); }
static __attribute__((unused)) inline void vobj_put_param (const double v) { PR_Pushf(v); }
static __attribute__((unused)) inline void vobj_put_param (const bool v) { PR_Push(!!v); }
static __attribute__((unused)) inline void vobj_put_param (const VStr v) { PR_PushStr(v); }
static __attribute__((unused)) inline void vobj_put_param (const VName v) { PR_PushName(v); }
static __attribute__((unused)) inline void vobj_put_param (const TVec v) { PR_Pushv(v); }
static __attribute__((unused)) inline void vobj_put_param (const TAVec v) { PR_Pushav(v); }
//static __attribute__((unused)) inline void vobj_put_param (VObject *v) { PR_PushPtr((void *)v); }
//static __attribute__((unused)) inline void vobj_put_param (const VObject *v) { PR_PushPtr((void *)v); }
template<class C> static __attribute__((unused)) inline void vobj_put_param (C *v) { PR_PushPtr((void *)v); }

#define VC_DEFINE_OPTPARAM_PUT(tname_,type_,defval_,push_) \
struct VOptPutParam##tname_ { \
  bool specified; \
  type_ value; \
  VOptPutParam##tname_ () : specified(false), value(defval_) {} \
  VOptPutParam##tname_ (type_ avalue, bool aspecified=true) : specified(aspecified), value(avalue) {} \
  inline operator type_ () { return value; } \
}; \
static __attribute__((unused)) inline void vobj_put_param (VOptPutParam##tname_ &v) { \
  push_(v.value); \
  PR_Push(!!v.specified); \
}

VC_DEFINE_OPTPARAM_PUT(Int, int, 0, PR_Push) // VOptPutParamInt
VC_DEFINE_OPTPARAM_PUT(Float, float, 0.0f, PR_Pushf) // VOptPutParamFloat
VC_DEFINE_OPTPARAM_PUT(Double, double, 0.0, PR_Pushf) // VOptPutParamDouble
VC_DEFINE_OPTPARAM_PUT(Bool, bool, false, PR_Push) // VOptPutParamBool
VC_DEFINE_OPTPARAM_PUT(Str, VStr, VStr::EmptyString, PR_PushStr) // VOptPutParamStr
VC_DEFINE_OPTPARAM_PUT(Name, VName, NAME_None, PR_PushName) // VOptPutParamName
VC_DEFINE_OPTPARAM_PUT(Vec, TVec, TVec(0.0f, 0.0f, 0.0f), PR_Pushv) // VOptPutParamVec
VC_DEFINE_OPTPARAM_PUT(AVec, TAVec, TAVec(0.0f, 0.0f, 0.0f), PR_Pushav) // VOptPutParamAVec

template<class C> struct VOptPutParamPtr {
  bool specified;
  C *value;
  VOptPutParamPtr () : specified(false), value(nullptr) {}
  VOptPutParamPtr (C *avalue) : specified(true), value(avalue) {}
  inline operator C* () { return value; }
};
template<class C> static __attribute__((unused)) inline void vobj_put_param (VOptPutParamPtr<C> v) {
  PR_PushPtr((void *)v.value);
  PR_Push(!!v.specified);
}


template<typename T, typename... Args> static __attribute__((unused)) inline void vobj_put_param (T v, Args... args) {
  vobj_put_param(v);
  vobj_put_param(args...);
}


template<typename... Args> static __attribute__((unused)) inline void vobjPutParam (Args... args) {
  vobj_put_param(args...);
}

#define vobjPutParamSelf(...)  vobjPutParam(this, ##__VA_ARGS__)