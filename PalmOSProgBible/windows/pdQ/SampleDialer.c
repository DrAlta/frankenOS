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
      									SampleDialer                                 
                                                                            
  This is an application that provides a simple phone user interface. 
  Users can:                     
                                                                            
			Make outgoing calls (with pauses)
			Answer incoming calls (incl call waiting)
			Release and cancel pauses
			View phone state (incoming,missed,calling,conversation,idle, etc.)
			Generate DTMFs while in conversation state

******************************************************************************/


// 1/6/99 CCW
// Note: Parts of this sample code have been taken from Palm's stationery.
//			These include the functions: AppHandleEvent and AppEventLoop
//

#include <Pilot.h>
#include <SysEvtMgr.h>
#include "SampleDialerRsc.h"

#include "pdqCore.h"


/***********************************************************************
 *
 *   Globals
 *
 ***********************************************************************/

static Char gDigitCollect[MAX_DIGITS+1];
static UInt	gCoreRefNum = sysInvalidRefNum;
static CallInfoType	gCallInfo;
static Boolean gHardPause;

#define CallState gCallInfo.dwSignalHist

/***********************************************************************
 *
 *   Constants
 *
 ***********************************************************************/

#define SimpleDialerCreator			'QCDL'

#define SCREEN_TOP						15
#define SCREEN_LEFT						0
#define SCREEN_WIDTH						160
#define SCREEN_HEIGHT					30
#define SCREEN_TEXT_HEIGHT				15

#define MAX_TEXT_STR						64


/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/

static Boolean MainFormHandleEvent(EventPtr eventP);
static Boolean MainFormDoMenuCommand(Word command);
static Boolean AppHandleEvent( EventPtr eventP);
static void AppEventLoop(void);
DWord PilotMain( Word cmd, Ptr cmdPBP, Word launchFlags);
static void UpdateScreen(CharPtr line1, CharPtr line2);
static void HandleDigit(Char digit);
static void DispatchSignal(SignalParamsPtr param);



/***********************************************************************
 *
 * FUNCTION:   MainFormHandleEvent 
 *
 * DESCRIPTION:   handle opening main form
 *						handle button presses, grafitti, and menu 
 *
 * PARAMETERS:  eventP
 *
 * RETURNED:    true if handled
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr eventP)
{
    Boolean handled = false;
    FormPtr frmP;

	switch (eventP->eType) 
	{
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm (frmP);
			handled = true;
			UpdateScreen(NULL, NULL);
			break;

		case keyDownEvent:
			HandleDigit(eventP->data.keyDown.chr);
			break;

		case ctlSelectEvent:
		{
			switch (eventP->data.ctlSelect.controlID)
			{
				case MainSendButton:
					if(gHardPause)
						PDQTelResumeDialing(gCoreRefNum);
					else
					if(CallState&SGN_TAPI_INCOMING)
						PDQTelAnswerCall(gCoreRefNum);
					else						
					if(CallState&SGN_TAPI_CONVERSATION)
						HandleDigit(DIG_FLASH);
					else
						PDQTelMakeCall(gCoreRefNum, gDigitCollect[0] ? gDigitCollect : NULL);
					break;				
				
				case MainEndButton:
					gDigitCollect[0]=0;
					if(CallState&SGN_TAPI_CONVERSATION)
						PDQTelEndCall(gCoreRefNum);
					UpdateScreen(gDigitCollect, NULL);
					break;				
				
				case MainClearButton:
					if(gHardPause)
						PDQTelCancelPause(gCoreRefNum);
					else
					{
						UShort len = StrLen(gDigitCollect);
						if(len)
							gDigitCollect[--len]=0;
						UpdateScreen(gDigitCollect,NULL);
					}
					break;				

				case MainPoundButton:
					HandleDigit('#');
					break;
					
				case MainStarButton:
					HandleDigit('*');
					break;

				case Main0Button:
					HandleDigit('0');
					break;
				
				case Main1Button:
				case Main2Button:
				case Main3Button:
				case Main4Button:
				case Main5Button:
				case Main6Button:
				case Main7Button:
				case Main8Button:
				case Main9Button:
					HandleDigit(eventP->data.ctlSelect.controlID - Main1Button + '1');
					break;
			}
			break;			
		}		

		case menuEvent:
			handled =  MainFormDoMenuCommand(eventP->data.menu.itemID);
			break;
	}	
	return handled;
}



/***********************************************************************
 *
 * FUNCTION:   MainFormDoMenuCommand 
 *
 * DESCRIPTION:	Allow inserting of hard and soft pauses   
 *
 * PARAMETERS:  command - menu item
 *
 * RETURNED:    true if handled
 *
 ***********************************************************************/
