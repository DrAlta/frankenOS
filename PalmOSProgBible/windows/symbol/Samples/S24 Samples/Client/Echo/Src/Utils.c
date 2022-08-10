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
* FUNCTIONS:   GetObjectPtr		// Get a UI element contained in a Form.
*				       SetFieldText		// Set the text in a field.
*				       FreeFieldText	// Free the field's allocated text handle.
*				       cmdStart       // Service Start Button
*
* HISTORY:     3/2/97    SS   Created
*              10/22/98  dcat Modified for sockets
*              ...
*************************************************************************/
#include "Pilot.h"
#include <sys_socket.h>
#include "EchoRsc.h"

extern char msg[40];
extern char	*send_buf; // allocated in BuildData
extern char	*recv_buf;

/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    VoidPtr
 *
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 *
 ***********************************************************************/
static VoidPtr GetObjectPtr(Word objectID)
	{
	FormPtr frmP;

	frmP = FrmGetActiveForm();
	return (FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID)));
}


/***********************************************************************
 *
 * FUNCTION:    SetFieldText
 *
 * DESCRIPTION:Perform all necessary actions to set a Field control's
 *					text and redraw, if necesary.  Allocates a text handle.
 *              
 *
 * PARAMETERS: nFieldID(in) - The resource ID of the field.
 *					pSrcText(in) - The text to copy into field.
 *					nMaxSize(in) - Max size that the field can grow.
 *					bRedraw(in)  - Should the text be redrawn now? 
 *
 * RETURNED:   None
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *
 ***********************************************************************/
void SetFieldText( UInt nFieldID, const CharPtr pSrcText, Int nMaxSize, Boolean bRedraw )
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
		if( bRedraw )
			FldDrawField( pField );
	}
}

/***********************************************************************
 *
 * FUNCTION:    BuildData        
 *
 * DESCRIPTION: Called by cmdStart to allocate and fill the data buffer
 *
 * PARAMETERS:  how big do you want them to be  
 *
 * RETURNED:    0 if aok  1 if out of memory 
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *              
 ***********************************************************************/
int BuildData (int sendBytes)
{
  int   i, k;
  char  ch;
  
  send_buf = MemPtrNew(sendBytes);
  recv_buf = MemPtrNew(sendBytes);
  
  if ((send_buf == NULL) || (recv_buf == NULL))
  {
    return 1;
  }    

  //___generate the test data pattern
  i = 0;
  ch = (char) ('!' + i % 94);  // 33 to 126

  for (k = 0; k < sendBytes && (char)(ch + k) <= '~'; k ++)
    send_buf [k] = (char )(ch + k);

  i = k;
  for (; k < sendBytes; k ++)
    send_buf [k] = (char) ('!' + (k - i) % 94);
    
  return 0;
}

/***********************************************************************
 *
 * FUNCTION:   CheckForNetwork        
 *
 * DESCRIPTION:Called by cmdStart to open the NetLib    
 *
 * PARAMETERS: None  
 *
 * RETURNED:   0 if aok, else error code     
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/

int CheckForNetwork ( void )
{
 Word ifErrs;
 Err error; 

		error = SysLibFind( "Net.lib", &AppNetRefnum );
		if (error)
		{
			return error;
		}
		
    error = NetLibOpen( AppNetRefnum, &ifErrs );
    if ( error == netErrAlreadyOpen )
    {
      return 0;  // thats ok
    }

    if ( error || ifErrs )
    {
      NetLibClose( AppNetRefnum, false );
      return netErrNotOpen;
    }

    return 0;
}

/***********************************************************************
 *
 * FUNCTION:    cmdStart    
 *
 * DESCRIPTION: Called by MainFormHandleEvent   
 *
 * PARAMETERS:  Inputs main form then passes the parms to Echo
 *
 * RETURNED:      
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/
int cmdStart ( void)
{
	VoidHand	hFieldText;
	CharPtr		pFieldText;
	FieldPtr	pField;
	int 			sendBytes, delay,useTCP;
	ULong 		count;
	
	//___opent the NetLib interface
	count  = CheckForNetwork();  
	if (count)
	{
		SetFieldText (MainStatusField, "Can't open NetLib", 79, true);
		return 1;
	}  
	
	//___determine if this is Udp or Tcp
	useTCP =CtlGetValue (GetObjectPtr (MainModeCheckbox));

	//___read the delay from the main form	
	pField = GetObjectPtr (MainDelayField);
	if( !pField )
		return 3;
		
	hFieldText = FldGetTextHandle (pField);
	if (hFieldText)
	{		
		pFieldText = MemHandleLock (hFieldText);		
		MemHandleUnlock (hFieldText);
		delay = StrAToI (pFieldText);          
	}	
	
	//___read the size from the main form
	pField = GetObjectPtr (MainSizeField);
	if( !pField )
		return 4;

	hFieldText = FldGetTextHandle (pField);
	if (hFieldText)
	{		
		pFieldText = MemHandleLock (hFieldText);		
		MemHandleUnlock (hFieldText);
		sendBytes = StrAToI (pFieldText);          
	}		
	
	//___allocate the send and receive buffers		
	if (BuildData(sendBytes))
	{
		SetFieldText(MainStatusField,"Out of memory",79, true);
		return 5;
	}

	//___read the Host name from the main form
	pField = GetObjectPtr( MainHostIPField );
	if( !pField )
		return 6;
		
	hFieldText = FldGetTextHandle (pField);
	if( hFieldText )
	{		
		pFieldText = MemHandleLock(hFieldText);		
		MemHandleUnlock (hFieldText);
	}

	sprintf (msg, "%s=%d=%d", pFieldText, sendBytes, delay);		
	SetFieldText(MainStatusField,msg,79, true);	
	
	//__now call the CmdStevens modified routines...
	Echo (pFieldText, sendBytes, delay, useTCP);
		
	return 0;	

}

