#!/bin/sh
gcc -c -no-pie -fpic src/call_sysV64.S -o call_sysV64.o

gcc -m64 -no-pie -fpic -shared -I"lib" call_sysV64.o src/ffi.c src/userdata.c src/sysV64.c -llua5.4 -o ffi.so
