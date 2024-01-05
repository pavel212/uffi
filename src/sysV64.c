#include <stdint.h>
#include "lua.h"
#include "lauxlib.h"
#include "asm_x64.h"

#define _GNU_SOURCE
#include <link.h>
#include <dlfcn.h>
#include <stdio.h>

//dynlib
void* libopen(const char* filename){ return dlopen(filename, 0); }
void* libsym(void* lib, const char* name){  return dlsym(lib, name); }
int   libclose(void* lib){ return dlcose(lib); }

static int libsyminfo(uint32_t* lib, Elf64_Sym ** symtab, char ** strtab){
  if (lib == 0) return 0;
  struct link_map * map = 0;
  dlinfo(lib, RTLD_DI_LINKMAP, &map);
  int syment = 0;
  for (ElfW(Dyn) * section = map->l_ld; section->d_tag != DT_NULL; section++){
    switch(section->d_tag){
      case DT_SYMTAB: *symtab = (Elf64_Sym *)section->d_un.d_ptr; break;
      case DT_STRTAB: *strtab = (char*)section->d_un.d_ptr; break;
      case DT_SYMENT: syment = section->d_un.d_val; break;
    }
  }
  return (*strtab - (char*)*symtab) / syment;
}

const char* libsymname(uint32_t* lib, uint32_t idx) {
  Elf64_Sym * symtab = 0;
  char * strtab = 0;
  int size = libsyminfo(lib, &symtab, &strtab);
  int num = 0;
  for (int i = 0; i < size; i++){
    if ((symtab[i].st_shndx != SHN_UNDEF) && (ELF64_ST_TYPE(symtab[i].st_info) == STT_FUNC)){
      if (num == idx) return &strtab[symtab[i].st_name];
      num += 1;
    }
  }
  return 0;
}

int libsymnum(uint32_t* lib) {
  Elf64_Sym * symtab = 0;
  char * strtab = 0;
  int size = libsyminfo(lib, &symtab, &strtab);
  int num = 0;
  for (int i = 0; i < size; i++){
    if ((symtab[i].st_shndx != SHN_UNDEF) && (ELF64_ST_TYPE(symtab[i].st_info) == STT_FUNC)) num += 1;
  }
  return num;
}

int mprotect_exec(void* addr, size_t len) { return mprotect(addr, len,  PROT_READ | PROT_WRITE | PROT_EXEC); }
int mprotect_noexec(void* addr, size_t len) { return mprotect(addr, len,  PROT_READ | PROT_WRITE); }

//codegen
void* luaF_typeto(lua_State* L, int idx);
void* luaF_to(const char t);
void* luaF_push(const char t);

