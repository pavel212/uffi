cl src\ffi.c /c /GS- /sdl- /EHs-c- /GL /Oi /I"lib" /Zc:inline /Zc:forScope /fp:fast /O1 /MD
ml64 /c src\call.asm
link ffi.obj call.obj "lib\lua54.lib" "lib\msvcrt.lib" "kernel32.lib" /MANIFEST:NO /LTCG /NXCOMPAT /DYNAMICBASE /DLL /MACHINE:X64 /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /OPT:ICF /NOLOGO /NODEFAULTLIB
@rem upx --ultra-brute ffi.dll
