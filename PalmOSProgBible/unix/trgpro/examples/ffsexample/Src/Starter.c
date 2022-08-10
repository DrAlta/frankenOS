/***********************************************************************
 *
 Copyright © 1995 - 1998, 3Com Corporation or its subsidiaries ("3Com").  
 All rights reserved.
   
 This software may be copied and used solely for developing products for 
 the Palm Computing platform and for archival and backup purposes.  Except 
 for the foregoing, no part of this software may be reproduced or transmitted 
 in any form or by any means or used to make any derivative work (such as 
 translation, transformation or adaptation) without express written consent 
 from 3Com.

 3Com reserves the right to revise this software and to make changes in content 
 from time to time without obligation on the part of 3Com to provide notification 
 of such revision or changes.  
 3COM MAKES NO REPRESENTATIONS OR WARRANTIES THAT THE SOFTWARE IS FREE OF ERRORS 
 OR THAT THE SOFTWARE IS SUITABLE FOR YOUR USE.  THE SOFTWARE IS PROVIDED ON AN 
 "AS IS" BASIS.  3COM MAKES NO WARRANTIES, TERMS OR CONDITIONS, EXPRESS OR IMPLIED, 
 EITHER IN FACT OR BY OPERATION OF LAW, STATUTORY OR OTHERWISE, INCLUDING WARRANTIES, 
 TERMS, OR CONDITIONS OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND 
 SATISFACTORY QUALITY.

 TO THE FULL EXTENT ALLOWED BY LAW, 3COM ALSO EXCLUDES FOR ITSELF AND ITS SUPPLIERS 
 ANY LIABILITY, WHETHER BASED IN CONTRACT OR TORT (INCLUDING NEGLIGENCE), FOR 
 DIRECT, INCIDENTAL, CONSEQUENTIAL, INDIRECT, SPECIAL, OR PUNITIVE DAMAGES OF 
 ANY KIND, OR FOR LOSS OF REVENUE OR PROFITS, LOSS OF BUSINESS, LOSS OF INFORMATION 
 OR DATA, OR OTHER FINANCIAL LOSS ARISING OUT OF OR IN CONNECTION WITH THIS SOFTWARE, 
 EVEN IF 3COM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

 3Com, HotSync, Palm Computing, and Graffiti are registered trademarks, and 
 Palm III and Palm OS are trademarks of 3Com Corporation or its subsidiaries.

 IF THIS SOFTWARE IS PROVIDED ON A COMPACT DISK, THE OTHER SOFTWARE AND 
 DOCUMENTATION ON THE COMPACT DISK ARE SUBJECT TO THE LICENSE AGREEMENT 
 ACCOMPANYING THE COMPACT DISK.

 *************************************************************************
 *
 * PROJECT:  Pilot
 * FILE:     Starter.c
 * AUTHOR:   Roger Flores: May 20, 1997
 *
 * DECLARER: Starter
 *
 * DESCRIPTION:
 *	  
 *
 **********************************************************************/

/***********************************************************************
 *
 * Copyright (c) 1999, TRG, All Rights Reserved
 *
 * PROJECT:         Nomad FAT demo
 *
 * FILE:     Starter.c       
 *
 * DESCRIPTION:  Main body of Ffs demo app. Slightly modified version of
 *               CodeWarrior starter app.
 *
 * AUTHOR: Trevor Meyer         
 *
 * DATE: 8/9/99           
 *
 **********************************************************************/
#define PILOT_PRECOMPILED_HEADERS_OFF

#include <Pilot.h>
#include <SysEvtMgr.h>
#include <NotifyMgr.h>

#include "StarterRsc.h"
#include "cmd.h"
#include "com.h"
#include "ffslib.h"
#include "trglib.h"
#include "notify.h"
#include "hdw.h"


/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/
typedef struct 
{
    UInt8 replaceme;
} StarterPreferenceType;

typedef struct 
{
    UInt8 replaceme;
} StarterAppInfoType;

typedef StarterAppInfoType* StarterAppInfoPtr;


/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
UInt16    FfsLibRef;
LocalID   appID;
UInt16    status = 3, old_status = 0;

/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator         'trv2'
#define appVersionNum          0x01
#define appPrefID              0x00
#define appPrefVersionNum      0x01



/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/

/*--------------------------------------------------------------------------
 * Function    : CommProcessing()
 * Description : Check for a character on the serial port, and if found,
 *               pass it to the command interpreter.
 * Params      : None
 * Returns     : Nothing
 *--------------------------------------------------------------------------*/
static void CommProcessing(void)
{
    char chr;
    
    if (COMBytesAvailable()>0)
    {
      	COMGetC(&chr);
    	CMDProcess(chr);
    }
}


