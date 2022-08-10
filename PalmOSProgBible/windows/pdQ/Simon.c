/******************************************************************************
QUALCOMM Incorporated
License Terms for pdQ SDK

pdQ SDK SOFTWARE IS PROVIDED TO THE USER "AS IS". QUALCOMM MAKES NO WARRANTIES,
EITHER EXPRESS OR IMPLIED, WITH RESPECT TO THE pdQ SDK SOFTWARE AND/OR ASSOCIATED
MATERIALS PROVIDED TO THE USER, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR AGAINST INFRINGEMENT. QUALCOMM
DOES NOT WARRANT THAT THE FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET YOUR
REQUIREMENTS, OR THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED OR
ERROR-FREE, OR THAT DEFECTS IN THE SOFTWARE WILL BE CORRECTED. FURTHERMORE, QUALCOMM
DOES NOT WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OR THE RESULTS OF
THE USE OF THE SOFTWARE OR ANY DOCUMENTATION PROVIDED THEREWITH IN TERMS OF THEIR
CORRECTNESS, ACCURACY, RELIABILITY, OR OTHERWISE. NO ORAL OR WRITTEN 
INFORMATION OR ADVICE GIVEN BY QUALCOMM OR A QUALCOMM AUTHORIZED REPRESENTATIVE SHALL
CREATE A WARRANTY OR IN ANY WAY INCREASE THE SCOPE OF THIS WARRANTY.

LIMITATION OF LIABILITY -- QUALCOMM AND ITS LICENSORS ARE NOT LIABLE FOR ANY CLAIMS
OR DAMAGES WHATSOEVER, INCLUDING PROPERTY DAMAGE, PERSONAL INJURY, INTELLECTUAL 
PROPERTY INFRINGEMENT, LOSS OF PROFITS, OR INTERRUPTION OF BUSINESS, OR FOR ANY SPECIAL,
CONSEQUENTIAL OR INCIDENTAL DAMAGES, HOWEVER CAUSED, WHETHER ARISING OUT OF BREACH OF
WARRANTY, CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY, OR OTHERWISE.

QUALCOMM grants to Distributors a nonexclusive, nontransferable license to
use, distribute and sublicense the pdQ SDK Software to its end user
customers, subject to the provisions of this Agreement.  Distributor's
sublicenses will not be materially inconsistent with the terms and
conditions of this license regarding the rights granted and obligations
imposed upon Distributor by QUALCOMM.

Copyright (c) 1999 by QUALCOMM Incorporated. All rights reserved.
QUALCOMM is a registered trademark and registered service mark of QUALCOMM
Incorporated. All other trademarks and service marks are the property of their
respective owners.
1/20/99 
******************************************************************************/


/******************************************************************************
      									Simon Sez                              
                                                                            
	This application demonstrates the usage of the pdQ Alert Library SDK by:
		Turning on the LED either Red, Green, or Orange.
		Turning off the LED.
		Turning the Vibrator on and off.

******************************************************************************/


// 1/6/99 c_mweiss
// Note: Parts of this sample code have been taken from Palm's stationery.
//			These include the functions: AppHandleEvent and AppEventLoop
//

/******************************************************************************
 *
 *   INCLUDES
 *
 ******************************************************************************/
// pilot includes
#include <Pilot.h>
#include <SysEvtMgr.h>

// pdQ includes
#include <pdqAlert.h>

// our includes
#include "Simon.h"
#include "Simon_res.h"

/*****************************************************************************
 *
 *   ENUMS AND STRUCTS
 *
 *****************************************************************************/
enum
{
	gs_Idle = 0,
	gs_Talk,
	gs_Listen	
};

enum
{
	led_Off = 0,
	led_Red,
	led_Orange,
	led_Green
};

/*****************************************************************************
 *
 *   INTERNAL CONSTANTS
 *
 *****************************************************************************/
#define appFileCreator			'simn'
#define appVersionNum			0x01
#define appPrefID					0x00
#define appPrefVersionNum 		0x01


// Define the minimum OS version we support
#define ourMinVersion			sysMakeROMVersion(2,0,0,sysROMStageRelease,0)


// sounds associated with LED colors. Frequency in Hz.
#define tone_Red					495
#define tone_Orange				392
#define tone_Green				294

// length in milliseconds that the sequence tone plays.
#define tone_Duration			200

