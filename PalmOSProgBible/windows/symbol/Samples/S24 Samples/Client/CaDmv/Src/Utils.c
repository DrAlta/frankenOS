/************************************************************************
* COPYRIGHT:   Copyright  ©  1999, 2000, & 2001 Symbol Technologies, Inc. 
*
* FILE:        Utils.c
*
* SYSTEM:      Symbol barcode scanner for Palm III.
*
* MODULE:      Utility functions.
*
* DESCRIPTION: Commonly used utility functions.
*
* HISTORY: 19981105	dcat Created from ReceiveDemo and NetSample.
* 19990601 dcat First code cut for release. The conditional compile switch
*               is defined in debug.h.
*************************************************************************/
#include "Pilot.h"

//___our application stuff
#include "CaDmvRsc.h"		
#include "CaDmv.h"
#include "NdkLib.h"
#include "MsrMgrLib.h"
#include <sys_socket.h>

//___declared in CaDmv.c___
extern char msg[];
extern int GotMilk;
extern SocketType SocketParms;
extern DmvType		DmvRecord;
extern struct sockaddr_in srv_addr;
extern struct sockaddr_in cli_addr;	

extern VoidPtr theMagStripP;
extern VoidPtr theImageP;

/***********************************************************************
 *
 * FUNCTION:   RefreshNetwork        
 *
 * DESCRIPTION: Called by Echo to make sure we're still up and running.
 * Check the status of the connection and re-establish it (if it's down),
 * if recconnectIfDown is true. returns true if connected when this
 * routine returns, false else.
 *
 * This handles the case where the user powers the unit off/on and expects
 * the socket to maintain its connection.
 *
 * Don't call this unless tcp_open() has been called.
 *
 * PARAMETERS: pointer to our socket data structure   
 *
 * RETURNED:        
 *             
 * REVISION HISTORY:
 *			Name	Date		    Description
 *			----	----		    -----------
 *      dcat  05/01/1999 Initial Revision  
 ***********************************************************************/
Boolean RefreshNetwork (SocketTypeP SocketParmsP)
{
	Word	ifErrs		= 0;
	Err		sysError	= 0;
	Boolean	allUp		= false;
	Boolean	isConn		= false;

	// check the status of the connection, and bring it up if 
	// reconnectIfDown is true. allUp will contain the
	// connection status.
	sysError = NetLibConnectionRefresh ( SocketParmsP->NetLibRefNum, true,
										&allUp, &ifErrs );
													
	if (sysError)
	{
		FrmCustomAlert (GeneralAlert, "Refresh Network Failed", NULL, NULL );
	}
	else if (ifErrs)
	{
		sprintf (msg, "Refresh Network Err: x%x",ifErrs);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
	}
	else
		isConn = allUp;
	
	return isConn;
			
}	

