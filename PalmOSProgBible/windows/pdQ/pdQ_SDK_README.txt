===========================================
	pdQ Software Development Kit
	version 1.0 
	April 8, 1999
===========================================

=========================================================================================
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
=========================================================================================


==================================
	Information
==================================

You can keep abreast of pdQ developer information by becoming a regular visitor to 
the pdQ Developer Zone web site at <http://www.qualcomm.com/pdQ/developer.html>. 
You may also be interested in these other sources of information about the pdQ smartphone: 

pdQdevtalk@qualcomm.com

	Our forum for developers of applications for the pdQ. To subscribe, 
	send e-mail to majordomo@qualcomm.com with the following in the message body: 
	subscribe pdQdevtalk@qualcomm.com <your email address> 

pdQdevnews@qualcomm.com

	Our announcement mailing list for the latest developer news and information regarding 
	the pdQ smartphone. To subscribe, send e-mail to majordomo@qualcomm.com with the 
	following in the message body: subscribe pdQdevnews@qualcomm.com <your email address> 

pdQdeveloper@qualcomm.com

	Please feel free to contact us at pdQdeveloper@qualcomm.com with any questions or 
	suggestions for how we may improve developer services. 


==================================
	Contents
==================================

This SDK contains the following:

	pdQ_SDK_README.txt 	- This file
	pdQ_API_Manual.pdf	- API manual
	Samples			- Folder containing sample Metrowerks projects
		SampleDialer	- Sample code for a simple phone dialer.
		SimonSez	- Sample code for a game using Alert library.
	Includes		- Folder containing pdQ includes
		pdQCore.h	- Core header (#include for signal and telephony APIs)
		pdQRegistry.h	- Registry header (#include for all registry APIs)
		pdQAlert.h	- Alert header (#include for all alert APIs)
		
In order to begin developing, you should:

	1) Read the pdQ_API_Manual.
	2) Include the necessary header file(s) in your source (pdQCore.h, pdQRegistry.h,pdQAlert.h)
	3) Call the pdQ APIs (See sample code for examples).
	4) Compile your prc and hotsync onto pdQ. 

		
==================================
	Revision history
==================================

1/13/99 1.0 beta version released.
4/8/99	1.0 version released.
	Added Appendix C- tel: URL 
	Added Appendix D- pdQ Specifications for Developers (mailto: and http: URLs)