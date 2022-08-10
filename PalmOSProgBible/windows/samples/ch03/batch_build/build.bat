REM This batch file builds the Hello World sample application from Chapter 4.

m68k-palmos-gcc -O2 -c hello.c -o hello.o 
m68k-palmos-gcc -O2 hello.o -o hello 
m68k-palmos-obj-res hello 
pilrc hello.rcp
build-prc hello.prc "Hello" LFhe *.grc *.bin
 