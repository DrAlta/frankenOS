/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: RockMusic.c
 *
 * Description:
 *             	Shows how to create, play and install new MIDI alarm sounds.
 *              Also uses a gadget for UI, and though the UI is a bit strange,
 *              it's an acceptable sample of how to use a gadget.
 *
 * History:
 *                    2/12/98 David Fedor - created.
 * 	 
 * 	 
 * Note: the UI in here is a bit strange, but it does in fact work.
 *       It'll be revised in the near term.  The MIDI handling is
 *       reasonably correct at this point.
 *
 *****************************************************************************/

#include <PalmOS.h>				// all the system toolbox headers
#include <FeatureMgr.h>			// Needed to get the ROM version
#include <SoundMgr.h>
#include "RockMusicRsc.h"		// application resource defines
#include "makeSMF.h"			// utility routines to create an SMF on the fly

// now many notes can be displayed/edited
#define maxNotes 5
// the UI for the pitch values is offset since really low tones are inaudible
#define pitchOffset 40


/***********************************************************************
 * Global variables for this module
 **********************************************************************/
static UInt32 gRomVersion;
static Int16 CurrentView;			// id of current form
static Int16 pitches[maxNotes];
static Int16 durations[maxNotes];
static CustomPatternType myPattern [8] = 
	{
	0xAA, 0x55,
	0xAA, 0x55,
	0xAA, 0x55,
	0xAA, 0x55,
	};

/***********************************************************************
 * Global defines for this module
 **********************************************************************/
#define version20	0x02000000	// PalmOS 2.0 version number
#define version30	0x03000000	// PalmOS 3.0 version number


/***********************************************************************
 * Prototypes for internal functions
 **********************************************************************/
static Boolean StartApplication(void);
static void StopApplication(void);								// clean up before app exit
static void MainViewInit(void);									// initialize data, objects for the main form
static Boolean MainViewHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);			// handle form load events
static void EventLoop(void);
static void DrawGadget();
static void GadgetTapped();
static void PlayShortSound(int theNote);
static int AddSmfToDatabase(MemHandle smfH, Char * trackName);
static MemHandle CreateTheSMF();



/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  This routine sets up the initial state of the application.
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     false for success.
 *
 ***********************************************************************/
static Boolean StartApplication(void)
{
	// find out what version of the OS we're running on
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &gRomVersion);
	
	// Set current view so the first form to display is the main form.
	CurrentView = RockMusicMainForm;
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: Check that the ROM version meets your
 *              minimum requirement.  Warn if the app was switched to.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags indicating how the application was
 *											 launched.  A warning is displayed only if
 *                                these flags indicate that the app is 
 *											 launched normally.
 *
 * RETURNED:    zero if rom is compatible else an error code
 *                             
 ***********************************************************************/
static Err RomVersionCompatible (UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;
	
	// See if we're on in minimum required version of the ROM or later.
	// The system records the version number in a feature.  A feature is a
	// piece of information which can be looked up by a creator and feature
	// number.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
		{
		// If the user launched the app from the launcher, explain
		// why the app shouldn't run.  If the app was contacted for something
		// else, like it was asked to find a string by the system find, then
		// don't bother the user with a warning dialog.  These flags tell how
		// the app was launched to decided if a warning should be displayed.
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
			{
			FrmAlert (RomIncompatibleAlert);
		
			// Pilot 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.  The sysFileCDefaultApp is considered "safe".
			if (romVersion < 0x02000000)
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			}
		
		return (sysErrRomIncompatible);
		}

	return 0;
}

/***********************************************************************
 *
 * FUNCTION:    StopApplication
 *
 * DESCRIPTION: This routine closes the application's database
 *              and saves the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void StopApplication(void)
{
	// Close all open forms to allow their frmCloseEvent handlers
	// to execute.  An appStopEvent doesn't send frmCloseEvents.
	// FrmCloseAllForms will send all opened forms a frmCloseEvent.
	FrmCloseAllForms ();
}


/***********************************************************************
 *
 * FUNCTION:		MainViewInit
 *
 * DESCRIPTION:     Initializes the main form and the list object.
 *
 * PARAMETERS:		frm - pointer to the main form.
 *
 * RETURNED:		Nothing.
 *
 ***********************************************************************/