//rdi,rsi,rdx,rcx,r8,r9 / xmm0..7
int make_func(char* code, const void* func, const char* argt) {
  int argc = (int)strlen(argt);
  int stack = 16 * ((argc + 1) >> 1);  //space for function arguments multiple of 16 bytes to keep stack alignment
  int argi = 6, argf = 8, args = 0, stack0 = 0;   //number of arguments: integer reg, float reg, on stack.

  char* p = code;
  {
//prologue
    _push_rbx(p);            //additional push rbx makes stack aligned to 16.
    _mov_rbx_rdi(p);         //as we are in luaCfunction: int f(lua_State * L); L is passed in rdi. store lua_State * L in rbx,
    _sub_rsp_DW(p, stack);   //reserve stack.

//get arguments from lua stack and store them on stack
    for (int i = 1; i < argc; i++) {  //argt[0] - return type
      if (argt[i] == 'l') {     //automatic, from Lua type at runtime
        _mov_rdi_rbx(p);        //first argument lua_State * L 
        _mov_rsi_DW(p, i + 1);  //second argument n+1 - position of argument on lua stack, [1] == self, [2] == first arg, ...
        _call(p, luaF_typeto);
      }
      else _mov_rax_QW(p, (uintptr_t)luaF_to(argt[i]));

      _mov_rdi_rbx(p);        //first argument lua_State * L 
      _mov_rsi_DW(p, i + 1);  //second argument n+1 - position of argument on lua stack, [1] == self, [2] == first arg, ...
      _clr_rdx(p);             //some lua_to functions have 3rd argument, #define lua_tostring(L, i) lua_tolstring(L, i, 0)
      _call_rax(p);

      if (argt[i] == 'f') _mov_xmm0f_xmm0(p);     //double -> float conversion, double lua_tonumber();
      if (argt[i] == 'f' || argt[i] == 'd'){      //store argument on stack
        _st_xmm0(p, i * 8 - 8);
        if (argf) argf--;
      } else {
        _st_rax(p, i * 8 - 8);    
        if (argi) argi--;
      }
      if (argf && argi) stack0 += 8;                 //position of first arg that did not fit into regs
    }

//put back arguments into registers
    argf = argi = args = 0;
    for (int i = 1; i < argc; i++){
      if (argt[i] == 'f' || argt[i] == 'd'){
        switch(argf++) {
          case 0: _ld_xmm0(p, i * 8 - 8); break;
          case 1: _ld_xmm1(p, i * 8 - 8); break;
          case 2: _ld_xmm2(p, i * 8 - 8); break;
          case 3: _ld_xmm3(p, i * 8 - 8); break;
          case 4: _ld_xmm4(p, i * 8 - 8); break;
          case 5: _ld_xmm5(p, i * 8 - 8); break;
          case 6: _ld_xmm6(p, i * 8 - 8); break;
          case 7: _ld_xmm7(p, i * 8 - 8); break;
          default: _ld_rax(p, i * 8 - 8); _st_rax(p, stack0 + 8 * args++); break;
        }
      } else {
        switch(argi++) {
          case 0: _ld_rdi(p, i * 8 - 8); break;
          case 1: _ld_rsi(p, i * 8 - 8); break;
          case 2: _ld_rdx(p, i * 8 - 8); break;
          case 3: _ld_rcx(p, i * 8 - 8); break;
          case 4: _ld_r8 (p, i * 8 - 8); break;
          case 5: _ld_r9 (p, i * 8 - 8); break;
          default:_ld_rax(p, i * 8 - 8); _st_rax(p, stack0 + 8 * args++); break;
        }
      }
    }

    _add_rsp_DW(p, stack0);    //adjust rsp to first nonreg argument

    _call(p, func);

//lua_push() function return value to lua stack
    _mov_rdi_rbx(p);        //first argument lua_State * L 
    if (argt[0] == 'f') _mov_xmm1_xmm0f(p);      //convert float to double and mov return value(xmm0) to (xmm1) second argument  of lua_push
    else if (argt[0] == 'd') _mov_xmm1_xmm0(p);       //mov double without conversion
    else _mov_rsi_rax(p);                             //other types returned in rax -> second argument in rsi
    _clr_rdx(p);                                      //some lua_push functions could have 3rd argument
    _call(p, luaF_push(argt[0]));
//return 1
    _mov_rax_DW(p, 1);                                //return 1, even void func() returns 1 and lua_pushnil just for uniformity.
    _add_rsp_DW(p, stack - stack0);                            //restore stack pointer
    _pop_rbx(p);                                      //and rbx
    _ret(p);
  }
  return (int)(p - code); //length of generated code
}

