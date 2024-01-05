tcc -m64 -shared -I"lib" -D_WIN64 src\call_win64.S src\ffi.c src\userdata.c src\win64.c lib\lua54.def -o ffi.dll
@rem  
@rem upx --ultra-brute ffi.dll
