#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

int ffi__call(lua_State* L);

//call.asm
int func__call_auto(lua_State* L);

//userdata.c
int ffi_userdata(lua_State* L);


//win64.c
int mprotect_exec(void* addr, size_t len);
int mprotect_noexec(void* addr, size_t len);

void* libopen(const char* filename);
void* libsym(void* handle, const char* symbol);
int libclose(void* handle);
const char* libsymname(uint32_t* lib, uint32_t idx);
int libsymnum(uint32_t* lib);

int make_func(char* code, const void* func, const char* argt);
int make_cb(char* code, lua_State* L, int ref, const char* argt);

int ispackedfloat(double x){ return (((*(uint64_t*)&x) & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000); }
double packfloat(float x)  { uint64_t u = 0xFFFFFFFF00000000 | *(uint32_t*)&x; return *(double*)&u; }
float unpackfloat(double x){ return *(float*)&x; }

void voidfunc() {}
int luaF_isfloat (lua_State * L, int idx){ return ((lua_type(L, idx) == LUA_TNUMBER) && (!lua_isinteger(L, idx))); }
void luaF_tonil(lua_State * L, int idx){}
float luaF_tofloat(lua_State* L, int idx) { return unpackfloat(lua_tonumber(L, idx)); }
void* luaF_topointer(lua_State * L, int idx) { return lua_isinteger(L, idx) ? (void*)lua_tointeger(L, idx) : lua_touserdata(L, idx); }

void* luaF_totable(lua_State* L, int idx) { 
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
    const char ** data = lua_newuserdatauv(L, num * sizeof(char*), 0);
    for (size_t i = 0; i < num; i++) {
      lua_rawgeti(L, idx, i + 1);
      data[i] = lua_tostring(L, -1);
    }
    return data;
  }

///////////////////!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!/////////////////////////////
  if (lua_isinteger(L, -1)) {
    lua_pop(L, 1);
    size_t num = lua_rawlen(L, idx);
    uint8_t * data = lua_newuserdatauv(L, num * sizeof(char), 0);
    for (size_t i = 0; i < num; i++) {
      lua_rawgeti(L, idx, i + 1);
      data[i] = (uint8_t)lua_tointeger(L, -1);
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
    case LUA_TNUMBER:        return lua_isinteger(L, idx) ? (void*)lua_tointegerx : ispackedfloat(lua_tonumber(L, idx)) ? (void*)luaF_tofloat : (void*)lua_tonumberx;
    case LUA_TSTRING:        return lua_tolstring;
    case LUA_TTABLE:         return luaF_totable;  //luaL_error(L, "ffi: type of argument %d is 'table'", idx); return luaF_tonil;
    case LUA_TFUNCTION:      break;
    case LUA_TUSERDATA:      return lua_touserdata;
    case LUA_TTHREAD:        break;
  }
  luaL_error(L, "ffi: type of argument %d unknown type %d", idx, t);
  return luaF_tonil;
}

void * luaF_to  (const char t){ 
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

void * luaF_push  (const char t){
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

int func__call_typed(lua_State * L){
  int (*f)(lua_State *) = lua_touserdata(L, 1);
  return f ? f(L) : 0;
}

int func__call_cb(lua_State * L){ //get rid of self argument and call lua function by reference from uservalue
  lua_getiuservalue(L, 1, 1);
  if (lua_type(L, -1) == LUA_TNUMBER) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, lua_tointeger(L, -1));
    lua_insert(L, 2);             //put lua callback function above self
    lua_pop(L, 1);                //pop ref
    int num = lua_gettop(L) - 2;  //+2: [1] = self, [2] func
    lua_call(L, num, LUA_MULTRET);
    return lua_gettop(L) - num;
  } 
  lua_pop(L, 1);
  return 0;
}

int func__gc(lua_State * L){
  lua_getiuservalue(L, 1, 1);
  if (lua_islightuserdata(L, -1)){
    void * lib = lua_touserdata(L, -1);
    if (lib) libclose(lib);
  } 
  if (lua_isinteger(L, -1)){
    int ref = (int)lua_tointeger(L, -1);
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
  }
  lua_pop(L, 1);
//  mprotect_noexec(lua_touserdata(L, 1), lua_rawlen(L, 1));
  return 0;
}

static int codesize_func_arg(char t){
  return 0;
}

static int codesize_func(const char * argt){
  int size = 1024;
  while (*argt) size += codesize_func_arg(*argt++);
  return size;
}

static int codesize_cb_arg(char t){
  return 0;
}

static int codesize_cb(const char * argt){
  int size = 1024;
  while (*argt) size += codesize_cb_arg(*argt++);
  return size;
}

static void pushfuncmetatable(lua_State * L, int(*__call)(lua_State*)){
  lua_createtable(L, 0, 2);
  lua_pushcfunction(L, func__gc);
  lua_setfield(L, -2, "__gc");
  lua_pushcfunction(L, __call);
  lua_setfield(L, -2, "__call");
}

//generate code for library function call and push it as userdata on lua stack
static void pushfunc(lua_State* L, void* lib, void* func, const char* argt) {
  if (argt) {
    int size = codesize_func(argt);
    char* code = lua_newuserdatauv(L, size, 1); //1 uservalue to store lib pointer for FreeLibrary in __gc
    pushfuncmetatable(L, func__call_typed);
    lua_setmetatable(L, -2);
    size = make_func(code, func, argt);
    mprotect_exec(code, size);
  } else {
    uintptr_t* code = lua_newuserdatauv(L, sizeof(uintptr_t*), 1); //void* userdata to store func pointer, 1 uservalue to store lib pointer for FreeLibrary in __gc
    pushfuncmetatable(L, func__call_auto);
    lua_setmetatable(L, -2);
    *code = (uintptr_t)func;
  }
  if (lib) {
    lua_pushlightuserdata(L, lib);
    lua_setiuservalue(L, -2, 1);
  }
}

int lib__index(lua_State* L) {      // stack: self,     func
  lua_getmetatable(L, 1);           // stack: self,     func,    metatable
  lua_pushstring(L, "lib");         // stack: self,     func,    metatable, "lib"
  int libtype = lua_rawget(L, -2);  // stack: self,     func,    metatable,  libt
  lua_insert(L, 2);                 // stack: self,     libt,     func,          metatable
  lua_pop(L, 1);                    // stack: self,     libt,     func

  int len = 0;
  if (libtype == LUA_TTABLE)  len = (int)lua_rawlen(L, 2);
  if (libtype == LUA_TSTRING) len = 1;

  for (int i = 0; i < len; i++) {
    lua_pushcfunction(L, ffi__call);  // stack: self,     libt,    func,    ffi_call
    lua_pushvalue(L, 1);              // stack: self,     libt,    func,    ffi_call,  self

    if (libtype == LUA_TSTRING) lua_pushvalue(L, 2);
                                      // stack: self,     lib,     func,    ffi_call,  self,   lib
    if (libtype == LUA_TTABLE)  lua_rawgeti(L, 2, i + 1);
                                      // stack: self,     libt,    func,    ffi_call,  self,   lib
    lua_pushvalue(L, 3);              // stack: self,     libt,    func,    ffi_call,  self,   lib,   func
    lua_call(L, 3, 1);                // stack: self,     libt,    func,    code
    if (lua_type(L, -1) == LUA_TUSERDATA) {
      lua_pushvalue(L, -1);           // stack: self,     libt,    func,    code,      code
      lua_insert(L, 3);               // stack: self,     libt,    code,    func,      code, 
      lua_settable(L, 1);             // self[func] = code    // stack: self,    libt,   code
      return 1;
    }
    else lua_pop(L, 1);
  }
  return 0;
}

int ffi__call(lua_State* L) {
  switch (lua_type(L, 2)) {    //ffi("lib.dll"[, "func"][, "typestr"]): [1] - self, [2] - lib, [3] - func, [4] - types
    case LUA_TSTRING:{         //lib name
      const char* libname = lua_tostring(L, 2);
      switch (lua_type(L, 3)) {
        case LUA_TNONE:
        case LUA_TNIL: {
          //return empty table with __index metamethod that load functions when first called (indexed)
          lua_createtable(L, 0, 0);               //self, lib, {}
          lua_createtable(L, 0, 2);               //self, lib, {}, mt
          lua_pushcfunction(L, lib__index);       //self, lib, {}, mt, __index
          lua_setfield(L, -2, "__index");         //self, lib, {}, mt
          lua_pushvalue(L, 2);   //lib name       //self, lib, {}, mt, lib
          lua_setfield(L, -2, "lib");             //self, lib, {}, mt
          lua_setmetatable(L, -2);                //self, lib, {}
          return 1;
        }
        case LUA_TSTRING: {   //func name
          const char* funcname = lua_tostring(L, 3);
          if ((funcname[0] == '*') && (funcname[1] == '\0')) {// funcname == "*", load all
            void* lib = libopen(libname);
            if (lib == 0) return 0;
            int num = libsymnum(lib);
            libclose(lib);
            lua_createtable(L, 0, num);
            for (int i = 0; i < num; i++) {
              void* lib = libopen(libname);  //loadlibrary for each function to increment internal counter
              if (lib == 0) return 0;
              const char* funcname = libsymname(lib, i);
              if (funcname == 0) { libclose(lib); return 0; }
              void* func = libsym(lib, funcname);
              if (func == 0) { libclose(lib); continue; }
              lua_pushstring(L, funcname);           //push key
              pushfunc(L, lib, func, 0);             //push value
              lua_rawset(L, -3);                     //t[key]=value
            }
            return 1;
          }
          void* lib = libopen(libname);
          if (lib == 0) return 0;
          void* func = libsym(lib, funcname);
          if (func == 0) { libclose(lib); return 0; }
          if (lua_type(L, 4) == LUA_TNUMBER) {
            lua_pushcfunction(L, ffi_userdata);
            lua_pushlightuserdata(L, func);
            lua_pushvalue(L, 4);
            lua_call(L, 1, 1);
          } else {
            pushfunc(L, lib, func, lua_tostring(L, 4));
          }
        } return 1;

        case LUA_TTABLE: {  //list of functions
          int num = (int)lua_rawlen(L, 3);
          lua_createtable(L, 0, num);
          for (int i = 0; i < num; i++) {
            void* lib = libopen(libname);
            if (lib == 0) return 0;
            lua_rawgeti(L, 3, i + 1);  //push key
            const char * funcname = lua_tostring(L, -1);
            if (funcname == 0) { lua_pop(L, 1); libclose(lib); return 0; }
            void * func = libsym(lib, funcname);
            if (func == 0) { lua_pop(L, 1); libclose(lib); return 0; }
            pushfunc(L, lib, func, 0); //push value
            lua_rawset(L, -3);         //t[key]=value
          }
        } return 1;
      }
    } return 0;

    case LUA_TNUMBER: {         //function pointer
      void* func = (void*)lua_tointeger(L, 2);
      if (func == 0) return 0;
      pushfunc(L, 0, func, lua_tostring(L, 3));
    } return 1;

    case LUA_TUSERDATA:         //function pointer
    case LUA_TLIGHTUSERDATA: {  //function pointer
      void* func = lua_touserdata(L, 2);
      if (func == 0) return 0;
      pushfunc(L, 0, func, lua_tostring(L, 3));
    } return 1;

    case LUA_TFUNCTION:{       //callback
      lua_pushvalue(L, 2);                         //make reference to lua function to store it in user value.
      int ref = luaL_ref(L, LUA_REGISTRYINDEX);
      const char* argt = lua_tostring(L, 3);
      int size = codesize_cb(argt);
      char* code = lua_newuserdatauv(L, size, 1); //1 uservalue to store lua function reference for luaL_unref, for __gc
      lua_pushinteger(L, ref);
      lua_setiuservalue(L, -2, 1);
      pushfuncmetatable(L, func__call_cb);
      lua_setmetatable(L, -2);
      make_cb(code, L, ref, argt);
      mprotect_exec(code, size);
    } return 1;
  }
  return 0;
}

int ffi_float(lua_State* L) {   //__call_auto func returns two values: integer 'rax' and double 'xmm0'
  if (lua_isinteger(L, 1) && lua_isnumber(L, 2)) lua_pushnumber(L, unpackfloat(lua_tonumber(L, 2)));
  else lua_pushnumber(L, packfloat((float)lua_tonumber(L, 1)));
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
    lua_pushlightuserdata(L, (void*)lua_tointeger(L, 1));
    return 1;
  }
  uint64_t* p = lua_touserdata(L, 1);
  return p ? (lua_pushinteger(L, *p), 1) : 0;
}

