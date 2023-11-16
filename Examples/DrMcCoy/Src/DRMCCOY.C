/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: DRMCCOY.C
 *
 * Description:
 *             	 Upon launch, sets the bit in the database header which
 *               tells the launcher that this app shouldn't be beamable.
 *
 *               The ProtectOurDatabase() routine is the only code in
 *               here that does anything besides show a minimal UI.
 *
 * History:
 *                   written 2/11/98 by David Fedor
 *
 *****************************************************************************/

#include <PalmOS.h>				// all the system toolbox headers
#include <FeatureMgr.h>			// Needed to get the ROM version
#include "DrMcCoyRsc.h"			// application resource defines


/***********************************************************************
 * Global variables for this module
 **********************************************************************/
static Int16				CurrentView;			// id of current form


/***********************************************************************
 * Global defines for this module
 **********************************************************************/
#define version20	0x02000000	// PalmOS 2.0 version number


/***********************************************************************
 * Prototypes for internal functions
 **********************************************************************/
static Boolean StartApplication(void);
static void StopApplication(void);								// clean up before app exit
static void MainViewInit(void);									// initialize data, objects for the main form
static Boolean MainViewHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);			// handle form load events
static void EventLoop(void);





/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  This routine sets up the initial state of the application.
 *               It opens the application's database and sets up global variables. 
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     true if error (database couldn't be created)
 *
 ***********************************************************************/
static Boolean StartApplication(void)
{
	// Set current view so the first form to display is the main form.
	CurrentView = DrMcCoyMainForm;
	
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
 *								  launched.  A warning is displayed only if
 *                                these flags indicate that the app is 
 *								  launched normally.
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

	// Get a pointer to the main form.
	frm = FrmGetActiveForm();

	// Draw the form.
	FrmDrawForm(frm);
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
 ***********************************************************************/
static Boolean MainViewHandleEvent(EventPtr event)
{
	Boolean		handled = false;


	switch (event->eType)
		{
  		case frmOpenEvent:	// The form was told to open.
  			// Initialize the main form.
			MainViewInit();
			handled = true;
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
 *				passed to a higher level handler.
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
			case DrMcCoyMainForm:
				FrmSetEventHandler(frm, MainViewHandleEvent);
				break;
			}
		handled = true;
		}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:	EventLoop
 *
 * DESCRIPTION:	A simple loop that obtains events from the Event
 *				Manager and passes them on to various applications and
 *				system event handlers before passing them on to
 *				FrmDispatchEvent for default processing.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
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
 * FUNCTION:	 ProtectOurDatabase
 *
 * DESCRIPTION:	 Sets the bit in the database header which tells the
 *               launcher that this app shouldn't be beamable.
 *
 *				 Note that this function assumes we're the active UI app.
 *				 (See note in the code for what to do if you're not.)
 *				 
 *				 Once this routine has been run, the launcher will not
 *				 allow this app to be beamed.  You can call this routine
 *				 as many times as you want; calling it when the app is
 *				 launched is convenient and won't slow down the rest of
 *				 the OS by wasting time during the other launch codes.
 *				 
 *				 Setting this bit at compile-time would be better, but
 *				 none of the current tools allow this yet.  When they
 *				 do, you can get rid of this routine.
 *
 ***********************************************************************/
static void ProtectOurDatabase()
{
// temporary definition, in case old headers are being used
#ifndef dmHdrAttrCopyPrevention
#define	dmHdrAttrCopyPrevention		0x0040
#endif

	UInt16 cardNo;
	LocalID dbID;
	UInt16 attributes;
	
	// Find our database - only works if you're the running UI application.
	// If you need to do this when you're not the running app, then call
	// DmFindDatabase() with your app's database name instead.
	SysCurAppDatabase(&cardNo, &dbID);
	
	if (dbID) {
		// get the current attributes, turn on protection, and save them.
		DmDatabaseInfo(cardNo, dbID, 0, &attributes, 0,0,0,0,0,0,0,0,0);
		attributes = attributes | dmHdrAttrCopyPrevention;
		DmSetDatabaseInfo(cardNo, dbID, 0, &attributes, 0,0,0,0,0,0,0,0,0);
	}
}

/***********************************************************************
 *
 * FUNCTION:	PilotMain
 *
 * DESCRIPTION:	This function is the equivalent of a main() function
 *				under standard ÒCÓ.  It is called by the Emulator to begin
 *				execution of this application.
 *
 * PARAMETERS:	cmd - command specifying how to launch the application.
 *				cmdPBP - parameter block for the command.
 *				launchFlags - flags used to configure the launch.			
 *
 * RETURNED:	Any applicable error codes.
 *
 ***********************************************************************/
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	UInt16 error;		// error starting the app
	
	// This app makes use of PalmOS 2.0 features.  It will crash if
	// run on an earlier version of PalmOS.  Detect and warn if this happens,
	// then exit.
	error = RomVersionCompatible (version20, launchFlags);
	if (error)
		return error;

	// Check for a normal launch.
	if (cmd == sysAppLaunchCmdNormalLaunch)
		{
		ProtectOurDatabase();  // don't let us be beamed around

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

