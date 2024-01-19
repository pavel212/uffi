.globl func__call_auto

.section .text

func__call_auto:
pushq %rbx
pushq %r12
pushq %r13
pushq %r14
pushq %r15

mov %rdi, %rbx             #first argument lua_State * L passed in rdi, store in rbx

#mov %rbx, %rdi
call lua_gettop
mov %rax, %r12             #store (number of arguments+1) in r12

shr $1, %eax
shl $4, %eax
sub %rax, %rsp             #reserve stack ((n+1)/2)*16  [+1] - self

#get C function
mov %rbx, %rdi
mov $1, %rsi
call lua_touserdata
pushq %rax                  #put C function on stack
pushq %rbx                  #lua_state *L as well just to keep stack aligned

#for (int i = 1; i < argc; i++) arg[i] = arg_to(L, i+1)
mov $5, %r10               #argi
mov $7, %r11               #argf
xor %r15, %r15             #stack0
xor %r14, %r14             #isfloat bitfield, <64 function float args!
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

#movq %xmm0, (%rsp,%r13,8)     #store  ,r13 +1 (self), +8 -> +16bytes, stack: func, L, args...
#tcc bug

movq %xmm0, %rax               #store  ,r13 +1 (self), +8 -> +16bytes, stack: func, L, args...
movq %rax, (%rsp,%r13,8)       #store  ,r13 +1 (self), +8 -> +16bytes, stack: func, L, args...

test %r11, %r11
jz store_end
dec %r11
jmp store_end
store_int:
mov %rax, (%rsp,%r13,8)   #store
test %r10, %r10
jz store_end
dec %r10
store_end:

mov %r10, %rax
imul %r11, %rax
jz arg_loop     #argi == 0 || argf == 0
inc %r15

jmp arg_loop
arg_loop_end:

#shift r14 to the left
mov $65, %cl
sub %r12b, %cl
shl %cl, %r14

#load args to registers/stack
#for (int i = 1; i < argc; i++) load_arg[i] -> reg/stack before func call
xor %r13, %r13
inc %r13

xor %r10, %r10        #r10 - argi counter
xor %r11, %r11        #r11 - argf counter
                      #r12 - number of arguments
                      #r13 - i
                      #r14 - bit mask for float args
                      #r15 - offset for first stack arg
mov %r15, %rbx

ld_loop:
cmp %r12, %r13
jge ld_loop_end
inc %r13

mov (%rsp,%r13,8), %rax

shl $1, %r14
jc ld_float

ld_int:
                     #jump table?, PIC?
cmp $0, %r10
cmove %rax, %rdi
cmp $1, %r10
cmove %rax, %rsi
cmp $2, %r10
cmove %rax, %rdx
cmp $3, %r10
cmove %rax, %rcx
cmp $4, %r10
cmove %rax, %r8
cmp $5, %r10
cmove %rax, %r9
mov %rax, 16(%rsp,%rbx,8)
inc %rbx
jmp ld_end

ld_float:
cmp $0, %r11
je ld_xmm0
cmp $1, %r11
je ld_xmm1
cmp $2, %r11
je ld_xmm2
cmp $3, %r11
je ld_xmm3
cmp $4, %r11
je ld_xmm4
cmp $5, %r11
je ld_xmm5
cmp $6, %r11
je ld_xmm6
cmp $7, %r11
je ld_xmm7
mov %rax, 16(%rsp,%rbx,8)
inc %rbx
jmp ld_end

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

ld_end:
inc %r10
inc %r11
jmp ld_loop
ld_loop_end:

popq %rbx                   #lua_State * L
popq %rax                   #function to call

imul $8, %r15
add %r15, %rsp             # adjust stack to stack0 so nonreg arguments are at proper position

call_cfunc:
call *(%rax)
movq %xmm0, %r14

mov %rbx, %rdi
mov %rax, %rsi
call lua_pushinteger         #first return integer

mov %rbx, %rdi
movq %r14, %xmm0
call lua_pushnumber          #then return floating point

shr $1, %r12
shl $4, %r12
sub %r15, %r12
add %r12, %rsp                  #restore stack


popq %r15
popq %r14
popq %r13
popq %r12
popq %rbx

mov $2, %eax
ret
