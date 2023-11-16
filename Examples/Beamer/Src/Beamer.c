/******************************************************************************
 *
 * Copyright (c) 1995-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: Beamer.c
 *
 * Description:
 *              Sample application to demonstrate the various
 * 	features available.
 *
 * History:
 * 	 6/5/97	Gavin Peacock		Initial version
 *
 *****************************************************************************/

#include <PalmOS.h>				// all the system toolbox headers
#include <Window.h>
#include <Bitmap.h>
#include "Beamer_Res.h"			// application resource defines
#include "ExgMgr.h"				// object exchange functions

#define ChunkSize 		20			// memory block allocate size for receive
#define BeamerName		"Beamer"	// Internal name of beamer app
#define BeamerFileName	"Beamer.prc" // External name for beamer app
#define beamerCreator	'Bemr' 		// Creator ID for ExgTest app
#define BitmapExt		"pbm"		// Palm Bitmap file extension
#define beamerDBType	'DATA'
#define beamerDBName	"BeamerDB"
#define version3_5		0x03500000	// PalmOS 3.5 version number (new window APIs)
#define version3_0		0x03000000	// PalmOS 3.0 version number (beaming support)

#define DrawAreaTop		16			// Top of drawing area (leave room for title)
#define DrawAreaBottom	120			// Bottom of drawing area (room for buttons)

// This is used to work around a clipping bug in the PalmOS drawLine logic
#define BottomClip		DrawAreaTop+DrawAreaBottom  

#define OptionsColorPick 1008		// dynamically added menu option


/***********************************************************************
 * Data type declarations
 **********************************************************************/
enum drawChoices {penChoice=0,eraserChoice};
				  
typedef enum drawChoices drawChoiceType;

/***********************************************************************
 * Global data
 **********************************************************************/
drawChoiceType 	 curDrawChoice = penChoice;
RectangleType 	 drawArea;
IndexedColorType drawIndex = 0;			   // index of color to draw with
Boolean 		 changePending = false;    // indicates when canvas needs updating
Boolean			 newAPI = false;			// if true, we can use 3.5 window APIs
WinHandle		 canvasWinH = NULL;			// offscreen window we are editing

/***********************************************************************
 * Prototypes for internal functions
 **********************************************************************/
static Err StartApplication(void);
static Boolean MainFormHandleEvent(EventType * event);
static void EventLoop(void);
static BitmapPtr PrvGetBitmap (WinHandle winH, UInt32 *sizeP, Err *errorP);
static void SaveWindow(void);
static void RestoreWindow(void);
static void DrawBitmap(BitmapPtr bmpP);
static Err LoadData(void);
static Err SaveData(void);


/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:  This routine sets up the initial state of the application.
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Nothing.
 *
 ***********************************************************************/
static Err StartApplication(void)
{
	UInt32 romVersion;
	Err    err = 0;
	
	ExgRegisterData(beamerCreator,exgRegExtensionID,BitmapExt);
	
		
	RctSetRectangle(&drawArea,0,DrawAreaTop,160,DrawAreaBottom);

	canvasWinH = WinCreateOffscreenWindow(drawArea.extent.x, drawArea.extent.y, genericFormat, &err);
	if (err) return err;

	// force an initial save of the drawWindow contents
	changePending = true; 
	
	// set global if we can use new API calls	
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion >= version3_5) 
		{	// set up an initial color
			RGBColorType rgb = {0,0,0,0};  // black
			drawIndex = WinRGBToIndex(&rgb);
			
			newAPI = true;  // it is OK to use 3.5 APIs
		}
	// Initialize and draw the main memo pad form.
	FrmGotoForm(BeamerMainForm);
	return err;
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
	SaveData();
	FrmCloseAllForms();
	if (canvasWinH) WinDeleteWindow(canvasWinH,false);
}


/***********************************************************************
 *
 * FUNCTION:		ReceiveData
 *
 * DESCRIPTION:		Receives data into the output field using the Exg API
 *
 * PARAMETERS:		exgSocketP, socket from the app code
 *						 sysAppLaunchCmdExgReceiveData
 *
 * RETURNED:		error code or zero for no error.
 *
 ***********************************************************************/
