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

#if !defined(__PDQCORE_H__)
#define __PDQCORE_H__	1

#ifdef _PDQCORE_EXPORT
#define _EXTERN	extern
#define PDQ_TRAP(trapNum)
#else
#define _EXTERN 
#define PDQ_TRAP(trapNum) SYS_TRAP(trapNum)
#endif

// *** Internal library name which can be passed to SysLibFind() ***

#define PDQCoreLibName        	"pdQCore Library"


//*************************************************************************
//
// Signal Definitions
//
//*************************************************************************

//
// Applications can register for the following classes
//

#define SGN_CLASS_TAPI				1L
#define SGN_CLASS_MSG				2L


//
// Applications can use these masks to register/unregister
//

#define SGN_ALL						0xFFFFFFFFL
#define SGN_NONE						0


// 
// Applications should register with a priority. PRIORITY_SYSTEM is reserved
// for system level applications and receive signals first. Most applications
// can register with PRIORITY_0.
//

typedef enum
{
   PRIORITY_0=0,	// lowest priority
   PRIORITY_1,
   PRIORITY_2,
   PRIORITY_3,
   PRIORITY_4,
   PRIORITY_5,
   PRIORITY_6,
   PRIORITY_7,
   PRIORITY_8,
   PRIORITY_9,
   PRIORITY_SYSTEM	// Highest priority
} SigPriorityType;


//
// Applications should respond to this launch code and receive SignalParamsPtr
// as launch code parameter
//

#define sysAppLaunchCmd_PDQSignal  0x4000

//
// Applications should examine signal parameter
//

typedef union
{
	VoidPtr		pVoidArg;
   DWord			dwArg;
   Boolean		bArg;
   Int			nArg;
	CharPtr		szArg;
} SigParamType;

typedef struct
{
   ULong				signalClass; // signal class as defined above
   ULong				signal;      // signal as defined within class
   SigParamType	params;      // params as defined for signal mask
} SignalParamsType, *SignalParamsPtr;


//
// Applications should return the following after processing
// 
// Note: Applications are called in order of priority.
//       SIG_CMD_STOP only stops registered apps with equal or lower priority
//    	from receiving the signal.
//

#define SIG_CMD_CONTINUE      0
#define SIG_CMD_STOP          1


//
// The Max name length to be used for class name.
// 

#define MAX_CLASS_NAME_LEN 32


//*************************************************************************
//
//
//
// SGN_CLASS_TAPI - TAPI related Signals
//
//
//
//*************************************************************************

typedef enum {
   SGN_TAPI_IDLE            = 0x00000001L,      
   SGN_TAPI_INCOMING        = 0x00000002L,      // CallInfoPtr
   SGN_TAPI_LOST            = 0x00000004L,      // CallInfoPtr
   SGN_TAPI_MISSED          = 0x00000008L,      // CallInfoPtr
   SGN_TAPI_FAILED          = 0x00000010L,      // CallInfoPtr
   SGN_TAPI_CALLING         = 0x00000020L,      // CallInfoPtr
   SGN_TAPI_CONVERSATION    = 0x00000040L,      // CallInfoPtr
   SGN_TAPI_ENDED           = 0x00000080L,      // CallInfoPtr
   SGN_TAPI_DIALPAUSED      = 0x00000100L,    	 // PauseType
   SGN_TAPI_CALLERID        = 0x00000200L,	    // CallInfoPtr
   SGN_TAPI_DIALEDDTMF		 = 0x00000400L,    	 // szArg is remaining dial string
	SGN_TAPI_ANSWERING		 = 0x00000800L			// CallInfoPtr
} TAPIMaskType;

//
// CallFlagsType - Indicator of type of call in-progresss.  Also indicates
// call waiting status.
//

typedef enum {
   CT_NORMAL = 0,  			// Two parties
   CT_CONFERENCE,       	// Conference call
   CT_WAITING,           	// Call waiting
   CT_DATA					 	// Data call.
} CallFlagsType;

