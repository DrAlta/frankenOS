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


#ifndef __PDQALERT_H__
#define __PDQALERT_H__

// *** Internal library name which can be passed to SysLibFind() ***

#define	PDQAlertLibName	"Alert Library"		


// *** Error codes returned by PDQAlert functions ***

#define PDQAlertErrParam			(appErrorClass | 1)	// invalid parameter
#define PDQAlertErrNotOpen			(appErrorClass | 2)	// library is not open
#define PDQAlertErrStillOpen		(appErrorClass | 3)	// returned from 
																		// PDQAlert_Close if the 
																		// library is still open by 
																		// others
#define PDQAlertErrMemory			(appErrorClass | 4)	// memory error occurred
#define PDQAlertNotImplemented	(appErrorClass | 5)	// feature not implemented


// *** Enumeration used when calling PDQAlertSwitch/PDQAlertFlash functions ***
typedef enum {
	LED_RED = 0,
	LED_ORANGE,
	LED_GREEN,
	VIBRATOR,	// not supported in version 1.0
	BACKLIGHT	
} AlertDeviceType;

// *** Enumeration used when calling PDQAlert*Switch functions ***
typedef enum {

	ALERT_SWITCH_OFF = 0,
	ALERT_SWITCH_ON,
	ALERT_REVERSE_STATE,
	ALERT_GET_STATE
	
} AlertSwitchType;


// *** Trap enumeration ***
typedef enum {
	PDQAlertGetLibAPIVersion_Trap = sysLibTrapCustom,
	
	PDQAlertSwitch_Trap,
	PDQAlertFlash_Trap
} PDQAlertTrapNumberEnum;

#define PDQAlertLastPublicAPI PDQAlertFlash_Trap



// *** Function prototypes ***

#ifdef __cplusplus
extern "C" {
#endif

Err PDQAlertGetLibAPIVersion(UInt refNum, DWordPtr dwVerP)
	SYS_TRAP(PDQAlertGetLibAPIVersion_Trap);
 
Err PDQAlertSwitch(UInt refNum, AlertDeviceType type, AlertSwitchType turn, Boolean *prevStateP)
	SYS_TRAP(PDQAlertSwitch_Trap);

Err PDQAlertFlash(UInt refNum, AlertDeviceType type, UInt numberOfFlashes, UInt onDuration,
					UInt offDuration, UInt periodOfSilence, UInt timeout)
	SYS_TRAP(PDQAlertFlash_Trap);

#ifdef __cplusplus 
}
#endif

#endif	// __PDQALERT_H__



