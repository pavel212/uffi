#!/bin/sh
gcc -m64 -s -z noexecstack -fpic -shared -I"lib" src/call_sysV64.s src/ffi.c src/userdata.c src/sysV64.c -llua5.4 -o ffi.so

#gcc -g -c -fpic src/call_sysV64.s -o call_sysV64.o
#gcc -g -m64 -z noexecstack -fpic -shared -I"lib" call_sysV64.o src/ffi.c src/userdata.c src/sysV64.c -llua5.4 -o ffi.so