/***********************************************************************
 *
 * FUNCTION:    MainFormInit
 *
 * DESCRIPTION: This routine initializes the MainForm form.
 *
 * PARAMETERS:  frm - pointer to the MainForm form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormInit(FormPtr frmP)
{
}


/***********************************************************************
 *
 * FUNCTION:    MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean MainFormDoCommand(UInt16 command)
{
    Boolean handled = false;

    switch (command)
    {
    }
    return handled;
}


/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "MainForm" of this application.
 *
 * PARAMETERS:  eventP  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr eventP)
{
    Boolean handled = false;
    FormPtr frmP;

    switch (eventP->eType) 
    {
	case menuEvent:
	    return MainFormDoCommand(eventP->data.menu.itemID);

	case frmOpenEvent:
	    frmP = FrmGetActiveForm();
	    MainFormInit( frmP);
	    FrmDrawForm ( frmP);
	    handled = true;
	    break;

	default:
	    break;		
    }
	
    return handled;
}


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
    UInt16 formId;
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
	}
        return(true);
    }
    else if (eventP->eType == ctlSelectEvent)
    {
        if (eventP->data.ctlSelect.controlID == MainDoneButton)
            eventP->eType = appStopEvent;
    }
    return(false);
}


/***********************************************************************
 *
 * FUNCTION:    CheckCFEvent
 *
 * DESCRIPTION: Indicate when the insert/remove event is detected
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void CheckCFEvent(void)
{
    Boolean       changed = false;
    UInt16	  saved_ints;

    /* see if the current card status has changed due to a CF event */
    saved_ints = HdwDisableInts();
    if (status != old_status)
    {
        old_status = status;
        changed = true;
    }
    HdwRestoreInts(saved_ints);

    if (changed)
    {
        changed = false;

        /* 3 means power was cycled -- need to check status of card */
        if (status == 3)
            if (FfsCardIsInserted(FfsLibRef, 1))
                status = old_status = 1;
            else
                status = old_status = 0;

        /* 0 means no card, 1 means card is present */
        if (status == 0)
            COMPrintf("CF Card Removed\r\n");
        else 
            COMPrintf("CF Card Inserted\r\n");
    }
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
    UInt16 error;
    EventType event;

    do 
    {
        /* Note that we set a event timeout, and use the timeout return to */
        /* check the serial port for user input.                           */
        EvtGetEvent(&event, 10);
		
	if (! SysHandleEvent(&event))
	    if (! MenuHandleEvent(0, &event, &error))
		if (event.eType != nilEvent)
		{
		    if (! AppHandleEvent(&event))
			FrmDispatchEvent(&event);
		}
		else
                {
                    CheckCFEvent();
                    CommProcessing();
                }

    } while (event.eType != appStopEvent);
}


/*--------------------------------------------------------------------------
 * Function    : HandleError()
 * Description : Ffs Library critical error handler. When a critical error
 *               occurs on the CF drive, the library will call this routine
 *               with a code that indicates the type of error and appropriate
 *               response choices. It will also pass in the drive number and
 *               and error string. This routine display the error in an
 *               alert box and prompts the user for the course of action
 *               to take (ie. Abort, Retry, etc). Critical errors typically
 *               occur if the card is removed during an operation, or is
 *               not formatted, or if the library tries to access a non-ATA
 *               card.
 * Params      : drive -- drive number (currently 0)
 *               choices -- the catagory of error that occurs. Indicates the
 *                      set of valid responses. For example, choice
 *                      CRERR_NOTIFY_ABORT_RETRY allows responses
 *                      CRERR_RESP_ABORT or CRERR_RESP_RETRY.
 *               msg -- string containing specific error message to display.
 * Returns     : Response code indicating action to be taken by the
 *               library. ABORT will terminate the current library call,
 *               RETRY will attempt the call again, FORMAT will reformat
 *               the drive, and CLEAR will reinitialize the bad sector.
 *--------------------------------------------------------------------------*/
static Int16 HandleError(Int16 drive, Int16 choices, char *msg)
{
    UInt16 val;
    char   drive_str[6];

    sprintf(drive_str,"%d",drive);

    /* General drive error: allow Abort, Retry */
    if (choices == CRERR_NOTIFY_ABORT_RETRY)
    {
        val = FrmCustomAlert(AbortRetryAlert, drive_str, msg, NULL);
        if (val == 0)
            return(CRERR_RESP_ABORT);
        else
            return(CRERR_RESP_RETRY);
    }

    /* Bad/unreadable format: allow Abort, Format */
    else if (choices == CRERR_NOTIFY_ABORT_FORMAT)
    {
        val = FrmCustomAlert(AbortFormatAlert, drive_str, msg, NULL);
        if (val == 0)
            return(CRERR_RESP_ABORT);
        else
            return(CRERR_RESP_FORMAT);
    }

    /* Read/write error: allow Clear, Abort, Retry (Clear tries to */
    /* reinitialize the corrupt sector).                           */
    else if (choices == CRERR_NOTIFY_CLEAR_ABORT_RETRY)
    {
        val = FrmCustomAlert(ClearAbortRetryAlert, drive_str, msg, NULL);
        if (val == 0)
            return(CRERR_RESP_ABORT);
        else if (val == 1)
            return(CRERR_RESP_RETRY);
        else
            return(CRERR_RESP_CLEAR);
    }

    /* shouldn't get here, but if so, just abort library call */
    return(CRERR_RESP_ABORT);
}


