@echo off

setlocal EnableDelayedExpansion

set COMPILER=cl
set COMPILE_FLAGS= /O2 /MP /FC /nologo /Zi  /W4 /Wall /wd4464 /wd4100 /wd4189 /wd4820 /wd4061 /wd4477 /wd4505 /wd5045 /wd4710 /wd4711 /D_CRT_SECURE_NO_WARNINGS

set LINK_FLAGS= /incremental:no /opt:ref

set PLATFORM=..\src\win32
set SRC=..\src

if not exist build mkdir build
pushd build
%COMPILER% %COMPILE_FLAGS% ..\src\stygatore.cpp /link %LINK_FLAGS% /out:stygatore.exe
popd