typedef enum {
	PAUSE_NONE = 0,
	PAUSE_SOFT,					// Timed pause
	PAUSE_HARD					// Hard (User released) pause
} PauseType;

//
// CallerIDStatusType - Indicates the status of caller ID associated
// with the current call.
//

typedef enum {
   PI_ALLOWED          = 0,
   PI_RESTRICTED,
   PI_NOTAVAILABLE,
   PI_RESERVED
} CallerIDStatusType;

//
// CallInfoType - Structure containing information regarding current call.
//

#define MAX_DIGITS	32

typedef struct _CallInfoType {
   DWord                dwSignalHist;                 // History of all bits set on call
   CallFlagsType      	callFlags;                    // Data/Voice, etc.
   CallerIDStatusType  	nCallerIDStatus;              // Reserved, restricted, unavailable...
   Boolean              bDialingPaused;               // Set to TRUE when we hard-pause
   Char                 szNumber[MAX_DIGITS + 1];		// Dialed or Incoming phone number
   ULong                lTimeOfCall;                  // When the call began (in seconds)
   ULong                lCallDuration;                // How long call lasted (in seconds)
   Char						szWaiting[MAX_DIGITS + 1];		// Last call waiting number (if available)
   Char						szExt[MAX_DIGITS + 1];			// Extension or extra DTMF codes...
	Err						nErr;									// Error code set when call fails, etc.
} CallInfoType, *CallInfoPtr;


//*************************************************************************
//
//
//
// SGN_CLASS_MSG - Message Signals
//
//
//
//*************************************************************************

typedef enum
{
	SGN_MSG_RAW_SMS				= 0x00000001L,	 // pVoidArg is Raw SMS message (IS-95)
   SGN_MSG_CARRIER_VOICE_MAIL	= 0x00000002L,  // MsgSigPtr - Carrier voice mail
   SGN_MSG_LOCAL_VOICE_MAIL	= 0x00000004L,  // MsgSigPtr - (not supported in v1.0)
   SGN_MSG_TEXT_MESSAGE			= 0x00000008L,  // MsgSigPtr
   SGN_MSG_EMAIL					= 0x00000010L   // MsgSigPtr - (not supported in v1.0)
} MSGMaskType;


typedef enum
{
   MSG_NONE,
   MSG_NORMAL,
   MSG_URGENT
} MsgStatusType;

// SMS application fields SMS text message.  It can then build the MsgSigType
// structure and Signal() this to other applications.  The Phone Manager could 
// register a notification for this signal and present the message to the user.
// If the user hit's OK (or whatever) the SMS application could choose not 
// to add it to their database of pending messages...

typedef struct _MsgType
{
   UShort				nNumUnread; // if 0, other params ignored
   MsgStatusType		status;		// status of latest mesg
   CharPtr				szMessage;	
} MsgSigType, * MsgSigPtr;


//
// Keypad digit (DTMF) codes.
//

typedef enum {
   DIG_0         = 0x30,
   DIG_1 ,
   DIG_2 ,
   DIG_3 ,
   DIG_4 ,
   DIG_5 ,
   DIG_6 ,
   DIG_7 ,
   DIG_8 ,
   DIG_9 ,
   DIG_A         = 0x41,
   DIG_B ,
   DIG_C ,
   DIG_D ,
   DIG_POUND     = 0x23,
   DIG_STAR      = 0x2A,
   DIG_FLASH     = 0x50,		// HS_SEND_K
   DIG_CLEAR     = 0x52,		// HS_CLR_K
   DIG_SOFTPAUSE = 0x2C,		
   DIG_HARDPAUSE = 0x25
} DialDigitType;


//
// Version structure
//

typedef struct
{
	Word		wStructVer;
	Char		szVer[8];
	Char		szDate[32];
} PDQVerType, * PDQVerPtr;

//
// Error codes
//

