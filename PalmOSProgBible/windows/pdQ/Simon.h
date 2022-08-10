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

#ifndef __SIMON_H__
#define __SIMON_H__

// --------------------------------------------------
DWord 				PilotMain(
							Word	cmd,
							Ptr	cmdPBP,
							Word	launchFlags);

static DWord 		SimonPilotMain(
							Word	cmd,
							Ptr	cmdPBP,
							Word	launchFlags);

// --------------------------------------------------
static Err 			RomVersionCompatible(
							DWord	requiredVersion,
							Word	launchFlags);
						
static VoidPtr		GetObjectPtr(
							Word	objectID);

// --------------------------------------------------
static Boolean		AppHandleEvent(
							EventPtr	eventP);

static void			AppEventLoop();

static Err			AppStart();

static void			AppStop();		

// --------------------------------------------------
static void			MainFormInit(
							FormPtr		frmP);

static Boolean		MainFormDoCommand(
							Word		command);

static Boolean		MainFormHandleEvent(
							EventPtr	eventP);
						
static Boolean		MainFormHandleButtonTap(
							Word	iButton);
						
static void			MainFormNewGame();

static void			MainFormGameOver(
							Boolean		isLoser);

static void			MainFormHandleIdle();

static void			MainFormNoSound(
							int	milliseconds);						

// --------------------------------------------------
static void			pdQAlert_Vibrate(
							Boolean	inVibOn);

static void			pdQAlert_SetLED(
							int	inLEDValue);

#endif // __SIMON_H__




