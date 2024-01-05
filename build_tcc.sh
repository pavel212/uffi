#!/bin/sh
tcc -m64 -shared -I"lib" src\call_sysV64.S src\ffi.c src\userdata.c src\sysV64.c lib\lua54.def -o ffi.so
