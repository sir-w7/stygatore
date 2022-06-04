@echo off

setlocal EnableDelayedExpansion

set PLATFORM=..\src\win32
set SRC=..\src

set COMPILER=cl
set COMPILE_FLAGS=/MP /FC /nologo /Zi 

if not exist build mkdir build
pushd build
%COMPILER% %COMPILE_FLAGS% %PLATFORM%\win32_platform.c %SRC%\common.c %SRC%\stygatore.c %SRC%\tokenizer.c /link /out:stygatore.exe
popd