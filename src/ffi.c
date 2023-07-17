#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#include "asm_x64.h"

static int ispackedfloat(double x){ return (((*(uint64_t*)&x) & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000); }
static double packfloat(float x)  { uint64_t u = 0xFFFFFFFF00000000 | *(uint32_t*)&x; return *(double*)&u; }
static float unpackfloat(double x){ return *(float*)&x; }

int luaF_isfloat (lua_State * L, int idx){ return ((lua_type(L, idx) == LUA_TNUMBER) && (!lua_isinteger(L, idx))); }

static void voidfunc(){}
static void luaF_tonil(lua_State * L, int idx){}
static float luaF_tofloat(lua_State* L, int idx) { return unpackfloat(lua_tonumber(L, idx)); }
static void* luaF_topointer(lua_State * L, int idx) { return lua_isinteger(L, idx) ? lua_tointeger(L, idx) : lua_touserdata(L, idx); }

static void* luaF_totable(lua_State* L, int idx) { 
  lua_rawgeti(L, idx, 1);
  if (LUA_TTABLE == lua_type(L, -1)) {
    lua_pop(L, 1);
    size_t num = lua_rawlen(L, idx);
    void** data = lua_newuserdatauv(L, num * sizeof(void*), 0);
    for (size_t i = 0; i < num; i++) {
      lua_rawgeti(L, idx, i + 1);
      data[i] = luaF_totable(L, -1);
    }
    return data;
  }

  if (LUA_TSTRING == lua_type(L, -1)) {
    lua_pop(L, 1);
    size_t num = lua_rawlen(L, idx);
    char ** data = lua_newuserdatauv(L, num * sizeof(char*), 0);
    for (size_t i = 0; i < num; i++) {
      lua_rawgeti(L, idx, i + 1);
      data[i] = lua_tostring(L, -1);
    }
    return data;
  }

  if (lua_isinteger(L, -1)) {
    lua_pop(L, 1);
    size_t num = lua_rawlen(L, idx);
    char * data = lua_newuserdatauv(L, num * sizeof(char), 0);
    for (size_t i = 0; i < num; i++) {
      lua_rawgeti(L, idx, i + 1);
      data[i] = lua_tonumber(L, -1);
    }
    return data;
  }
  return 0;
}

void * luaF_typeto (lua_State * L, int idx){ 
  int t = lua_type(L, idx);
  switch (t){
    case LUA_TNONE:          
    case LUA_TNIL:           return luaF_tonil;
    case LUA_TBOOLEAN:       return lua_toboolean;
    case LUA_TLIGHTUSERDATA: return lua_touserdata;
    case LUA_TNUMBER:        return lua_isinteger(L, idx) ? lua_tointegerx : ispackedfloat(lua_tonumber(L, idx)) ? luaF_tofloat : lua_tonumberx;
    case LUA_TSTRING:        return lua_tolstring;
    case LUA_TTABLE:         return luaF_totable;  //luaL_error(L, "ffi: type of argument %d is 'table'", idx); return luaF_tonil;
    case LUA_TFUNCTION:      break;
    case LUA_TUSERDATA:      return lua_touserdata;
    case LUA_TTHREAD:        break;
  }
  luaL_error(L, "ffi: type of argument %d unknown type %d", idx, t);
  return luaF_tonil;
}

/*
static void * luaF_to  (char t){ 
  const char* s = "bdfipstv", *p = s;
  while (t != *p++) if (*p == '\0') return luaF_tonil;
  return (void*[]){lua_toboolean, lua_tonumberx, lua_tonumberx, lua_tointegerx, luaF_topointer, lua_tolstring, luaF_totable, luaF_tonil} [(p - s - 1)];
}
*/

static void * luaF_to  (const char t){ 
  switch (t){
    case 'b': return lua_toboolean;
    case 'd': return lua_tonumberx;
    case 'f': return lua_tonumberx;
    case 'i': return lua_tointegerx;
    case 'p': return luaF_topointer;
    case 's': return lua_tolstring;
    case 't': return luaF_totable;
    case 'v': return luaF_tonil;
  }
  return luaF_tonil;
}

static void * luaF_push  (const char t){
  switch (t){
    case 'b': return lua_pushboolean;
    case 'd': return lua_pushnumber;
    case 'f': return lua_pushnumber;
    case 'i': return lua_pushinteger;
    case 'p': return lua_pushlightuserdata;
    case 's': return lua_pushstring;
    case 'v': return lua_pushnil;
  }
  return voidfunc;
}

/*
int strlen(const char * str){
  const char * p = str;
  while(*p) p += 1;
  return p - str;
}
*/

int make_func(char * code, const void * func, const char * argt){
  int argc = strlen(argt);
  int stack = 16 * ((argc + 4)>>1);  //32 bytes of shadow space + space for function arguments multiple of 16 bytes to keep stack alignment
  char * p = code;
  {
//prologue
    _push_rbx(p);        //additional push rbx makes stack aligned to 16.
    _mov_rbx_rcx(p);         //as we are in luaCfunction: int f(lua_State * L); L is passed in rcx. store lua_State * L in rbx,
    _sub_rsp_DW(p,stack);   //reserve stack

//get arguments from lua stack and store them on stack
    for (int i = 1; i < argc; i++){  //argt[0] - return type
      if (argt[i] == 'l') {     //automatic, from Lua type at runtime
        _mov_rcx_rbx(p);        //first argument lua_State * L 
        _mov_rdx_DW(p, i + 1);  //second argument n+1 - position of argument on lua stack, [1] == self, [2] == first arg, ...
        _call(p, luaF_typeto);
      } else _mov_rax_QW(p, (uintptr_t)luaF_to(argt[i]));

      _mov_rcx_rbx(p);        //first argument lua_State * L 
      _mov_rdx_DW(p, i + 1);  //second argument n+1 - position of argument on lua stack, [1] == self, [2] == first arg, ...
      _clr_r8(p);             //some lua_to functions have 3rd argument, #define lua_tostring(L, i) lua_tolstring(L, i, 0)
      _call_rax(p);

      if (argt[i] == 'f') _mov_xmm0f_xmm0(p);                                                            //double -> float conversion, double lua_tonumber();
      if (argt[i] == 'f' || argt[i] == 'd') _st_xmm0(p, 24+i*8); else _st_rax(p, 24+i*8);    //store argument on stack with 32 bytes offset of shadow store
    }

    _add_rsp_DW(p, 32);    //adjust stack by shadow 32 bytes that were reserved for lua_to functions, so other (4+) arguments on stack with a proper offset

//put back first four arguments to registers
    if (argc > 1){ if (argt[1] == 'f' || argt[1] == 'd') _ld_xmm0(p, 0);  else _ld_rcx(p, 0);  }
    if (argc > 2){ if (argt[2] == 'f' || argt[2] == 'd') _ld_xmm1(p, 8);  else _ld_rdx(p, 8);  }
    if (argc > 3){ if (argt[3] == 'f' || argt[3] == 'd') _ld_xmm2(p, 16); else _ld_r8 (p, 16); }
    if (argc > 4){ if (argt[4] == 'f' || argt[4] == 'd') _ld_xmm3(p, 24); else _ld_r9 (p, 24); }

    _call(p, func);

//lua_push() function return value to lua stack
    _mov_rcx_rbx(p);                                  //first argument lua_State * L 
    if      (argt[0] == 'f') _mov_xmm1_xmm0f(p);      //convert float to double and mov return value(xmm0) to (xmm1) second argument  of lua_push
    else if (argt[0] == 'd') _mov_xmm1_xmm0(p);       //mov double without conversion
    else _mov_rdx_rax(p);                             //other types returned in rax -> second argument in rdx
    _clr_r8(p);                                       //some lua_push functions could have 3rd argument
    _call(p, luaF_push(argt[0]));
//return 1
    _mov_rax_DW(p, 1);                                //return 1, even void func() returns 1 and lua_pushnil just for uniformity.
    _add_rsp_DW(p, stack-32);                         //restore stack pointer
    _pop_rbx(p);                                      //and rbx
    _ret(p);
  }
  return p - code; //length of generated code
}

int make_cb(char * code, lua_State * L, int ref, const char * argt){
  int argc = strlen(argt);
  char * p = code;
  {
    _push_rbx(p);     //push rbx to align stack to 16 and store lua_State * L in rbx
    _mov_rbx_QW(p, (uintptr_t)L);   //as we are generating ordinary C function, lua_State * L is passed as argument from ffi and stored as immediate constant in code.

    if (argc > 1) if (argt[1] == 'f' || argt[1] == 'd') _st_xmm0(p, 16); else _st_rcx(p, 16);  //store 4 arguments in shadow
    if (argc > 2) if (argt[2] == 'f' || argt[2] == 'd') _st_xmm1(p, 24); else _st_rdx(p, 24);
    if (argc > 3) if (argt[3] == 'f' || argt[3] == 'd') _st_xmm2(p, 32); else _st_r8 (p, 32);
    if (argc > 4) if (argt[4] == 'f' || argt[4] == 'd') _st_xmm3(p, 40); else _st_r9 (p, 40);

    _sub_rsp_DW(p, 48); //32 shadow space + additional 8 bytes for 5th argument of lua_callk + 8 bytes to keep 16 bytes alignment

    _mov_rcx_rbx(p);
    _mov_rdx_DW(p, LUA_REGISTRYINDEX);
    _mov_r8_DW(p, ref);        //lua function we are going to wrap (ref) as C callback is stored in lua registry, so it is not garbage collected accidentally
    _call(p, lua_rawgeti);   //get lua function on stack from registry lua_rawgeti(L, REIGSTRYINDEX, funcref)

    for (int i = 1; i < argc; i++){
      _mov_rcx_rbx(p);   //first argument lua_State * L 
      if (argt[i] == 'f' || argt[i] == 'd') _ld_xmm1(p, 56+i*8); else _ld_rdx(p, 56+i*8);           //get second argument for lua_push from stack
      if (argt[i] == 'f') _mov_xmm1_xmm1f(p);                                                           //float -> double conversion for lua_pushnumber
      _clr_r8(p);     //some lua_push functions have 3rd argument
      _call(p, luaF_push(argt[i]));
    }

    _mov_rcx_rbx(p);
    _mov_rdx_DW(p, argc-1);
    _mov_r8_DW(p, 1);
    _clr_r9(p);
    _st_r9(p, 32);
    _call(p, lua_callk);   //lua_callk(L, argc-1, 1, 0, 0); lua function always return 1 value, nil for void f() callback

    _mov_rcx_rbx(p);        //first argument lua_State * L 
    _mov_rdx_DW(p, -1);   //return value is on top of the lua stack after lua_call
    _clr_r8(p);         //some functions have 3rd argument, lua_tostring == lua_tolstring(L, idx, 0);
    _call(p, luaF_to(argt[0]));

    if (argt[0] == 'f' || argt[0] == 'd') _st_xmm0(p, 32); else _st_rax(p, 32); //store return value on stack to call lua_pop(L, 1)

    _mov_rcx_rbx(p);
    _mov_rdx_DW(p, -2);
    _call(p, lua_settop); //lua_pop(L, 1) == lua_settop(L, -2);

    if (argt[0] == 'f' || argt[0] == 'd') _ld_xmm0(p, 32); else _ld_rax(p, 32); //return value in rax/xmm0

    _add_rsp_DW(p, 48);    //restore stack
    _pop_rbx(p);        //and rbx
    _ret(p);
  }
  return p - code;
}

int func__call_auto(lua_State * L);

int func__call_typed(lua_State * L){
  int (*f)(lua_State *) = lua_touserdata(L, 1);
  return f ? f(L) : 0;
}

int func__call_cb(lua_State * L){ //get rid of self argument and call lua function by reference from uservalue
  lua_getiuservalue(L, 1, 1);
  if (lua_type(L, -1) == LUA_TNUMBER) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, lua_tointeger(L, -1));
    lua_replace(L, 1);        //put lua callback function as first argument instead of self
    lua_pop(L, 1);            //pop ref
    int num = lua_gettop(L);  //+1: [1] = self -> func
    lua_call(L, num - 1, LUA_MULTRET);
    return lua_gettop(L) - num;
  } 
  lua_pop(L, 1);
  return 0;
}

