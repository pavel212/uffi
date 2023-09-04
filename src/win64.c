#include <windows.h>
#include <stdint.h>
#include "lua.h"
#include "lauxlib.h"

#include "asm_x64.h"

//nostdlib
void* mem_cpy(void* dst, const void* src, size_t len) {
  void* ret = dst;
  while (len > 7) { *((uint64_t*)dst)++ = *((uint64_t*)src)++; len -= 8; }
  while (len    ) { *((uint8_t*) dst)++ = *((uint8_t*) src)++; len -= 1; }
  return ret;
}

//void* (*mem_cpy)(void*, const void*, size_t);

#ifndef _DEBUG
int _fltused = 0;
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved) { 
//  mem_cpy = (void* (*)(void*, const void*, size_t)) GetProcAddress(LoadLibrary("msvcrt.dll"), "memcpy");
  return TRUE; }
#endif

//dynlib
void* libopen(const char* filename){ return LoadLibrary(filename); }
void* libsym(void* lib, const char* name){  return GetProcAddress(lib, name); }
int libclose(void* lib){ return FreeLibrary(lib); }

const char* libsymname(uint32_t* lib, uint32_t idx) {
  if (lib == 0) return 0;
  uint32_t* p = lib;
  if ((p[0] & 0xFFFF) != 0x5A4D) return 0; //MZ
  p = &lib[p[15] >> 2];                    //e_lfanew
  if (p[0] != 0x00004550) return 0;        //signature
  if ((p[1] & 0xFFFF) != 0x8664) return 0; //machine
  p = &lib[p[34] >> 2];                    //EXPORT_DIRECTORY
  if (idx >= p[6]) return 0;//NumberOfNames
  p = &lib[(p[8] >> 2) + idx];             //AddressOfNames[idx]
  return &((char*)lib)[*p];
}

int libsymnum(uint32_t* lib) {
  if (lib == 0) return 0;
  uint32_t* p = lib;
  if ((p[0] & 0xFFFF) != 0x5A4D) return 0; //MZ
  p = &lib[p[15] >> 2];                    //e_lfanew
  if (p[0] != 0x00004550) return 0;        //signature
  if ((p[1] & 0xFFFF) != 0x8664) return 0; //machine
  p = &lib[p[34] >> 2];                    //EXPORT_DIRECTORY
  return p[6];                             //NumberOfNames
}

int mprotect_exec(void* addr, size_t len) {
  DWORD protect;
  return VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &protect);
}

int mprotect_noexec(void* addr, size_t len) {
  DWORD protect;
  return VirtualProtect(addr, len, PAGE_READWRITE, &protect);
}

//codegen
void* luaF_typeto(lua_State* L, int idx);
void* luaF_to(const char t);
void* luaF_push(const char t);