/***********************************************************************
 *
 * FUNCTION:     AppStart
 *
 * DESCRIPTION:  Get the current application's preferences.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err value 0 if nothing went wrong
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err AppStart(void)
{
    StarterPreferenceType 	prefs;
    UInt16                      prefsSize;
    Err				err;

    // Read the saved preferences / saved-state information.
    prefsSize = sizeof(StarterPreferenceType);
    if (PrefGetAppPreferences(appFileCreator, appPrefID, &prefs, &prefsSize, true) != 
		noPreferenceFound)
    {
    }

    if (COMOpen() != 0)
        return(-1);

    CMDInit();

    /* get our app ID before opening lib, in case this fails somehow */
    if ((appID = DmFindDatabase(0,"FfsDemo")) == 0)
    {
        COMPrintf("Error getting appID\r\n");
        COMClose();
        return(1);
    }

    /* register for CF change notification */
    if (SysNotifyRegister(0, appID, sysNotifyCFEvent, NULL, 0, NULL) != 0)
        COMPrintf("Unable to register for CF event notification\r\n");
    else
        COMPrintf("Registered for CF event notification\r\n");
    
    /* load the Ffs Library */
    err = SysLibFind(FfsLibName, &FfsLibRef);
    if (err == 0)
        COMPrintf("Ffs library already loaded\r\n");
    else
    {
        // The library wasn't pre-loaded -- we need to load it ourselves
        if ((err = SysLibLoad(FfsLibTypeID, FfsLibCreatorID, &FfsLibRef)) != 0)
        {
            COMPrintf("Could not load the Ffs library\r\n");
            COMClose();
            return(err);
        }
    }
    
    /* Library loaded -- now open it */
    if ((err = FfsLibOpen(FfsLibRef)) != 0)
    {
        COMPrintf("Could not open the Ffs library\r\n");
        COMClose();
        return(err);
    }

    /* install our Ffs error handler routine */
    FfsInstallErrorHandler(FfsLibRef, HandleError);

    /* check initial status of card */
    CheckCFEvent();

    return(0);
}


/***********************************************************************
 *
 * FUNCTION:    AppStop
 *
 * DESCRIPTION: Save the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void AppStop(void)
{
    StarterPreferenceType prefs;
   
    // Write the saved preferences / saved-state information.  This data 
    // will be backed up during a HotSync.
    PrefSetAppPreferences (appFileCreator, appPrefID, appPrefVersionNum, 
		&prefs, sizeof (prefs), true);
    
    /* unregister with notification manager */
    SysNotifyUnregister(0, appID, sysNotifyCFEvent, 0);

    /* remove our error handler, in case someone else has the lib open */
    FfsUnInstallErrorHandler(FfsLibRef);

    /* close and unload the Ffs library */
    FfsLibClose(FfsLibRef);
    SysLibRemove(FfsLibRef);    // for now, this needs to be done

    COMClose();
}



/***********************************************************************
 *
 * FUNCTION:    StarterPilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 *
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt32 StarterPilotMain(UInt16 cmd, Ptr cmdPBP, UInt16 launchFlags)
{
    Err                 error;
    SysNotifyParamType *notify_param;

    switch (cmd)
    {
	case sysAppLaunchCmdNormalLaunch:
	    error = AppStart();
	    if (error) 
		return error;
			
	    FrmGotoForm(MainForm);
	    AppEventLoop();
	    AppStop();
	    break;

        case sysAppLaunchCmdNotify:
            notify_param = (SysNotifyParamType *)cmdPBP;
            switch(*((UInt32 *)(notify_param->notifyParamP)))
            {
                case CFEventCardInserted:
                    status = 1;
                    break;
                case CFEventCardRemoved:
                    status = 0;
                    break;
                case CFEventPowerIsBackOn:
                    status = 3;
                    break;
            }
            break;

        default:
	    break;
    }
	
    return 0;
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt32 PilotMain( UInt16 cmd, Ptr cmdPBP, UInt16 launchFlags)
{
    return StarterPilotMain(cmd, cmdPBP, launchFlags);
}

