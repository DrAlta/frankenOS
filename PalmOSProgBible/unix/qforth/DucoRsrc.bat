@echo off
rem Requires ASDK1a1 or later.

rem Create Duco.res and type####.bin files:
pilrc -r DucoRsrc.res Duco.rcp

rem Create Duco.asm from Duco.rcp:
echo   appl "DucoResources",'p4ap' >DucoRsrc.asm
echo   res 'WBMP',$7ffe,"IconBig.bmp" >>DucoRsrc.asm
type DucoRsrc.res >>DucoRsrc.asm

rem Assemble DucoRsrc.asm into DucoRsrc.prc:
pila -t Duco DucoRsrc.asm