int make_cb(char* code, lua_State* L, int ref, const char* argt) {
  int argc = (int)strlen(argt);
  int stack = 16 * ((argc + 1) >> 1);  //space for function arguments multiple of 16 bytes to keep stack alignment
  char* p = code;
  {
//prologue
    _push_rbx(p);     //push rbx to align stack to 16 and store lua_State * L in rbx
    _mov_rbx_QW(p, (uintptr_t)L);   //as we are generating ordinary C function, lua_State * L is passed as argument from ffi and hardcoded.

    _sub_rsp_DW(p, stack);

//store arguments from regs onto stack before calling rawgeti that gets lua function and lua_push to put arguments on lua stack
    argf = argi = args = 0;
    for (int i = 1; i < argc; i++){
      if (argt[i] == 'f' || argt[i] == 'd'){
        switch(argf++) {
          case 0: _st_xmm0(p, i * 8 - 8); break;
          case 1: _st_xmm1(p, i * 8 - 8); break;
          case 2: _st_xmm2(p, i * 8 - 8); break;
          case 3: _st_xmm3(p, i * 8 - 8); break;
          case 4: _st_xmm4(p, i * 8 - 8); break;
          case 5: _st_xmm5(p, i * 8 - 8); break;
          case 6: _st_xmm6(p, i * 8 - 8); break;
          case 7: _st_xmm7(p, i * 8 - 8); break;
          default: _ld_rax(p, stack + 8 + 8 * args++); _st_rax(p, i * 8 - 8);
        }
      } else {
        switch(argi++) {
          case 0: _st_rdi(p, i * 8 - 8); break;
          case 1: _st_rsi(p, i * 8 - 8); break;
          case 2: _st_rdx(p, i * 8 - 8); break;
          case 3: _st_rcx(p, i * 8 - 8); break;
          case 4: _st_r8 (p, i * 8 - 8); break;
          case 5: _st_r9 (p, i * 8 - 8); break;
          default: _ld_rax(p, stack + 8 + 8 * args++); _st_rax(p, i * 8 - 8);
        }
      }
    }

//push lua function
    _mov_rdi_rbx(p);
    _mov_rsi_DW(p, LUA_REGISTRYINDEX);
    _mov_rdx_DW(p, ref);        //lua function we are going to wrap (ref) as C callback is stored in lua registry, so it is not garbage collected accidentally
    _call(p, lua_rawgeti);      //get lua function on stack from registry lua_rawgeti(L, REIGSTRYINDEX, funcref)

//push arguments to lua stack
    for (int i = 1; i < argc; i++) {
      _mov_rdi_rbx(p);   //first argument lua_State * L 
      if (argt[i] == 'f' || argt[i] == 'd') _ld_xmm0(p, i * 8 - 8); else _ld_rsi(p, i * 8 - 8);          //get second argument for lua_push from stack
      if (argt[i] == 'f') _mov_xmm0_xmm0f(p);                                                           //float -> double conversion for lua_pushnumber
      _clr_rdx(p);                                                                                     //some lua_push functions have 3rd argument
      _call(p, luaF_push(argt[i]));
    }

//lua_call
    _mov_rdi_rbx(p);
    _mov_rsi_DW(p, argc - 1);
    _mov_rdx_DW(p, 1);
    _clr_rcx(p);
    _st_r8(p, 32);
    _call(p, lua_callk);   //lua_callk(L, argc-1, 1, 0, 0); lua function always return 1 value, nil for void f() callback

//get return value to C with lua_to
    _mov_rdi_rbx(p);       //first argument lua_State * L 
    _mov_rsi_DW(p, -1);   //return value is on top of the lua stack after lua_call
    _clr_rdx(p);         //some functions have 3rd argument, lua_tostring == lua_tolstring(L, idx, 0);
    _call(p, luaF_to(argt[0]));

//lua_pop return value
    _mov_rdi_rbx(p);     //first argument lua_State * L, we do not need L anymore, so save return value in rbx while calling lua_pop
    if (argt[0] == 'f' || argt[0] == 'd') _mov_rbx_xmm0(p); else _mov_rbx_rax(p); //store return value in rbx to call lua_pop(L, 1)
    _mov_rsi_DW(p, -2);
    _call(p, lua_settop); //lua_pop(L, 1) == lua_settop(L, -2);
    if (argt[0] == 'f' || argt[0] == 'd') _mov_xmm0_rbx(p); else _mov_rax_rbx(p); //load return value of lua_to() back to rax/xmm0

//return
    _add_rsp_DW(p, stack);    //restore stack
    _pop_rbx(p);             //and rbx
    _ret(p);
  }
  return (int)(p - code);
}