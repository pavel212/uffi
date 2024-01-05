#!/bin/sh
<<<<<<< HEAD
tcc -m64 -shared -I"lib" src/call_sysV64.S src/ffi.c src/userdata.c src/sysV64.c -llua5.4 -o ffi.so
=======
tcc -m64 -shared -I"lib" src\call_sysV64.S src\ffi.c src\userdata.c src\sysV64.c lib\lua54.def -o ffi.so
>>>>>>> 188e16889261aed12cc7ee53e53cc1aa37234ece
