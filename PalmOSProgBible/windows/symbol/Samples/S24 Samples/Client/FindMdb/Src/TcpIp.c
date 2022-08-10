/***********************************************************************
 *
 *	Copyright (c) Palm Computing 1997 -- All Rights Reserved 
 *
 * PROJECT:  Pilot Sample Internet Application
 * FILE:     Tcpip.c
 *
 * DESCRIPTION:
 * A bunch of Sockets API test routines obtained mostly from Richard
 * Steven's books. Modified to use only the Tcpip socket stuff.  This 
 * file was is a modified CmdStevens.c from the unsupported NetSample
 * project on the CodeWarrior CD.
 * 
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *         
 **********************************************************************/
#include "Pilot.h"
// Include these for the EvtResetAutoOff() call.
#include <SysAll.h>
#include <SysEvtMgr.h>
#include <sys_socket.h>
#include "S24API.h"
#include "FindMdbRsc.h"
#include "Utils.h"

extern UInt TimeOut;
extern int mySocket;
extern char msg[];
extern char recv_buf[];
struct sockaddr_in	tcp_srv_addr;	/* server's Internet socket addr */
struct servent		tcp_serv_info;	/* from getservbyname() */
struct hostent		tcp_host_info;	/* from gethostbyname() */

Word AppNetRefnum;
long AppNetTimeout = (10 * sysTicksPerSecond);

Err h_errno;
Err errno;

NetHostInfoBufType	AppHostInfo;
NetServInfoBufType	AppServInfo;


/***********************************************************************
 *
 * FUNCTION:   RefreshNetwork        
 *
 * DESCRIPTION: Called by Echo to make sure were still up
 * Check the status of the connection and reestablish it (if it is down),
 * if recconnectIfDown is true. returns true if connected when this
 * routine returns, false else.
 *
 * Don't call this unless tcp_open() has been called.
 *
 * PARAMETERS:   
 *
 * RETURNED:        
 *             
 * REVISION HISTORY:
 *			Name	Date		    Description
 *			----	----		    -----------
 *      dcat  06/14/1999 Initial Revision  
 ***********************************************************************/
Boolean RefreshNetwork (Boolean reconnectIfDown)
{

	Word		ifErrs		= 0;
	Err			sysError	= 0;
	Boolean	allUp			= false;
	Boolean	isConn		= false;

	// check the status of the connection, and bring it up if 
	// reconnectIfDown is true. allUp will contain the
	// connection status.
	sysError = NetLibConnectionRefresh ( AppNetRefnum, reconnectIfDown,
										&allUp, &ifErrs );
													
	if ( sysError != 0 )
	{
		FrmCustomAlert (GeneralAlert, "Refresh Network Failed", NULL, NULL );
	}
	else if ( ifErrs != 0 )
	{
		sprintf (msg, "Refresh Network Err: x%x",ifErrs);
		FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
	}
	else
		isConn = allUp;
	
	return isConn;
			
}	

