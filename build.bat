@echo off

set linker_flags=user32.lib gdi32.lib winmm.lib OpenGL32.lib
set compiler_flags=-Zi /std:c++17

del *.pdb > NUL 2> NUL
cl code\win32_tds.cpp %compiler_flags%  /link %linker_flags% 