int func__gc(lua_State * L){
  lua_getiuservalue(L, 1, 1);
  if (lua_islightuserdata(L, -1)){
    void * lib = lua_touserdata(L, -1);
    if (lib) FreeLibrary(lib);
  } 
  if (lua_isinteger(L, -1)){
    int ref = lua_tointeger(L, -1);
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
  }
  lua_pop(L, 1);
  DWORD protect;
  VirtualProtect(lua_touserdata(L, 1), lua_rawlen(L, 1), PAGE_READWRITE, &protect);
  return 0;
}

int libfuncnum(uint32_t * lib, int ** dir_exp){
  if (lib == 0) return 0;
  uint32_t * p = lib;
  if ((p[0] & 0xFFFF) != 0x5A4D) return 0; //MZ
  p = &lib[p[15]>>2];                      //e_lfanew
  if (p[0] != 0x00004550) return 0;        //signature
  if ((p[1] & 0xFFFF) != 0x8664) return 0; //machine
  p = &lib[p[34]>>2];                      //EXPORT_DIRECTORY
  if (dir_exp) *dir_exp = p;               //
  return p[6];                             //NumberOfNames
}

const char * libfuncname(uint32_t * lib, int idx){
  uint32_t * p;
  if ((idx < 0) || (idx >= libfuncnum(lib, &p))) return 0; 
  p = &lib[(p[8]>>2) + idx];               //AddressOfNames[idx]
  return &((char*)lib)[*p];   
}

