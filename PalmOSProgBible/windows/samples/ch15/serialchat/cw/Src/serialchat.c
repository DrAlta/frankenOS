/***********************************************************************
 *
 * PROJECT:  Serial Chat
 * FILE:     serialchat.c
 * AUTHOR:   Lonnon R. Foster
 *
 * DESCRIPTION:  Main routines for Serial Chat, a sample serial
 *               communications program.
 *
 * From Palm OS Programming Bible
 * ©2000 Lonnon R. Foster.  All rights reserved.
 *
 ***********************************************************************/

#include <PalmOS.h>
#include "serialchat.h"
#include "serialchatRsc.h"


/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator			  'LFsc'
#define appVersionNum              0x01

// Define the minimum OS version we support (2.0 for now).
#define ourMinVersion	sysMakeROMVersion(2,0,0,sysROMStageRelease,0)


/***********************************************************************
 *
 *   Global Variables
 *
 ***********************************************************************/
Boolean  gConnected = false;
UInt16   gPortID;


/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/


/***********************************************************************
 *
 * FUNCTION:    ClearField
 *
 * DESCRIPTION: Clears the contents of a text field.
 *
 * PARAMETERS:  fieldID - resource ID of the field to clear
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ClearField(UInt16 fieldID)
{
	FieldType  *field;
	MemHandle  oldTextH;
	
	
	field = GetObjectPtr(fieldID);
	oldTextH = FldGetTextHandle(field);
	
	if (oldTextH) {
		// Detach the text handle from the field and dispose of it.
		FldSetTextHandle(field, NULL);
		MemHandleFree(oldTextH);
		
		// Redraw the field and update its scroll bar.
		FldDrawField(field);
		MainFormUpdateScrollBar(fieldID);
	}
}


/***********************************************************************
 *
 * FUNCTION:    CloseSerial
 *
 * DESCRIPTION: Closes the serial port.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void CloseSerial(void)
{
	Err  error;
	
	
	error = SrmSendWait(gPortID);
	ErrNonFatalDisplayIf(error == serErrBadPort, "SrmClose: bad port");
	if (error == serErrTimeOut)
		FrmAlert(SerialTimeoutAlert);

	SrmClose(gPortID);
	
	gConnected = false;
}


/***********************************************************************
 *
 * FUNCTION:    GetFocusFieldPtr
 *
 * DESCRIPTION: Returns a pointer to the field object, in the current
 *              form, that has the focus.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    pointer to a field object or NULL of there is no field
 *              object with the focus
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static FieldPtr GetFocusFieldPtr(void)
{
	FormPtr  form;
	UInt16   focus;
	
	form = FrmGetActiveForm();
	focus = FrmGetFocus(form);
	if (focus == noFocus)
		return (NULL);
	else		
		return (FrmGetObjectPtr(form, focus));
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  objectID - resource ID of the object to return
 *
 * RETURNED:    pointer to the requested object
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void * GetObjectPtr(UInt16 objectID)
{
	FormPtr form;

	form = FrmGetActiveForm();
	return FrmGetObjectPtr(form, FrmGetObjectIndex(form, objectID));
}


/***********************************************************************
 *
 * FUNCTION:    OpenSerial
 *
 * DESCRIPTION: Opens the serial port.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    0 if successful, error code otherwise
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err OpenSerial(void)
{
    Err     error = 0;
    UInt32  flags = 0;
    UInt16  flagsSize = sizeof(flags);

   
	// Open the serial port with an initial baud rate of 9600.
    error = SrmOpen(serPortCradlePort, 9600, &gPortID);
	ErrNonFatalDisplayIf(error == serErrBadPort,
	                     "OpenSerial: serErrBadPort");
    switch (error) {
    	case errNone:
    		break;
    	
    	case serErrAlreadyOpen:
    		SrmClose(gPortID);
    		FrmAlert(SerialBusyAlert);
    		return error;
    		break;
    		
    	default:
    		FrmAlert(SerialOpenAlert);
    		return error;
    		break;
    }
    
    gConnected = true;

	// Clear the port in case garbage data is hanging around.
    SrmReceiveFlush(gPortID, 100);
    
    // Set communications parameters to 8-N-1 with hardware flow control.
    flags = srmSettingsFlagBitsPerChar8 |
            srmSettingsFlagStopBits1 |
            srmSettingsFlagRTSAutoM;
    error = SrmControl(gPortID, srmCtlSetFlags, &flags, &flagsSize);
    ErrNonFatalDisplayIf(error, "OpenSerial: port config error");
    
    return error;
}
   

/***********************************************************************
 *
 * FUNCTION:    ReadSerial
 *
 * DESCRIPTION: Reads data from the serial port.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void ReadSerial(void)
{
    static Char  buffer[maxFieldLength];
    static UInt16  index = 0;
    Err     error;
    UInt32  bytes;


    if (gConnected == false) return;
    
    // See if there is anything in the queue.
    error = SrmReceiveCheck(gPortID, &bytes);
    if (error) {
        FrmAlert(SerialCheckAlert);
        return;
    }

	// Make sure the data in the queue won't overflow the buffer.
	// If there is too much data waiting, only retrieve as much
	// data as will fit in the buffer.
    if (bytes + index > sizeof(buffer)) {
    	bytes = sizeof(buffer) - index - sizeOf7BitChar('\0');
    }
    
    // Retrieve data.
    while (bytes) {
        SrmReceive(gPortID, &buffer[index], 1, 0, &error);
        if (error) {
            SrmReceiveFlush(gPortID, 1);
            index = 0;
            return;
        }
        switch (buffer[index]) {
            case chrCarriageReturn:
            	// Treat a carriage return as the end of an incoming
            	// message, since some terminals may send CR instead of
            	// linefeed.  Convert the CR to a LF so the message is
            	// displayed correctly in the incoming text field.
            	buffer[index] = chrLineFeed;
            	
            	// Fall through...
            
            case chrLineFeed:
                // Treat a linefeed as the end of an incoming message.
                // Leave the linefeed intact in the incoming data to
                // properly format the incoming text field, tack
                // a terminating null onto the string in the buffer,
                // then display the message in the incoming field.
                buffer[index + 1] = chrNull;
                MainFormDisplayMessage(buffer);
                index = 0;
                break;
            
            default:
                index++;
                break;
        }
        bytes--;
    }
}


/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: Checks that the ROM version meets a minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum ROM version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err RomVersionCompatible(UInt32 requiredVersion,
                                UInt16 launchFlags)
{
	UInt32  romVersion;
	UInt32  value;
	Err     error;
	

	// See if we're on in minimum required version of the ROM or later,
	// and whether that ROM includes the new serial manager.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	error = FtrGet(sysFileCSerialMgr, sysFtrNewSerialPresent, &value);
	if ( (romVersion < requiredVersion) || (error || value == 0) ) {
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) {
			// Let the user know that Serial Chat won't run on this
			// system.
			if (romVersion < requiredVersion)
				FrmAlert(RomIncompatibleAlert);
			else if (error || value == 0)
				FrmAlert(NoNewSerialAlert);
		
			// Palm OS 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < ourMinVersion) {
				AppLaunchWithCommand(sysFileCDefaultApp,
				                     sysAppLaunchCmdNormalLaunch, NULL);
			}
		}
		
		return sysErrRomIncompatible;
	}
	
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    WriteSerial
 *
 * DESCRIPTION: Writes the contents of the outgoing field to the serial
 *              port.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void WriteSerial(void)
{
    Err     error;
    FieldType  *field;
    MemHandle  textH;
    Char    *text;
    Char    lineFeed = chrLineFeed;


    // Bail out if not connected.
    if (gConnected == false) return;

    // Retrieve a pointer to the outgoing field's text.
    field = GetObjectPtr(MainOutgoingField);
    textH = FldGetTextHandle(field);
    if (textH) {
	    text = MemHandleLock(textH);
	
	    // Send the contents of the outgoing field.
	    SrmSend(gPortID, text, StrLen(text), &error);
	    if (error)
	        FrmAlert(SerialSendAlert);
	        
	    MemHandleUnlock(textH);
	    
	    // Send a linefeed character.
	    SrmSend(gPortID, &lineFeed, 1, &error);
	    if (error)
	    	FrmAlert(SerialSendAlert);
	}
}


#pragma mark ----------------

/***********************************************************************
 *
 * FUNCTION:    MainFormDisplayMessage
 *
 * DESCRIPTION: Adds incoming data to the end of the incoming field and
 *              scrolls the display to show the new data.
 *
 * PARAMETERS:  buffer - string containing incoming data
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormDisplayMessage(Char *buffer)
{
	FieldType  *field;
	MemHandle  textH;
	Char       *text;
	Int16      textLength;
	Int16      bufferLength;
	Int16      numCharsToMove;
	Char       *p, *q;
	Err        error;
	
	
	field = GetObjectPtr(MainIncomingField);
	textH = FldGetTextHandle(field);
	if (textH) {
		FldSetTextHandle(field, NULL);
		text = MemHandleLock(textH);
		textLength = StrLen(text);
		bufferLength = StrLen(buffer);
		if (textLength < maxFieldLength) {
			// The new field contents will be larger than the chunk
			// currently allocated to hold them.  Resize the text
			// handle to contain the new data, but be sure not to
			// resize the chunk larger than what the field can actually
			// hold (maxFieldLength + terminating null character).
			MemHandleUnlock(textH);
			error = MemHandleResize(textH, min(maxFieldLength +
			                            sizeOf7BitChar(chrNull),
			                            textLength + bufferLength +
			                            sizeOf7BitChar(chrNull)));
			ErrFatalDisplayIf(error, "Out of memory");
			text = MemHandleLock(textH);
		}

		if (textLength + bufferLength > maxFieldLength) {
			// Remove enough characters from the beginning of the field
			// to accomodate the incoming data.
			numCharsToMove = bufferLength + textLength -
			                 maxFieldLength;
			q = &text[numCharsToMove];
			for (p = text; *q != chrNull; ++p, ++q) {
				*p = *q;
			}

			// Be sure to terminate the modified string.
			*p = chrNull;
		}
		
		// Tack the new data onto the end of the field.
		StrCat(text, buffer);
	} else {
		// Allocate a new chunk for the field and fill it with the
		// buffer's contents.
		textH = MemHandleNew(StrLen(buffer) + 1);
		ErrFatalDisplayIf(! textH, "Out of memory");
		
		text = MemHandleLock(textH);
		StrCopy(text, buffer);
	}
	MemHandleUnlock(textH);
	FldSetTextHandle(field, textH);
	FldSetScrollPosition(field, FldGetTextLength(field));
	FldDrawField(field);
	MainFormUpdateScrollBar(MainIncomingField);
}


/***********************************************************************
 *
 * FUNCTION:    MainFormDoCommand
 *
 * DESCRIPTION: Performs the specified menu command.
 *
 * PARAMETERS:  menuID - resource ID of the menu command
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean MainFormDoCommand(UInt16 menuID)
{
	Boolean    handled = false;
	FormType   *form;
	FieldType  *field;
	
	
	field = GetFocusFieldPtr();
	switch (menuID) {
		case EditUndo:
			if (! field) return false;
			FldUndo(field);
			handled = true;
			break;
			
		case EditCut:
			if (! field) return false;
			FldCut(field);
			handled = true;
			break;
			
		case EditCopy:
			if (! field) return false;
			FldCopy(field);
			handled = true;
			break;
			
		case EditPaste:
			if (! field) return false;
			FldPaste(field);
			handled = true;
			break;
			
		case EditSelectAll:
			if (! field) return false;
			FldSetSelection(field, 0, FldGetTextLength(field));
			handled = true;
			break;
			
		case EditKeyboard:
			SysKeyboardDialog(kbdDefault);
			handled = true;
			break;
			
		case EditGraffitiHelp:
			SysGraffitiReferenceDialog(referenceDefault);
			handled = true;
			break;
			
		case MainOptionsAboutSerialChat:
			MenuEraseStatus(0);
			form = FrmInitForm(AboutForm);
			FrmDoDialog(form);
			FrmDeleteForm(form);
			handled = true;
			break;
			
		default:
			ErrFatalDisplay("Invalid menu ID");
			break;
	}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: Handles events in the main form.
 *
 * PARAMETERS:  event - a pointer to an event to handle
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventType *event)
{
   Boolean   handled = false;
   FormType  *form;
   UInt16    fieldID;

	switch (event->eType)  {
		case menuEvent:
			return MainFormDoCommand(event->data.menu.itemID);

		case frmOpenEvent:
			form = FrmGetActiveForm();
			MainFormInit(form);
			FrmDrawForm(form);
			handled = true;
			break;

		case fldChangedEvent:
			form = FrmGetActiveForm();
			MainFormUpdateScrollBar(event->data.fldChanged.fieldID);
			handled = true;
			break;
		
		case keyDownEvent:
			if (event->data.keyDown.chr == chrLineFeed) {
				WriteSerial();
				ClearField(MainOutgoingField);
				handled = true;
			}
			break;
		
		case ctlSelectEvent:
			if (event->data.ctlSelect.controlID == MainClearButton) {
				ClearField(MainIncomingField);
				ClearField(MainOutgoingField);
				handled = true;
			}
			break;
			
		case sclRepeatEvent:
			if (event->data.sclRepeat.scrollBarID ==
			    MainIncomingScrollBar) {
				fieldID = MainIncomingField;
			} else {
				fieldID = MainOutgoingField;
			}
			MainFormScroll(fieldID,
			               event->data.sclRepeat.newValue - 
                           event->data.sclRepeat.value, false);
            // Leave unhandled so system can take care of repeating event.
            break;
		
		default:
			break;
	}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    MainFormInit
 *
 * DESCRIPTION: Initializes the main form.
 *
 * PARAMETERS:  form - pointer to the main form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormInit(FormType *form)
{
	UInt16  fieldIndex = FrmGetObjectIndex(form, MainOutgoingField);
	
	
	FrmSetFocus(form, fieldIndex);
}


/***********************************************************************
 *
 * FUNCTION:    MainFormScroll
 *
 * DESCRIPTION: Scrolls a given field on the main form.
 *
 * PARAMETERS:  linesToScroll - the number of lines to scroll,
 *                              positive for winDown,
 *                              negative for winUp
 *				updateScrollbar - if true, force a scrollbar update
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormScroll(UInt16 fieldID, Int16 linesToScroll,
                           Boolean updateScrollbar)
{
	UInt16    blankLines;
	FieldPtr  field;
	
	field = GetObjectPtr(fieldID);
	blankLines = FldGetNumberOfBlankLines(field);

	if (linesToScroll < 0)
		FldScrollField(field, -linesToScroll, winUp);
	else if (linesToScroll > 0)
		FldScrollField(field, linesToScroll, winDown);

	// If there were blank lines visible at the end of the field
	// then we need to update the scroll bar.
	if (blankLines || updateScrollbar) {
		ErrNonFatalDisplayIf(blankLines && linesToScroll > 0,
		                     "blank lines when scrolling winDown");
		
		MainFormUpdateScrollBar(fieldID);
	}
}


/***********************************************************************
 *
 * FUNCTION:    MainFormUpdateScrollBar
 *
 * DESCRIPTION: Updates the scroll bars on the main form.
 *
 * PARAMETERS:  fieldID - resource ID of the field that changed
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormUpdateScrollBar(UInt16 fieldID)
{
	UInt16     scrollPos;
	UInt16     textHeight;
	UInt16     fieldHeight;
	Int16      maxValue;
	FieldType  *field;
	ScrollBarType  *bar;

	field = GetObjectPtr(fieldID);
	if (fieldID == MainIncomingField) {
		bar = GetObjectPtr(MainIncomingScrollBar);
	} else if (fieldID == MainOutgoingField) {
		bar = GetObjectPtr(MainOutgoingScrollBar);
	} else {
		ErrFatalDisplay("Invalid field ID");
	}
	
	FldGetScrollValues(field, &scrollPos, &textHeight, &fieldHeight);

	if (textHeight > fieldHeight) {
		// On occasion, such as after deleting a multi-line selection of text,
		// the display might be the last few lines of a field followed by some
		// blank lines.  To keep the current position in place and allow the user
		// to "gracefully" scroll out of the blank area, the number of blank lines
		// visible needs to be added to max value.  Otherwise the scroll position
		// may be greater than maxValue, get pinned to maxvalue in SclSetScrollBar
		// resulting in the scroll bar and the display being out of sync.
		maxValue = (textHeight - fieldHeight) +
		           FldGetNumberOfBlankLines(field);
	} else if (scrollPos)
		maxValue = scrollPos;
	else
		maxValue = 0;
		
	SclSetScrollBar(bar, scrollPos, 0, maxValue, fieldHeight - 1);
}


#pragma mark ----------------

/***********************************************************************
 *
 * FUNCTION:    ApplicationHandleEvent
 *
 * DESCRIPTION: Loads form resources and sets the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event - a pointer to an event to handle
 *
 * RETURNED:    true if the event was handled and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean ApplicationHandleEvent(EventPtr event)
{
	UInt16   formId;
	FormPtr  form;

	if (event->eType == frmLoadEvent) {
		// Load the form resource.
		formId = event->data.frmLoad.formID;
		form = FrmInitForm(formId);
		FrmSetActiveForm(form);

		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId) {
			case MainForm:
				FrmSetEventHandler(form, MainFormHandleEvent);
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
 * FUNCTION:    EventLoop
 *
 * DESCRIPTION: Central event loop for the application.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void EventLoop(void)
{
	Err        error;
	EventType  event;
	static UInt32  lastResetTime;


	lastResetTime = TimGetSeconds();
	do {
		// Retrieve an event about once every second.
		EvtGetEvent(&event, 100);
		
		// Prevent the auto-off timer from putting the handheld into
		// sleep mode by resetting the auto-off timer every 50 seconds.
		if (TimGetSeconds() - lastResetTime > 50) {
			EvtResetAutoOffTimer();
			lastResetTime = TimGetSeconds();
		}
		
		// Read data from the serial port.
		ReadSerial();

		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(0, &event, &error))
				if (! ApplicationHandleEvent(&event))
					FrmDispatchEvent(&event);

	} while (event.eType != appStopEvent);

}


/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  Perform startup tasks as the application starts.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err value 0 if nothing went wrong
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err StartApplication(void)
{
	return OpenSerial();
}


/***********************************************************************
 *
 * FUNCTION:    StopApplication
 *
 * DESCRIPTION: Perform shutdown tasks as the application exits.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void StopApplication(void)
{
	// Shut down the serial connection.
	CloseSerial();
	
	// Close all the open forms.
	FrmCloseAllForms();
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: Main entry point for the application.
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
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err  error;

	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) return (error);

	switch (cmd) {
		case sysAppLaunchCmdNormalLaunch:
			error = StartApplication();
			if (error) 
				return error;
				
			FrmGotoForm(MainForm);
			EventLoop();
			StopApplication();
			break;

		default:
			break;
	}
	
	return errNone;
}

