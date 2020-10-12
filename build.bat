@echo off

REM COMPULER (MSVC OR CLANG)
set compiler_clang=clang
set compiler_msvc=msvc

REM DEBUG OR RELEASE
set debug=-Z7 -Od -MTd -DMODE_DEBUG
set release=-O2 -MT

REM BUILD_PLATFORM
set windows=-DIMGVU_WINDOWS

REM ================
REM PROJECT SETTINGS
REM ================

set defines=-D_CRT_SECURE_NO_WARNINGS -D_UNICODE
set output_file_name=imgvu
set mode=%debug%
set platform=%windows%
set input_file_name=imgvu_windows.c
set compiler=msvc
set lnk_libs=kernel32.lib user32.lib gdi32.lib shell32.lib shlwapi.lib

REM =================
REM COMPILATION SETUP
REM =================

REM WARNINGS
set msvc_dwarnings=-wd4100 -wd4710 -wd5045 -wd4068 -wd4201 -wd4221 -wd4127
set clang_dwarnings=-Wno-unused-parameter -Wno-unused-function -Wno-unused-macros -Wno-cast-align

REM COMPILATION FLAGS
set common_flags=%mode% %defines% -nologo -FC -Wall -WX -TC -Ob1 -Oi -EHa -c -Zp4
set msvc_flags=%common_flags% -Qspectre -Zf ^
	-Fe"%output_file_name%.exe"^
	-Fo"%output_file_name%.obj"^
	-Fd"%output_file_name%.pdb"^
	-U__cplusplus
	
set clang_flags=%common_flags%^
	-showFilenames -fdiagnostics-absolute-paths^
	-o"%output_file_name%.exe"^
	-Fo"%output_file_name%.obj"^
	-Fd"%output_file_name%.pdb"^
	-Wmain

REM LINKING FLAGS
set lnk_flags=-nologo -incremental:no -debug "%output_file_name%.obj" %lnk_libs%

if %compiler%==%compiler_clang% (

	REM COMPILING WITH CLANG, CHECKING IF COMPILES WITH MSVC
	echo ^>^>^> clang compile... %clang_flags% %clang_dwarnings% "src\%input_file_name%"
	clang-cl %clang_flags% "src\%input_file_name%" %clang_dwarnings%
	echo ^>^>^> link...
	link %lnk_flags%

	echo ^>^>^> msvc compile... %msvc_flags% %msvc_dwarnings% "src\%input_file_name%"
	cl %msvc_flags% %msvc_dwarnings% "src\%input_file_name%"

) else if %compiler%==%compiler_msvc% (
	
	REM COMPILING WITH MSVC, CHECKING IF COMPILES WITH CLANG
	echo ^>^>^> msvc compile...
	cl %msvc_flags% %msvc_dwarnings% "src\%input_file_name%"
	echo ^>^>^> link...
	link %lnk_flags%
	
	echo ^>^>^> clang compile...
	clang-cl %clang_flags% %clang_dwarnings% "src\%input_file_name%"

) else (
	echo unknown compiler specified
)

del *.ilk 0> nul 1> nul 2> nul
del *.obj 0> nul 1> nul 2> nul