static int codesize_func_arg(char t){
  return 16;
}

static int codesize_func(const char * argt){
  int size = 1024;
  while (*argt) size += codesize_func_arg(*argt++);
  return size;
}

static int codesize_cb_arg(char t){
  return 16;
}

static int codesize_cb(const char * argt){
  int size = 1024;
  while (*argt) size += codesize_cb_arg(*argt++);
  return size;
}

static void pushmetatable(lua_State * L, int(*__call)(lua_State*)){
  lua_createtable(L, 0, 2);
  lua_pushcfunction(L, func__gc);
  lua_setfield(L, -2, "__gc");
  lua_pushcfunction(L, __call);
  lua_setfield(L, -2, "__call");
}

static void pushfunc(lua_State* L, void* lib, void* func, const char* argt) {
  if (argt) {
    int size = codesize_func(argt);
    char* code = lua_newuserdatauv(L, size, 1); //1 uservalue to store lib pointer for FreeLibrary in __gc
    size = make_func(code, func, argt);
    DWORD protect;
    VirtualProtect(code, size, PAGE_EXECUTE_READWRITE, &protect);
    pushmetatable(L, func__call_typed);
    lua_setmetatable(L, -2);
  } else {
    uintptr_t* code = lua_newuserdatauv(L, sizeof(uintptr_t*), 1); //void* userdata to store func pointer, 1 uservalue to store lib pointer for FreeLibrary in __gc
    *code = (uintptr_t)func;
    pushmetatable(L, func__call_auto);
    lua_setmetatable(L, -2);
  }

  if (lib) {
    lua_pushlightuserdata(L, lib);
    lua_setiuservalue(L, -2, 1);
  }
}

