/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: SampleCalc.c
 *
 * Description:
 *		Sample Calculator demonstration application based on FloatMgr
 *
 *
 * Future possibilities:
 *		Could remember stack values in application prefs
 *		Could let user specify display preferences (like gDecDispDigits)
 *		Could display more than eight digits in the mantissa
 *
 * History:
 *   	1/30/97 SCL - Started life as a NewFloatMgr sample
 *   	6/01/97 SCL - Overhauled for new PalmOS 2.0 SDK
 *
 *****************************************************************************/

#include <PalmOS.h>			// Includes all Palm headers
//#include <SysAll.h>			// Includes all Palm headers
#include <StringMgr.h>		// Includes all Palm headers
#include <FloatMgr.h>	// Include Floating Point headers

#include "SampleCalc.h"		// Include our equates file
#include "SampleCalcRsc.h"	// Include Constructor's equates file

#define kDefAccuracy 0		// default accuracy for FlpCorrectedAdd & FlpCorrectedSub
#define version20		0x02000000	// PalmOS 2.0 version number

/************************************************************************
 * Globals -- only accessible during sysAppLaunchCmdNormalLaunch
 ***********************************************************************/
Int16			gDecDispDigits;							// digits displayed after decimal, if possible
UInt16		gMantissaLen;								// current length of gMantissaStr
UInt16		gExponentLen;								// current length of gExponentStr
Char			gMantissaSign;								// current mantissa sign
Char			gExponentSign;								// current exponent sign
Char			gMantissaStr[kMaxMantissaDigits+1];	// currently displayed mantissa
Char			gExponentStr[kMaxExponentDigits+1];	// currently displayed exponent
Boolean		gHasExponent;								// in exponent entry mode
Boolean		gHasDecimal;								// decimal has been entered
Boolean		gStackNotYetLifted;						// should a value entry lift the stack?
Boolean		gFiller1;									// (for alignment)
UInt16		gDecimalPos;								// current position of decimal

FlpCompDouble	gStackRegX, gStackRegY;				// stack registers
FlpCompDouble	gStackRegZ, gStackRegT;				// stack registers

// Useful macros:
#define mNeedToConvertX		(gMantissaLen!=0)		// are we entering a new X value?

/************************************************************************
 * Function prototypes
 ***********************************************************************/

static void				StackLiftPushX(Boolean dontCheck);
static FlpCompDouble	StackDropPopY(void);

static Boolean DisplayStringAppendDigit(Char theDigit);
static Boolean DisplayStringAppendDecimal(void);
static Boolean DisplayStringNegate(void);
static Boolean DisplayStringAddExponent(void);
static Boolean DisplayStringBackspace(void);
static void 	DisplayStringConvertToXReg(void);
static void		DisplayStringReset(void);
static void 	DisplayUpdate(void);
static Boolean FunctionPushX(void);
static Boolean FunctionCalculate(CalculationSelectorType whichOp);

static void		MainFormInit(FormPtr frmP);
static void		MainFormDoCommand (UInt16 command);
static Boolean	MainFormHandleEvent(EventPtr eventP);

static Boolean	AppHandleEvent(EventPtr eventP);
static Boolean	StartApplication(void);
static void		StopApplication(void);
UInt32			PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags);


/***********************************************************************
 *
 * FUNCTION:    StackLiftPushX
 *
 * DESCRIPTION: This routine performs a stack lift operation
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 ***********************************************************************/
static void StackLiftPushX(Boolean dontCheck)
{
	if ( dontCheck ||
			( (gMantissaLen == 0) && gStackNotYetLifted ) ) {
		gStackRegT = gStackRegZ;
		gStackRegZ = gStackRegY;
		gStackRegY = gStackRegX;
		gStackNotYetLifted = false;
	}
}


/***********************************************************************
 *
 * FUNCTION:    StackDropPopY
 *
 * DESCRIPTION: This routine pops the Y value off the stack
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 ***********************************************************************/
static FlpCompDouble StackDropPopY(void)
{
	FlpCompDouble valueToReturn;

	valueToReturn = gStackRegY;
	gStackRegY = gStackRegZ;
	gStackRegZ = gStackRegT;
	return(valueToReturn);
}


/***********************************************************************
 *
 * FUNCTION:    DisplayStringAppendDigit
 *
 * DESCRIPTION: This routine appends the requested digit to the display
 *
 * PARAMETERS:  digit to add
 *
 * RETURNED:    true if display should be updated
 *
 ***********************************************************************/