// the maximum number of items in the sequence.
#define	maxSeqLen				100

// the max number of idle ticks to wait for the user to tap a key.
#define sListenCountMax			15	

/*****************************************************************************
 *
 *   STATIC VARS
 *
 *****************************************************************************/
// a reference to the pdQ alert library.
static UInt		sAlertRefNum = sysInvalidRefNum;	

// the current tristate of the game. Defualt to idle.
static int		sGameState = gs_Idle;				

// vars used by the computer to "play" the sequence of tones.
static int		sSequence[maxSeqLen];
static int		sSequenceIndex = 0;

static int		sListenIdleCount = 0;
static int		sListenIndex = 0;

/*****************************************************************************
 *
 *   FUNCTIONS
 *
 *****************************************************************************/

//****************************************************************************
// FUNCTION:  	PilotMain
//
// DESC:			This is the main entry point for the application.
//
// PARAMETERS:	cmd - word value specifying the launch code. 
//            	cmdPB - pointer to a structure that is associated with the 
//						launch code. 
//            	launchFlags -  word value providing extra information about the launch.
// RETURNED:	Result of launch
//
// REVISION HISTORY:
//****************************************************************************
DWord		PilotMain(
				Word	cmd,
				Ptr		cmdPBP,
				Word	launchFlags)
{
    // call our main specific to this app.
    return SimonPilotMain(cmd, cmdPBP, launchFlags);
}


//****************************************************************************
// FUNCTION:    SimonPilotMain
//
// DESC:			This is the main entry point for the application.
// PARAMETERS:	cmd - word value specifying the launch code. 
//					cmdPB - pointer to a structure that is associated with the launch code. 
//					launchFlags -  word value providing extra information about the launch.
//
// RETURNED:	Result of launch
//
// REVISION HISTORY:
//****************************************************************************
static DWord	SimonPilotMain(
						Word	cmd,
						Ptr	cmdPBP,
						Word	launchFlags)
{
	Err		error;

	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error)
	{
		return error;
	}


	switch (cmd)
	{
		case sysAppLaunchCmdNormalLaunch:
			error = AppStart();
			if (error)
			{
				return error;
			}
				
			FrmGotoForm(MainForm);
			AppEventLoop();
			AppStop();
			break;

		default:
			break;
	}
	
	return 0;
}


//****************************************************************************
// FUNCTION:	RomVersionCompatible
//
// DESC:			This routine checks that a ROM version is meet your minimum 
//						requirement.
//
// PARAMETERS:	requiredVersion - minimum rom version required
//						(see sysFtrNumROMVersion in SystemMgr.h for format)
//            	launchFlags - flags that indicate if the application UI is 
//						initialized.
//
// RETURNED:	error code or zero if rom is compatible
//
// REVISION HISTORY:
//****************************************************************************
static Err		RomVersionCompatible(
						DWord		requiredVersion,
						Word		launchFlags)
{
	DWord	romVersion;

	// See if we're on in minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
	{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) == (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
		{
			FrmAlert (RomIncompatibleAlert);
	
			// Pilot 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < sysMakeROMVersion(2,0,0,sysROMStageRelease,0))
			{
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
			}
		}
		return (sysErrRomIncompatible);
	}
	return (0);
}


//****************************************************************************
// FUNCTION:	GetObjectPtr
//
// DESC:			This routine returns a pointer to an object in the current form.
//
// PARAMETERS:	formId - id of the form to display
//
// RETURNED:	VoidPtr
//
// REVISION HISTORY:
//****************************************************************************
static VoidPtr	GetObjectPtr(
						Word		objectID)
{
	FormPtr		frmP;


	frmP = FrmGetActiveForm();
	return (FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID)));
}


#pragma mark -
//****************************************************************************
// FUNCTION:	AppHandleEvent
//
// DESC:			This routine loads form resources and set the event handler 
//						for the form loaded.
//
// PARAMETERS:	event - a pointer to an EventType structure.
//
// RETURNED:	true if the event has handle and should not be passed
//          		to a higher level handler.
//
// REVISION HISTORY:
//****************************************************************************
static Boolean		AppHandleEvent(
							EventPtr		eventP)
{
	Word		formId;
	FormPtr		frmP;


	if (eventP->eType == frmLoadEvent)
	{
		// Load the form resource.
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId)
		{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

			default:
				// ErrFatalDisplay("Invalid Form Load Event");
				break;

		}
		return true;
	}
	
	return false;
}