int ffi__call(lua_State* L);

int lib__index(lua_State* L) {      // stack: libt, funcname
  lua_pushcfunction(L, ffi__call);  // stack: libt,     func,  ffi_call
  lua_insert(L, 1);                 // stack: ffi_call, libt,   func
  lua_getmetatable(L, 1);           // stack: ffi_call, libt,   func,    metatable
  lua_pushstring(L, "libname");     // stack: ffi_call, libt,   func,    metatable, "libname"
  lua_rawget(L, -2);                // stack: ffi_call, libt,   func,    metatable,  lib
  lua_insert(L, 3);                 // stack: ffi_call, libt,   libname, func,       metatable
  lua_pop(L, 1);                    // stack: ffi_call, libt,   libname, func
  lua_call(L, 3, 1);
  if (lua_type(L, -1) == LUA_TUSERDATA) return 1;
  return 0;
}

int ffi__call(lua_State* L) {
  switch (lua_type(L, 2)) {    //ffi("lib.dll"[,"func"]): [1] - self, [2] - lib [3] - func
    case LUA_TSTRING:{         //lib name
      const char* libname = lua_tostring(L, 2);
      void* lib = LoadLibrary(libname);
      if (lib == 0) return 0;
      switch (lua_type(L, 3)) {
      case LUA_TSTRING: {  //func name
        const char* funcname = lua_tostring(L, 3);
        if (funcname == 0) { FreeLibrary(lib); return 0; }
        if ((funcname[0] == '*') && (funcname[1] == '\0')) {// funcname == "*", load all
          int num = libfuncnum(lib, 0);
          lua_createtable(L, 0, num);
          for (int i = 0; i < num; i++) {
            lib = LoadLibrary(libname);  //loadlibrary for each function to increment internal counter
            if (lib == 0) return 0;
            const char* funcname = libfuncname(lib, i);
            lua_pushstring(L, funcname);                //push key
            void* func = GetProcAddress(lib, funcname);
            if (func == 0) { lua_pop(L, 1); continue; }
            pushfunc(L, lib, func, 0);             //push value
            lua_rawset(L, -3);                    //t[key]=value
          }
          return 1;
        }

        void* func = GetProcAddress(lib, funcname);
        if (func == 0) { FreeLibrary(lib); return 0; }
        pushfunc(L, lib, func, lua_tostring(L, 4));
      }return 1;

      case LUA_TTABLE: {  //list of functions
        int num = lua_rawlen(L, 3);
        lua_createtable(L, 0, lua_rawlen(L, 3));

        for (int i = 0; i < num; i++) {
          lua_rawgeti(L, 3, i + 1);  //push key
          const char * funcname = lua_tostring(L, -1);
          if (funcname == 0) { FreeLibrary(lib); return 0; }
          void * func = GetProcAddress(lib, funcname);
          if (func == 0) { FreeLibrary(lib); return 0; }
          pushfunc(L, lib, func, 0); //push value
          lua_rawset(L, -3);         //t[key]=value
        }
      }return 1;

      case LUA_TNONE:
      case LUA_TNIL:
        //__index
        lua_createtable(L, 0, 0);

        lua_createtable(L, 0, 2);  //metatable
        lua_pushcfunction(L, lib__index);
        lua_setfield(L, -2, "__index");
        lua_pushvalue(L, 2);   //lib name
        lua_setfield(L, -2, "libname");
        lua_setmetatable(L, -2);

        return 1;
      }
    }return 1;

    case LUA_TNUMBER: {         //function pointer
      void* func = lua_tointeger(L, 2);
      if (func == 0) return 0;
      pushfunc(L, 0, func, lua_tostring(L, 3));
    }return 1;

    case LUA_TLIGHTUSERDATA: {  //function pointer
      void* func = lua_touserdata(L, 2);
      if (func = 0) return 0;
      pushfunc(L, 0, func, lua_tostring(L, 3));
    }return 1;

    case LUA_TFUNCTION:{       //callback
      lua_pushvalue(L, 2);                         //make reference to lua function to store it in user value.
      int ref = luaL_ref(L, LUA_REGISTRYINDEX);
      const char* argt = lua_tostring(L, 3);
      int size = codesize_cb(argt);
      char* code = lua_newuserdatauv(L, size, 1); //1 uservalue to store lua function reference for luaL_unref, for __gc
      lua_pushinteger(L, ref);
      lua_setiuservalue(L, -2, 1);
      make_cb(code, L, ref, argt);
      DWORD protect;
      VirtualProtect(code, size, PAGE_EXECUTE_READWRITE, &protect);
      pushmetatable(L, func__call_cb);
      lua_setmetatable(L, -2);
    }return 1;
  }
  return 0;
}
/*
int ffi__call(lua_State * L){
  int arg1 = lua_type(L, 2);  //ffi("lib.dll"[,"func"]): [1] - self, [2] - lib [3] - func [4] argt
  if (arg1 == LUA_TSTRING){
    int arg2 = lua_type(L, 3);
    if (arg2 == LUA_TSTRING){  //func name
      void * lib = LoadLibrary(lua_tostring(L, 2));
      if (lib == 0) return 0;
      void * func = GetProcAddress(lib, lua_tostring(L, 3));
      if (func == 0) { FreeLibrary(lib); return 0; }
      const char * argt = lua_tostring(L, 4);
      if (argt){  //arg type provided
        int size = codesize(argt);
        char * code = lua_newuserdatauv(L, size, 1); //1 uservalue to store lib pointer for FreeLibrary in __gc
        lua_pushlightuserdata(L, lib);
        lua_setiuservalue(L, -2, 1);
        size = make_func(code, func, argt);
        DWORD protect;
        VirtualProtect(code, size, PAGE_EXECUTE_READWRITE, &protect);
        func_setmetatable(L, func__call_typed);
      }else{      //automatic arg types from lua types
        uintptr_t * code = lua_newuserdatauv(L, sizeof(uintptr_t*), 1); //void* userdata to store func pointer, 1 uservalue to store lib pointer for FreeLibrary in __gc
        *code = (uintptr_t)func;
        lua_pushlightuserdata(L, lib);
        lua_setiuservalue(L, -2, 1);
        func_setmetatable(L, func__call_auto);
      }
    } else {  //no function name, table with names or import all
      int num;
      if (arg2 == LUA_TTABLE) num = lua_rawlen(L, 3); 
      else {
        void * lib = LoadLibrary(lua_tostring(L, 2));
        num = libfuncnum(lib, 0);
        if (lib) FreeLibrary(lib);  //loadlibrary for every function, to automatically freelibrary in __gc
      }
      lua_createtable (L, 0, num);  
      for (int i = 0; i < num; i++){
        void * lib = LoadLibrary(lua_tostring(L, 2));
        if (lib) {
          if (arg2 == LUA_TTABLE) lua_rawgeti(L, 3, i + 1); else lua_pushstring(L, libfuncname(lib, i));
          void* func = GetProcAddress(lib, lua_tostring(L, -1));
          if (func == 0) {
            lua_pop(L, 1);
            continue;
          }
          uintptr_t* code = lua_newuserdatauv(L, sizeof(uintptr_t*), 1); //1 uservalue to store lua function reference for luaL_unref, for __gc
          *code = (uintptr_t)func;
          lua_pushlightuserdata(L, lib);
          lua_setiuservalue(L, -2, 1);
          func_setmetatable(L, func__call_auto);
          lua_settable(L, -3);
        }
        else {
          lua_pop(L, 1);
          return 0;
        }
      }
    }
    return 1;
  } 
  else if (arg1 == LUA_TLIGHTUSERDATA) {
    const char* argt = lua_tostring(L, 3);
    void* func = lua_touserdata(L, 2);
    if (func == 0) return 0;
    if (argt) {  //arg type provided
      int size = codesize(argt);
      char* code = lua_newuserdatauv(L, size, 0);
      make_func(code, func, argt);
      DWORD protect;
      VirtualProtect(code, size, PAGE_EXECUTE_READWRITE, &protect);
      func_setmetatable(L, func__call_typed);
    }
    else {      //automatic arg types from lua types
      uintptr_t* code = lua_newuserdatauv(L, sizeof(uintptr_t*), 0); //void* userdata to store func pointer
      *code = (uintptr_t)func;
      func_setmetatable(L, func__call_auto);
    }
    return 1;
  } 
  else if (arg1 == LUA_TFUNCTION) { // callback
    const char * argt = lua_tostring(L, 3);
    int size = codesize_cb(argt);
    char * code = lua_newuserdatauv(L, size, 1); //1 uservalue to store lua function reference for luaL_unref, for __gc
    lua_pushvalue(L, 2);                         //make reference to lua function and store it in user value.
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushinteger(L, ref);
    lua_setiuservalue(L, -2, 1);
    make_cb(code, L, ref, argt);
    DWORD protect;
    VirtualProtect(code, size, PAGE_EXECUTE_READWRITE, &protect);
    func_setmetatable(L, func__call_cb);
    return 1;
  }
  return 0;
}
*/

