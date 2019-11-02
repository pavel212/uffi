#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <windows.h>
#include <stdlib.h>

typedef signed char       int8_t;
typedef signed short      int16_t;
typedef signed int        int32_t;
typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int      uint32_t;

//inline int isspace(char c){ return (c == ' ') || (c == '\t') || (c == '\n'); }

int call(lua_State * L){ return ((int (*)(lua_State *))luaL_checkudata(L, 1, "binary code"))(L); } 

int gc(lua_State * L){
  if (lua_getmetatable(L, 1)){
    lua_getfield(L, -1, "HMODULE");
    HMODULE lib = (HMODULE) lua_touserdata(L, -1);
    if (lib) FreeLibrary(lib);

    lua_getfield(L, -1, "PROTECT");
    DWORD protect = (DWORD)lua_tointeger(L, -1);

    lua_getfield(L, -1, "SIZE");
    size_t size = (size_t) lua_tointeger(L, -1);

    VirtualProtect(lua_touserdata(L, 1), size, protect, &protect);
//    VirtualFree(lua_touserdata(L, 1), size, MEM_RELEASE);  
    lua_pop(L, 4);
  }
  return 0;
}

#define _push_eax(p)           {*p++ = 0x50;}
#define _push_ecx(p)           {*p++ = 0x51;}
#define _push_edx(p)           {*p++ = 0x52;}
#define _push_ebx(p)           {*p++ = 0x53;}
#define _push_ebp(p)           {*p++ = 0x55;}
#define _push_esi(p)           {*p++ = 0x56;}
#define _push_edi(p)           {*p++ = 0x57;}
#define _pop_ebx(p)            {*p++ = 0x5B;}
#define _pop_ebp(p)            {*p++ = 0x5D;}
#define _pop_esi(p)            {*p++ = 0x5E;}
#define _pop_edi(p)            {*p++ = 0x5F;}
#define _nop(p)                {*p++ = 0x90;}
#define _call_eax(p)           {*p++ = 0xFF; *p++ = 0xD0;}
#define _mov_ebp_esp(p)        {*p++ = 0x8B; *p++ = 0xEC;}
#define _mov_esp_ebp(p)        {*p++ = 0x8B; *p++ = 0xE5;}
#define _ret(p)                {*p++ = 0xC3;}
#define _sub_esp_uint8(p, x)   {*p++ = 0x83; *p++ = 0xEC; *p++ = x;}
#define _add_esp_uint8(p,x)    {*p++ = 0x83; *p++ = 0xC4; *p++ = x;}
#define _mov_eax_uint32(p,x)   {*p++ = 0xB8; *p++ = x;    *p++ = (x)>>8; *p++ = (x)>>16; *p++ = (x)>>24;}
#define _push_uint8(p,x)       {*p++ = 0x6A; *p++ = x;}
#define _push_uint32(p,x)      {*p++ = 0x68; *p++ = x;    *p++ = (x)>>8; *p++ = (x)>>16; *p++ = (x)>>24;}

/*
#define X(op) inline void op(unsigned char ** p)
X(_push_eax)    {*(*p)++ = 0x50;}
X(_push_ecx)    {*(*p)++ = 0x51;}
X(_push_edx)    {*(*p)++ = 0x52;}
X(_push_ebx)    {*(*p)++ = 0x53;}
//X(_push_ebp)    {*(*p)++ = 0x55;}
X(_push_esi)    {*(*p)++ = 0x56;}
X(_push_edi)    {*(*p)++ = 0x57;}
X(_pop_ebx)     {*(*p)++ = 0x5B;}
X(_pop_ebp)     {*(*p)++ = 0x5D;}
X(_pop_esi)     {*(*p)++ = 0x5E;}
X(_pop_edi)     {*(*p)++ = 0x5F;}
X(_nop)         {*(*p)++ = 0x90;}
X(_call_eax)    {*(*p)++ = 0xFF; *(*p)++ = 0xD0;}
X(_mov_ebp_esp) {*(*p)++ = 0x8B; *(*p)++ = 0xEC;}
X(_mov_esp_ebp) {*(*p)++ = 0x8B; *(*p)++ = 0xE5;}
X(_ret)         {*(*p)++ = 0xC3;}
#undef X

#define X(op,arg) inline void op(unsigned char**p, arg)
X(_sub_esp_uint8, uint8_t x)   {*(*p)++ = 0x83; *(*p)++ = 0xEC; *(*p)++ = x;}
X(_add_esp_uint8, uint8_t x)   {*(*p)++ = 0x83; *(*p)++ = 0xC4; *(*p)++ = x;}
X(_mov_eax_uint32, uint32_t x) {*(*p)++ = 0xB8; *(*p)++ = x;    *(*p)++ = x>>8; *(*p)++ = x>>16; *(*p)++ = x>>24;}
X(_push_uint8, uint8_t x)      {*(*p)++ = 0x6A; *(*p)++ = x;}
X(_push_uint32, uint32_t x)    {*(*p)++ = 0x68; *(*p)++ = x;    *(*p)++ = x>>8; *(*p)++ = x>>16; *(*p)++ = x>>24;}
#undef X
*/