//****************************************************************************
// FUNCTION:	AppEventLoop
//
// DESC:			This routine is the event loop for the application.  
//
// PARAMETERS:	nothing
//
// RETURNED:	nothing
//
// REVISION HISTORY:
//****************************************************************************
static void		AppEventLoop()
{
	// vars
	Word			error;
	EventType	event;


	do
	{
		// We want more idle time, so change the default EvtGetEvent params.
		// Otherwise the nil events are too far apart to do anything useful.
		EvtGetEvent(&event, 30);
		
		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(0, &event, &error))
				if (! AppHandleEvent(&event))
					FrmDispatchEvent(&event);

	} while (event.eType != appStopEvent);
}


//****************************************************************************
// FUNCTION:	AppStart
//
// DESC:			Get the current application's preferences.
//
// PARAMETERS:	nothing
//
// RETURNED:	Err value 0 if nothing went wrong
//
// REVISION HISTORY:
//****************************************************************************
static Err		AppStart()
{
	// vars
	Err	error;

	// get a library reference number to the pdQ Alert Library.
	error = SysLibFind(PDQAlertLibName, &sAlertRefNum);
	if ((error != 0) || (sAlertRefNum == sysInvalidRefNum))
	{
		return -1;
	}
	
   return 0;
}


//****************************************************************************
// FUNCTION:	AppStop
//
// DESC:			Save the current state of the application.
//
// PARAMETERS:	nothing
//
// RETURNED:	nothing
//
// REVISION HISTORY:
//****************************************************************************
static void		AppStop()
{
}


#pragma mark -
//****************************************************************************
// FUNCTION:	MainFormInit
//
// DESC:			This routine initializes the MainForm form.
//
// PARAMETERS:	frm - pointer to the MainForm form.
//
// RETURNED:	nothing
//
// REVISION HISTORY:
//****************************************************************************
static void		MainFormInit(
						FormPtr	frmP)
{
	sGameState = gs_Idle;
	pdQAlert_SetLED(led_Off);
	pdQAlert_Vibrate(false);
}


//****************************************************************************
// FUNCTION:	MainFormNewGame
//
// DESC:			A new game is starting, init all the game vars, and create the 
//						random sequence array of colors & sounds.
//
// PARAMETERS:	None
//
// RETURNED:	Nothing
//
// REVISION HISTORY:
//****************************************************************************
static void		MainFormNewGame()
{
	// vars
	int		i = 0;
	int		randNum = 0;
	
	 if (FrmAlert(ReadyBeginAlert) == ReadyBeginCancel)
	 {
	 	return;
	 }
	 
	 // Default the score to 0
	sSequenceIndex = 0;
	sListenIndex = 0;
	
	// build up a sequence of random LED colors.
	for (i=0;i<maxSeqLen; i++)
	{
		randNum = SysRandom(0);
		if (randNum > 0x5555)
		{
			sSequence[i] = led_Red;
		}
		else if (randNum > 0x2AAA)
		{
			sSequence[i] = led_Orange;
		}
		else
		{
			sSequence[i] = led_Green;
		}
	}
	
	pdQAlert_SetLED(led_Off);
	pdQAlert_Vibrate(false);
	
	// the computer goes first.
	sGameState = gs_Talk;
}


