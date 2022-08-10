##############################################################################
#
#  Makefile: Build the entire LispMe system                          
#                                                                    
#  LispMe System (c) FBI Fred Bayer Informatics                      
#                                                                    
#  Distributed under the GNU General Public License;                 
#  see the README file. This code comes with NO WARRANTY.            
#                                                                    
#  Modification history                                              
#                                                                    
#  When?      What?                                              Who?
#  -------------------------------------------------------------------
#  27.07.1997 New                                                FBI
#  25.10.1999 Prepared for GPL release                           FBI
#  30.03.2000 Adapted for PRC-tools 2.0 and SDK 3.5              FBI
#
##############################################################################

##############################################################################
#                                  
# This makefile is for Windows NT 4.0 using the GNU C compiler and PRC tools 
# for Pilot, version 2.0           
#
##############################################################################

##############################################################################
#
# change these for your environmant
#
##############################################################################
MATHLIB  = E:/pilot/sdk/mathlib
HAPI     = E:/pilot/handbase21/hapi

##############################################################################
# Application ID
##############################################################################
APPID    = fbLM
ADDEF    = -Wall

##############################################################################
# shell, compiler, tools
##############################################################################
SHELL    = sh
CC       = m68k-palmos-gcc
AS       = m68k-palmos-gcc
BUILDPRC = build-prc
OBJRES   = obj-res

##############################################################################
# compiler and linker flags
##############################################################################
CFLAGS   = -Wall -O2 -DAPPID=\'$(APPID)\' -I$(MATHLIB) -I$(HAPI) \
           -Wimplicit -Wmissing-declarations
ASFLAGS  = -c

#-----------------------------------------------------------------------------
# Dependencies
#-----------------------------------------------------------------------------
LispMe.o:  LispMe.c LispMe.h comp.h fpstuff.h store.h hbase.h
comp.o:    comp.c LispMe.h comp.h store.h
comp.s:    comp.c LispMe.h comp.h store.h
store.o:   store.c LispMe.h store.h
io.o:      io.c io.h store.h
gui.o:     gui.c gui.h store.h
util.o:    util.c util.h store.h
vm.o:      vm.c vm.h store.h file.h hbase.h
file.o:    file.c file.h store.h
fpstuff.o: fpstuff.c fpstuff.h store.h
cplx.o:    cplx.c cplx.h fpstuff.h store.h
sess.o:    sess.c sess.h store.h
setup.o:   setup.c setup.h store.h
hbase.o:   hbase.c hbase.h store.h
graphic.o: graphic.c graphic.h store.h
ParHack.o: ParHack.c
Launch.o:  Launch.c

#-----------------------------------------------------------------------------
# build final prc from code, data, reloc and gui resources
# Extract only relevant code resource from Launch and put it into LispMe.prc
# under a different resource id
#-----------------------------------------------------------------------------
LispMe.prc: LispMe Launch LispMe.stamp
	$(OBJRES) Launch 
	mv code0001.Launch.grc code2329.Launch.grc
	$(BUILDPRC) LispMe.def LispMe *.bin code2329.Launch.grc
	rm -f *.bin
	rm -f *.grc

ParHack.prc: ParHack ParHack.stamp
	$(BUILDPRC) ParHack.def ParHack *.bin
	rm -f *.bin 

#-----------------------------------------------------------------------------
# gui resources
#-----------------------------------------------------------------------------
LispMe.stamp: LispMe.rcp Launch.rcp bitmaps/*.bmp
	pilrc -q Launch.rcp
	mv tAIB03e9.bin tAIB2329.bin
	mv tAIB03e8.bin tAIB2328.bin
	pilrc -q LispMe.rcp

ParHack.stamp: ParHack.rcp
	pilrc -q ParHack.rcp 

#-----------------------------------------------------------------------------
# linking coff-executable
#-----------------------------------------------------------------------------
LispMe:  LispMe.o util.o store.o fpstuff.o io.o hbase.o graphic.o \
         cplx.o gui.o comp.o vm.o file.o sess.o setup.o \
         LispMe-sections.o LispMe-sections.ld 

LispMe-sections.o: LispMe-sections.s

LispMe-sections.s LispMe-sections.ld: LispMe.def
	multigen LispMe.def 

ParHack: ParHack.o
	$(CC) -nostartfiles ParHack.o -o ParHack

Launch: Launch.o
	$(CC) -nostartfiles Launch.o -o Launch

##############################################################################
# directory cleanup
##############################################################################
clean: tidy
	rm -f *.prc  

tidy:
	rm -f *.o *.a LispMe ParHack Launch 
	rm -f LispMe-sections*

##############################################################################
# Build the entire LispMe system
##############################################################################
all:    LispMe.prc ParHack.prc

##############################################################################
# Create the archives
##############################################################################
zip:
	zip ../lisp_270.zip *.prc README doc/* gui/* samples/*
	zip -j ../lisp_270.zip $(MATHLIB)/MathLib.prc
	zip ../ParHack.zip ParHack.prc doc/lm_parh.htm
	zip ../lisp_270s.zip * doc/* gui/* samples/* bitmaps/*

