/************************************************************************
* COPYRIGHT: 	Copyright  ©  1998 Symbol Technologies, Inc. 
*
* FILE: 		SetupDlgs.h
*
* SYSTEM: 		Symbol barcode scanner for Palm III.
* 
* HEADER: 		Routines for form handling of all forms, except Main Form.
*
* DESCRIPTION: 	Form-handling code for all the scan-parameter setup forms.
* 				This file defines the functions called from outside of 
* 				SetupDlgs.c (for instance, ones called from ScanDemo.c).
*
* HISTORY: 		3/2/97    SS   Created
*              ...
*************************************************************************/
#pragma once

// Params Setup Diaog
void OnParamSetup();
Boolean ParamSetupHandleEvent( EventPtr ev );

// Hardware Setup Dialog
void OnHardwareSetup();
Boolean HardwareSetupHandleEvent( EventPtr ev );

// Control Setup Dialog
void OnControlSetup();
Boolean ControlSetupHandleEvent( EventPtr ev );

// Code Format Setup Dialog
void OnCodeFormatSetup();
Boolean CodeFormatSetupHandleEvent( EventPtr ev );

// UPC Setup Dialog
void OnUPCSetup();
Boolean UPCSetupHandleEvent( EventPtr ev );

// More UPC Setup Dialog
void OnUPCMoreSetup();
Boolean UPCMoreSetupHandleEvent( EventPtr ev );

// Code128 Setup Dialog
void OnCode128Setup();
Boolean Code128SetupHandleEvent( EventPtr ev );

// Code39 Setup Dialog
void OnCode39Setup();
Boolean Code39SetupHandleEvent( EventPtr ev );

// More Code 39 Setup Dialog
void OnMoreCode39Setup();
Boolean MoreCode39SetupHandleEvent( EventPtr ev );

// Code93 Setup Dialog
void OnCode93Setup();
Boolean Code93SetupHandleEvent( EventPtr ev );

// I2of5 Setup Dialog
void OnI2of5Setup();
Boolean I2of5SetupHandleEvent( EventPtr ev );

// D2of5 Setup Dialog
void OnD2of5Setup();
Boolean D2of5SetupHandleEvent( EventPtr ev );

// Codabar Setup Dialog
void OnCodabarSetup();
Boolean CodabarSetupHandleEvent( EventPtr ev );

// MSIPlessey Setup Dialog
void OnMSIPlesseySetup();
Boolean MSIPlesseySetupHandleEvent( EventPtr ev );

// Beep Frequency Setup Dialog
void OnBeepFrequencySetup();
Boolean BeepFrequencySetupHandleEvent( EventPtr ev );

// Beep Duration Setup Dialog
void OnBeepDurationSetup();
Boolean BeepDurationSetupHandleEvent( EventPtr ev );

// Beep Test Dialog
void OnBeepTest();
Boolean BeepTestHandleEvent( EventPtr ev );

// About Dialog
void OnAbout();
Boolean AboutHandleEvent( EventPtr ev );
