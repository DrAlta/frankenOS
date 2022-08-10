/***********************************************************************
 *
 * PROJECT:  Serial Chat
 * FILE:     serialchat.h
 * AUTHOR:   Lonnon R. Foster
 *
 * DESCRIPTION:  Header file for Serial Chat sample application.
 *
 * From Palm OS Programming Bible
 * ©2000 Lonnon R. Foster.  All rights reserved.
 *
 ***********************************************************************/

/***********************************************************************
 *
 *   Constants
 *
 ***********************************************************************/
#define maxFieldLength 255


/***********************************************************************
 *
 *   Function Prototypes
 *
 ***********************************************************************/
// Utility functions
static void     ClearField(UInt16 fieldID);
static void     CloseSerial(void);
static FieldPtr GetFocusFieldPtr(void);
static void *   GetObjectPtr(UInt16 objectID);
static Err      OpenSerial(void);
static void     ReadSerial(void);
static Err      RomVersionCompatible(UInt32 requiredVersion,
                                UInt16 launchFlags);
static void     WriteSerial(void);

// Main form
static void     MainFormDisplayMessage(Char *buffer);
static Boolean  MainFormDoCommand(UInt16 menuID);
static Boolean  MainFormHandleEvent(EventType *event);
static void     MainFormInit(FormType *form);
static void     MainFormScroll(UInt16 fieldID, Int16 linesToScroll,
                           Boolean updateScrollbar);
static void     MainFormUpdateScrollBar(UInt16 fieldID);

// Application framework
static Boolean  ApplicationHandleEvent(EventPtr event);
static void     EventLoop(void);
       UInt32   PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags);
static Err      StartApplication(void);
static void     StopApplication(void);