static Boolean DisplayStringAppendDigit(Char theDigit)
{	
	Boolean needsUpdate;

	if (gHasExponent) {

		if (gExponentLen < kMaxExponentDigits) {
			gExponentStr[ gExponentLen++ ] = theDigit;
		} else {
			UInt16 count;
			for (count = 1; count < gExponentLen; count++) {
				gExponentStr[count-1] = gExponentStr[count];
			}
			gExponentStr[ gExponentLen-1 ] = theDigit;
		}
		needsUpdate = true;

	} else {

		if (gMantissaLen < (gHasDecimal ? kMaxMantissaDigits+1 : kMaxMantissaDigits) ) {
			if (gMantissaLen==0) {
				StackLiftPushX(false);							// lift the stack, if appropriate
			}
			gMantissaStr[ gMantissaLen++ ] = theDigit;
			gMantissaStr[ gMantissaLen] = kCharNull;
			needsUpdate = true;
		} else {
			needsUpdate = false;
		}

	}
	return(needsUpdate);
}


/***********************************************************************
 *
 * FUNCTION:    DisplayStringAppendDecimal
 *
 * DESCRIPTION: This routine adds a decimal to the display, if valid.
 *					 If decimal is initial "digit", prepend a zero.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    true if display should be updated
 *
 ***********************************************************************/
static Boolean DisplayStringAppendDecimal(void)
{	
	Boolean needsUpdate;

	if (!gHasExponent && !gHasDecimal && (gMantissaLen < kMaxMantissaDigits) ) {
		if (gMantissaLen == 0) {
			StackLiftPushX(false);							// lift the stack, if appropriate
			gMantissaStr[ gMantissaLen++ ] = kCharZero;
		}
		gHasDecimal = true;
		gDecimalPos = gMantissaLen;
		gMantissaStr[ gMantissaLen++ ] = kCharDecimal;
		needsUpdate = true;
	} else {
		needsUpdate = false;
	}
	return(needsUpdate);
}


/***********************************************************************
 *
 * FUNCTION:    DisplayStringNegate
 *
 * DESCRIPTION: This routine negates the exponent or mantissa
 *
 * PARAMETERS:  none
 *
 * RETURNED:    true if display should be updated
 *
 ***********************************************************************/
static Boolean DisplayStringNegate(void)
{	
	Boolean needsUpdate = true;

	if (gHasExponent) {

		gExponentSign = gExponentSign ? kCharNull : kCharMinus;

	} else {

		if (gMantissaLen != 0) {

			gMantissaSign = gMantissaSign ? kCharNull : kCharMinus;

		} else {		// negate the currently displayed value

			if (!FlpIsZero(gStackRegX)) {
				FlpNegate( gStackRegX );
			} else {
				needsUpdate = false;
			}

		}

	}
	return(needsUpdate);
}


/***********************************************************************
 *
 * FUNCTION:    DisplayStringAddExponent
 *
 * DESCRIPTION: This routine switches to exponent mode
 *
 * PARAMETERS:  none
 *
 * RETURNED:    true if display should be updated
 *
 ***********************************************************************/
static Boolean DisplayStringAddExponent(void)
{	
	Boolean needsUpdate;

	if (!gHasExponent) {
		gHasExponent = true;
		needsUpdate = true;
		if (gMantissaLen == 0) {							// if no mantissa,
				StackLiftPushX(false);							// lift the stack, if appropriate
			gMantissaStr[ gMantissaLen++ ] = kCharOne;	// set it to 1.0
		}
	} else {
		needsUpdate = false;
	}
	return(needsUpdate);
}


/***********************************************************************
 *
 * FUNCTION:    DisplayStringBackspace
 *
 * DESCRIPTION: This routine backspaces out the previous entry
 *
 * PARAMETERS:  none
 *
 * RETURNED:    true if display should be updated
 *
 ***********************************************************************/