static void MainViewInit(void)
{
	FormPtr			frm;
	int i;
	
	// initialize the notes to something reasonable
	for (i=0; i<maxNotes; i++) {
		pitches[i]=pitchOffset + 24 + (i*6);
		durations[i]=10;
	}
	
	
	// Get a pointer to the main form.
	frm = FrmGetActiveForm();

	// Draw the form.
	FrmDrawForm(frm);
}

/***********************************************************************
 *
 * FUNCTION:		DrawGadget
 *
 * DESCRIPTION:	    Handles drawing the note/duration gadget
 *
 * RETURNED:		Nothing.
 *
 ***********************************************************************/
void DrawGadget()
{
	UInt16 			objIndex;
	FormPtr 		frm;
	RectangleType 	r;
	int i;
	CustomPatternType origPattern [8];
	int leftEdge, rightEdge;
	
	frm = FrmGetActiveForm();
	if (!frm) 
		return;
	objIndex = FrmGetObjectIndex (frm, RockMusicMainInputGadget);
	FrmGetObjectBounds (frm, objIndex, &r);

	leftEdge = r.topLeft.x + 3;
	rightEdge = r.extent.x - 6;
	
	WinDrawRectangleFrame(roundFrame, &r);
	
	// draw the lines for the notes
	WinGetPattern(origPattern);
	WinSetPattern(myPattern);
	
	r.topLeft.y += 3;
	r.extent.y = 4;
	for (i=0; i<maxNotes; i++) {
		r.topLeft.x = leftEdge;
		r.extent.x = (2 * (pitches[i]-pitchOffset));
		WinFillRectangle(&r, 0);

		r.topLeft.x = leftEdge + r.extent.x;
		r.extent.x = rightEdge - r.topLeft.x;
		WinEraseRectangle(&r, 0);

		r.topLeft.y += 6;

		r.topLeft.x = leftEdge;
		r.extent.x = (durations[i]);
		WinFillRectangle(&r, 0);

		r.topLeft.x = leftEdge + r.extent.x;
		r.extent.x = rightEdge - r.topLeft.x;
		WinEraseRectangle(&r, 0);

		r.topLeft.y += 14;
	}

	WinSetPattern(origPattern);
}

/***********************************************************************
 *
 * FUNCTION:		PlayShortSound
 *
 * DESCRIPTION:	    Plays the given MIDI note for a short period of time
 *
 * RETURNED:		Nothing.
 *
 ***********************************************************************/
void PlayShortSound(int theNote)
{
	SndCommandType cmd;
	int octave;
	float freq;
	float cheapLogTable[] = {1.0, 1.0595, 1.1225, 1.1892, 1.2599, 1.3348, 1.4142, 1.4983, 1.5874, 1.6818, 1.7818, 1.8877};
	
	if (gRomVersion >= version30) {
		cmd.cmd = sndCmdNoteOn;
		cmd.param1 = theNote;		// the midi note to play
		cmd.param2 = 300;  			// milliseconds to play the note
		cmd.param3 = 127;			// play at max. amplitude
		
		// play the sound asynchronously
		SndDoCmd(0, &cmd, true);
	}
	else {	 // we're on a device which doesn't support MIDI... use the old mechanisms.
		// hack a conversion from the midi note to the frequency (this isn't exact but is close)
		octave = theNote / 12;
		theNote = theNote - (octave * 12);
		freq = 55 * (cheapLogTable[theNote]);
		while (octave >3) {		// we're starting at baseline of 55hz already so don't go all the way
			freq = freq * 2;
			octave--;
		}
		
		// play the sound (synchronously since that's all that 2.0 supports)
		cmd.cmd = sndCmdFreqDurationAmp;
		cmd.param1 = freq;		// the frequency to play
		cmd.param2 = 200;  		// milliseconds to play the note
		cmd.param3 = sndMaxAmp;	// play at max. amplitude
		
		SndDoCmd(0, &cmd, true);
	}
}


