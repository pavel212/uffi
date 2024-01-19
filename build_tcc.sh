#!/bin/sh
tcc -shared -I"lib" src/call_sysV64.s src/ffi.c src/userdata.c src/sysV64.c -llua5.4 -o ffi.so

#tcc -c src/call_sysV64.s -o call_sysV64.o
#tcc -m64 -g -shared -I"lib" call_sysV64.o src/ffi.c src/userdata.c src/sysV64.c -llua5.4 -o ffi.so
