@echo off

set use_asan_f=-fsanitize=address

REM DEBUG OR RELEASE
set debug=-Z7 -Od -MT -DMODE_DEBUG
set release=-O2 -MT

REM ================
REM PROJECT SETTINGS
REM ================

set use_asan=0
set defines=-D_CRT_SECURE_NO_WARNINGS -D_UNICODE -DUNICODE
set mode=%debug%
set lnk_libs=user32.lib gdi32.lib shell32.lib shlwapi.lib Userenv.lib opengl32.lib

REM =================
REM COMPILATION SETUP
REM =================

set warnings=-Wno-unused-function
if %use_asan% == 1 (set mode=%mode% %use_asan_f%)

set cmp_flags=%mode% %defines% -nologo -FC -Wall -WX -TC -Ob1 -Oi -EHa -Zp4 %warnings%
set cmp_flags=%cmp_flags% 

set cmp_flags=%cmp_flags% -o"%output_file_name%.exe"
set cmp_flags=%cmp_flags% -Fo"%output_file_name%.obj"
set cmp_flags=%cmp_flags% -Fd"%output_file_name%.pdb"

set cmp_flags=%cmp_flags% -fdiagnostics-absolute-paths

set lnk_flags=-nologo -incremental:no -debug -ignore:4042

clang-cl %cmp_flags% "%input_file_name%" ^
/link %lnk_flags% "%output_file_name%.obj" %lnk_libs%
	