@echo off

set defines=-D_CRT_SECURE_NO_WARNINGS -D_UNICODE -DUNICODE
set lnk_libs= -lkernel32 -luser32 -lgdi32 -lshell32 -lshlwapi

tcc -w -g -b %input_file_name% -o %output_file_name% %defines% %lnk_libs%