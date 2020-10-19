@echo off

set defines=-D_CRT_SECURE_NO_WARNINGS -D_UNICODE -DUNICODE
set lnk_libs= -lkernel32 -luser32 -lgdi32 -lshell32 -lshlwapi

tcc -w -g -b src\imgvu_windows.c -o imgvu %defines% %lnk_libs%