/***********************************************************************
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  objectID (in) - The resource ID of the control to get.
 *
 * RETURNED:    Pointer to the control.
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
VoidPtr GetObjectPtr (Word objectID)
{
	FormPtr frm;
	
	frm = FrmGetActiveForm ();
	if( frm )
		return (FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, objectID)));
	else
		return NULL;
}

/***********************************************************************
 * FUNCTION:    FreeFieldHandle
 *
 * DESCRIPTION: Free text handle for field
 *
 * PARAMETERS:  field id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
void FreeFieldHandle( int ID )
{
	FieldPtr pField;
	VoidHand handle;
	
	pField = GetObjectPtr( ID );
	handle = FldGetTextHandle( pField );
	if (handle)
	{
		FldSetTextHandle( pField, 0 );
		MemHandleFree( handle );
	}
}

/***********************************************************************
 * FUNCTION:    SetFieldText
 *
 * DESCRIPTION:Perform all necessary actions to set a Field control's
 *					   text and redraw, if necesary.  Allocates a text handle.
 *
 * PARAMETERS: nFieldID(in) - The resource ID of the field.
 *					   pSrcText(in) - The text to copy into field.
 *					   nMaxSize(in) - Max size that the field can grow.
 *
 * RETURNED:   None
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
void SetFieldText( UInt nFieldID, const CharPtr pSrcText, Int nMaxSize)
{
	VoidHand hFieldText;
	CharPtr  pFieldText;
	FieldPtr pField;

	pField = GetObjectPtr( nFieldID );
	if( !pField )
		return;
		
	hFieldText = FldGetTextHandle( pField );
	if( !hFieldText )
		hFieldText = MemHandleNew( nMaxSize );

	// If already allocated, make sure it can handle nMaxSize already.
	// If not, realloc that buffer
	else
	{
		Err	err = 0;
		ULong curSize = MemHandleSize( hFieldText );
		if( curSize < nMaxSize )
			err = MemHandleResize(hFieldText, nMaxSize ) ;
		
		ErrNonFatalDisplayIf(err, "Unable to resize field!");
		
	}

	if( hFieldText )
	{
		int len = StrLen(pSrcText);
		
		pFieldText = MemHandleLock( hFieldText );
		
		if (len > nMaxSize)
		{
			StrNCopy( pFieldText, pSrcText, nMaxSize-1);
			pFieldText[nMaxSize-1] = '\0';
		}
		else
			StrCopy( pFieldText, pSrcText );

		MemHandleUnlock( hFieldText );

		FldSetTextHandle( pField, hFieldText );
		FldSetTextAllocatedSize( pField, nMaxSize );
		
		FldSetMaxChars( pField, nMaxSize-1);

		FldRecalculateField( pField, true );
		
		FldDrawField( pField );
	}
}



/***********************************************************************
 * FUNCTION:    GetSocket-Called by MainFormHandleEvent
 *
 * DESCRIPTION: We set up our socket here 
 *
 * PARAMETERS:  none 
 *
 * RETURNED:    0 if AOK else error number    
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
int GetSocket (void)
{	
	int	status = 0;
	char	myHost [16] = "157.235.93.2";	
	int	myPort = 1968;		

	if (SocketParms.mySocket) 	// already been here, return with grace
		return 0;
	
	//___clear our primary data structure and initialize
	MemSet ((char*)&SocketParms, sizeof(SocketParms), 0);		
	SocketParms.NetLibTimeOut = (2 * sysTicksPerSecond);
	SocketParms.TCPResendTimeout = 3000;	
	SocketParms.hostIP = &myHost[0];	
	SocketParms.port  = myPort;
	SocketParms.srv_addrP = &srv_addr;
	SocketParms.cli_addrP = &cli_addr;

	status = NdkLibGetSocket (&SocketParms, 0); // 0=TCP, 1=UDP	
	if (status < 0)	
	{
		NdkLibCloseSocket(&SocketParms);
		return 1;
	}
	
	SocketParms.mySocket = status;	//__the socket number from the stack	
	
	return 0;
	
}


/***********************************************************************
 *
 * FUNCTION:    DriverLookUp-called by MainFormHandleEvent
 *
 * DESCRIPTION: Send the MagStripe data to the host and read back the
 * image associated with this driver. 
 *
 * PARAMETERS:  pointer to our socket data structure
 *
 * RETURNED:    0 if all is well, else an error code
 *             
 * REVISION HISTORY:; *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
int DriverLookUp (SocketTypeP SocketParmsP)
{
	int			status = 0, loop = 0, bytesRcvd = 0;
	char		*host_err_str(), msgstr[100];
	Word		ifErrs		= 0;
	Boolean bAssoc, allUp;

	// If the user powers the unit down & up, or the unit suspends, the 
	// radio will be unassociated.  We need the following code to handle
	// this case.
retry:
	status =  S24GetAssociationStatus (&bAssoc); // are we associated?
	if (status) // we have a problem...
	{
		StrPrintF (msg,"S24GetAssociationStatus error: %d", status);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
	}
	
	if (!bAssoc)
	{
		StrPrintF (msg,"UnAssociated: %d", loop++);
		SetFieldText (MainStatusField, msg, 39);
		if (loop >10)
			return 1;
				
		status = NetLibConnectionRefresh (SocketParmsP->NetLibRefNum, true,	&allUp, &ifErrs );
		if (status) 
		{
			SysErrString(SocketParmsP->errNo, msgstr, 100);
			StrPrintF (msg,"NetLibConnectionRefresh error: %d, %s", status, msgstr);
			FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
			return 2;
		}
		goto retry;	
	}	
			
		
	//___set our socket to point to the MagStripe data 	
	SocketParms.data_size = MAX_CARD_DATA+2;
	SocketParms.data_buffP = theMagStripP;	

	status = NdkLibWriteSocket (&SocketParms);
	if (status) // we have a problem...
	{
		SysErrString(SocketParmsP->errNo, msgstr, 100);
		StrPrintF (msg,"WriteSocket error: %d, %s", status, msgstr);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
		return 1;
	}

	//___point the socket to our image receive buffer
	SocketParms.data_size = ImageSize;
	SocketParms.data_buffP = theImageP;
		  	
	status = NdkLibReadSocket (&SocketParms);
	//___Not all 160x150x2 images are the same length yet. Ignore the
	//   timeout from the above read until we understand it better.	
	bytesRcvd = SocketParms.data_size;
	if(bytesRcvd != 3016) // we have an incomplete image
		GotMilk = false;
	else
		GotMilk = true; 

	return 0;
	
}

/***********************************************************************
 *
 * FUNCTION:    StrMid-Called from ParseIt		
 *
 * DESCRIPTION: 	
 *
 * PARAMETERS:		
 *
 * RETURNED:		
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
void StrMid ( char *input,  char *out, char start, char length )
{  
  char * strptr;
   
  strptr = input + (long)start;
  
  StrNCopy  (out, strptr, length);
  
  out [length] = '\0';
}
  
/***********************************************************************
 *
 * FUNCTION:	ParseIt-Called from ParseDisplay	
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:		
 *
 * RETURNED:		
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
void ParseIt ( char *input,  char *out )
{  
  
  char * commaPtr;
  int Len1, Len2;   

  commaPtr = StrChr (input, ','); 
   
  Len1 = commaPtr - input;		// how big is this field
  
  StrNCopy (out, input, Len1);  
  out [Len1] = '\0';
  
  //___now we need to strip off the field we just parsed
  Len2 = StrLen(theMagStripP) - Len1;	// how much is left
  
  StrMid (input, input, Len1+1, Len2);
} 
 
/***********************************************************************
 *
 * FUNCTION:    ParseDisplay-Called from MainFormHandleEvent		
 *
 * DESCRIPTION: Inputs MSRbuff and formats the display data	
 *
 * PARAMETERS:  none		
 *
 * RETURNED:	 0 if all is well, else an error code	
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   05/01/1999 Initial Release
 ***********************************************************************/
