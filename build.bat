@echo off

set c=%1

if not defined c set c=clang
echo %c%

if %c% == clang (call build\clang.bat)^
else if %c% == msvc (call build\msvc.bat)^
else if %c% == tcc (call build\tcc-c.bat)

del *.ilk 2> nul
del *.obj 2> nul
del *.lib 2> nul
del *.exp 2> nul