//****************************************************************************
// FUNCTION:	MainFormGameOver
//
// DESC:			The game has ended, show the user their score, and run the
//						vibrator if they have lost the game. The only way to really 
//						"win" is to finish the whole sequence. But 100 items is 
//						pretty hard to do!
//
// PARAMETERS:	isLoser - if true then vibrate the phone, false do not vibrate.
//
// RETURNED:	Nothing
//
// REVISION HISTORY:
//****************************************************************************
static void		MainFormGameOver(
						Boolean		isLoser)
{
	// vars
	SndCommandType	sndCmdRec;
	CharPtr			tempString;
	
	if (isLoser == true)
	{
		MainFormNoSound(500);
		
		// init the sound record.
		sndCmdRec.cmd = sndCmdFreqDurationAmp;	// synchronous.
		sndCmdRec.param2 = tone_Duration;		// duration in milliseconds.
		sndCmdRec.param3 = sndMaxAmp;				// amplitude (0 - sndMaxAmp); if 0, will return immediately
		
		// vibrate on
		pdQAlert_Vibrate(true);
		
		// loser song.
		sndCmdRec.param2 = tone_Duration * 3;
		sndCmdRec.param1 = 164;
		SndDoCmd(0, &sndCmdRec, true);
		
		sndCmdRec.param2 = tone_Duration * 1;
		sndCmdRec.param1 = 184;
		SndDoCmd(0, &sndCmdRec, true);
		
		sndCmdRec.param2 = tone_Duration * 2;
		sndCmdRec.param1 = 195;
		SndDoCmd(0, &sndCmdRec, true);
		
		sndCmdRec.param2 = tone_Duration * 1;
		sndCmdRec.param1 = 164;
		SndDoCmd(0, &sndCmdRec, true);
	}
	
	// vibrate off. LED off.
	pdQAlert_SetLED(led_Off);
	pdQAlert_Vibrate(false);
	
	//wait a second before showing score.
	MainFormNoSound(1000);
	
	// show the user his score.
	tempString = MemPtrNew(24);
	MemSet(tempString, StrLen(tempString), 0);
	StrIToA(tempString, sSequenceIndex);
	FrmCustomAlert(YourScoreAlert, tempString, " ", " ");
	MemPtrFree(tempString);
	
	// reset the game state to idle. Game over.
	sGameState = gs_Idle;
}


//****************************************************************************
// FUNCTION:	MainFormDoCommand
//
// DESC:			This routine performs the menu command specified.
//
// PARAMETERS:	command - menu item id
//
// RETURNED:	nothing
//
// REVISION HISTORY:
//****************************************************************************
static Boolean		MainFormDoCommand(
							Word	command)
{
	Boolean		handled = false;

	switch (command)
	{
		case OptionsAboutSimonSez:
			MenuEraseStatus (0);
			FrmAlert(AboutBoxAlert);
			handled = true;
			break;
	}
	
	return handled;
}


//****************************************************************************
// FUNCTION:	MainFormHandleEvent
//
// DESC:			This routine is the event handler for the "MainForm" of 
//						this application.
//
// PARAMETERS:	eventP  - a pointer to an EventType structure
//
// RETURNED:	true if the event has handle and should not be passed to a 
//						higher level handler.
//
// REVISION HISTORY:
//****************************************************************************
static Boolean	MainFormHandleEvent(
						EventPtr	eventP)
{
    Boolean		handled = false;
    FormPtr 	frmP;

	switch (eventP->eType) 
	{
		case menuEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			MainFormInit(frmP);
			FrmDrawForm (frmP);
			handled = true;
			break;

		// button taps.
		case ctlSelectEvent:
			handled = MainFormHandleButtonTap(eventP->data.ctlSelect.controlID);
			break;
				
		case nilEvent:
				if (sGameState != gs_Idle)
				{
					MainFormHandleIdle();
				}	
			break;
		
		default:
			break;
		
	}
	
	return handled;
}


