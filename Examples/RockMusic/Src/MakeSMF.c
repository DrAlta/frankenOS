/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: MakeSMF.c
 *
 * Description:
 *             	utility routines to create an SMF on the fly
 *
 * History:
 *                    2/12/98 David Fedor - created.
 *
 *****************************************************************************/

#include <PalmOS.h>				// all the system toolbox headers
#include "makeSMF.h"

// A standard header for MIDI format 0 sounds, with one track.
const UInt8 MidiHeader[] = {
0x4D, 0x54, 0x68, 0x64,              // MThd
0x00, 0x00, 0x00, 0x06,              // header data is 6 bytes long
0x00, 0x00, 0x00, 0x01, 0x01, 0x90,  // format 0, 1 track, 0190 tempo (microseconds per quarter note)

0x4D, 0x54, 0x72, 0x6B,              // MTrk
0x00, 0x00, 0x00, 0x02,              // length in bytes of following data
0x00, 0x90							 // start playing a sound...
// then things like:
//             0x5C, 0x40,    // note 5c at velocity 40
// 0x81, 0x60, 0x5C, 0x00,    // after 160 ticks, turn off 5c
// (repeat above two lines ad nauseum)
// 0xFF, 0x2F, 0x00     // shut everything down
};
const MidiHeaderLength = 24;

MemHandle StartSMF()
{
	UInt8 *buf;
	MemHandle bufH;

	bufH = MemHandleNew(MidiHeaderLength);
	
	if (bufH) {
		buf = MemHandleLock(bufH);
		MemMove(buf, (void *) MidiHeader, MidiHeaderLength);
		MemHandleUnlock(bufH);
	}
	
	return bufH;
}

MemHandle AppendNote(MemHandle bufH, int note, int dur, int vel, int pause)
{
	UInt8 *buf;
	UInt32 bufsize, neededSize;
	int pos; // , startpos;
	
	if (!bufH) return 0;		// make error-handling easy for our caller
	
	// how big is our buffer?
	bufsize = MemHandleSize(bufH);
	buf = MemHandleLock(bufH);
	pos = (*((UInt32 *)(&buf[18]))) + MidiHeaderLength -2;	// position of next spot to put data
	MemHandleUnlock(bufH);
	
	// allocate more memory if necessary
	neededSize = 9 + pos; // 9 is maximum number of bytes we'd use in here
	if (bufsize < neededSize)
		if (MemHandleResize(bufH, neededSize))
			return 0;
	
	buf = MemHandleLock(bufH);
	
	// start the note
	buf[pos++] = note;	// frequency
	if (vel >= 128)		// velocity i.e. volume
		buf[pos++] = 0x80 + ((vel & 0x0780) >> 7);
	buf[pos++] = (vel & 0x07f);

	// wait for the specified duration
	if (dur >= 128)
		buf[pos++] = 0x80 + ((dur & 0x7f80) >> 7);
	buf[pos++] = (dur & 0x07f);

	// stop playing it
	buf[pos++] = note;
	buf[pos++] = 0;

	// pause afterwards
	if (pause >= 128)
		buf[pos++] = 0x80 + ((pause & 0x7f80) >> 7);
	buf[pos++] = (pause & 0x07f);
	
	// update header to show how much data is there
	*((UInt32 *)(&buf[18])) = pos - (MidiHeaderLength -2);

	MemHandleUnlock(bufH);
	return bufH;
}

MemHandle FinishSMF(MemHandle bufH)
{
	UInt8 *buf;
	UInt32 bufsize, neededSize;
	int pos;

	if (!bufH) return 0;		// make error-handling easy for our caller

	// how big is our buffer?
	bufsize = MemHandleSize(bufH);
	buf = MemHandleLock(bufH);
	pos = (*((UInt32 *)(&buf[18]))) + MidiHeaderLength -2;	// position of next spot to put data
	MemHandleUnlock(bufH);
	
	// allocate more memory if necessary
	neededSize = 3 + pos; // 3 more bytes to terminate things
	if (bufsize < neededSize)
		if (MemHandleResize(bufH, neededSize))
			return 0;
	
	buf = MemHandleLock(bufH);

	// now stop everything
	buf[pos++] = 0xff;
	buf[pos++] = 0x2f;
	buf[pos++] = 0x00;

	// update header to show how much data is there
	*((UInt32 *)(&buf[18])) = pos - (MidiHeaderLength -2);

	MemHandleUnlock(bufH);
	return bufH;
}

