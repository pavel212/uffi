#!/bin/sh
tcc -m64 -g -shared -I"lib" src/call_sysV64.S src/ffi.c src/userdata.c src/sysV64.c -llua5.4 -o ffi.so