static Boolean MainFormDoMenuCommand(Word command)
{
	Boolean handled = false;
	switch (command)
	{
		case MainOptionsAboutSampleDialer:
			{
			FormPtr frmP;
			frmP = FrmInitForm(AboutForm);
			FrmDoDialog(frmP);
			FrmDeleteForm(frmP);
			}
			break;
		case MainOptionsInsertTimedPause:
			handled = true;
			HandleDigit(DIG_SOFTPAUSE);
			break;
		case MainOptionsInsertUserPause:
			handled = true;
			HandleDigit(DIG_HARDPAUSE);
			break;
	}
	return handled;
}



//	1/6/99 CCW - Beginning of Palm code 
// The following have been copied verbatim from Palm Project stationery
//

/***********************************************************************
 *
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean AppHandleEvent( EventPtr eventP)
{
	Word formId;
	FormPtr frmP;

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
//				ErrFatalDisplay("Invalid Form Load Event");
				break;

			}
		return true;
		}
	
	return false;
}



/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void AppEventLoop(void)
{
	Word error;
	EventType event;


	do {
		EvtGetEvent(&event, evtWaitForever);
		
		
		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(0, &event, &error))
				if (! AppHandleEvent(&event))
					FrmDispatchEvent(&event);

	} while (event.eType != appStopEvent);
}

//
// 1/6/99 CCW - End of Palm Code
//


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION:   standard Palm main
 *						
 *
 * PARAMETERS:  	cmd
 *						cmdPBP
 *						launchFlags
 *
 * RETURNED:    0
 *
 ***********************************************************************/
DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
	Err err = 0;

	switch (cmd)
	{
		// This app only cares about signals when it's launched, so it registers on
		// startup and unregisters when quitting.
		// Apps that care about signals all the time may want to register for signals 
		// on sysAppLaunchCmdSystemReset instead of sysAppLaunchCmdNormalLaunch. 

		case sysAppLaunchCmdNormalLaunch:
			err = SysLibFind(PDQCoreLibName, &gCoreRefNum);			
			if(!err)
				err = PDQSigRegister(gCoreRefNum, SGN_CLASS_TAPI, SGN_ALL, PRIORITY_0, SimpleDialerCreator, sysFileTApplication);
			if(!err)
				PDQTelGetCallInfo(gCoreRefNum,&gCallInfo);
			else
				return 0;
				
			FrmGotoForm(MainForm);
			AppEventLoop();
			PDQSigUnregister(gCoreRefNum, SGN_CLASS_TAPI, SGN_ALL, SimpleDialerCreator, sysFileTApplication);
			break;

		// Application that care about signals should handle this launch code
		// This app should only receive signals when it is launched, but it checks 
		// the sysAppLaunchFlagSubCall anyway.
		
		// Apps that receive signals when not already launched (when sysAppLaunchFlagSubCall
		// flag is not set), won't be able to safely access globals. Developers may want to
		// store globals in a database. Another solution is to allocate memory on the dynamic 
		// heap, set the owner of the heap to 0, and stuff the handle or ptr in a feature.
		// Please note that dynamic memory is limited.		

		case sysAppLaunchCmd_PDQSignal:
			if(launchFlags&sysAppLaunchFlagSubCall)
				DispatchSignal((SignalParamsPtr)cmdPBP);
			break;
	}	
	return 0;
}