int ffi_float(lua_State* L) {   //__call_auto func returns two values: integer 'rax' and double 'xmm0'
  if (lua_isinteger(L, 1) && lua_isnumber(L, 2)) lua_pushnumber(L, unpackfloat(lua_tonumber(L, 2)));
  else lua_pushnumber(L, packfloat(lua_tonumber(L, 1)));
  return 1;
}

int ffi_double(lua_State* L) {
  lua_pushvalue(L, 2);
  return 1;
}

int ffi_int(lua_State* L) {
  lua_pushvalue(L, 1);
  return 1;
}

int ffi_string(lua_State* L) {
  if (lua_isinteger(L, 2)) lua_pushlstring(L, (char*)lua_tointeger(L, 1), lua_tointeger(L, 2));
  else lua_pushstring(L, (char*)lua_tointeger(L, 1));
  return 1;
}

int ffi_pointer(lua_State* L) {
  if (lua_isinteger(L, 1)) {
    lua_pushlightuserdata(L, lua_tointeger(L, 1));
    return 1;
  }
  uint64_t* p = lua_touserdata(L, 1);
  return p ? (lua_pushinteger(L, *p), 1) : 0;
}

int ffi_boolean(lua_State* L) {
  lua_pushboolean(L, lua_tointeger(L, 1));
  return 1;
}