enum ARG_TYPE {TYPE_VOID, TYPE_INT8, TYPE_INT16, TYPE_INT32, TYPE_INT64, TYPE_FLOAT, TYPE_DOUBLE, TYPE_STRING, TYPE_POINTER};

static enum ARG_TYPE get_type(const char * begin, const char * end){
  const char * p;

  p = strpbrk(begin, "*[");
  if (p && p <= end){
    p = strstr(begin, "char");
    if (p && p <= end) return TYPE_STRING;
    return TYPE_POINTER;
  }

  p = strstr(begin, "char");
  if (p && p <= end) return TYPE_INT8;

  p = strstr(begin, "short");
  if (p && p <= end) return TYPE_INT16;

  p = strstr(begin, "int");
  if (p && p <= end) return TYPE_INT32;

  p = strstr(begin, "long int");
  if (p && p <= end) return TYPE_INT64;

  p = strstr(begin, "string");
  if (p && p <= end) return TYPE_STRING;

  p = strstr(begin, "float");
  if (p && p <= end) return TYPE_FLOAT;

  p = strstr(begin, "double");
  if (p && p <= end) return TYPE_DOUBLE;
  
  return TYPE_VOID;
}

static enum ARG_TYPE func_ret_type(const char * str, const char ** end){
  const char * p = str;
  while (*p && *p != '(') p++;
  p--;
  while (p > str && isspace(*p)) p--;
  while (p > str && (!isspace(*p)) && *p != '*') p--;
  if (end) *end = p;
  return get_type(str, p);
}

static const char * func_name(const char * str, const char ** end){
  const char * p = str;
  while (*p && *p != '(') p++;
  p--;
  while (p > str && isspace(*p)) p--;
  if (end) *end = p+1;
  while (p > str && (!isspace(*p)) && *p != '*') p--;
  p++;
  return p;
}

static enum ARG_TYPE func_arg_type(const char * str, int num){
  const char * p = str;
  const char * begin = str;
  int argc = 0;
  while (*p && *p != '(') p++;
  p++;
  while (*p && *p != ')') {
    if (argc == 0 && !isspace(*p)) {argc = 1; begin = p;}
    if (*p == ',') {argc += 1;
      if (argc-2 == num) return get_type(begin, p);
      else begin = p + 1;
    }
    p++;
  }
  return get_type(begin, p);
}

static int func_arg_num(const char * str){
  const char * p = str;
  int argc = 0;
  while (*p && *p != '(') p++;
  p++;
  while (*p && *p != ')') {
    if (argc == 0 && !isspace(*p)) argc = 1;
    if (*p == ',') argc += 1;
    p++;
  }
  return argc;
}

