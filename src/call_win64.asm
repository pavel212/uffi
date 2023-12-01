extrn lua_pushnumber : proc
extrn lua_pushinteger: proc
extrn lua_touserdata : proc
extrn lua_gettop : proc
extrn luaF_isfloat : proc
extrn luaF_typeto : proc

.code
func__call_auto PROC
  push rbx
  push r12
  push r13
  push r14
  push r15
  
  sub rsp, 32

  mov rbx, rcx               ;first argument lua_State * L passed in rcx, store in rbx

  mov rdx, 1
  call lua_touserdata
  mov r15, [rax]             ;store function pointer in r15

  mov rcx, rbx
  call lua_gettop           
  mov r12, rax               ;store number of arguments in r12

  shr eax, 1
  shl eax, 4
  sub rsp, rax               ;reserve stack ((n+1)/2)*16  [1] - self

;//for (int i = 1; i < argc; i++) arg[i] = arg_to(L, i+1)
  xor r13, r13
  inc r13
arg_loop:
  cmp r13, r12
  jge arg_loop_end
  inc r13

  mov rcx, rbx
  mov rdx, r13
  call luaF_isfloat
  mov r14, rax               ;r14 - arg_isfloat?

  mov rcx, rbx
  mov rdx, r13
  call luaF_typeto                ;get pointer to proper lua_to function
 
  mov rcx, rbx
  mov rdx, r13
  xor r8, r8
  call rax                   ;call lua_to
 
  cmp r14, 0
  jz store_int
store_float:
  movq  QWORD PTR [rsp + r13*8 + 16], xmm0  ;store floating point arg
  jmp arg_loop
store_int:
  mov [rsp + r13*8 + 16], rax               ;store integer arg
  jmp arg_loop
arg_loop_end:

  add rsp, 32

;load args to registers, both rcx & xmm0, faster than check for floating point / integer type
;and loading all 4 args (garbage if <4) probably also faster than checking for actual number of arguments

;  mov r13, r12

;  dec r13
;  jz call_cfunc
  mov rcx, [rsp]
  movq xmm0, rcx
  
;  dec r13
;  je call_cfunc
  mov rdx, [rsp+8]
  movq xmm1, rdx

;  dec r13
;  jz call_cfunc
  mov r8, [rsp+16]
  movq xmm2, r8

;  dec r13
;  jz call_cfunc
  mov r9, [rsp+24]
  movq xmm3, r9

call_cfunc:
  call r15 
  movq r14, xmm0

  mov rcx, rbx
  mov rdx, rax
  call lua_pushinteger         ;first return integer

  mov rcx, rbx
  movq xmm1, r14
  call lua_pushnumber          ;then return floating point

  shr r12, 1
  shl r12, 4
  add rsp, r12                 ;restore stack

  pop r15
  pop r14
  pop r13
  pop r12
  pop rbx

  mov eax, 2
  ret
func__call_auto ENDP
END