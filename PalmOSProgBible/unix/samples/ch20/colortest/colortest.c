/***********************************************************************
 *
 * PROJECT:  Color Test
 * FILE:     colortest.c
 * AUTHOR:   Lonnon R. Foster
 *
 * DESCRIPTION:  Main routines for Color Test sample application.
 *
 * From Palm OS Programming Bible
 * ©2000 Lonnon R. Foster.  All rights reserved.
 *
 ***********************************************************************/

#include <PalmOS.h>
#include "colortestRsc.h"


/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
UInt32 gOldDepth;             // Original display depth
UInt32 gSupportedDepths;      // Supported screen depths
UInt16 gCurrentPushButtonID;  // Currently selected push button
RGBColorType gCustomPalette[256];  // Custom palette


/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator			  'LFct'
#define appVersionNum              0x01
#define appPrefID                  0x00
#define appPrefVersionNum          0x01

// Define the minimum OS version we support (3.5)
#define ourMinVersion	sysMakeROMVersion(3,5,0,sysROMStageRelease,0)


/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/


/***********************************************************************
 *
 * FUNCTION:    pow
 *
 * DESCRIPTION: Calculates an integer raised to a positive integer power.
 *
 * PARAMETERS:  x - base
 *              y - power
 *
 * RETURNED:    x to the y power
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static UInt16 pow(UInt16 x, UInt16 y)
{
	int     i;
	UInt16  result = x;
	
	
	if (y == 0)
		return 1;

	for (i = 1; i < y; i++) {
		result *= x;
	}
	
	return result;
}

/***********************************************************************
 *
 * FUNCTION:    sqrt
 *
 * DESCRIPTION: Calculates the approximate square root of a number.
 *
 * PARAMETERS:  x - the number for which to find a square root
 *
 * RETURNED:    the approximate square root of x
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static float sqrt(float x)
{
	float  guess = 1.0;
	int    i;


	for (i = 0; i < 10; i++) {
		guess = ( guess + (x / guess) ) / 2;
	}
	
	return guess;
}



/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version is meet your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
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
static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	// See if we're on in minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
		{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
			{
			FrmAlert (RomIncompatibleAlert);
		
			// Palm OS 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < ourMinVersion)
				{
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
				}
			}
		
		return sysErrRomIncompatible;
		}

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    void *
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
static void MainFormInit(FormPtr form)
{
	UInt16  ctlID;
	
	
	// Determine current color depth and its corresponding push button.
	switch (gOldDepth) {
		case 1:
			ctlID = MainColorDepth1BitPushButton;
			break;
			
		case 2:
			ctlID = MainColorDepth2BitPushButton;
			break;
			
		case 4:
			ctlID = MainColorDepth4BitPushButton;
			break;
			
		case 8:
			ctlID = MainColorDepth8BitPushButton;
			
			// Display the custom palette check box.
			FrmShowObject(form, FrmGetObjectIndex(form,
			    MainCustomPaletteCheckbox));
			
			break;
			
		default:
			break;
	}
	
	// Set the push button selection to reflect the appropriate color
	// depth.
	FrmSetControlGroupSelection(form, 1, ctlID);
	gCurrentPushButtonID = ctlID;
	
	// Set the push button selection to indicate a bitmap family.
	FrmSetControlGroupSelection(form, 2, MainBitmapFamilyPushButton);

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
	Boolean  handled = false;
	FormPtr  form;

	switch (command) {
		case MainOptionsAboutColorTest:
			MenuEraseStatus(0);					// Clear the menu status from the display.
			form = FrmInitForm (AboutForm);
			FrmDoDialog(form);					// Display the About Box.
			FrmDeleteForm(form);
			handled = true;
			break;
	}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    MainFormDrawColorGrid
 *
 * DESCRIPTION: Draws the available colors into the gadget on the
 *              main form.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormDrawColorGrid(void)
{
	FormType  *form = FrmGetActiveForm();
	UInt32    depth;
	UInt16    i;
	UInt16    total;  // Total number of colors in the grid
	UInt16    size;   // Size, in pixels, of a single color square
	UInt16    num;    // Number of rows (and columns) in the grid
	RectangleType  bounds, r;
	
	
	WinScreenMode(winScreenModeGet, NULL, NULL, &depth, NULL);
	ErrFatalDisplayIf(depth > 8, "Invalid color depth");
	
	// Set up values for the color grid.
	total = pow(2, depth);
	
	if (total == 2)
		num = 2;
	else
		num = (UInt16) sqrt((float)total);
	
	FrmGetObjectBounds(form,
		FrmGetObjectIndex(form, MainColorSquareGadget), &bounds);
	size = bounds.extent.x / num;
	
	r.topLeft.x = bounds.topLeft.x;
	r.topLeft.y = bounds.topLeft.y;
	r.extent.x = size;
	if (total != 2)
		r.extent.y = size;
	else
		r.extent.y = bounds.extent.y;

	// Save the current draw state.
	WinPushDrawState();

	// Draw the color grid.
	for (i = 0; i < total; i++) {
		WinSetForeColor(i);
		WinDrawRectangle(&r, 0);
		if ( (i + 1) % num == 0) {
			r.topLeft.x = bounds.topLeft.x;
			r.topLeft.y += size;
		} else {
			r.topLeft.x += size;
		}
	}
	
	// Restore the saved draw state.
	WinPopDrawState();
	
	// Draw a border around the grid.
	WinDrawRectangleFrame(simpleFrame, &bounds);
}


/***********************************************************************
 *
 * FUNCTION:    MainFormSwitchBitmap
 *
 * DESCRIPTION: Switches between the bitmap family and single bitmap.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormSwitchBitmap(UInt16 controlID)
{
	FormType  *form = FrmGetActiveForm();
	
	
	if (controlID == MainBitmapFamilyPushButton) {
		FrmHideObject(form, FrmGetObjectIndex(form,
			MainEarthSingleBitMap));
		FrmShowObject(form, FrmGetObjectIndex(form,
			MainEarthFamilyBitMap));
	} else {
		FrmHideObject(form, FrmGetObjectIndex(form,
			MainEarthFamilyBitMap));
		FrmShowObject(form, FrmGetObjectIndex(form,
			MainEarthSingleBitMap));
	}
}


/***********************************************************************
 *
 * FUNCTION:    MainFormSwitchPalette
 *
 * DESCRIPTION: Switches between the default and custom color palettes.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormSwitchPalette(void)
{
	FormType  *form = FrmGetActiveForm();
	ControlType  *ctl = GetObjectPtr(MainCustomPaletteCheckbox);
	
	
	if (CtlGetValue(ctl) == 1) {
		// Switch to the custom palette.
		WinPalette(winPaletteSet, 0, 256, gCustomPalette);
	} else {
		// Switch to the default palette.
		WinPalette(winPaletteSetToDefault, NULL, NULL, NULL);
	}
}


/***********************************************************************
 *
 * FUNCTION:    MainFormSelectColorDepth
 *
 * DESCRIPTION: Handles selection of color depth from the main form's
 *              push buttons.
 *
 * PARAMETERS:  controlID - ID of the push button that was selected
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormSelectColorDepth(UInt16 controlID)
{
	FormType  *form = FrmGetActiveForm();
	UInt32    depth;
	UInt32    depthHex;
	UInt16    ctlIndex;
	
	
	// If the push button is already selected, there is nothing to do,
	// so return.
	if (controlID == gCurrentPushButtonID)
		return;
		
	switch (controlID) {
		case MainColorDepth1BitPushButton:
			depth = 1;
			depthHex = 0x01;
			break;
			
		case MainColorDepth2BitPushButton:
			depth = 2;
			depthHex = 0x02;
			break;
			
		case MainColorDepth4BitPushButton:
			depth = 4;
			depthHex = 0x08;
			break;
			
		case MainColorDepth8BitPushButton:
			depth = 8;
			depthHex = 0x80;
			break;
			
		default:
			ErrFatalDisplay("Invalid ID");
	}
	
	if (depthHex & gSupportedDepths) {
		// Change color depth and refresh the screen.
		WinScreenMode(winScreenModeSet, NULL, NULL, &depth, NULL);
		FrmUpdateForm(MainForm, frmRedrawUpdateCode);
		gCurrentPushButtonID = controlID;
		
		// Display the custom palette check box if 8-bit mode is selected,
		// or hide it when another color depth is selected.
		ctlIndex = FrmGetObjectIndex(form, MainCustomPaletteCheckbox);
		if (depth == 8)
			FrmShowObject(form, ctlIndex);
		else
			FrmHideObject(form, ctlIndex);

	} else {
		// Alert the user that the selected depth is not supported.
		FrmAlert(UnsupportedDepthAlert);
		
		// Set push button back to where it was.
		FrmSetControlGroupSelection(form, 1, gCurrentPushButtonID);
	}
}


/***********************************************************************
 *
 * FUNCTION:    MainFormUpdateDisplay
 *
 * DESCRIPTION: Updates the display of the main form.
 *
 * PARAMETERS:  updateCode - a code that indicates what changes have been
 *                           made to the form
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormUpdateDisplay(UInt16 updateCode)
{
	ErrNonFatalDisplayIf(updateCode != frmRedrawUpdateCode,
	                     "Invalid update code");
	
	FrmDrawForm(FrmGetActiveForm());
		
	// Switch to the custom palette if the check box is selected.
	MainFormSwitchPalette();
			
	MainFormDrawColorGrid();
}


/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "MainForm" of this application.
 *
 * PARAMETERS:  event - a pointer to an EventType structure
 *
 * RETURNED:    true if the event was handled; false if the event should
 *              be passed to a higher level event handler
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr event)
{
	Boolean  handled = false;
	FormPtr  form;

	switch (event->eType) {
		case menuEvent:
			handled = MainFormDoCommand(event->data.menu.itemID);
			break;

		case frmOpenEvent:
			form = FrmGetActiveForm();
			MainFormInit(form);
			FrmDrawForm(form);
			MainFormDrawColorGrid();
			handled = true;
			break;

		case ctlSelectEvent:
			switch (event->data.ctlSelect.controlID) {
				case MainCustomPaletteCheckbox:
					MainFormSwitchPalette();
					break;
					
				case MainBitmapFamilyPushButton:
				case MainBitmapSinglePushButton:
					MainFormSwitchBitmap(event->data.ctlSelect.controlID);
					break;
					
				default:
					MainFormSelectColorDepth
						(event->data.ctlSelect.controlID);
					break;
			}
			handled = true;
			break;
		
		
		case frmUpdateEvent:
			MainFormUpdateDisplay(event->data.frmUpdate.updateCode);
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
static Boolean AppHandleEvent(EventPtr event)
{
	UInt16 formId;
	FormPtr form;

	if (event->eType == frmLoadEvent)
		{
		// Load the form resource.
		formId = event->data.frmLoad.formID;
		form = FrmInitForm(formId);
		FrmSetActiveForm(form);

		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId)
			{
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
static void EventLoop(void)
{
	UInt16 error;
	EventType event;

	do {
		EvtGetEvent(&event, evtWaitForever);

		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(0, &event, &error))
				if (! AppHandleEvent(&event))
					FrmDispatchEvent(&event);

	} while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:     StartApplication
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
static Err StartApplication(void)
{
	// Store the current display depth so it may be restored on exit.
	WinScreenMode(winScreenModeGet, NULL, NULL, &gOldDepth, NULL);
	WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL,
	              &gSupportedDepths, NULL);
	
	// If 8-bit color depth is supported, construct a custom palette for
	// later use.  This palette is entirely composed of different
	// shades of green.
	if (0x80 & gSupportedDepths) {
		int  i;
		
		for (i = 0; i < 256; i++) {
			gCustomPalette[i].index = i;
			gCustomPalette[i].r = 0;
			gCustomPalette[i].g = 255 - i;
			gCustomPalette[i].b = 0;
		}
	}
	
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    StopApplication
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
static void StopApplication(void)
{
	// Close all the open forms.
	FrmCloseAllForms();
	
	// Restore the original screen depth.
	WinScreenMode(winScreenModeSet, NULL, NULL, &gOldDepth, NULL);
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
UInt32 PilotMain( UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	error = RomVersionCompatible(ourMinVersion, launchFlags);
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