int userdata_string(lua_State* L) {
  int len = lua_tointeger(L, 2);
  if ((len <= 0) || (len > lua_rawlen(L, 1))) len = lua_rawlen(L, 1);
  lua_pushlstring(L, lua_touserdata(L, 1), len);
  return 1;
}

int userdata_int(lua_State* L) {
  int64_t i = 0;
  int len = lua_rawlen(L, 1);
  int size = len;
  if (lua_isnumber(L, 2)) size = lua_tointeger(L, 2);
  if (size > len) size = len;
  if (size <= 0) return 0;
  if (size > 8) size = 8;
  memcpy(&i, lua_touserdata(L, 1), size);
  lua_pushinteger(L, i);
  return 1;
}

int userdata_boolean(lua_State* L) {
  char* p = lua_touserdata(L, 1);
  if (p == 0) return 0;
  uint64_t i = 0;
  int len = lua_tointeger(L, 2);
  if (len > 8) len = 8;
  if (len <= 0) len = 1;
  memcpy(&i, p, len);
  lua_pushboolean(L, i != 0);
  return 1;
}

int userdata_float(lua_State* L) {
  float * p = lua_touserdata(L, 1);
  return p ? (lua_pushnumber(L, *p), 1) : 0;
}

int userdata_double(lua_State* L) {
  double * p = lua_touserdata(L, 1);
  return p ? (lua_pushnumber(L, *p), 1) : 0;
}