/***********************************************************************
 *
 * FUNCTION:	UpdateScreen    
 *
 * DESCRIPTION:   display text parameter or collected digits
 *
 * PARAMETERS:  	line1 - line 1
 *						line2 - line 2
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void UpdateScreen(CharPtr line1, CharPtr line2)
{
	RectangleType 	rect;

	if(line1)
	{
		RctSetRectangle(&rect,SCREEN_LEFT,SCREEN_TOP,SCREEN_WIDTH,SCREEN_TEXT_HEIGHT);
		WinEraseRectangle(&rect,0);
		WinDrawChars(line1,StrLen(line1),SCREEN_LEFT+3,SCREEN_TOP+2);
	}

	if(line2)
	{
		RctSetRectangle(&rect,SCREEN_LEFT,SCREEN_TOP+SCREEN_TEXT_HEIGHT,SCREEN_WIDTH,SCREEN_TEXT_HEIGHT);
		WinEraseRectangle(&rect,0);
		WinDrawChars(line2,StrLen(line2),SCREEN_LEFT+3,SCREEN_TOP+SCREEN_TEXT_HEIGHT);
	}

	RctSetRectangle(&rect,SCREEN_LEFT,SCREEN_TOP,SCREEN_WIDTH,SCREEN_HEIGHT);
	WinDrawRectangleFrame(boldRoundFrame,&rect);
}



/***********************************************************************
 *
 * FUNCTION:	HandleDigit    
 *
 * DESCRIPTION:   Collect digit
 *
 * PARAMETERS:  digit
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void HandleDigit(Char digit)
{
	UShort len = StrLen(gDigitCollect);
						
	if(CallState&SGN_TAPI_CONVERSATION)
		PDQTelGenerateDTMF(gCoreRefNum, digit, true);
						
	gDigitCollect[len] = digit;
	len++;
	gDigitCollect[len] = 0;
	UpdateScreen(gDigitCollect,NULL);
}



/***********************************************************************
 *
 * FUNCTION: DispatchSignal    
 *
 * DESCRIPTION:   Handle pdQ signals
 *
 * PARAMETERS:  pSignal
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void DispatchSignal(SignalParamsPtr pSignal)
{
	Char text[MAX_TEXT_STR];
	Char formatDigits[MAX_TEXT_STR];
	
	text[0]=0;
	
	switch(pSignal->signal) {
					
		case SGN_TAPI_IDLE:
			// Inform user that phone is idle.
			MemSet(&gCallInfo,sizeof(gCallInfo),0);
			UpdateScreen(NULL,"Phone Idle");
			break;
			
		case SGN_TAPI_INCOMING:
			// Inform user that phone is ringing.
			MemMove(&gCallInfo,pSignal->params.pVoidArg,sizeof(gCallInfo));
			switch(gCallInfo.callFlags) 
			{
				case CT_NORMAL:
					UpdateScreen(NULL,"Incoming Call. Hit Send.");
					break;
				case CT_WAITING:
					UpdateScreen(NULL,"Incoming Call Waiting. Hit Send.");
					break;
			}
			break;

		case SGN_TAPI_LOST:
			// Inform user that the call was lost
			MemMove(&gCallInfo,pSignal->params.pVoidArg,sizeof(gCallInfo));
			gDigitCollect[0]=0;
			UpdateScreen(NULL,"Call Lost");
			break;
			
		case SGN_TAPI_MISSED:
			// Inform user that a call was missed.
			// If the caller ID was available, inform who the caller was
			// May also want to display the date: gCallInfo.lTimeOfCall
			MemMove(&gCallInfo,pSignal->params.pVoidArg,sizeof(gCallInfo));
			if(gCallInfo.szNumber)
			{
				PDQTelFormatNumber(gCoreRefNum, gCallInfo.szNumber, formatDigits, MAX_TEXT_STR, TFN_FORMATTED);
				StrPrintF(text,"Call Missed from %s", formatDigits);
			}
			else
				StrCopy(text,"Call Missed");
			UpdateScreen(NULL,text);
			break;
			
		case SGN_TAPI_FAILED:
			// Inform user that the call failed
			MemMove(&gCallInfo,pSignal->params.pVoidArg,sizeof(gCallInfo));
			gDigitCollect[0]=0;
			StrCopy(text,"Call Failed");
			switch(gCallInfo.nErr)
			{
				case PDQErrLocked:
					StrCat(text,". Locked.");
					break;
				case PDQErrNoSignal:
					StrCat(text,". No signal.");
					break;
				case PDQErrNoService:
					StrCat(text,". No service.");
					break;
			}
			UpdateScreen(NULL,text);
			break;
			
		case SGN_TAPI_CALLING:
			// Inform user that an outgoing call was placed
			MemMove(&gCallInfo,pSignal->params.pVoidArg,sizeof(gCallInfo));
			gDigitCollect[0]=0;
			if(gCallInfo.szNumber)
			{
				PDQTelFormatNumber(gCoreRefNum, gCallInfo.szNumber, formatDigits, MAX_TEXT_STR, TFN_FORMATTED);
				StrPrintF(text,"Calling %s", formatDigits);
			}
			else
				StrCopy(text,"Calling");
			UpdateScreen(NULL,text);
			break;
			
		case SGN_TAPI_CONVERSATION:
			// Inform user that a call was established
			MemMove(&gCallInfo,pSignal->params.pVoidArg,sizeof(gCallInfo));
	
			gDigitCollect[0]=0;
			switch(gCallInfo.callFlags) {
				case CT_NORMAL: 
					if(gCallInfo.szNumber)
					{
						PDQTelFormatNumber(gCoreRefNum, gCallInfo.szNumber, formatDigits, MAX_TEXT_STR, TFN_FORMATTED);
						StrPrintF(text,"Conversation with %s", formatDigits);
					}
					else
						StrCopy(text,"Conversation");
					UpdateScreen(NULL,text);
					break;
				case CT_DATA:
					UpdateScreen(NULL,"Data Call");
					break;
				case CT_CONFERENCE:
					break;
			}
			break;

		case SGN_TAPI_ENDED:
			// Inform user that a call ended
			// May also want to display:
			//		 	 call length: gCallInfo.lCallDuration
			//		 	 call start: 	gCallInfo.lTimeOfCall
			MemMove(&gCallInfo,pSignal->params.pVoidArg,sizeof(gCallInfo));
			gDigitCollect[0]=0;
			if(gCallInfo.szNumber)
			{
				PDQTelFormatNumber(gCoreRefNum, gCallInfo.szNumber, formatDigits, MAX_TEXT_STR, TFN_FORMATTED);
				StrPrintF(text,"Call Ended with %s", formatDigits);
			}
			else
				StrCopy(text,"Call Ended");
			UpdateScreen(NULL,text);
			break;
		
		case SGN_TAPI_DIALPAUSED:
			// Inform user of pause state
			switch(pSignal->params.nArg)
			{
				case PAUSE_HARD:
					gHardPause = true;
					UpdateScreen(NULL,"User Pause. Hit Clr or Send.");
					break;
				case PAUSE_SOFT:
					gHardPause = false;
					UpdateScreen(NULL,"Timed Pause");
					break;
				case PAUSE_NONE:
					gHardPause = false;
					UpdateScreen(NULL,"Conversation");
					break;
			}
			break;
			
		case SGN_TAPI_CALLERID:
			// Inform user of caller id
			MemMove(&gCallInfo,pSignal->params.pVoidArg,sizeof(gCallInfo));
			
			if(gCallInfo.nCallerIDStatus!=PI_ALLOWED)
				break;
			if(gCallInfo.callFlags==CT_WAITING)
			{
				if(gCallInfo.szWaiting)
				{
					PDQTelFormatNumber(gCoreRefNum, gCallInfo.szWaiting, formatDigits, MAX_TEXT_STR, TFN_FORMATTED);
					StrPrintF(text,"Incoming Call Waiting from %s", formatDigits);
				}
				else
					StrCopy(text,"Incoming Call Waiting");
			}
			else
			{
				if(gCallInfo.szNumber)
				{
					PDQTelFormatNumber(gCoreRefNum, gCallInfo.szNumber, formatDigits, MAX_TEXT_STR, TFN_FORMATTED);
					StrPrintF(text,"Incoming Call from %s", formatDigits);
				}
			}
			UpdateScreen(NULL,text);
			break;

		case SGN_TAPI_DIALEDDTMF:
			// Inform user of remaining DTMFs
			{
				CharPtr digits = pSignal->params.pVoidArg;
				if(digits && *digits)
				{
					StrPrintF(text,"Dialing %s", (CharPtr)pSignal->params.pVoidArg);
					UpdateScreen(NULL,text);
				}
				else
					UpdateScreen(NULL,"Conversation");
			}
			break;			
	}									
}