//****************************************************************************
// FUNCTION:	MainFormHandleButtonTap
//
// DESC:			An event routine to handle all the button taps. This keeps the
//						HandleEvent routine nice and clean.				
//
// PARAMETERS:	iButton - The button that the user tapped.
//
// RETURNED:	Nothing
//
// REVISION HISTORY:
//****************************************************************************
static Boolean	MainFormHandleButtonTap(
						Word	iButton)
{
	//vars
	Boolean			handled = false;
	SndCommandType	sndCmdRec;
	
	// handle buttons if idle.
	if (sGameState == gs_Idle)
	{
		// ignore taps on color buttons if we are not listening.
		CtlSetValue(GetObjectPtr(MainRedPushButton), false);
		CtlSetValue(GetObjectPtr(MainOrangePushButton), false);
		CtlSetValue(GetObjectPtr(MainGreenPushButton), false);
		
		// we accept game buttons in idle mode.
		switch (iButton)
		{
			case MainHowToPlayButton:
				FrmAlert(HowToPlayAlert);
				handled = true;
				break;
				
			case MainNewGameButton:
				MainFormNewGame();
				handled = true;
				break;
				
			case MainRedPushButton:
			case MainOrangePushButton:
			case MainGreenPushButton:
				FrmAlert(PressNewGameAlert);
				break;
		}
	}
	
	// ignore game buttons if not in idle mode.
	else if (sGameState == gs_Listen)
	{
		// init the sound record.
		sndCmdRec.cmd = sndCmdFreqDurationAmp;	// synchronous.
		sndCmdRec.param2 = tone_Duration;		// duration in milliseconds.
		sndCmdRec.param3 = sndMaxAmp;			// amplitude (0 - sndMaxAmp).
	
		switch (iButton)
		{		
			case MainRedPushButton:
			{
				// led on
				pdQAlert_SetLED(led_Red);
				
				// unhilite button and play button sound.
				CtlSetValue(GetObjectPtr(MainRedPushButton), false);
				sndCmdRec.param1 = tone_Red;
				SndDoCmd(0, &sndCmdRec, true);
				
				// if the user tapped the wrong color, then end the game.
				if (sSequence[sListenIndex] != led_Red)
				{
					MainFormGameOver(true);
				}
				
				// user hit the right color. Continue the sub-sequence.
				else
				{
					sListenIndex++;			// inc the listen index.
					sListenIdleCount = 0;	// reset idle counter.
				}
				
				// led off
				pdQAlert_SetLED(led_Off);
				
			}	
			break;	
				
			case MainOrangePushButton:
			{
				// led on
				pdQAlert_SetLED(led_Orange);
				
				// unhilite button and play button sound.
				CtlSetValue(GetObjectPtr(MainOrangePushButton), false);
				sndCmdRec.param1 = tone_Orange;
				SndDoCmd(0, &sndCmdRec, true);
				
				// if the user tapped the wrong color, then end the game.
				if (sSequence[sListenIndex] != led_Orange)
				{
					MainFormGameOver(true);
				}
				
				// user hit the right color. Continue the sub-sequence.
				else
				{
					sListenIndex++;			// inc the listen index.
					sListenIdleCount = 0;	// reset idle counter.
				}
				
				// led off
				pdQAlert_SetLED(led_Off);
			}	
			break;
				
			case MainGreenPushButton:
			{
				// led on
				pdQAlert_SetLED(led_Green);
				
				// unhilite button and play button sound.
				CtlSetValue(GetObjectPtr(MainGreenPushButton), false);
				sndCmdRec.param1 = tone_Green;
				SndDoCmd(0, &sndCmdRec, true);
				
				// if the user tapped the wrong color, then end the game.
				if (sSequence[sListenIndex] != led_Green)
				{
					MainFormGameOver(true);
				}
				
				// user hit the right color. Continue the sub-sequence.
				else
				{
					sListenIndex++;			// inc the listen index.
					sListenIdleCount = 0;	// reset idle counter.
				}
				
				// led off
				pdQAlert_SetLED(led_Off);
			}	
			break;			
		}
		
		// we have finished the current sub-sequence. play it again + 1.
		if (sListenIndex > sSequenceIndex)
		{
			// play one more tone next time.
			sSequenceIndex++;
			
			// wait 1 second before starting the next sequence.
			MainFormNoSound(1000);	
			
			// It's the computer's turn to talk now.
			sGameState = gs_Talk;
		}
	}
	
	return handled;
}


//****************************************************************************
// FUNCTION:	MainFormHandleIdle
//
// DESC:			When we are not processing and keypresses or button taps, this 
//						idle routine gets called. If the gameState = talk, then we 
//						start playing the sequence of tones and colors. If 
//						gameState = listen, then we keep checking for a time out
//						condition; If the user does not tap a color button after a
//						time period, then the game is ended.
//
// PARAMETERS:	None
//
// RETURNED:	Nothing
//
// REVISION HISTORY:
//****************************************************************************
static void		MainFormHandleIdle()
{
	// vars
	int					i = 0;
	SndCommandType		sndCmdRec;
	
	if (sGameState == gs_Idle)
	{
		return;	// we should not get here. game state idle does not dispatch idle events here.
	}
	
	else if (sGameState == gs_Talk)
	{
		// major winner!! Finished the whole sequence.
		if (sSequenceIndex >= maxSeqLen)
		{
			// go to idle mode, game over with a win!
			sGameState = gs_Idle;
			
			// finished a winner, do not play loser sound or vibrate.
			MainFormGameOver(false);
			
			return;
		}
		
		// init the sound record.
		sndCmdRec.cmd = sndCmdFreqDurationAmp;	// synchronous.
		sndCmdRec.param2 = tone_Duration;		// duration in milliseconds.
		sndCmdRec.param3 = sndMaxAmp;				// amplitude (0 - sndMaxAmp); if 0, will return immediately
		
		for (i=0; i<sSequenceIndex+1;i++)
		{
			switch (sSequence[i])
			{
				case led_Red:
					pdQAlert_SetLED(led_Red);
					sndCmdRec.param1 = tone_Red;
					SndDoCmd(0, &sndCmdRec, true);
					break;
					
				case led_Orange:
					pdQAlert_SetLED(led_Orange);
					sndCmdRec.param1 = tone_Orange;
					SndDoCmd(0, &sndCmdRec, true);
					break;
				
				case led_Green:
					pdQAlert_SetLED(led_Green);
					sndCmdRec.param1 = tone_Green;
					SndDoCmd(0, &sndCmdRec, true);
					break;
			}
			
			// make a sound gap between notes.
			MainFormNoSound(tone_Duration);
			pdQAlert_SetLED(led_Off);
		}
		
		sGameState = gs_Listen;
		sListenIdleCount = 0;
		sListenIndex = 0;
	}
	
	else if (sGameState == gs_Listen)
	{
		// increment the listening count. If the user goes to long without
		// hitting a key, then the game is over.
		sListenIdleCount++;
		
		if (sListenIdleCount > sListenCountMax)
		{
			MainFormGameOver(true);
		}
	}
}