static Boolean DisplayStringBackspace(void)
{	
	Boolean needsUpdate;

	if (gMantissaLen) {								// has a new value been started?
		needsUpdate = true;
		if (gHasExponent) {							// working on the exponent...
			if (gExponentLen) {
				gExponentLen--;
			} else {
				if (gExponentSign) {
					gExponentSign = kCharNull;
				} else {
					gHasExponent = false;
				}
			}
		} else {											// working on the mantissa...
			gMantissaLen--;
			if (gMantissaLen == 0) {				// if we just removed the only digit,
				gMantissaSign = kCharNull;			// also remove the sign, if any
			} else {
				if (gMantissaStr[gMantissaLen] == kCharDecimal) {
					gHasDecimal = false;				// did we just remove the decimal?
				}
			}
		}
	} else {
		// Clear any displayed error since we don't have a clear key
		if (!FlpIsZero(gStackRegX)) {
			gStackRegX.d = 0.0;						// clear out the displayed X register
			gStackNotYetLifted = false;			// this zero shouldn't be lifted
			needsUpdate = true;
		} else {
			needsUpdate = false;
		}
	}
	return(needsUpdate);
}


/***********************************************************************
 *
 * FUNCTION:    DisplayStringReset
 *
 * DESCRIPTION: This routine resets the display value storage
 *					 to '0.00000000 00'
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 ***********************************************************************/
static void DisplayStringReset(void)
{	
	UInt16 count;

	gMantissaLen = 0;
	gExponentLen = 0;
	gMantissaSign = kCharNull;
	gExponentSign = kCharNull;

	gMantissaStr[0] = kCharZero;
	gMantissaStr[1] = kCharDecimal;
	gDecimalPos = 1;						// it's there
	gHasDecimal = false;					// but it hasn't been "entered" yet

	for (count=2; count<kMaxMantissaDigits; count++) {
		gMantissaStr[count] = kCharZero;
	}
	gMantissaStr[count] = kCharNull;

	for (count=0; count<kMaxExponentDigits; count++) {
		gExponentStr[count] = kCharZero;
	}
	gExponentStr[count] = kCharNull;

	gHasExponent = false;

	return;
}


/***********************************************************************
 *
 * FUNCTION:    DisplayStringConvertToXReg
 *
 * DESCRIPTION: This routine converts the entered value into the X register,
 *						and resets the DisplayString.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 ***********************************************************************/
static void DisplayStringConvertToXReg(void)
{	
	Char	stringToConvert[kMaxMantissaDigits+kMaxExponentDigits+5];
	UInt16	curPos = 0;
	UInt16	count;

	// Start with mantissa sign, if any
	if (gMantissaSign)
		stringToConvert[curPos++] = gMantissaSign;

	// Append mantissa, including decimal point
	for (count=0; count<gMantissaLen; count++) {
		stringToConvert[curPos++] = gMantissaStr[count];
	}

	// Append exponent, if any
	if (gHasExponent) {
		stringToConvert[curPos++] = 'e';

		if (gExponentSign)
			stringToConvert[curPos++] = gExponentSign;

		for (count=0; count<gExponentLen; count++) {
			stringToConvert[curPos++] = gExponentStr[count];
		}
	}

	stringToConvert[curPos++] = kCharNull;

	gStackRegX.fd = FlpAToF(stringToConvert);

	DisplayStringReset();		// we're no longer entering a DisplayString
	return;
}