/***********************************************************************
 *
 * FUNCTION:   CheckForNetwork        
 *
 * DESCRIPTION:Called by MainFormOnInit to check for and open NetLib    
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

int CheckForNetwork (void)
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
 * FUNCTION:host_err_str        
 *
 * DESCRIPTION: Called by tcp_open & udp_open    
 *
 * PARAMETERS:  
 * Return a string containing some additional information after a
 * host name or address lookup error - gethostbyname() or gethostbyaddr().
 *
 * RETURNED: text msg associated with h_errno
 *     
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/
static char * host_err_str()
{
	static char	msgstr[100];

	if (h_errno != 0) 
		SysErrString(h_errno, msgstr, 100);
	else 
		msgstr[0] = '\0';

	return(msgstr);
}


/***********************************************************************
 *
 * FUNCTION:       tcp_open        
 *
 * DESCRIPTION:    called by Connect    
 *
 * PARAMETERS:
 * char	*host;		 name or dotted-decimal addr of other system
 * char	*service;	 name of service being requested 
 *					       can be NULL, iff port > 0 
 * int	port;			 if == 0, nothing special - use port# of service 
 *                 if < 0, bind a local reserved port 
 *                 if > 0, it's the port# of server (host-byte-order)
 *
 * RETURNED:       return socket descriptor if OK, else -1 on error     
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/
static int tcp_open(host, service, port)
char	*host;		
char	*service;	
int	port;		
{
	int		fd, resvport;
	unsigned long	inaddr;
	char		*host_err_str();
	struct servent	*sp;
	struct hostent	*hp;

	/*
	 * Initialize the server's Internet address structure.
	 * We'll store the actual 4-byte Internet address and the
	 * 2-byte port# below.
	 */
	SetFieldText(MainStatusField, "At start of tcp_open", 39, true);
	
	bzero((char *) &tcp_srv_addr, sizeof(tcp_srv_addr));
	tcp_srv_addr.sin_family = AF_INET;

	if (service != NULL) 
	{
		if ((sp = getservbyname(service, "tcp")) == NULL) 
		{
			//sprintf(msg, "tcp_open: unknown service: %s/tcp", service);
			//SetFieldText(MainStatusField,msg,39, true);
			return(-1);
		}	
			
		tcp_serv_info = *sp;			/* structure copy */
		
		if (port > 0)							/* caller's value */
			tcp_srv_addr.sin_port = htons(port);							
		else									/* service's value */
			tcp_srv_addr.sin_port = sp->s_port;
							
	} 
	else 
	{
		if (port <= 0) 
		{
			//SetFieldText(MainStatusField,"tcp_open: must specify either service or port",39, true);
			return(-2);
		}
		tcp_srv_addr.sin_port = htons(port);
	}

	/*
	 * First try to convert the host name as a dotted-decimal number.
	 * Only if that fails do we call gethostbyname().
	 */
	 
	sprintf (msg,"Look up %s", host);
	SetFieldText(MainStatusField, msg, 39, true);
	 
	if ((inaddr = inet_addr(host)) != INADDR_NONE) /* it's dotted-decimal */
	{
		bcopy((char *) &inaddr, (char *) &tcp_srv_addr.sin_addr,sizeof(inaddr));
		tcp_host_info.h_name = NULL;
	} 
	else /* its a name */
	{
		if ((hp = gethostbyname(host)) == NULL) 
		{
			//sprintf (msg, "tcp_open: host name error: %s %s",host, host_err_str());
			//SetFieldText(MainStatusField,msg,39, true);
			return(-3);
		}
		tcp_host_info = *hp;	/* found it by name, structure copy */
		bcopy(hp->h_addr, (char *) &tcp_srv_addr.sin_addr,hp->h_length);
	}

	if (port >= 0) 
	{
		if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		{
			//SetFieldText(MainStatusField,"tcp_open: can't create TCP socket",39, true);
			return(-4);
		}
	} 
	else if (port < 0) 
	{
		resvport = IPPORT_RESERVED - 1;
		if ((fd = -1 /*rresvport(&resvport)*/) < 0) 
		{
			//SetFieldText(MainStatusField,"tcp_open: can't get a reserved TCP port",39, true);
			return(-5);
		}
	}

	/*
	 * Connect to the server.
	 */

	if (connect(fd, (struct sockaddr *) &tcp_srv_addr, sizeof(tcp_srv_addr)) < 0) 
	{
		//SetFieldText(MainStatusField,"tcp_open: can't connect to server",39, true);
		close(fd);
		return(-6);
	}
	
	SetFieldText(MainStatusField, "Connect complete...", 39, true);
	return(fd);	/* all OK */
}



