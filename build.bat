@echo off
call build\clang.bat
rem call build\msvc.bat

del *.ilk 2> nul
del *.obj 2> nul
del *.lib 2> nul
del *.exp 2> nul
