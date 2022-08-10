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


#ifndef __PDQREGISTRY_H__
#define __PDQREGISTRY_H__

#ifdef _PDQREGISTRY_EXPORT
#define _REGEXTERN	extern
#define PDQREG_TRAP(trapNum)
#else
#define _REGEXTERN 
#define PDQREG_TRAP(trapNum) SYS_TRAP(trapNum)
#endif

// *** Internal library name which can be passed to SysLibFind() ***

#define  PDQRegistryLibName	"Registry Library"


// Launch code sent to applications to tell them to process a URL.  

#define oemAppLaunchCmdURL  		(4000)

// Capability Flags

#define REGCAP_LAUNCHURL		0x0001	// switch to application
#define REGCAP_SUBLAUNCH		0x0002	// launch app as subroutine call

// Enumeration callbacks...

typedef Boolean (*PFNREGENUM)(VoidPtr pUser, CharPtr pszScheme, CharPtr pszName, CharPtr pszShortName, ULong uCreator, ULong dwFlags, Boolean bEnabled);
typedef Boolean (*PFNMACROENUM)(VoidPtr pUser, CharPtr pszURL, CharPtr pszShortName, CharPtr pszDesc);

// *** Trap enumeration ***

typedef enum {
	PDQRegGetVersion_Trap = sysLibTrapCustom,
	PDQRegAddScheme_Trap,
	PDQRegRemoveScheme_Trap,
	PDQRegEnumSchemes_Trap,
	PDQRegSetHandlerName_Trap,
	PDQRegEnableScheme_Trap,
	PDQRegGetSchemeHandler_Trap,
	PDQRegProcessURL_Trap,
	PDQRegProcessMailAddress_Trap,
	PDQRegAddMacro_Trap,
	PDQRegRemoveMacro_Trap,
	PDQRegEnumMacros_Trap
} PDQRegTrapNumberEnum;

#define PDQRegLastPublicAPI PDQRegEnumMacros_Trap

// *** Function Prototypes ***

#ifdef __cplusplus
extern "C" {
#endif

_REGEXTERN Err		PDQRegGetVersion(UInt refNum, DWordPtr dwVerP) PDQREG_TRAP(PDQRegGetVersion_Trap);
_REGEXTERN Err		PDQRegAddScheme(UInt refNum, CharPtr pszScheme, ULong uCreator, ULong uCapFlags) PDQREG_TRAP(PDQRegAddScheme_Trap);
_REGEXTERN Err		PDQRegRemoveScheme(UInt refNum, CharPtr pszScheme, ULong uCreator) PDQREG_TRAP(PDQRegRemoveScheme_Trap);
_REGEXTERN Err		PDQRegSetHandlerName(UInt refNum, CharPtr pszScheme, CharPtr pszName, CharPtr pszShortName) PDQREG_TRAP(PDQRegSetHandlerName_Trap);
_REGEXTERN Err		PDQRegEnableScheme(UInt refNum,CharPtr pszScheme, ULong uCreator, Boolean bEnable) PDQREG_TRAP(PDQRegEnableScheme_Trap);
_REGEXTERN Err		PDQRegEnumSchemes(UInt refNum, PFNREGENUM pfn, VoidPtr pUser) PDQREG_TRAP(PDQRegEnumSchemes_Trap);
_REGEXTERN ULong	PDQRegGetSchemeHandler(UInt refNum, CharPtr pszScheme) PDQREG_TRAP(PDQRegGetSchemeHandler_Trap);
_REGEXTERN Err		PDQRegProcessURL(UInt refNum, CharPtr pszURL) PDQREG_TRAP(PDQRegProcessURL_Trap);
_REGEXTERN Err		PDQRegProcessMailAddress(UInt refNum, CharPtr pszFirstName, CharPtr pszLastName, CharPtr pszAddress) PDQREG_TRAP(PDQRegProcessMailAddress_Trap);
_REGEXTERN Err		PDQRegAddMacro(UInt refNum, CharPtr pszURL, CharPtr pszShortName, CharPtr pszDesc) PDQREG_TRAP(PDQRegAddMacro_Trap);
_REGEXTERN Err		PDQRegRemoveMacro(UInt refNum, CharPtr pszURL) PDQREG_TRAP(PDQRegRemoveMacro_Trap);
_REGEXTERN Err		PDQRegEnumMacros(UInt refNum, PFNMACROENUM pfn, VoidPtr pUser) PDQREG_TRAP(PDQRegEnumMacros_Trap);

#ifdef __cplusplus 
}
#endif

#endif