//****************************************************************************
// FUNCTION:	MainFormNoSound
//
// DESC:			Play no sound for a certain duration. This is a lazy way to 
//						create a milliseconds delay. There is a side effect of a 
//						click being heard through the speaker at 1hz. It actually 
//						helps the game a bit though.The click gives you a clue 
//						that it is the users turn.
//
// PARAMETERS:	milliseconds - Number of milliseconds to play 1hz sound.
//
// RETURNED:	Nothing
//
// REVISION HISTORY:
//****************************************************************************
static void		MainFormNoSound(
						int		milliseconds)
{
	// vars
	SndCommandType		sndCmdRec;
	
	sndCmdRec.cmd = sndCmdFreqDurationAmp;	// synchronous.
	sndCmdRec.param2 = milliseconds;		// duration in milliseconds.
	sndCmdRec.param3 = sndMaxAmp;			// amplitude (0 - sndMaxAmp); if 0, will return immediately
	sndCmdRec.param1 = 1;					// 1 hz, is really low. Basically off.
	SndDoCmd(0, &sndCmdRec, true);
}

#pragma mark -
//****************************************************************************
// FUNCTION:	pdQAlert_Vibrate
//
// DESC:			Turns the phone vibrator on or off.
//
// PARAMETERS:	inVibOn - true = start vibrating, false = stop vibrating.
//
// RETURNED:	Nothing
//
// REVISION HISTORY:
//****************************************************************************
static void		pdQAlert_Vibrate(
						Boolean	inVibOn)
{
#if _VIBRATOR_SUPPORTED
// vibrator is not supported in version 1.0
	// vars
	Boolean		prevState = false;
	
	if (inVibOn)
	{
		PDQAlertSwitch(sAlertRefNum, VIBRATOR, ALERT_SWITCH_ON, &prevState);
	}
	else
	{
		PDQAlertSwitch(sAlertRefNum, VIBRATOR, ALERT_SWITCH_OFF, &prevState);
	}
#endif
}			


//****************************************************************************
// FUNCTION:	pdQAlert_SetLED
//
// DESC:			Turns leds on or off by color.
//
// PARAMETERS:	inLEDValue - the led color to show. Use led_Off for no color.
//
// RETURNED:	Nothing
//
// REVISION HISTORY:
//****************************************************************************
static void		pdQAlert_SetLED(
						int	inLEDValue)
{
	// vars
	Boolean		prevState = false;
	
	switch (inLEDValue)
	{
		case led_Off:
			PDQAlertSwitch(sAlertRefNum, LED_ORANGE, ALERT_SWITCH_OFF, &prevState);
			break;
			
		case led_Red:
			PDQAlertSwitch(sAlertRefNum, LED_RED, ALERT_SWITCH_ON, &prevState);
			break;
			
		case led_Orange:
			PDQAlertSwitch(sAlertRefNum, LED_ORANGE, ALERT_SWITCH_ON, &prevState);
			break;
			
		case led_Green:
			PDQAlertSwitch(sAlertRefNum, LED_GREEN, ALERT_SWITCH_ON, &prevState);
			break;
	}
}			




