@echo off

setlocal EnableDelayedExpansion

set COMPILER=cl
set COMPILE_FLAGS=/MP /FC /nologo /Zi 

set PLATFORM=..\src\win32
set SRC=..\src

set SRC_FILES=%PLATFORM%\win32_platform.c %SRC%\common.c %SRC%\stygatore.c %SRC%\tokenizer.c %SRC%\parser.c

if not exist build mkdir build
pushd build
%COMPILER% %COMPILE_FLAGS% %SRC_FILES% /link /out:stygatore.exe
popd