static Err ReceiveData(ExgSocketPtr exgSocketP)
{
	Err err;
	MemHandle dataH;
	UInt16 size;
	UInt8 *dataP;
	Int16 len;
	UInt16 dataLen = 0;
	
	if (exgSocketP->length)
		size = exgSocketP->length;
	else
		size = ChunkSize;  // allocate in chunks just as an example
							// could use data size from header if available
	dataH = MemHandleNew(size);  
	if (!dataH) return -1;  // 
	// This block needs to belong to the system since it could be disposed when apps switch
	MemHandleSetOwner(dataH,0);
	// accept will open a progress dialog and wait for your receive commands
	err = ExgAccept(exgSocketP);
	if (!err)
		{
		dataP = MemHandleLock(dataH);
		do {
			len = ExgReceive(exgSocketP,&dataP[dataLen],size-dataLen,&err);
			if (len && !err)
				{
				dataLen+=len;
				// resize block when we reach the limit of this one...
				if (dataLen >= size)
					{
					MemHandleUnlock(dataH);
					err = MemHandleResize(dataH,size+ChunkSize);
					dataP = MemHandleLock(dataH);
					if (!err) size += ChunkSize;
					}
				}
			}
		while (len && !err);   // reading 0 bytes means end of file....
		MemHandleUnlock(dataH);
		
		ExgDisconnect(exgSocketP,err); // closes transfer dialog
		
		if (!err)
			{
			exgSocketP->goToCreator = beamerCreator;  // tell exgmgr to launch this app after receive...
			exgSocketP->goToParams.matchCustom = (UInt32)dataH;
			}
		}
	// release memory if an error occured	
	if (err) MemHandleFree(dataH);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:		SendData
 *
 * DESCRIPTION:	Sends data in the input field using the Exg API
 *
 * PARAMETERS:		none
 *
 * RETURNED:		error code or zero for no error.
 *
 ***********************************************************************/
static Err SendData(void)
{
	ExgSocketType exgSocket;
	UInt32 size = 0;
	Err err = 0;
	BitmapPtr bmpP;
	Boolean freeBmp = false;
	

	// copy draw area into the bitmap
	SaveWindow();
	
	bmpP = PrvGetBitmap(canvasWinH, &size, &err);

 	// Is there data in the field?
	if (!err && size)
		{
		// important to init structure to zeros...
		MemSet(&exgSocket,sizeof(exgSocket),0);
		exgSocket.description = "Beamer picture";
		exgSocket.name = "Beamer.pbm";
		exgSocket.length = size;
		exgSocket.target = beamerCreator;
		err = ExgPut(&exgSocket);   // put data to destination
		if (!err)
			{
			ExgSend(&exgSocket,bmpP,size,&err);
			ExgDisconnect(&exgSocket,err);
			}
		}
		
	if (bmpP) MemPtrFree(bmpP);	
		
	return err;
}



/***********************************************************************
 *
 * FUNCTION:		GotoItem
 *
 * DESCRIPTION: 	Displays data referenced by GotoParam
 *
 * PARAMETERS:		standard goto parameters and launched flags
 *
 * RETURNED:		error code or zero for no error.
 *
 ***********************************************************************/
static Err GoToItem(GoToParamsPtr gtP,Boolean launched)
{
	MemHandle  bmpH;
	BitmapPtr bmpP;
	Err err = 0;
	
	// get bits that were stored in matchCustom field
	bmpH = (MemHandle)gtP->matchCustom;
	ErrFatalDisplayIf(!bmpH,"null bitmap on goto");
	
	bmpP = (BitmapPtr)MemHandleLock(bmpH);
	
	// don't allow color bitmaps on old ROMs
	if (!(!newAPI && (bmpP->pixelSize > 1)))
		{
		DrawBitmap(bmpP);
		SaveData(); 		  // save it for later
		}
	MemHandleFree(bmpH);  //  get rid of the bitmap now.
	return err;
}


/***********************************************************************
 *
 * FUNCTION:	WriteDBData
 *
 * DESCRIPTION: Callback for ExgDBWrite to send data with exchange manager
 *
 * PARAMETERS:  dataP : buffer containing data to send
 *				sizeP : number of bytes to send
 *				userDataP: app defined buffer for context
 (					(holds exgSocket when using ExgManager)
 *
 * RETURNED:    error if non-zero
 * 
 ***********************************************************************/
static Err WriteDBData(const void* dataP, UInt32* sizeP, void* userDataP)
{
	Err			err;

	// Try to send as many bytes as were requested by the caller
	*sizeP = ExgSend((ExgSocketPtr)userDataP, (void*)dataP, *sizeP, &err);

	return err;
}

/***********************************************************************
 *
 * FUNCTION:		SendDatabase
 *
 * DESCRIPTION:	Sends data in the input field using the Exg API
 *
 * PARAMETERS:	cardNo: card number of db to send (usually 0)
 *				dbID:	databaseId of database to send
 *				nameP:  public filename for this database
 *						This is the name as it appears on a PC file listing
 *						It should end with a .prc or .pdb extension
 *				description: Optional text description of db to show to user
 *						who receives the database.	
 *
 * RETURNED:		error code or zero for no error.
 *
 ***************************************************************************/
static Err SendDatabase (UInt16 cardNo, LocalID dbID, Char *nameP, Char *descriptionP)
{
	ExgSocketType		exgSocket;
	Err					err;

	// Create exgSocket structure
	MemSet(&exgSocket, sizeof(exgSocket), 0);
	exgSocket.description = descriptionP;
	exgSocket.name = nameP;

	// Start and exchange put operation
	err = ExgPut(&exgSocket);
	if (!err)
		{
		// This function converts a palm database into its external (public)
		// format. The first parameter is a callback that will be passed parts of 
		// the database to send or write.
		err = ExgDBWrite(WriteDBData, &exgSocket, NULL, dbID, cardNo);
		// Disconnect Exg and pass error
		err = ExgDisconnect(&exgSocket, err);
		}
	return err;
}

/***********************************************************************
 *
 * FUNCTION:		SendMe
 *
 * DESCRIPTION:	Sends this application
 *
 * PARAMETERS:	none
 *
 * RETURNED:	error code or zero for no error.
 *
 ***************************************************************************/
static Err SendMe(void)
{
	Err err;
	// Find our app using its internal name
	LocalID dbID = DmFindDatabase(0, "Beamer");
	if (dbID)    // send it giving external name and description
		err = SendDatabase(0, dbID, "Beamer.prc", "Beamer application");
	else
		err = DmGetLastErr();
	return err;
}

/***********************************************************************
 *
 * FUNCTION:  GetNextPoint
 *
 * DESCRIPTION:	waits for the pen to move or lift from the display
 *				This avoids uncessary draw operations
 *
 * PARAMETERS:	current coordinates of the pen
 *
 * RETURNED:	true if the pen is still down (false if lifted)
 *
 ***************************************************************************/
static Boolean GetNextPoint(short *screenX, short *screenY)
{	Boolean penDown;
	Int16 lastScreenX, lastScreenY;
	
	lastScreenX = *screenX;
	lastScreenY = *screenY;
	
 	do {
		/* should do filtering here... */
	  EvtGetPen(screenX,screenY,&penDown);
	} while (penDown &&
             (*screenX == lastScreenX) && (*screenY == lastScreenY));

	return(penDown);
}

/***********************************************************************
 *
 * FUNCTION:  DoPen
 *
 * DESCRIPTION:	Draws pen "ink" on the display (in current color)
 *
 * PARAMETERS:	current coordinates of the pen
 *
 * RETURNED:	nothing
 *
 ***************************************************************************/
static void DoPen(short screenX, short screenY)
{
	Int16 lastScreenX, lastScreenY;
	
	lastScreenX = screenX;
	lastScreenY = screenY;
	if (newAPI)
		{
		WinPushDrawState();
		WinSetForeColor(drawIndex);
		}
	do {
		
	  	// clip Y coordinate to avoid clipping bug in WinDrawLine
	  	// This doesn't clip, it just drops the whole line...
	  	if (screenY < BottomClip && lastScreenY < BottomClip)
			WinDrawLine(lastScreenX,lastScreenY,screenX,screenY);
		
		lastScreenX = screenX;
		lastScreenY = screenY;
	} while (GetNextPoint(&screenX,&screenY));
	if (newAPI) WinPopDrawState();
}

/***********************************************************************
 *
 * FUNCTION:  DoErase
 *
 * DESCRIPTION:	Erases areas of the display
 *
 * PARAMETERS:	current coordinates of the pen
 *
 * RETURNED:	nothing
 *
 ***************************************************************************/
static void DoErase(short screenX, short screenY)
{
	// not implemented yet...
}


/***********************************************************************
 *
 * FUNCTION:		MainFormHandleEvent
 *
 * DESCRIPTION:	Handles processing of events for the ÒmainÓ form.
 *
 * PARAMETERS:		event	- the most recent event.
 *
 * RETURNED:		True if the event is handled, false otherwise.
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr event)
{
	Boolean		handled = false;
	RectangleType saveClip;

	
	switch (event->eType)
	{
		case penDownEvent:
			if (RctPtInRectangle(event->screenX,event->screenY,&drawArea) ) 
 				{
					WinGetClip(&saveClip);
					WinSetClip(&drawArea);
  					
  					// always save previous changes before next move, this allows undo...
  					if (changePending) SaveWindow(); // save all changes before this action
  					changePending = true; // assume we will change something;
					switch (curDrawChoice)
					{
						case penChoice:
							DoPen( event->screenX,event->screenY);
							break;
						case eraserChoice:
							DoErase(event->screenX,event->screenY);
							break;
					}
					WinSetClip(&saveClip);
					handled = true;
				}
			break;
			
  		 case ctlSelectEvent:
	  	 	if (event->data.ctlEnter.controlID == BeamerMainSendButton)
				{
				SendData();
		 	 	}
	 		 else if (event->data.ctlEnter.controlID == BeamerMainClearButton)
	 		 	{
	 		 	WinEraseRectangle(&drawArea,0);
	 		 	changePending = true;
				}
			break;
			
		case menuEvent:				
			// First clear the menu status from the display.
			MenuEraseStatus(0);
			switch (event->data.menu.itemID)
			{	
				case OptionsGetInfo:
					FrmAlert(InfoAlert);
					break;
					
				case OptionsBeamMe:
					SendMe();
					break;
					
				case OptionsColorPick:
					{
 					RGBColorType rgb;
					WinIndexToRGB(drawIndex,&rgb);
  					if (UIPickColor(&drawIndex, &rgb, UIPickColorStartPalette, NULL, NULL)) {

	   				}
	   				break;
	   				}
					
			}
			handled = true;
			break;

		case menuOpenEvent:	
			if (newAPI)
				{
				MenuAddItem(OptionsBeamMe,OptionsColorPick, 0, "Pick Color");
				}
			if (changePending) SaveWindow();
			handled = true;
			break;


      	case keyDownEvent:
      		// handle the send data (ronamatic stroke) chr
      		if (event->data.keyDown.chr == sendDataChr)
  				{
           		SendData();
           		handled = true;
           		}	
           	break;
           	
     	case frmUpdateEvent:
     		FrmDrawForm(FrmGetActiveForm());
     		RestoreWindow();
     		handled = true;
           	break;
           	
     	case frmOpenEvent:
     		FrmDrawForm(FrmGetActiveForm());
     		LoadData();
           	break;
            
		}
	return(handled);
}

/***********************************************************************
 *
 * FUNCTION:    ApplicationHandleEvent
 *
 * DESCRIPTION: This routine loads a form resource and sets the event handler for the form.
 *
 * PARAMETERS:  event - a pointer to an EventType structure
 *
 * RETURNED:    True if the event has been handled and should not be
 *						passed to a higher level handler.
 *
 ***********************************************************************/
static Boolean ApplicationHandleEvent(EventPtr event)
{
	FormPtr	formP;
	UInt16		formId;
	Boolean	handled = false;

	if (event->eType == frmLoadEvent)
		{
		// Load the form resource specified in the event then activate the form.
		formId = event->data.frmLoad.formID;
		formP = FrmInitForm(formId);
		FrmSetActiveForm(formP);

		// Set the event handler for the form.  The handler of the currently 
		// active form is called by FrmDispatchEvent each time it receives an event.
		switch (formId)
			{
			case BeamerMainForm:
				FrmSetEventHandler(formP, MainFormHandleEvent);
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
 *						FrmHandleEvent for default processing.
 *
 * PARAMETERS:		None.
 *
 * RETURNED:		Nothing.
 *
 ***********************************************************************/
static void EventLoop(void)
{
	EventType	event;
	UInt16		error;
	
	do
		{
		// Get the next available event.
		EvtGetEvent(&event, evtWaitForever);
		
		// Give the system a chance to handle the event.
		if (! SysHandleEvent (&event))

			// Give the menu a chance to handle the event
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
static Err RomVersionCompatible (UInt32 requiredVersion, UInt8 launchFlags)
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
				{
				Err err = 0;
				
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
				}
			}
		
		return (sysErrRomIncompatible);
		}

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:		PilotMain
 *
 * DESCRIPTION:	This function is the equivalent of a main() function
 *						in standard ÒCÓ.  It is called by the Emulator to begin
 *						execution of this application.
 *
 * PARAMETERS:		cmd - command specifying how to launch the application.
 *						cmdPBP - parameter block for the command.
 *						launchFlags - flags used to configure the launch.			
 *
 * RETURNED:		Any applicable error code.
 *
 ***********************************************************************/
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	UInt32 err = 0;
	
	// This app makes use of PalmOS 3.0 features.  It will crash if
	// run on an earlier version of PalmOS.  Detect and warn if this happens,
	// then exit. (Mainly beaming support)
	err = RomVersionCompatible (version3_0, launchFlags);
	if (err)
		return err;

	// Check for a normal launch.
	if (cmd == sysAppLaunchCmdNormalLaunch)
		{
		// Set up initial form.
		err = StartApplication();
		if (err) return err;
			
		// Start up the event loop.
		EventLoop();
        
        StopApplication();   
		}
	else if (cmd == sysAppLaunchCmdSyncNotify)
		{
		// register our extension on syncNotify so we do not need to
		// be run before we can receive data.
		ExgRegisterData(beamerCreator,exgRegExtensionID,BitmapExt);
		}
	else if (cmd == sysAppLaunchCmdExgAskUser)
		{
		ExgAskParamPtr paramP = (ExgAskParamPtr)cmdPBP;
		// This call may be made from another application, so your
		// app must not assume that it's globals are initialized.
		Boolean appIsActive = launchFlags & sysAppLaunchFlagSubCall;
    	if (!appIsActive)
    		{
    		// app is not currently running, so
      		// pop up dialog here asking if user wants to get data
    		if ( FrmAlert(AskAlert) == AskYes)
    			paramP->result = exgAskOk;
    		else
    			paramP->result = exgAskCancel;
    		}
    	else
    		{ // app is running so always autoconfirm the ask dialog
    		paramP->result = exgAskOk;
    		}
		}
	else if (cmd == sysAppLaunchCmdExgReceiveData)
		{
		// This call may be made from another application, so your
		// app must not assume that it's globals are initialized.
		Boolean appIsActive = launchFlags & sysAppLaunchFlagSubCall;
    	if (!appIsActive)
    		{
    		// if this test passses,
    		// your app is NOT running,so you cannot use app globals
    		}
		err = ReceiveData((ExgSocketPtr)cmdPBP);
		}
	else if (cmd == sysAppLaunchCmdGoTo)
    	{
		// This call may be made from another application, so your
		// app must not assume that it's globals are initialized.
       	Boolean launched;
        
        
        launched = launchFlags & sysAppLaunchFlagNewGlobals;

        if (launched) 
        	{
           err = StartApplication ();
           if (err) 
              return (err);
        	}

        GoToItem((GoToParamsPtr)cmdPBP, launched);

        if (launched) 
        	{
           	EventLoop ();
           	
          	StopApplication();   
        	}      
         }
	
	return(err);
}



/***********************************************************************
 *
 * FUNCTION:   PrvGetBitmap
 *
 * DESCRIPTION: This routine creates a bitmap copy of the bits in the window passed
 *				This is for compatability with pre version 3.5 roms as well as 3.5 roms
 *
 * PARAMETERS:  winH  	 	window handle to create bitmap from
 *							(output bitmap will have same dimensions as this window)
 *              sizeP  	    pointer to the size of the created bitmap
 *              errorP   	 pointer to any error encountered by this function
 *
 * RETURNED:    Handle to the new bitmap
 *
 *
 ***********************************************************************/
static BitmapPtr PrvGetBitmap (WinHandle winH, UInt32 *sizeP, Err *errorP)
{
	UInt16 		width, height;
	UInt16 		rowBytes;
	BitmapPtr	bitmapP;
	UInt8 		depth;
	WindowType  *winP;

	*errorP = 0;
	*sizeP = 0;
	
	// can we use the new window functions?
	if (newAPI)
		{
		BitmapPtr winBmpP = WinGetBitmap(WinGetDrawWindow());
		ColorTableType *clrTableP = BmpGetColortable(winBmpP);
		UInt8 depth = winBmpP->pixelSize;
		WinHandle winH;
		
		// we create a bitmap and then make a window to it so that we can 
		// create a bitmap whose depth does not match that of the screen
		bitmapP = BmpCreate(drawArea.extent.x, drawArea.extent.y, depth, clrTableP, errorP);	
		if (!*errorP)
			{	
			winH = WinCreateBitmapWindow(bitmapP,errorP);
			}
		if (!*errorP)
			{
			// copy draw area into the bitmap
			WinCopyRectangle (WinGetDrawWindow(), winH, &drawArea, 0, 0, winPaint);
			WinDeleteWindow(winH,false);
	
			// compress the bitmap 
			*errorP = BmpCompress(bitmapP,BitmapCompressionTypeScanLine);
		
			// get the size of the bitmap
			*sizeP = BmpSize(bitmapP);
			}
		} 
	else
		{
		winP = WinGetWindowHandle(winH);

		width = winP->windowBounds.extent.x;
		height = winP->windowBounds.extent.y;
		
		depth = 1; // assume one pixel depth
			
		if ((width == 0) || (height == 0))
		{	*errorP = memErrNotEnoughSpace;
			return NULL;
		}
		// Create a display buffer for the window.  Round the line width
		// up to an even word boundary. 
		rowBytes = ((((width*depth)+15) >> 4) << 1);
		*sizeP = (rowBytes * height) + sizeof(BitmapType);
		// note that we do NOT output a color map here...
		
		bitmapP = (BitmapPtr) MemPtrNew(*sizeP);	
		if (! bitmapP)
		{	*errorP = memErrNotEnoughSpace;
			return NULL;
		}
			
		MemSet(bitmapP,*sizeP,0);  // erase it ...
		
		bitmapP->width = width;
		bitmapP->height = height;
		bitmapP->rowBytes = rowBytes;
		bitmapP->pixelSize = depth;
		bitmapP->version = 1;

		// copy the bitmap contents from the offscreen window
		// The only way to do this on older ROMS is to acesss the obsolete v20 field.
		MemMove(bitmapP+1,winP->displayAddrV20, rowBytes * height);
		}
	return (bitmapP);
}

/***********************************************************************
 *
 * FUNCTION:     SaveWindow
 *
 * DESCRIPTION:  Copies current drawarea to our saved offscreen window
 *
 * PARAMETERS:   the bitmap to draw
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
static void SaveWindow(void)
{
	// copy draw area into the bitmap
	WinCopyRectangle (WinGetDrawWindow(), canvasWinH, &drawArea, 0, 0, winPaint);
	changePending = false;
}

/***********************************************************************
 *
 * FUNCTION:     RestoreWindow
 *
 * DESCRIPTION:  Copies the offscreen window to the drawarea
 *
 * PARAMETERS:   the bitmap to draw
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
static void RestoreWindow(void)
{
	// copy bitmap tp draw area
	RectangleType r;
	RctSetRectangle(&r, 0,0, drawArea.extent.x,drawArea.extent.y);
	WinCopyRectangle (canvasWinH, WinGetDrawWindow(), &r, drawArea.topLeft.x, drawArea.topLeft.y, winPaint);
}

/***********************************************************************
 *
 * FUNCTION:     DrawBitmap
 *
 * DESCRIPTION:  Copies a bitmap onto the display clipped to the drawarea
 *
 * PARAMETERS:   the bitmap to draw
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
static void DrawBitmap(BitmapPtr bmpP)
{
	RectangleType r;
	RectangleType saveClip;
	// copy bitmap to draw area (and make sure it stays within the draw area)
	RctSetRectangle(&r,drawArea.topLeft.x, drawArea.topLeft.y,
		min(bmpP->width,drawArea.extent.x),min(bmpP->height,drawArea.extent.y));
	WinGetClip(&saveClip);
	WinSetClip(&r);
	WinDrawBitmap(bmpP, drawArea.topLeft.x, drawArea.topLeft.y);
	WinSetClip(&saveClip);
}	

/***********************************************************************
 *
 * FUNCTION:     LoadData
 *
 * DESCRIPTION:  This routine loads the save image from a database
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Error if any
 *
 ***********************************************************************/
static Err LoadData(void)
{
	Err error = 0;
	DmOpenRef dbP;
	MemHandle recordH;
	BitmapPtr recordP;
 	UInt16    recIndex = 0;
 	 
  // Find the application's data file.  If it doesn't exist create it.
	dbP = DmOpenDatabaseByTypeCreator (beamerDBType, beamerCreator, dmModeReadOnly);
	if (dbP)
		{
			recordH = DmQueryRecord (dbP, recIndex);
			if (recordH)
				{
				recordP = (BitmapPtr)MemHandleLock(recordH);
				DrawBitmap(recordP);
				MemHandleUnlock(recordH);
				}
      	DmCloseDatabase(dbP);
      	}
 	return 0;

}
/***********************************************************************
 *
 * FUNCTION:     SaveData
 *
 * DESCRIPTION:  This routine saves the image to a database
 *
 * PARAMETERS:   None.
 *
 * RETURNED:     Error if any
 *
 ***********************************************************************/
static Err SaveData(void)
{
	Err error = 0;
	DmOpenRef dbP;
	UInt16 recIndex = 0;
  	MemHandle recordH;
	UInt32 size = 0;
	Err err = 0;
	BitmapPtr bmpP;
	void * recordP;
	

	// copy draw area into the bitmap
	SaveWindow();
	
	bmpP = PrvGetBitmap(canvasWinH, &size, &err);
	
	if (err) return err;
	
  // Find the application's data file.  If it doesn't exist create it.
	dbP = DmOpenDatabaseByTypeCreator (beamerDBType, beamerCreator, dmModeReadWrite);
	if (!dbP)
		{
		error = DmCreateDatabase (0, beamerDBName, beamerCreator, beamerDBType, false);
		if (error)
			return error;
		
		dbP = DmOpenDatabaseByTypeCreator(beamerDBType, beamerCreator, dmModeReadWrite);
		if (!dbP)
			return (1);
		}
	if (dbP)
		{
			// Gotta check first because remove record displays fatal errors 
			// for out of range index values
			if (DmNumRecords(dbP) > recIndex)
				error = DmRemoveRecord (dbP, recIndex);
				
			recordH = DmNewRecord(dbP,&recIndex,size);
			if (recordH)
				{
				recordP = MemHandleLock(recordH);
				DmWrite(recordP,0, bmpP, size );
				MemHandleUnlock(recordH);
				DmReleaseRecord(dbP, recIndex, true);
				}
      	DmCloseDatabase(dbP);
      	}
    if (bmpP) MemPtrFree(bmpP);
 	return 0;

}