//parse C function declaration, return userdata with __call metatable set to call
int load(lua_State * L){

  const char * func_str = luaL_checkstring(L, 1);
  int argc = func_arg_num(func_str);
  int ret_type = func_ret_type(func_str, 0);
  int ret_num = 0;
  const char *end, *n = func_name(func_str, &end);
  char name[256];
  memcpy(name, n, min(end - n, 256));
  name[end - n] = 0;
  const char * lib_str = luaL_checkstring(L, 2);
  HMODULE lib = NULL;
  void * func = NULL;
  if (0 == strcmp(lib_str, "wgl")){
    func = wglGetProcAddress(name);
    if (NULL == func) {//error
      return 0;
    }
  } else {
    lib = LoadLibrary(lib_str);
    if (NULL == lib) return 0;
    func = GetProcAddress(lib, name);
    if (NULL == func) {//error
      FreeLibrary(lib);
      return 0;
    }
  }

//  unsigned char * program = (unsigned char *) VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);  
  unsigned char * program = (unsigned char *) malloc(256);  
//  unsigned char * program = (unsigned char *)lua_newuserdata(L, 256);
  unsigned char * p = program;
           
  _push_ebp(p);
  _mov_ebp_esp(p);
  _sub_esp_uint8(p, 0x40);
//  _push_ebx(p);
//  _push_esi(p);
//  _push_edi(p);

  for (uint8_t i = argc; i > 0; i--){
    switch (func_arg_type(func_str, i-1)){
      case TYPE_INT8:
      case TYPE_INT16:
      case TYPE_INT32:
        _push_uint8(p, i+1);
        _push_uint32(p, ((int)L));
        _mov_eax_uint32(p, ((int)luaL_checkinteger));
        _call_eax(p);
        _add_esp_uint8(p, 8);
        _push_eax(p);
      break;

      case TYPE_INT64:
        _push_uint8(p, i+1);
        _push_uint32(p, ((int)L));
        _mov_eax_uint32(p, ((int)luaL_checkinteger));
        _call_eax(p);
        _add_esp_uint8(p, 8);
        _push_edx(p);
        _push_eax(p);
      break;

      case TYPE_STRING:
        _push_uint8(p, 0);
        _push_uint8(p, i+1);
        _push_uint32(p, ((int)L));
        _mov_eax_uint32(p, ((int)luaL_checklstring));
        _call_eax(p);
        _add_esp_uint8(p, 12);
        _push_eax(p);
      break;
    }
  }

  _mov_eax_uint32(p, ((int)func));
  _call_eax(p);

  switch (ret_type){
    case TYPE_INT8:
    case TYPE_INT16:
    case TYPE_INT32:
      //extend sign...
      _push_uint32(p, 0);
      _push_eax(p);
      _mov_eax_uint32(p, ((int)L));
      _push_eax(p);
      _mov_eax_uint32(p, ((int)lua_pushinteger));
      _call_eax(p);
      _add_esp_uint8(p, 12);
    break;
    case TYPE_STRING:
      _push_eax(p);
      _mov_eax_uint32(p, ((int)L));
      _push_eax(p);
      _mov_eax_uint32(p, ((int)lua_pushstring));
      _call_eax(p);
      _add_esp_uint8(p, 8);
    break;
  }
//  _pop_edi(p);
//  _pop_esi(p);
//  _pop_ebx(p);
  _mov_esp_ebp(p);
  _pop_ebp(p);
  if (ret_type != TYPE_VOID) ret_num += 1;
  _mov_eax_uint32(p, ret_num);
  _ret(p);

  size_t size = p - program;
  p = lua_newuserdata(L, size);
  memcpy(p, program, size);

  DWORD protect;
  VirtualProtect(program, size, PAGE_EXECUTE_READWRITE, &protect);

  luaL_newmetatable(L, "binary code");
  lua_pushcfunction(L, call);
  lua_setfield(L, -2, "__call");

  lua_pushcfunction(L, gc);
  lua_setfield(L, -2, "__gc");

  lua_pushlightuserdata(L, lib);
  lua_setfield(L, -2, "HMODULE");

  lua_pushinteger(L, protect);
  lua_setfield(L, -2, "PROTECT");

  lua_pushinteger(L, size);
  lua_setfield(L, -2, "SIZE");

  lua_setmetatable(L, -2);

//  for (int i = 0; i < size; i++){ printf("%02X ", program[i]); if ((i & 0x0F) == 0x0F) printf("\n"); }
  return 1;
}

//extern "C" 
int __declspec(dllexport) luaopen_uffi(lua_State *L) {
  lua_pushcfunction(L, load);
  return 1;
}