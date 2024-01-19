.globl func__call_auto

func__call_auto:
push %rbx
push %r12
push %r13
push %r14
push %r15

sub $32, %rsp

mov %rcx, %rbx             //first argument lua_State * L passed in rcx, store in rbx

mov $1, %rdx
call lua_touserdata
mov (%rax), %r15           //store function pointer in r15

mov %rbx, %rcx
call lua_gettop           
mov %rax, %r12             //store number of arguments in r12

shr $1, %eax
shl $4, %eax
sub %rax, %rsp             //reserve stack ((n+1)/2)*16  [1] - self

//for (int i = 1; i < argc; i++) arg[i] = arg_to(L, i+1)
xor %r13, %r13
inc %r13
arg_loop:
cmp %r12, %r13
jge arg_loop_end
inc %r13

mov %rbx, %rcx
mov %r13, %rdx
call luaF_isfloat
mov %rax, %r14             //r14 - arg_isfloat?

mov %rbx, %rcx
mov %r13, %rdx
call luaF_typeto           //get pointer to proper lua_to function

mov %rbx, %rcx
mov %r13, %rdx
xor %r8, %r8
call * %rax                  //call lua_to

cmp $0, %r14
je store_int
movq %xmm0, %rax
store_int:
mov %rax, 16(%rsp,%r13,8)    //store
jmp arg_loop
arg_loop_end:

add $32, %rsp

//load args to registers, both rcx & xmm0, faster than check for floating point / integer type
//and loading all 4 args (garbage if <4) probably also faster than checking for actual number of arguments
mov (%rsp), %rcx
movq %rcx, %xmm0

mov 8(%rsp), %rdx
movq %rdx, %xmm1

mov 16(%rsp), %r8
movq %r8, %xmm2

mov 24(%rsp), %r9
movq %r9, %xmm3

call_cfunc:
call * %r15 
movq %xmm0, %r14

mov %rbx, %rcx
mov %rax, %rdx
call lua_pushinteger         //first return integer

mov %rbx, %rcx
movq %r14, %xmm1
call lua_pushnumber          //then return floating point

shr $1, %r12
shl $4, %r12
add %r12, %rsp                  //restore stack

pop %r15
pop %r14
pop %r13
pop %r12
pop %rbx

mov $2, %eax
ret