/***********************************************************************
 *
 * FUNCTION:		GadgetTapped
 *
 * DESCRIPTION:	    Handles processing of taps on the note/duration gadget.
 *                  This code (and UI) is pretty rough; needs improvement...
 *
 * PARAMETERS:		x and y coordinates of the tap
 *
 * RETURNED:		Nothing.
 *
 ***********************************************************************/
void GadgetTapped()
{
	int line, setting, lastSetting;
	UInt16 			objIndex;
	FormPtr 		frm;
	RectangleType   r;	
	Boolean changingPitch;
	Int16 x,y;
	Boolean penDown;
	
	// convert to gadget coordinates
	frm = FrmGetActiveForm();
	if (!frm) 
		return;
	objIndex = FrmGetObjectIndex (frm, RockMusicMainInputGadget);
	FrmGetObjectBounds (frm, objIndex, &r);

	PenGetPoint (&x, &y, &penDown);
	y -= r.topLeft.y;
	line = (y / 20);
	if (line > maxNotes)
		return;
	changingPitch = ((y % 20) < 8);
	
	lastSetting = -1;
	
	do {
		x -= r.topLeft.x;		// convert to local coordinates
		setting = (x-3) >> 1;
		if (((setting >= 0) && (setting < 64)) && (setting != lastSetting)) {
			if (changingPitch) {
				pitches[line] = setting + pitchOffset;
				PlayShortSound(pitches[line]);
			}
			else
				durations[line] = setting * 2;
			DrawGadget();
			lastSetting = setting;
		}
		
		PenGetPoint (&x, &y, &penDown);
		if (!RctPtInRectangle (x, y, &r))
			penDown = false;
	} while (penDown);
}


/***********************************************************************
 *
 * FUNCTION:		CreateTheSMF
 *
 * DESCRIPTION:	    Generates an SMF from our UI structures
 *
 * PARAMETERS:		none
 *
 * RETURNED:		A handle to the new SMF, or null if an error occurs.
 *
 ***********************************************************************/
MemHandle CreateTheSMF()
{
	MemHandle smfH;
	int i;
	
	smfH = StartSMF();
	for (i=0; i<maxNotes; i++)
		smfH = AppendNote(smfH, pitches[i], 10 * durations[i], 127, 10);  // max velocity, 10 ms pause between notes
	smfH = FinishSMF(smfH);

	return smfH;
}

/***********************************************************************
 *
 * FUNCTION:		AddSmfToDatabase
 *
 * DESCRIPTION:	    Does the database work to install an alarm SMF
 *
 * PARAMETERS:		The SMF, and the user-visible name for it.
 *
 * RETURNED:		0 for success, otherwise something nonzero.
 *
 * REVISION HISTORY:  2/12/98 David Fedor - created.
 *					blt	10/25/99	DmReleaseRecord() needs to be called 
 *									after DmNewRecord() or DmGetRecord()
 *									to clear the busy bit.  Also changed
 *									recIndex to add record to the beginning
 *									of the db.
 *
 ***********************************************************************/
// Useful structure field offset macro
#define prvFieldOffset(type, field)		((UInt32)(&((type*)0)->field))

