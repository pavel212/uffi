tcc -m64 -shared -I"lib" src\call_win64.S src\*.c lib\lua54.def -o ffi.dll

@rem upx --ultra-brute ffi.dll