int userdata_pack(lua_State* L) {  
  void* d = lua_touserdata(L, 1);
  lua_getglobal(L, "string");
  lua_getfield(L, -1, "pack");
  lua_rotate(L, 2, 1);  //stack: [1]=self, [2]=string.pack, [3]=fmt, [4...]=values, ...  [-1]=string
  lua_pop(L, 1);         //"string"
  lua_call(L, lua_gettop(L) - 2, 1);
  const char* s = lua_tostring(L, -1);
  if (s) memcpy(d, s, lua_rawlen(L, -1));
  lua_pop(L, 1);        //remove result of string.pack and return self
  return 1;
}

int userdata_unpack(lua_State* L) {
  lua_getglobal(L, "string");
  lua_getfield(L, -1, "unpack");
  lua_rotate(L, 2, 1);  //stack: [1]=self, [2]=string.unpack, [3]=fmt, [4]=pos, ...[-1]=string
  lua_pop(L, 1);         //"string"
  lua_pushlstring(L, lua_touserdata(L, 1), lua_rawlen(L, 1));
  lua_insert(L, 4);
  lua_call(L, lua_gettop(L) - 2, 1);
  return 1;
}

int userdata__index(lua_State* L) {  //array indexing
//!!!!!!!!!!!!!!!!!!!!!!!!!!!
  uint8_t* p = lua_touserdata(L, 1);
  size_t offset = lua_tointeger(L, 2);
  uint8_t* ud = lua_newuserdatauv(L, 0, 2);  //uservalue : parent & offset
  return 1;
}