#define PDQErrorClass            0x4000

#define PDQErrNoError				0
#define PDQErrNotImplemented		(PDQErrorClass | 1)
#define PDQErrNoMemory				(PDQErrorClass | 2)
#define PDQErrLibStillOpen			(PDQErrorClass | 3) 
#define PDQErrAlready				(PDQErrorClass | 4) 
#define PDQErrNotInCall				(PDQErrorClass | 5)	// No call info available
#define PDQErrInvalidMode			(PDQErrorClass | 6)	// Phone mode does not support request
#define PDQErrNoService				(PDQErrorClass | 7)	// Phone has no service
#define PDQErrBadNumber				(PDQErrorClass | 8)	// Bad phone number
#define PDQErrNoCallInfo			(PDQErrorClass | 9)	// No call info available
#define PDQErrPhoneOff				(PDQErrorClass | 10) // Phone is off...
#define PDQErrBadVersion			(PDQErrorClass | 11) // Private - get status...
#define PDQErrNoHack					(PDQErrorClass | 12)	// Hack error...
#define PDQErrInConversation		(PDQErrorClass | 13) // Conversation in progress
#define PDQErrNoConversation     (PDQErrorClass | 17) // No conversation in progress
#define PDQErrPower					(PDQErrorClass | 18) // Too little power to power module...
#define PDQErrNoSignal				(PDQErrorClass | 19) // No phone signal
#define PDQErrNotRegistered		(PDQErrorClass | 20)	// Signal not registered.
#define PDQErrInvalidSignaling	(PDQErrorClass | 21)	// Invalid call to SIGNAL
#define PDQErrSignalReentered		(PDQErrorClass | 22)	// Invalid call to SIGNAL
#define PDQErrBadDB					(PDQErrorClass | 23)	// Unable to open database
#define PDQErrBadIndex				(PDQErrorClass | 24)	// Bad database index
#define PDQErrBadParam				(PDQErrorClass | 25)  // Invalid parameter passed
#define PDQErrDBNotFound			(PDQErrorClass | 26)  // Database not found
#define PDQErrDBNotOpen				(PDQErrorClass | 27)  // Database can't be opened
#define PDQErrDBWriteFailed		(PDQErrorClass | 28)  // Failed to write the database
#define PDQErrNoHandler				(PDQErrorClass | 29)  // No URL handler found
#define PDQErrHandlerMissing		(PDQErrorClass | 30)  // The registered handler is missing
#define PDQErrBadAddress			(PDQErrorClass | 31)	// Bad/No address sent to mail 
#define PDQErrLocked					(PDQErrorClass | 32) // Phone is locked...

//***************************************************************************
//
// PUBLIC TRAP TABLE
//
// Trap IDs for all public interfaces.
//
//***************************************************************************

typedef enum {

	// All pdQ Shared Libraries begin with this call
	
   PDQCoreLibGetVersion_Trap = sysLibTrapCustom,
   
   // TAPI Interfaces
   
   PDQTelMakeCall_Trap,
   PDQTelAnswerCall_Trap,
   PDQTelEndCall_Trap,
   PDQTelGenerateDTMF_Trap,
	PDQTelResumeDialing_Trap,
	PDQTelCancelPause_Trap,
   PDQTelGetCallInfo_Trap,
	PDQTelFormatNumber_Trap,
   PDQTelGetDigit_Trap,
   
	// Traps reserved for future use...
	
   PDQReserved2_Trap,
   PDQReserved3_Trap,
   PDQReserved4_Trap,
   PDQReserved5_Trap,
   PDQGetVersion_Trap,
   PDQReserved7_Trap,
   PDQReserved8_Trap,
   PDQReserved9_Trap,
   PDQReserved10_Trap,
   
	// Signal Interface...
	   
   PDQSignal_Trap,
   PDQSigRegister_Trap,
   PDQSigUnregister_Trap,
   PDQSigEnumerate_Trap,
   PDQSigAddClass_Trap
   
} PDQCoreLibTrapEnum;

