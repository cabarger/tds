@echo off

set linker_flags=user32.lib gdi32.lib

del *.pdb > NUL 2> NUL
cl code\win32_tds.cpp /link %linker_flags%