// returns 0 for success, nonzero for error
int AddSmfToDatabase(MemHandle smfH, Char * trackName)
{
	Err					err = 0;
	DmOpenRef			dbP;
	UInt16				recIndex;
	MemHandle			recH;
	void *				recP;
//	UInt8*				recP;
	UInt8*				smfP;
	UInt8 				bMidiOffset;
	UInt32				dwSmfSize;
	SndMidiRecHdrType	recHdr;
	
	bMidiOffset = sizeof(SndMidiRecHdrType) + StrLen(trackName) + 1;
	dwSmfSize = MemHandleSize(smfH);
	
	recHdr.signature = sndMidiRecSignature;
	recHdr.reserved = 0;
	recHdr.bDataOffset = bMidiOffset;

	dbP = DmOpenDatabaseByTypeCreator(sysFileTMidi, sysFileCSystem, dmModeReadWrite | dmModeExclusive);
	if (!dbP)
		return 1;

	// Allocate a new record for the midi resource
//	recIndex = dmMaxRecordIndex;
	recIndex = 0;	// It's generally better to add to the beginning of the db, since 
					// deleted and archived records are kept at the back of the db.
	recH = DmNewRecord(dbP, &recIndex, dwSmfSize + bMidiOffset);
	if ( !recH )
		return 2;
	
	// Lock down the source SMF and the target record and copy the data
	smfP = MemHandleLock(smfH);
	recP = MemHandleLock(recH);
	
	err = DmWrite(recP, 0, &recHdr, sizeof(recHdr));
	if (!err) err = DmStrCopy(recP, prvFieldOffset(SndMidiRecType, name), trackName);
	if (!err) err = DmWrite(recP, bMidiOffset, smfP, dwSmfSize);
	
	// Unlock the pointers
	MemHandleUnlock(smfH);
	MemHandleUnlock(recH);
	
	DmReleaseRecord(dbP, recIndex, 1);
	//DmReleaseRecord(recP, recIndex, 1);
	
	//DmResetRecordStates(dbP);
	DmCloseDatabase(dbP);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:		MainViewHandleEvent
 *
 * DESCRIPTION:	    Handles processing of events for the ÒmainÓ form.
 *
 * PARAMETERS:		event	- the most recent event.
 *
 * RETURNED:		True if the event is handled, false otherwise.
 *
 * REVISION HISTORY:  2/12/98 David Fedor - created.
 *					blt	10/25/99	FrmDrawForm() clears the screen, so
 *									the gadget has to be drawn _after_ 
 *									a FrmDrawForm(). 
 *									Added MemHandleFree(smfH);
 *
 ***********************************************************************/
static Boolean MainViewHandleEvent(EventPtr event)
{
	Boolean			handled = false;
	UInt16 			objIndex;
	FormPtr 		frm;
	RectangleType 	r;
	MemHandle 		smfH;

	switch (event->eType) {
  		case frmOpenEvent:	// The form was told to open.
  			// Initialize the main form.
			MainViewInit();
			frm = FrmGetActiveForm ();
			FrmDrawForm (frm);
			DrawGadget();
			handled = true;
			break;
		
		case frmUpdateEvent:
			frm = FrmGetActiveForm ();
			FrmDrawForm (frm);
			DrawGadget();
			handled = true;
			break;
			
	   	case ctlSelectEvent:  // A control button was pressed and released.
		   	if (event->data.ctlEnter.controlID == RockMusicMainPlayButton) {
				smfH = CreateTheSMF();
				if (smfH) {
					if (gRomVersion >= version30) {
						SndPlaySmf(NULL, sndSmfCmdPlay, MemHandleLock(smfH), NULL, NULL, NULL, false);
						MemHandleUnlock(smfH);
					}
					else
						FrmCustomAlert(SaySomethingAlert, "Sorry, this version of PalmOS doesn't support playing SMF sounds.", " ", " ");
				}
				else
					FrmCustomAlert(SaySomethingAlert, "Couldn't create the SMF.", " ", " ");
				handled = true;
			}
		   	else if (event->data.ctlEnter.controlID == RockMusicMainInstallButton) {
				smfH = CreateTheSMF();
				if (smfH) {
					if (0==AddSmfToDatabase(smfH, "NewSound"))
						FrmCustomAlert(SaySomethingAlert, "The new sound was successfully installed.", " ", " ");
					else
						FrmCustomAlert(SaySomethingAlert, "Couldn't install the SMF.", " ", " ");
				}
				else
					FrmCustomAlert(SaySomethingAlert, "Couldn't create the SMF.", " ", " ");
				handled = true;
			}
			MemHandleFree(smfH);
			break;

	 	case penDownEvent:
			frm = FrmGetActiveForm ();
			objIndex = FrmGetObjectIndex (frm, RockMusicMainInputGadget);
			FrmGetObjectBounds (frm, objIndex, &r);
			if (RctPtInRectangle (event->screenX, event->screenY, &r)) {
				GadgetTapped ();
				handled=true;
			}
	 		break;
	}

	return(handled);
}


/***********************************************************************
 *
 * FUNCTION:    ApplicationHandleEvent
 *
 * DESCRIPTION: Loads a form resource and sets the event handler for the form.
 *
 * PARAMETERS:  event - a pointer to an EventType structure
 *
 * RETURNED:    True if the event has been handled and should not be
 *						passed to a higher level handler.
 *
 ***********************************************************************/
static Boolean ApplicationHandleEvent(EventPtr event)
{
	FormPtr	frm;
	Int16		formId;
	Boolean	handled = false;

	if (event->eType == frmLoadEvent)
		{
		// Load the form resource specified in the event then activate the form.
		formId = event->data.frmLoad.formID;
		frm = FrmInitForm(formId);
		FrmSetActiveForm(frm);

		// Set the event handler for the form.  The handler of the currently 
		// active form is called by FrmDispatchEvent each time it receives an event.
		switch (formId)
			{
			case RockMusicMainForm:
				FrmSetEventHandler(frm, MainViewHandleEvent);
				break;
			}
		handled = true;
		}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:		EventLoop
 *
 * DESCRIPTION:	A simple loop that obtains events from the Event
 *						Manager and passes them on to various applications and
 *						system event handlers before passing them on to
 *						FrmDispatchEvent for default processing.
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		Nothing.
 *
 ***********************************************************************/
static void EventLoop(void)
{
	EventType	event;
	UInt16			error;
	
	do
		{
		// Get the next available event.
		EvtGetEvent(&event, evtWaitForever);
		
		// Give the system a chance to handle the event.
		if (! SysHandleEvent(&event))

			// Give the menu bar a chance to update and handle the event.	
			if (! MenuHandleEvent(0, &event, &error))

				// Give the application a chance to handle the event.
				if (! ApplicationHandleEvent(&event))

					// Let the form object provide default handling of the event.
					FrmDispatchEvent(&event);
		}
	while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:		PilotMain
 *
 * DESCRIPTION:	This function is the equivalent of a main() function
 *						under standard ÒCÓ.  It is called by the Emulator to begin
 *						execution of this application.
 *
 * PARAMETERS:		cmd - command specifying how to launch the application.
 *						cmdPBP - parameter block for the command.
 *						launchFlags - flags used to configure the launch.			
 *
 * RETURNED:		Any applicable error codes.
 *
 ***********************************************************************/
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	UInt16 error;		// error starting the app
	
	// This app makes use of PalmOS 2.0 features.  It will crash if
	// run on an earlier version of PalmOS.  Detect and warn if this happens,
	// then exit.
	// (There are other checks for 3.0 inside the code; it'll work somewhat on a 2.0 device.)
	error = RomVersionCompatible (version20, launchFlags);
	if (error) return error;

	// We only handle a normal launch... all other messages are ignored.
	if (cmd == sysAppLaunchCmdNormalLaunch)
		{
		// Initialize the application's global variables and database.
		if (!StartApplication())
			{
			// Start the first form.
			FrmGotoForm(CurrentView);
				
			// Start the event loop.
			EventLoop();
			
			// Clean up before exiting the applcation.
			StopApplication();
			}
		}
		
	return 0;
}

