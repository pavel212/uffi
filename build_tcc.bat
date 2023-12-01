tcc -m64 -shared -I"lib" src\*.c lib\lua54.def src\call_win64.S -o ffi.dll

@rem upx --ultra-brute ffi.dll
