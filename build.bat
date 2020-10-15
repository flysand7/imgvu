@echo off

REM DEBUG OR RELEASE
set debug=-Z7 -Od -MTd -DMODE_DEBUG
set release=-O2 -MT

REM ================
REM PROJECT SETTINGS
REM ================

set defines=-D_CRT_SECURE_NO_WARNINGS -D_UNICODE
set mode=%debug%
set lnk_libs=kernel32.lib user32.lib gdi32.lib shell32.lib shlwapi.lib

set input_file_name=imgvu_windows.c
set output_file_name=imgvu

REM =================
REM COMPILATION SETUP
REM =================

set cmp_flags=%mode% %defines% -nologo -FC -Wall -WX -TC -Ob1 -Oi -EHa -c -Zp4 -Qspectre -wd5045
set cmp_flags=%cmp_flags% -Qspectre -Zf

set cmp_flags=%cmp_flags% -Fe"%output_file_name%.exe"
set cmp_flags=%cmp_flags% -Fo"%output_file_name%.obj"
set cmp_flags=%cmp_flags% -Fd"%output_file_name%.pdb"

set lnk_flags=-nologo -incremental:no -debug

cl %cmp_flags% "src\%input_file_name%"
link %lnk_flags% "%output_file_name%.obj" %lnk_libs%
	
del *.ilk 0> nul 1> nul 2> nul
del *.obj 0> nul 1> nul 2> nul
