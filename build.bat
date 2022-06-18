@echo off

setlocal EnableDelayedExpansion

set COMPILER=cl
set COMPILE_FLAGS=/MP /FC /nologo /Zi 

set LINK_FLAGS=/incremental:no

set PLATFORM=..\src\win32
set SRC=..\src

if not exist build mkdir build
pushd build
%COMPILER% %COMPILE_FLAGS% ..\src\stygatore.cpp /link %LINK_FLAGS% /out:stygatore.exe
popd