int make_func(char* code, const void* func, const char* argt) {
  int argc = (int)strlen(argt);
  int stack = 16 * ((argc + 4) >> 1);  //32 bytes of shadow space + space for function arguments multiple of 16 bytes to keep stack alignment
  char* p = code;
  {
    //prologue
    _push_rbx(p);        //additional push rbx makes stack aligned to 16.
    _mov_rbx_rcx(p);         //as we are in luaCfunction: int f(lua_State * L); L is passed in rcx. store lua_State * L in rbx,
    _sub_rsp_DW(p, stack);   //reserve stack

    //get arguments from lua stack and store them on stack
    for (int i = 1; i < argc; i++) {  //argt[0] - return type
      if (argt[i] == 'l') {     //automatic, from Lua type at runtime
        _mov_rcx_rbx(p);        //first argument lua_State * L 
        _mov_rdx_DW(p, i + 1);  //second argument n+1 - position of argument on lua stack, [1] == self, [2] == first arg, ...
        _call(p, luaF_typeto);
      }
      else _mov_rax_QW(p, (uintptr_t)luaF_to(argt[i]));

      _mov_rcx_rbx(p);        //first argument lua_State * L 
      _mov_rdx_DW(p, i + 1);  //second argument n+1 - position of argument on lua stack, [1] == self, [2] == first arg, ...
      _clr_r8(p);             //some lua_to functions have 3rd argument, #define lua_tostring(L, i) lua_tolstring(L, i, 0)
      _call_rax(p);

      if (argt[i] == 'f') _mov_xmm0f_xmm0(p);                                                            //double -> float conversion, double lua_tonumber();
      if (argt[i] == 'f' || argt[i] == 'd') _st_xmm0(p, 24 + i * 8); else _st_rax(p, 24 + i * 8);    //store argument on stack with 32 bytes offset of shadow store
    }

    _add_rsp_DW(p, 32);    //adjust stack by shadow 32 bytes that were reserved for lua_to functions, so other (4+) arguments on stack with a proper offset

    //put back first four arguments to registers
    if (argc > 1) { if (argt[1] == 'f' || argt[1] == 'd') _ld_xmm0(p, 0);  else _ld_rcx(p, 0); }
    if (argc > 2) { if (argt[2] == 'f' || argt[2] == 'd') _ld_xmm1(p, 8);  else _ld_rdx(p, 8); }
    if (argc > 3) { if (argt[3] == 'f' || argt[3] == 'd') _ld_xmm2(p, 16); else _ld_r8(p, 16); }
    if (argc > 4) { if (argt[4] == 'f' || argt[4] == 'd') _ld_xmm3(p, 24); else _ld_r9(p, 24); }

    _call(p, func);

    //lua_push() function return value to lua stack
    _mov_rcx_rbx(p);                                  //first argument lua_State * L 
    if (argt[0] == 'f') _mov_xmm1_xmm0f(p);      //convert float to double and mov return value(xmm0) to (xmm1) second argument  of lua_push
    else if (argt[0] == 'd') _mov_xmm1_xmm0(p);       //mov double without conversion
    else _mov_rdx_rax(p);                             //other types returned in rax -> second argument in rdx
    _clr_r8(p);                                       //some lua_push functions could have 3rd argument
    _call(p, luaF_push(argt[0]));
    //return 1
    _mov_rax_DW(p, 1);                                //return 1, even void func() returns 1 and lua_pushnil just for uniformity.
    _add_rsp_DW(p, stack - 32);                         //restore stack pointer
    _pop_rbx(p);                                      //and rbx
    _ret(p);
  }
  return (int)(p - code); //length of generated code
}

int make_cb(char* code, lua_State* L, int ref, const char* argt) {
  int argc = (int)strlen(argt);
  char* p = code;
  {
    _push_rbx(p);     //push rbx to align stack to 16 and store lua_State * L in rbx
    _mov_rbx_QW(p, (uintptr_t)L);   //as we are generating ordinary C function, lua_State * L is passed as argument from ffi and stored as immediate constant in code.

    if (argc > 1) if (argt[1] == 'f' || argt[1] == 'd') _st_xmm0(p, 16); else _st_rcx(p, 16);  //store 4 arguments in shadow
    if (argc > 2) if (argt[2] == 'f' || argt[2] == 'd') _st_xmm1(p, 24); else _st_rdx(p, 24);
    if (argc > 3) if (argt[3] == 'f' || argt[3] == 'd') _st_xmm2(p, 32); else _st_r8(p, 32);
    if (argc > 4) if (argt[4] == 'f' || argt[4] == 'd') _st_xmm3(p, 40); else _st_r9(p, 40);

    _sub_rsp_DW(p, 48); //32 shadow space + additional 8 bytes for 5th argument of lua_callk + 8 bytes to keep 16 bytes alignment

    _mov_rcx_rbx(p);
    _mov_rdx_DW(p, LUA_REGISTRYINDEX);
    _mov_r8_DW(p, ref);        //lua function we are going to wrap (ref) as C callback is stored in lua registry, so it is not garbage collected accidentally
    _call(p, lua_rawgeti);   //get lua function on stack from registry lua_rawgeti(L, REIGSTRYINDEX, funcref)

    for (int i = 1; i < argc; i++) {
      _mov_rcx_rbx(p);   //first argument lua_State * L 
      if (argt[i] == 'f' || argt[i] == 'd') _ld_xmm1(p, 56 + i * 8); else _ld_rdx(p, 56 + i * 8);           //get second argument for lua_push from stack
      if (argt[i] == 'f') _mov_xmm1_xmm1f(p);                                                           //float -> double conversion for lua_pushnumber
      _clr_r8(p);     //some lua_push functions have 3rd argument
      _call(p, luaF_push(argt[i]));
    }

    _mov_rcx_rbx(p);
    _mov_rdx_DW(p, argc - 1);
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
  return (int)(p - code);
}