//___At this point all the data is in the MSRbuff, comma delimited
int ParseDisplay (void)
{
	
#ifdef Debug	// we fake out the input from the MSR unit, here's what we expect to see from the MSR unit
StrCopy (theMagStripP, "N1234567,Hanover Phist,Name2 goes here,Address1,Address2.,San Jose,CA,M,BR,BL,510,180,"); 
#endif 

	//___ParseIt will consume the comma delimintated text in MSRbuff
	//   so we need to save it now for sending to host later
	StrCopy (theImageP, theMagStripP);
	
	MemSet ((char*)&DmvRecord, sizeof(DmvRecord), 0);
	ParseIt (theMagStripP, DmvRecord.LicId);
	ParseIt (theMagStripP, DmvRecord.Name1);
	ParseIt (theMagStripP, DmvRecord.Name2);
	
	ParseIt (theMagStripP, DmvRecord.Addr1);
	ParseIt (theMagStripP, DmvRecord.Addr2);	
	ParseIt (theMagStripP, DmvRecord.City);
	ParseIt (theMagStripP, DmvRecord.State);
			
	ParseIt (theMagStripP, DmvRecord.Sex);
	ParseIt (theMagStripP, DmvRecord.Hair);
	ParseIt (theMagStripP, DmvRecord.Eyes);
	ParseIt (theMagStripP, DmvRecord.Hght);
	ParseIt (theMagStripP, DmvRecord.Wght);
	
	SetFieldText (MainStatusField, DmvRecord.LicId, 39);
	SetFieldText (MainName1Field,  DmvRecord.Name1, 39);
	SetFieldText (MainName2Field,  DmvRecord.Name2, 39);
	SetFieldText (MainAddr1Field,  DmvRecord.Addr1, 39);
	SetFieldText (MainAddr2Field,  DmvRecord.Addr2, 39);
	SetFieldText (MainCityField,   DmvRecord.City,  39);
	SetFieldText (MainStateField,  DmvRecord.State, 39);
	
	SetFieldText (MainSexField,    DmvRecord.Sex,   39);
	SetFieldText (MainHairField,   DmvRecord.Hair,  39);
	SetFieldText (MainEyesField,   DmvRecord.Eyes,  39);
	SetFieldText (MainHieghtField, DmvRecord.Hght,  39);	
	SetFieldText (MainWieghtField, DmvRecord.Wght,  39);
	
	//___now restore teh MagStripe data from the image area
	StrCopy (theMagStripP, theImageP);
	return 0;
	
}

