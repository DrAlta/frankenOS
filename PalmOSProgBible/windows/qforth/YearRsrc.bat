@echo off
rem Requires ASDK1a1 or later.

rem Create Year.res and type####.bin files:
pilrc -r YearRsrc.res Year.rcp

rem Create Year.asm from Year.rcp:
echo   appl "YearResources",'p4ap' >YearRsrc.asm
echo   res 'WBMP',$7ffe,"IconBig.bmp" >>YearRsrc.asm
type YearRsrc.res >>YearRsrc.asm

rem Assemble YearRsrc.asm into YearRsrc.prc:
pila -t Year YearRsrc.asm

