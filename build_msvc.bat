cl src\*.c /c /GS- /sdl- /EHs-c- /GL /Oi /I"lib" /Zc:inline /Zc:forScope /fp:fast /O1 /MD
ml64 /c src\call.asm
link *.obj "lib\*.lib" "kernel32.lib" /OUT:"ffi.dll" /MANIFEST:NO /LTCG /NXCOMPAT /DYNAMICBASE /DLL /MACHINE:X64 /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /OPT:ICF /NOLOGO /NODEFAULTLIB
upx --ultra-brute ffi.dll