//uservalues of userdata: [1] - type, [2] - sizeof element in bytes, 3.. dimensions (elements, not bytes)
int ffi_userdata(lua_State* L) {
  if (lua_type(L, 1) == LUA_TNUMBER) {
    int len = lua_tointeger(L, 1);
    int depth = lua_gettop(L);
    for (int i = 2; i <= depth; i++) {
      if (LUA_TNUMBER == lua_type(L, i)) len *= lua_tointeger(L, i);
      else {
        depth = i - 1;
        break;
      }
    }
    if (len > 0) {
      memset(lua_newuserdatauv(L, len, depth + 2), 0, len);
    } else return 0;

    lua_pushinteger(L, 'i');      //array type, integer by default
    lua_setiuservalue(L, -2, 1);

    lua_pushinteger(L, 1);       //element size 1 by default
    lua_setiuservalue(L, -2, 2);

    for (int i = 1; i <= depth; i++) {
      lua_pushvalue(L, i);
      lua_setiuservalue(L, -2, i + 2);
    }
  }

  if (lua_type(L, 1) == LUA_TSTRING) {
    memcpy(lua_newuserdatauv(L, lua_rawlen(L, 1), 1), lua_tostring(L, 1), lua_rawlen(L, 1));
    lua_pushinteger(L, 's');      //userdata type string
    lua_setiuservalue(L, -2, 1);
  }

  if (lua_type(L, 1) == LUA_TTABLE) {
    size_t len = lua_rawlen(L, 1);
    uintptr_t * data = lua_newuserdatauv(L, sizeof(uintptr_t) * len, len); //array store pointers also in uservalues to prevent GC
    for (size_t i = 0; i < len; i++) {
      lua_pushcfunction(L, ffi_userdata);
      lua_rawgeti(L, 1, i + 1);
      lua_call(L, 1, 1);
      data[i] = lua_touserdata(L, -1);
      lua_setiuservalue(L, -2, i + 1);
    }
  }

  lua_createtable(L, 0, 2); //userdata metatable
  lua_pushcfunction(L, userdata_string);
  lua_setfield(L, -2, "__tostring");

  lua_createtable(L, 0, 7);  //__index table
  lua_pushcfunction(L, userdata_string);
  lua_setfield(L, -2, "string");
  lua_pushcfunction(L, userdata_int);
  lua_setfield(L, -2, "int");
  lua_pushcfunction(L, userdata_boolean);
  lua_setfield(L, -2, "bool");
  lua_pushcfunction(L, userdata_float);
  lua_setfield(L, -2, "float");
  lua_pushcfunction(L, userdata_double);
  lua_setfield(L, -2, "double");
  lua_pushcfunction(L, userdata_pack);
  lua_setfield(L, -2, "pack");
  lua_pushcfunction(L, userdata_unpack);
  lua_setfield(L, -2, "unpack");

  lua_createtable(L, 0, 1);  //metatable for __index which is called if no metamethod is found (e.g. for x[1])
  lua_pushcfunction(L, userdata__index);
  lua_setfield(L, -2, "__index");
  lua_setmetatable(L, -2);

  lua_setfield(L, -2, "__index");

  lua_setmetatable(L, -2);
  return 1;
}

int ffi__index(lua_State* L) {
  lua_pushstring(L, "libs");
  int type = lua_rawget(L, 1);
  if (type == LUA_TTABLE) {
    int len = lua_rawlen(L, -1);
    if (len <= 0) return 0;
    for (int i = 0; i < len; i++) {
      lua_pushcfunction(L, ffi__call);  //ffi_call
      lua_pushvalue(L, 1);             //ffi self
      lua_rawgeti(L, -3, i + 1);       //lib
      lua_pushvalue(L, 2);             //func
      lua_call(L, 3, 1);
      if (lua_type(L, -1) == LUA_TUSERDATA) return 1;
    }
  }
  else if (type == LUA_TSTRING) {
    lua_pushcfunction(L, ffi__call);  // stack: ffi,      func,  lib,   ffi_call
    lua_insert(L, 1);                 // stack: ffi_call, ffi,   func,  lib
    lua_insert(L, 3);                 // stack: ffi_call, ffi,   lib,   func
    lua_call(L, 3, 1);
    if (lua_type(L, -1) == LUA_TUSERDATA) return 1;
  }
  return 0;
}
//extern "C" 
__declspec(dllexport) int luaopen_ffi(lua_State * L){
  lua_newtable(L);  //lib

  lua_pushcfunction(L, ffi_string);
  lua_setfield(L, -2, "string");
  lua_pushcfunction(L, ffi_int);
  lua_setfield(L, -2, "int");
  lua_pushcfunction(L, ffi_float);
  lua_setfield(L, -2, "float");
  lua_pushcfunction(L, ffi_double);
  lua_setfield(L, -2, "double");
  lua_pushcfunction(L, ffi_boolean);
  lua_setfield(L, -2, "bool");
  lua_pushcfunction(L, ffi_pointer);
  lua_setfield(L, -2, "pointer");
  lua_pushcfunction(L, ffi_userdata);
  lua_setfield(L, -2, "userdata");

  lua_newtable(L);  //metatable
  lua_pushcfunction(L, ffi__call);
  lua_setfield(L, -2, "__call");
  lua_pushcfunction(L, ffi__index);
  lua_setfield(L, -2, "__index");

  lua_setmetatable(L, -2);
  return 1;
}

#ifndef _DEBUG
int _fltused = 0;
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved){ return TRUE; }
#endif