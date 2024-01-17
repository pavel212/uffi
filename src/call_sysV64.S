.globl func__call_auto

func__call_auto:
push %rbx
push %r12
push %r13
push %r14
push %r15

mov %rdi, %rbx             #first argument lua_State * L passed in rdi, store in rbx

//mov %rbx, %rdi
call lua_gettop
mov %rax, %r12             #store (number of arguments+1) in r12

shr $1, %eax
shl $4, %eax
sub %rax, %rsp             #reserve stack ((n+1)/2)*16  [+1] - self

#get C function
mov %rbx, %rdi
mov $1, %rsi
call lua_touserdata
push %rax                  #put C function on stack

#for (int i = 1; i < argc; i++) arg[i] = arg_to(L, i+1)
mov $6, %r10               #argi
mov $8, %r11               #argf
xor %r15, %r15             #stack0
xor %r14, %r14             #isfloat bitfield, <64 function args!
xor %r13, %r13             #i
inc %r13
arg_loop:
cmp %r12, %r13
jge arg_loop_end
inc %r13

mov %rbx, %rdi
mov %r13, %rsi
call luaF_isfloat
and $1, %rax
shl %r14
add %rax, %r14             #r14 - arg_isfloat bitfield

mov %rbx, %rdi
mov %r13, %rsi
call luaF_typeto           #get pointer to proper lua_to function

mov %rbx, %rdi
mov %r13, %rsi
xor %rdx, %rdx
call * %rax                #call lua_to

test $1, %r14
jz store_int
movq %xmm0, -8(%rsp,%r13,8)  #store
test %r11, %r11
jz store_end
dec %r11
jmp store_end
store_int:
mov %rax, -8(%rsp,%r13,8)  #store
test %r10, %r10
jz store_end
dec %r10
store_end:

mov %r10, %rax
imul %r11, %rax
jz arg_loop     #argi == 0 || argf == 0
inc %r15

jmp arg_loop

#just put here for tcc label arithmetics that require labels to be defined before
ld_rdi:
mov %rax, %rdi
jmp ld_end
ld_rsi:
mov %rax, %rsi
jmp ld_end
ld_rdx:
mov %rax, %rdx
jmp ld_end
ld_rcx:
mov %rax, %rcx
jmp ld_end
ld_r8:
mov %rax, %r8
jmp ld_end
ld_r9:
mov %rax, %r9
jmp ld_end
jmp ld_stack   # >6 int arg goes to stack

ld_xmm0:
movq %rax, %xmm0
jmp ld_end
ld_xmm1:
movq %rax, %xmm1
jmp ld_end
ld_xmm2:
movq %rax, %xmm2
jmp ld_end
ld_xmm3:
movq %rax, %xmm3
jmp ld_end
ld_xmm4:
movq %rax, %xmm4
jmp ld_end
ld_xmm5:
movq %rax, %xmm5
jmp ld_end
ld_xmm6:
movq %rax, %xmm6
jmp ld_end
ld_xmm7:
movq %rax, %xmm7
jmp ld_end
jmp ld_stack   # >6 int arg goes to stack
ld_stack:
mov %rax, (%rsp,%r10,8)
inc %r10
jmp ld_end

arg_loop_end:

#shift r14 to the left
mov %r12, %rax
mov $63, %cl
sub %cl, %al
shl %cl, %r14

#load args to registers
xor %r13, %r13
inc %r13

mov %r15, %r10      #offset to put nonreg args onto stack

ld_loop:
cmp %r12, %r13
jge arg_loop_end

mov (%rsp,%r13,8), %rax

shl $1, %r14
jc ld_float

ld_int:
mov $7, %r11
cmp %r13, %r11
cmovle %r13, %r11
dec %r11
imul ld_rsi - ld_rdi, %r11
//add ld_rdi, %r11

//lea 0(%rip), %rax
//add %rax, %r11

mov (%rsp,%r13,8), %rax
jmp *%r11

ld_float:
mov $9, %r11
cmp %r13, %r11
cmovle %r13, %r11
dec %r11
imul ld_xmm1 - ld_xmm0, %r11

//add ld_xmm0, %r11

mov (%rsp,%r13,8), %rax
jmp *%r11

ld_end:

inc %r13
jmp ld_loop
ld_loop_end:

pop %rax
imul $8, %r15
add %r15, %rsp             # adjust stack to stack0 so nonreg arguments are at proper position

call_cfunc:
call * %rax
movq %xmm0, %r15

mov %rbx, %rcx
mov %rax, %rdx
call lua_pushinteger         #first return integer

mov %rbx, %rcx
movq %r15, %xmm1
call lua_pushnumber          #then return floating point

shr $1, %r12
shl $4, %r12
sub %r15, %r12
add %r12, %rsp                  #restore stack


pop %r15
pop %r14
pop %r13
pop %r12
pop %rbx

mov $2, %eax
ret
