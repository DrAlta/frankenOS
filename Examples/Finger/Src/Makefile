###############################################################################
#
# Copyright (c) 2000 Palm Computing, Inc. or its subsidiaries.
# All rights reserved.
#
# File: Makefile
#
###############################################################################

# This Makefile would be a little tidier if we put .rcp files in the same
# directory as the source code.
R = ../Rsc

CC = m68k-palmos-gcc
CFLAGS = -O2 -Wall -I $(R)

Finger.prc: Finger tver0001.bin
	build-prc Finger.prc "Finger" Fngr Finger *.bin

Finger: FingerMain.o FingerNet.o
	$(CC) -o Finger FingerMain.o FingerNet.o -lNetSocket

FingerMain.o: FingerMain.c $(R)/FingerRsc.h
FingerNet.o: FingerNet.c $(R)/FingerRsc.h

tver0001.bin: $(R)/Finger.rcp $(R)/FingerRsc.h \
              $(R)/Finger.pbitm $(R)/NetPulse0.pbitm \
              $(R)/NetPulse1.pbitm $(R)/NetPulse2.pbitm \
              $(R)/NetPulse3.pbitm $(R)/NetPulse4.pbitm
	pilrc -I $(R) $(R)/Finger.rcp

clean:
	-rm Finger *.o *.bin