/***********************************************************************
 *
 * FUNCTION:    Connect    
 *
 * DESCRIPTION: called by MainFormOnInit()
 *
 * PARAMETERS:  pointer to host name string and port to open
 *
 * RETURNED:    socket descriptor
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/
int Connect (char *host, int portNum)
{
	int			fd=-1;
	CharPtr	serviceNameP = 0;
	char *host_err_str();
	
 	sprintf (msg,"Connect %s_%d", host, portNum);
	SetFieldText (MainStatusField,msg, 39, true);
	
	//__get a socket now and connect to the host
	if ((fd = tcp_open(host, serviceNameP, portNum)) < 0) 
	{
		sprintf (msg,"tcp_open error: %d, %s", fd, host_err_str());
		SetFieldText (MainStatusField,msg, 39, true);
		return fd;
	}
	
	TimeOut = SysSetAutoOffTime(0);          // Make sure we don't turn off
	
	sprintf (msg,"socket: %d, host: %s", fd, host);
	SetFieldText (MainStatusField,msg, 39, true);

	return fd;
}
/***********************************************************************
 *
 * FUNCTION:    DisConnect    
 *
 * DESCRIPTION: closes our socket
 *
 * PARAMETERS:  global mySocket
 *
 * RETURNED:    none
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/
void DisConnect (void)
{
	if (mySocket)
	{
		close (mySocket);
		mySocket = 0;
	}
	
	if (AppNetRefnum)
	{
		NetLibClose (AppNetRefnum, true);
	}
	
	if (TimeOut)
	{
		SysSetAutoOffTime(TimeOut);
	}
	
}
/***********************************************************************
 *
 * FUNCTION:    ReadN    
 *
 * DESCRIPTION: called by LookUp() to read "n" bytes from a descriptor.
 * fd______a socket descriptor  
 * ptr_____pointer to store the data received
 * nbytes__how much were expecting to receive
 *
 * RETURNED: error from read() or bytes actually read  
 *   
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/
static int ReadN (register int fd, register Byte *ptr, register int nbytes)
{
	int	nleft, nread;

	nleft = nbytes;
	while (nleft > 0) 
	{
		nread = read (fd, ptr, nleft);
		if (nread < 0)
			return(nread);		/* error, return < 0 */
		else if (nread == 0)
			break;			/* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return (nbytes - nleft);		/* return >= 0 */
}

/***********************************************************************
 *
 * FUNCTION:    WriteN             
 *
 * DESCRIPTION: called by LookUp() to write "n" bytes to a descriptor.    
 *              
 * PARAMETERS:
 * fd______a socket descriptor  
 * ptr_____pointer to data to be written
 * nbytes__how much to write
 *
 * RETURNED: error from write() or bytes actually written   
 *  
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/
static int WriteN (register int fd, register Byte *ptr, register int nbytes)
{
	int	nleft, nwritten;

	nleft = nbytes;
	while (nleft > 0) 
	{
		nwritten = write (fd, ptr, nleft);
		if (nwritten <= 0)
			return(nwritten);		/* error */

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return (nbytes - nleft);
}

/***********************************************************************
 *
 * FUNCTION:    LookUp   called by MainFormHandleEvent
 *
 * DESCRIPTION: Passes the barcode to the host program and reads back
 * the response from the host.  Display the response to the form.
 *
 * PARAMETERS:  pointer to barcode string to lookup 
 *
 * RETURNED:    0 if all is fine else 
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/
int LookUp (char *label)
{
	int	sentBytes, sendBytes, rcvBytes, ExpectedBytes, status;
	char *host_err_str();
	Word		ifErrs = 0;
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
			
		status = NetLibConnectionRefresh (AppNetRefnum, true,	&allUp, &ifErrs );
		if (status) 
		{
			StrPrintF (msg,"NetLibConnectionRefresh error: %d", status);
			FrmCustomAlert (GeneralAlert, msg, NULL, NULL );
			return 2;
		}
		goto retry;	
	}
	
	//___write the data to the host
	sendBytes = strlen (label);		
	RefreshNetwork (true);
	sentBytes = WriteN (mySocket, (BytePtr)label, sendBytes);
	if (sentBytes != sendBytes) 
	{
		sprintf (msg, "Write_err: %d, %s", sentBytes, host_err_str());
		SetFieldText(MainStatusField, msg, 39, true);
		return 3;
	}		
	SetFieldText (MainStatusField, "Send Complete...", 39, true);	
  
	//___read available data from host 3 lines of 40
	ExpectedBytes = 120;
	rcvBytes = ReadN (mySocket, (BytePtr) recv_buf, ExpectedBytes);	
	if (rcvBytes != ExpectedBytes) 
	{
		sprintf (msg, "Read_err: %d, %s", rcvBytes, host_err_str());
		SetFieldText (MainStatusField, msg, 39, true);
		return 4;
	}	

	//___update the form with the data from the host
	StrNCopy (msg, &recv_buf[0], 39);
	msg[39] = '\0';
	SetFieldText (MainTitleField, msg, 39, true);
	
	StrNCopy (msg, &recv_buf[40], 39);
	msg[39] = '\0';
	SetFieldText (MainDescField, msg, 39, true);
	
	StrNCopy (msg, &recv_buf[80], 39);
	msg[39] = '\0';
	SetFieldText (MainISBNField, msg, 39, true);
	
  SetFieldText (MainStatusField, "LookUp Complete...", 39, true);	
  	
	return 0;
}
