int ffi_bool(lua_State* L) {
  lua_pushboolean(L, (int)lua_tointeger(L, 1));
  return 1;
}

int userdata_pointer(lua_State* L);
int userdata_string(lua_State* L);
int userdata_int(lua_State* L);
int userdata_float(lua_State* L);
int userdata_double(lua_State* L);
int userdata_bits(lua_State* L);
int userdata_pack(lua_State* L);
int userdata_unpack(lua_State* L);
int userdata_type(lua_State* L);
int userdata__index(lua_State* L);

//#define X(x) {ffi_##x,#x}
//const luaL_Reg lib[] = { X(string), X(int), X(float), X(double), X(bool), X(pointer), X(userdata), { 0, 0 } };

//extern "C" 
__declspec(dllexport) int luaopen_ffi(lua_State * L){
//  luaL_newlib(L, lib);
  
  lua_createtable(L, 0, 7);  //lib
    lua_pushcfunction(L, ffi_string);
    lua_setfield(L, -2, "string");
    lua_pushcfunction(L, ffi_int);
    lua_setfield(L, -2, "int");
    lua_pushcfunction(L, ffi_float);
    lua_setfield(L, -2, "float");
    lua_pushcfunction(L, ffi_double);
    lua_setfield(L, -2, "double");
    lua_pushcfunction(L, ffi_bool);
    lua_setfield(L, -2, "bool");
    lua_pushcfunction(L, ffi_pointer);
    lua_setfield(L, -2, "pointer");
  
    lua_createtable(L, 0, 8);  //ffi.userdata table
      lua_pushcfunction(L, userdata_pointer);
      lua_setfield(L, -2, "pointer");
      lua_pushcfunction(L, userdata_string);
      lua_setfield(L, -2, "string");
      lua_pushcfunction(L, userdata_int);
      lua_setfield(L, -2, "int");
      lua_pushcfunction(L, userdata_float);
      lua_setfield(L, -2, "float");
      lua_pushcfunction(L, userdata_double);
      lua_setfield(L, -2, "double");
      lua_pushcfunction(L, userdata_bits);
      lua_setfield(L, -2, "bits");
      lua_pushcfunction(L, userdata_pack);
      lua_setfield(L, -2, "pack");
      lua_pushcfunction(L, userdata_unpack);
      lua_setfield(L, -2, "unpack");
      lua_pushcfunction(L, userdata_type);
      lua_setfield(L, -2, "type");

      lua_createtable(L, 0, 1); //ffi.userdata metatable
        lua_pushcfunction(L, ffi_userdata);
        lua_setfield(L, -2, "__call");
//        lua_pushcfunction(L, userdata__index);
//        lua_setfield(L, -2, "__index");
      lua_setmetatable(L, -2);

    lua_setfield(L, -2, "userdata");

    lua_createtable(L, 0, 1); //ffi metatable
      lua_pushcfunction(L, ffi__call);
      lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);

  return 1;
}