/***********************************************************************
 *
 * FUNCTION:    DisplayUpdate
 *
 * DESCRIPTION: This routine draws the display value to the screen.
 *						There are undoubtedly better ways to do this.
 *						SampleCalc is intended to be an example of how to
 *						build PalmOS floating point apps.  In the future it might
 *						also be an example of how to (flexibly) display
 *						floating point values. For now this will have to do...
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 ***********************************************************************/
static void DisplayUpdate(void)
{	
	RectangleType	displayRect;
	Err				theErr;

	displayRect.topLeft.x = kDispManSignLeft;
	displayRect.topLeft.y = kDispManTop -7;		// erase startup title
	displayRect.extent.x = kDispWidth;
	displayRect.extent.y = kDispHeight +7;
	WinEraseRectangle(&displayRect,0);
	FntSetFont(ledFont);

	if (!mNeedToConvertX) {		// display the X register since we're not in entry mode
		UInt32	xMantissa = 0;
		Int16 xExponent = 0;
		Int16 xSign = 0;
		Int16	xDecimalPos = 0;
		Int16	count = 0;
		Int16	xDecimalDigits = 0;
		Char	*mantissaStrP, *dispMantissaStrP;
		Char	mantissaStr[kMaxMantissaDigits+2];		// allow for '-' and '\0'
		Char	dispMantissaStr[kMaxMantissaDigits+4];	// allow for '-0.' and '\0'

		theErr = FlpBase10Info(gStackRegX.fd, &xMantissa, &xExponent, &xSign);
		if (theErr == flpErrOutOfRange) {

			// The value is out of range, so display an error signal
			WinDrawChars ("-\057-", 3, kDispErrorLeft, kDispManTop);	// "-e-" in ledFont

		} else {
		
			// One might want to consider rounding since we may not be showing all digits...

			// NewFloatMgr bug: FlpBase10Info says zero is "negative", with exponent of one.
			
			if (xMantissa == 0) {
				StrCopy (mantissaStr, "00000000");		// should really be kMaxMantissaDigits zeros
				xExponent = 1-kMaxMantissaDigits;		// force "zero" to be displayed as 0.000
				xSign = 0;										// clear the sign (NewFloatMgr bug workaround)
			} else {
				StrIToA(mantissaStr, xMantissa);
			}
			
			if (xSign /*&& xMantissa*/)	// if xMantissa was zero, we've already cleared xSign
				WinDrawChars ("-", 1, kDispManSignLeft, kDispManTop);	// display minus sign


			// Format the mantissa (install decimal point, etc.), and display it...

			if ((xExponent<=0) && (xExponent>(-kMaxMantissaDigits-gDecDispDigits))) {

				xDecimalPos = xExponent + kMaxMantissaDigits;	// place the decimal correctly
				xExponent = 0;												// no exponent displayed

			} else {					// xExponent > 0 || xExponent <= -8-gDecDispDigits
										// ex: 12345678e01 --> 1.234e08 and 12345678e-11 --> 1.234e-4

				xDecimalPos = 1;							// put decimal after first digit
				xExponent += kMaxMantissaDigits-1;	// bump exponent to reflect new decimal position
			
			}

			if (xDecimalPos<1) {										// put "0." at front of number
				dispMantissaStr[count++] = kCharZero;
				dispMantissaStr[count++] = kCharDecimal;
			}
			
			while ( (xDecimalPos + xDecimalDigits) < 0) {	// add any needed zeros after decimal
				dispMantissaStr[count++] = kCharZero;
				xDecimalDigits++;
			}

			mantissaStrP = mantissaStr;
			dispMantissaStrP = dispMantissaStr;
			
			while ((count<kMaxMantissaDigits+1) && (xDecimalDigits<gDecDispDigits)) {
				dispMantissaStr[count++] = *mantissaStrP++;
				if (count == xDecimalPos) {					// put the decimal here
					dispMantissaStr[count++] = kCharDecimal;
				} else if (count > xDecimalPos) {			// keep track of the decimal digits
					xDecimalDigits++;
				}
			}
			dispMantissaStr[count++] = kCharNull;			// terminate the string

			WinDrawChars (dispMantissaStr, StrLen(dispMantissaStr), kDispMantissaLeft, kDispManTop);


			// Display the exponent...

			if (xExponent) {
				Char	exponentStr[2];
				
				if (xExponent<0) {
					WinDrawChars ("-", 1, kDispExpSignLeft, kDispManTop);	// display minus sign
					xExponent = -xExponent;
				}

				exponentStr[1] = xExponent % 10 + '0';
				xExponent /= 10;
				exponentStr[0] = xExponent % 10 + '0';
				
				WinDrawChars (exponentStr, 2, kDispExponentLeft, kDispManTop);
			}
		}

	} else {		// We're in value-entry mode, so just display the entered string components

		if (gMantissaSign)
			WinDrawChars (&gMantissaSign, 1, kDispManSignLeft, kDispManTop);

		if (gMantissaLen) {
			WinDrawChars (gMantissaStr, gMantissaLen?gMantissaLen:kMaxMantissaDigits, kDispMantissaLeft, kDispManTop);
		} else {
			WinDrawChars ("0.0000", 6, kDispMantissaLeft, kDispManTop);	// force "zero" to be displayed as 0.000
		}
		
		if (gHasExponent) {
			Char	exponentStr[kMaxExponentDigits];
			UInt16	curPos = 0;
			UInt16	count;

			// right justify the exponent, padded with leading zeros
			for (count=gExponentLen; count<kMaxExponentDigits; count++) {
				exponentStr[curPos++] = kCharZero;
			}
			for (count=0; count<gExponentLen; count++) {
				exponentStr[curPos++] = gExponentStr[count];
			}
			
			if (gExponentSign)
				WinDrawChars (&gExponentSign, 1, kDispExpSignLeft, kDispManTop);

			WinDrawChars (exponentStr, kMaxExponentDigits, kDispExponentLeft, kDispManTop);
		}

	}

	return;
}