#define PDQLastPublicAPI	PDQSigAddClass_Trap


//
// PDQTelFormatNumber uses these flags (pass in wFlags)
//
 
#define TFN_RAW			0			// Unformatted: XXXXXXXXXXX
#define TFN_FORMATTED	0x0001	// Format:  X(XXX) XXX-XXXX
#define TFN_DEFAULTAREA	0x0002	// Fully-qualified 11 digit number
#define TFN_ADDAREA		0x0004	// Add area to 7 digit US number
#define TFN_PREPEND		0x0008	// Prepend user preferred local/ld string
#define TFN_APPEND		0x0010	// Append user preferred local/ld string


#if defined(__cplusplus)
extern "C" {
#endif

// Callback for PDQSigEnumerate
typedef Boolean (*PFNENUMSIGNALS)(VoidPtr pContext, ULong uClass,ULong uSig,SigPriorityType priority,ULong uCreator, ULong uType);

// Function prototypes

_EXTERN void		PDQGetVersion(UInt refNum, PDQVerPtr pv ) PDQ_TRAP(PDQGetVersion_Trap);
_EXTERN Err			PDQCoreLibGetVersion (UInt refNum, DWordPtr dwVerP) PDQ_TRAP(PDQCoreLibGetVersion_Trap);
_EXTERN Err			PDQTelMakeCall(UInt refNum, CharPtr pszNumber)	PDQ_TRAP(PDQTelMakeCall_Trap);
_EXTERN Err			PDQTelEndCall(UInt refNum)	PDQ_TRAP(PDQTelEndCall_Trap);
_EXTERN Err			PDQTelAnswerCall(UInt refNum)	PDQ_TRAP(PDQTelAnswerCall_Trap);
_EXTERN Err			PDQTelGenerateDTMF(UInt refNum, Byte nKey, Boolean bLong)	PDQ_TRAP(PDQTelGenerateDTMF_Trap);
_EXTERN Err			PDQTelGetCallInfo(UInt refNum,CallInfoPtr pi)	PDQ_TRAP(PDQTelGetCallInfo_Trap);
_EXTERN Err			PDQTelResumeDialing(UInt refNum)	PDQ_TRAP(PDQTelResumeDialing_Trap);
_EXTERN Err			PDQTelCancelPause(UInt refNum)	PDQ_TRAP(PDQTelCancelPause_Trap);
_EXTERN CharPtr	PDQTelFormatNumber(UInt refNum, CharPtr pszSrc, CharPtr pszDest, Int nSize, Word wFlags) PDQ_TRAP(PDQTelFormatNumber_Trap);
_EXTERN Char		PDQTelGetDigit(UInt refNum, Char ch, Boolean * bValid)	PDQ_TRAP(PDQTelGetDigit_Trap);
_EXTERN Boolean 	PDQSigEnumerate(UInt refNum,ULong uClass,ULong uMask,CharPtr szName,VoidPtr pContext,PFNENUMSIGNALS pCB) PDQ_TRAP( PDQSigEnumerate_Trap );
_EXTERN Err 		PDQSigRegister(UInt refNum, ULong uClass, ULong uMask, SigPriorityType nPrio, ULong uCreator, ULong uType) PDQ_TRAP( PDQSigRegister_Trap );
_EXTERN Err 		PDQSigUnregister(UInt refNum, ULong uClass, ULong uMask, ULong uCreator,ULong uType) PDQ_TRAP(PDQSigUnregister_Trap);
_EXTERN Err 		PDQSignal(UInt refNum, ULong uClass, ULong uSig, SigParamType params) PDQ_TRAP(PDQSignal_Trap);
_EXTERN DWord 		PDQSigAddClass(UInt refNum, CharPtr pszClass) PDQ_TRAP(PDQSigAddClass_Trap);

#if defined(__cplusplus)
}
#endif


#endif 



