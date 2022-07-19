@echo off

if not exist generated mkdir generated
cl /nologo /O2 gen_tests.c /link /out:gen_tests.exe
gen_tests.exe