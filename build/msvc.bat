@echo off

REM DEBUG OR RELEASE
set debug=-Z7 -Zf -Od -MTd -DMODE_DEBUG
set release=-O2 -MT

REM ================
REM PROJECT SETTINGS
REM ================

set defines=-D_CRT_SECURE_NO_WARNINGS -D_UNICODE -DUNICODE
set mode=%debug%
set lnk_libs=kernel32.lib user32.lib gdi32.lib shell32.lib shlwapi.lib

set input_file_name=imgvu_windows.c
set output_file_name=imgvu

REM =================
REM COMPILATION SETUP
REM =================

set warnings=-w34191 -w44242 -w44254 -w44255 -w44288
set cmp_flags=%mode% %defines% -nologo -FC -Wall -WX -TC -Ob1 -Oi -EHa -c -Zp4 -wd5045 %warnings%
set cmp_flags=%cmp_flags% -Qspectre

set cmp_flags=%cmp_flags% -Fe"%output_file_name%.exe"
set cmp_flags=%cmp_flags% -Fo"%output_file_name%.obj"
set cmp_flags=%cmp_flags% -Fd"%output_file_name%.pdb"

set lnk_flags=-nologo -incremental:no -debug

cl %cmp_flags% "src\%input_file_name%"
link %lnk_flags% "%output_file_name%.obj" %lnk_libs%