#define PILOT_PRECOMPILED_HEADERS_OFF
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
#include <Pilot.h>
#include <SysEvtMgr.h>
#include "StarterRsc.h"

#include "AudioLib.h"

/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
UInt16   audLibRef;


/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator            'DTMF'
#define appVersionNum              0x01
#define appPrefID                  0x00
#define appPrefVersionNum          0x01



/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/



/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    VoidPtr
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void *GetObjectPtr(UInt16 objectID)
{
    FormPtr frmP;

    frmP = FrmGetActiveForm();
    return(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID)));
}


/***********************************************************************
 * FUNCTION:    AboutInfo
 * DESCRIPTION: Display the info dialog
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
static void AboutInfo(void)
{
    FormPtr  curFormP;
    FormPtr  formP;

    curFormP = FrmGetActiveForm ();
    formP    = FrmInitForm(AboutForm);
    FrmDoDialog(formP);
    FrmDeleteForm(formP);
    FrmSetActiveForm(curFormP);
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
    Int16    index;
  
    index = FrmGetObjectIndex(frmP, MainDTMFStrField);
    FrmSetFocus( frmP, index);
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
        case MainOptionsAboutDTMFExample:
            AboutInfo();
            handled = true;
            break;
    }
    return(handled);
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
    Boolean  handled = false;
    FormPtr  frmP;
    FieldPtr fieldP;
    char    *textP;
    
    switch (eventP->eType) 
    {
        case menuEvent:
            return(MainFormDoCommand(eventP->data.menu.itemID));

        case frmOpenEvent:
            frmP = FrmGetActiveForm();
            FrmDrawForm (frmP);
            MainFormInit(frmP);
            handled = true;
            break;
 		
	    case ctlSelectEvent :
            switch(eventP->data.ctlSelect.controlID)
        	{
                case MainPlayItButton:
                    fieldP = GetObjectPtr(MainDTMFStrField);
                    textP = FldGetTextPtr(fieldP);
//Call the Audio Library function AudioPlayDTMFStr
//This will play the string of chars that is displayed in the text edit field
                    if (textP != NULL)
                        AudioPlayDTMFStr(audLibRef, textP, 5, 1);
                    break;
            }

            break;
            
        default:
            break;
    }

    return(handled);
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
static Boolean AppHandleEvent(EventPtr eventP)
{
    FormPtr  frmP;
    Boolean  handled = false;

    if (eventP->eType == frmLoadEvent)
    {
        // Load the form resource.
        frmP = FrmInitForm(eventP->data.frmLoad.formID);
        FrmSetActiveForm(frmP);

        switch (eventP->data.frmLoad.formID)
        {
            case MainForm:
                // Set the form's event handler
                FrmSetEventHandler(frmP, MainFormHandleEvent);
                handled = true;
                break;
            
            default:
                break;
        }
    }
	
    return(handled);
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
    UInt16     error;
    EventType  event;

    do
    {
        EvtGetEvent(&event, evtWaitForever);
		
        if (!SysHandleEvent(&event))
            if (!MenuHandleEvent(0, &event, &error))
                if (!AppHandleEvent(&event))
                    FrmDispatchEvent(&event);
    } while(event.eType != appStopEvent);
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
    Err err;
    
    // Load and open the Audio Library
    // Store libref as a global for later access
    if((SysLibFind(AudioLibName, &(audLibRef)))!=0)
    {
        // The library wasn't pre-loaded -- we need to load it ourselves
        if ((err = SysLibLoad(AudioLibTypeID, AudioLibCreatorID, &(audLibRef))) != 0)
        {
            FrmAlert(NeedAudioLibAlert);
            return(err);
        }
    }
    
    if ((err = AudioLibOpen(audLibRef)) != 0)
    {
        SysLibRemove(audLibRef);
        FrmAlert(CantOpenLibAlert);
        return(err);
    }

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
    FrmCloseAllForms();

    // Close and unload the Audio Library
    AudioLibClose(audLibRef);
    SysLibRemove(audLibRef);
}


/***********************************************************************
 *
 * FUNCTION:    StarterPilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 * PARAMETERS:  cmd - UInt16 value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  UInt16 value providing extra information about the launch.
 *
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt32 StarterPilotMain(UInt16 cmd, Ptr cmdPBP, UInt16 launchFlags)
{
    Err error;

    switch(cmd)
    {
        case sysAppLaunchCmdNormalLaunch:
            if ((error = AppStart()) != 0)
                return error;
				
            FrmGotoForm(MainForm);
                        
            AppEventLoop();
            AppStop();
            break;

        default:
            break;
    }
	
    return(0);
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - UInt16 value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  UInt16 value providing extra information about the launch.
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt32 PilotMain(UInt16 cmd, Ptr cmdPBP, UInt16 launchFlags)
{
    return(StarterPilotMain(cmd, cmdPBP, launchFlags));
}