/***********************************************************************
 *
 * FUNCTION:		FunctionPushX
 *
 * DESCRIPTION:	Pushes the displayed value onto the stack (Y).
 *						Converts ValueString to X register first, if needed.
 *
 * PARAMETERS:		none
 *
 * RETURNED:		true if display should be updated
 *
 ***********************************************************************/
static Boolean FunctionPushX(void)
{
	if (mNeedToConvertX) {
		DisplayStringConvertToXReg();
	}

	StackLiftPushX(true);		// lift the stack; no questions!

	return(true);					// don't beep; display error if necessary
}


/***********************************************************************
 *
 * FUNCTION:		FunctionCalculate
 *
 * DESCRIPTION:	Performs desired calculation. If a value was being
 *						entered, it is first pushed on the stack.
 *
 * PARAMETERS:		whichOp - the desired calculation
 *
 * RETURNED:		true if display should be updated
 *
 ***********************************************************************/
static Boolean FunctionCalculate(CalculationSelectorType whichOp)
{
	FlpCompDouble theResult, tempValue;

	if (mNeedToConvertX) {
		DisplayStringConvertToXReg();
	}

	theResult.d = 0.0;

	switch (whichOp)
		{
		case kfAdd:		gStackRegX.fd = FlpCorrectedAdd(StackDropPopY().fd, gStackRegX.fd, kDefAccuracy);
			break;

		case kfSub:		gStackRegX.fd = FlpCorrectedSub(StackDropPopY().fd, gStackRegX.fd, kDefAccuracy);
			break;

		case kfMul:		gStackRegX.fd = _d_mul(StackDropPopY().fd, gStackRegX.fd);
			break;

		case kfDiv:		gStackRegX.fd = _d_div(StackDropPopY().fd, gStackRegX.fd);
			break;

		case kfExchg:
			{
				tempValue = gStackRegX;
				gStackRegX = gStackRegY;
				gStackRegY = tempValue;
				break;
			}	// end case kfExchg

		default:			ErrDisplay("Invalid operation passed to FunctionCalculate!");
			break;
		
		}	// end switch (whichOp)

	gStackNotYetLifted = true;
	return(true);
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
 ***********************************************************************/
static void MainFormDoCommand (UInt16 command)
{
	FormPtr		frm;
	
	switch (command)
		{
		case MainOptionsAboutSampleCalc:
			{
				MenuEraseStatus(0);					// Clear the menu status from the display
				frm = FrmInitForm (AboutForm);
				FrmDoDialog (frm);					// Display the About Box
			 	FrmDeleteForm (frm);
				break;
			}	// end case kmOptionsAbout

		default:
			break;
		
		}	// end switch (command)
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
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr eventP)
{
	FormPtr						frmP;
	Boolean						handled = false;
	Boolean						displayNeedsUpdate = false;
	Char							digitToAppend = '\0';
	CalculationSelectorType functionToCalculate = kfNoOp;

	switch (eventP->eType)
		{
		case menuEvent:
			{
				MainFormDoCommand (eventP->data.menu.itemID);
				handled = true;
				break;
			}	// end case menuEvent

		case frmOpenEvent:
			{
				frmP = FrmGetActiveForm();
				// MainFormInit( frmP);			// Don't really need this (for now)
				FrmDrawForm ( frmP);
				handled = true;
				break;
			}	// end case frmOpenEvent

		case ctlSelectEvent:
			{
				handled = true;		// assume we'll handle it
				switch (eventP->data.ctlSelect.controlID)
					{
					case MainZeroButton:		digitToAppend = '0';
						break;
					case MainOneButton:		digitToAppend = '1';
						break;
					case MainTwoButton:		digitToAppend = '2';
						break;
					case MainThreeButton:	digitToAppend = '3';
						break;
					case MainFourButton:		digitToAppend = '4';
						break;
					case MainFiveButton:		digitToAppend = '5';
						break;
					case MainSixButton:		digitToAppend = '6';
						break;
					case MainSevenButton:	digitToAppend = '7';
						break;
					case MainEightButton:	digitToAppend = '8';
						break;
					case MainNineButton:		digitToAppend = '9';
						break;

					case MainDecimalButton:	displayNeedsUpdate = DisplayStringAppendDecimal();
						break;
					case MainCHSButton:		displayNeedsUpdate = DisplayStringNegate();
						break;
					case MainEXPButton:		displayNeedsUpdate = DisplayStringAddExponent();
						break;
					case MainBackButton:		displayNeedsUpdate = DisplayStringBackspace();
						break;
					case MainEnterButton:	displayNeedsUpdate = FunctionPushX();
						break;

					case MainAddButton:		functionToCalculate = kfAdd;
						break;
					case MainSubButton:		functionToCalculate = kfSub;
						break;
					case MainMulButton:		functionToCalculate = kfMul;
						break;
					case MainDivButton:		functionToCalculate = kfDiv;
						break;
					case MainExchgButton:	functionToCalculate = kfExchg;
						break;

					default:						handled = false;		// not yet handled
						break;

					}	// end switch(controlID)

				// If we have a digit to append or a function to calculate, do it!
				//	(Doing it here instead of inside each case statement above saves
				// many nearly identical function calls and makes the code much smaller.)

				if (digitToAppend) {
					displayNeedsUpdate = DisplayStringAppendDigit(digitToAppend);
				}
				else if (functionToCalculate != kfNoOp) {
					displayNeedsUpdate = FunctionCalculate(functionToCalculate);
				}

				// Play an appropriate sound - tick for valid keypresses, beep otherwise
				SndPlaySystemSound(displayNeedsUpdate ? sndClick : sndClick);

				break;
			}	// end case ctlSelectEvent

		case keyDownEvent:	// Process keystrokes from Graffiti or simulator keyboard
			{
				Char	theChr = eventP->data.keyDown.chr;

				if (theChr >= '0' && theChr <= '9') {
					handled = true;
					displayNeedsUpdate = DisplayStringAppendDigit(theChr);

				} else {

					handled = true;			
					switch (theChr)
						{
						case kCharDecimal:	displayNeedsUpdate = DisplayStringAppendDecimal();
							break;
						case kCharEquals:		displayNeedsUpdate = DisplayStringNegate();		// Equals key
							break;
						case 'E': case 'e':	displayNeedsUpdate = DisplayStringAddExponent();
							break;
						case '\b':				displayNeedsUpdate = DisplayStringBackspace();	// Delete key
							break;
						case '\006':			displayNeedsUpdate = FunctionPushX();				// Enter key
							break;

						case kCharPlus:		functionToCalculate = kfAdd;
							break;
						case kCharMinus:		functionToCalculate = kfSub;
							break;
						case kCharTimes:		functionToCalculate = kfMul;
							break;
						case kCharDivide:		functionToCalculate = kfDiv;
							break;

						default:					handled = false;		// not yet handled
							break;
						}	// end switch(theChr)
				}

				// If we have a function to calculate, do it!

				if (functionToCalculate != kfNoOp) {
					displayNeedsUpdate = FunctionCalculate(functionToCalculate);
				}

				break;
			}	// end case keyDownEvent

		default:
			break;
		
		}	// end switch (eventP->eType)
		
	// Update the display if something changed
	if (displayNeedsUpdate) {
		DisplayUpdate();
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

			default:
//				ErrFatalDisplay("Invalid Form Load Event");
				break;

			}	// end switch (formId)

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
 * FUNCTION:    StartApplication
 *
 * DESCRIPTION: This routine sets up the initial state of the application.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    true if error (database couldn't be created)
 *
 ***********************************************************************/
static Boolean StartApplication(void)
{
	// Initialize and draw the main form.
	FrmGotoForm(MainForm);
	
	// Initialize globals
	DisplayStringReset();
	
	gDecDispDigits = 5;		// digits displayed after decimal, if possible
	
	gStackRegX.d = 0.0;
	gStackRegY.d = 0.0;
	gStackRegZ.d = 0.0;
	gStackRegT.d = 0.0;

	gStackNotYetLifted = false;

	return(0);
}


/***********************************************************************
 *
 * FUNCTION:    StopApplication
 *
 * DESCRIPTION: P15. This routine closes the application's database
 *              and saves the current state of the application.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void StopApplication(void)
{
	FrmCloseAllForms ();
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
				{				
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
				}
			}
		
		return (sysErrRomIncompatible);
		}

	return 0;
}

/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the NewFloatSample
 *              application.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;		// error starting the app

	error = RomVersionCompatible (version20, launchFlags);
	if (error) return error;
	
	if (cmd != sysAppLaunchCmdNormalLaunch) {
		return sysErrParamErr;
	}

	// Initialize the application's global variables and database.
	error = StartApplication ();

	// Start the event loop.
	if (!error) 
		EventLoop ();

	// Clean up before exiting the applcation.
	StopApplication ();

	return 0;
}

