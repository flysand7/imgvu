@echo off

rem set compiler=%1
if not defined compiler (set compiler=clang)

set input_file_name=src\imgvu_windows.c
set output_file_name=imgvu

del %output_file_name%.exe 2> nul
del %output_file_name%.pdb 2> nul

if %compiler% == clang (echo compiling with clang... & call build\compile-clang.bat)^
else if %compiler% == msvc (echo compiling with msvc... & call build\compile-msvc.bat)^
else if %compiler% == tcc (echo compiling with tcc... & call build\compile-tcc.bat)

del *.ilk 2> nul
del *.obj 2> nul
del *.lib 2> nul
del *.exp 2> nul
