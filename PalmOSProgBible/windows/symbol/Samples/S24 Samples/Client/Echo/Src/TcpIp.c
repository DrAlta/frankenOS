/***********************************************************************
 *
 *	Copyright (c) Palm Computing 1997 -- All Rights Reserved 
 *
 * PROJECT:  Pilot Sample Internet Application
 * FILE:     TcpIp.c
 *
 * DESCRIPTION:
 *	
 * DESCRIPTION:
 *	A bunch of Sockets API test routines obtained mostly from Richard
 *	Steven's books. 
 *
 * 981024 dcat Modified to use only the echo command and supporting
 *             code. 
 **********************************************************************/
#include "Pilot.h"

// Include these for the EvtResetAutoOff() call.
#include <SysAll.h>
#include <SoundMgr.h>

#include <SysEvtMgr.h>
#include <sys_socket.h>
#include "EchoRsc.h"

extern int fd;
extern char msg[40];
extern char * send_buf;
extern char * recv_buf;
extern UInt TimeOut;

struct sockaddr_in	udp_srv_addr;	/* server's Internet socket addr */
struct sockaddr_in	udp_cli_addr;	/* client's Internet socket addr */
struct servent		udp_serv_info;	/* from getservbyname() */
struct hostent		udp_host_info;	/* from gethostbyname() */
struct sockaddr_in	tcp_srv_addr;	/* server's Internet socket addr */
struct servent		tcp_serv_info;	/* from getservbyname() */
struct hostent		tcp_host_info;	/* from gethostbyname() */

Word AppNetRefnum = 0;
long AppNetTimeout = (10 * sysTicksPerSecond);
Err h_errno;
Err errno;
NetHostInfoBufType	AppHostInfo;
NetServInfoBufType	AppServInfo;

/***********************************************************************
 *
 * FUNCTION:   Disconnect        
 *
 * DESCRIPTION: Called at end of Echo when user hits stop key
 * Closes socket, NetLib and frees all memory
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
void Disconnect (void)
{
	SetFieldText (MainStatusField,"ShutDown in progress...",79, true);
	if (fd >=0)
	{ 
		close (fd);
		fd = 0;
	}

	if (recv_buf)
	{ 
		MemPtrFree (recv_buf);
		recv_buf = 0;
	}
	
	if (send_buf)
	{ 
		MemPtrFree (send_buf);
		send_buf = 0;
	}
				
	if (AppNetRefnum)
	{
		NetLibClose (AppNetRefnum, true);		
	}
	
	if (TimeOut)
	{
		SysSetAutoOffTime (TimeOut);
	}
		
	SetFieldText (MainStatusField,"ShutDown complete...",79, true);
}
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
 * FUNCTION:    host_err_str
 *
 * DESCRIPTION: This routine is called by tcp_open & udp_open to report 
 *              error codes
 * PARAMETERS:  None
 *
 * RETURNED:    Associated message string for h_errno
 *              
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *         
 *
 ***********************************************************************/
 
static char * host_err_str()
{
	static char	msgstr[100];

	if (errno != 0) 
		SysErrString(errno, msgstr, 100);
	else 
		msgstr[0] = '\0';

	return(msgstr);
}

/***********************************************************************
 *
 * FUNCTION:   udp_open        
 *
 * DESCRIPTION: called by Echo to open a UDP socket   
 *
 * PARAMETERS:  
 * char	*host;			name of other system to communicate with 
 * char	*service;		name of service being requested 
 *					        can be NULL, iff port > 0 
 * int	port;				if == 0, nothing special - use port# of service 
 *					        if < 0, bind a local reserved port
 *                  if > 0, it's the port# of server (host-byte-order) 
 * int	dontconn;   if == 0, call connect(), else don't 
 *
 * RETURNED:        return socket descriptor if OK, else -1 on error 
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release     
 *             
 ***********************************************************************/
 
static int udp_open(host, service, port, dontconn)
char	*host;			
char	*service;
int	port;		
int	dontconn;		
{
	int				fd;
	unsigned long	inaddr;
	char			*host_err_str();
	struct servent	*sp;
	struct hostent	*hp;

	/*
	 * Initialize the server's Internet address structure.
	 * We'll store the actual 4-byte Internet address and the
	 * 2-byte port# below.
	 */
	bzero((char *) &udp_srv_addr, sizeof(udp_srv_addr));
	udp_srv_addr.sin_family = AF_INET;

	if (service != NULL) 
	{
		if ((sp = getservbyname(service, "udp")) == NULL) 
		{
			//sprintf(msg, "udp_open: unknown service: %s/udp", service);
			//SetFieldText(MainStatusField, msg, 79, true);
			return(-1);
		}
		udp_serv_info = *sp;			/* structure copy */
		
		if (port > 0)							/* caller's value */
			udp_srv_addr.sin_port = htons(port);															
		else
			udp_srv_addr.sin_port = sp->s_port; /* service's value */							
	} 
	else 
	{
		if (port <= 0) 
		{
			//SetFieldText(MainStatusField,"udp_open: must specify either service or port",79, true);
			return(-2);
		}
		udp_srv_addr.sin_port = htons(port);
	}

	/*
	 * First try to convert the host name as a dotted-decimal number.
	 * Only if that fails do we call gethostbyname().
	 */
	if ((inaddr = inet_addr(host)) != INADDR_NONE) /* it's dotted-decimal */ 
	{						
		bcopy((char *) &inaddr, (char *) &udp_srv_addr.sin_addr, sizeof(inaddr));
		udp_host_info.h_name = NULL;
	} 
	else 
	{
		if ( (hp = gethostbyname(host)) == NULL) 
		{
			//sprintf(msg, "udp_open: host name error: %s %s",host, host_err_str());
			//SetFieldText(MainStatusField,msg,79, true);
			return(-3);
		}
		
		udp_host_info = *hp;	/* found it by name, structure copy */
		bcopy(hp->h_addr, (char *) &udp_srv_addr.sin_addr, hp->h_length);
	}

	if (port < 0)
		SetFieldText(MainStatusField,"udp_open: reserved ports not implemeneted yet",79, true);

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		//SetFieldText(MainStatusField,"udp_open: can't create UDP socket",79, true);
		return(-4);
	}

	/*
	 * Bind any local address for us.
	 */
	bzero((char *) &udp_cli_addr, sizeof(udp_cli_addr));
	udp_cli_addr.sin_family      = AF_INET;
	udp_cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	udp_cli_addr.sin_port        = htons(0);
	if (bind(fd, (struct sockaddr *) &udp_cli_addr,sizeof(udp_cli_addr)) < 0) 
	{
		//SetFieldText(MainStatusField,"udp_open: bind error",79, true);
		close(fd);
		return(-5);
	}

	/*
	 * Call connect, if desired.  This is used by most caller's,
	 * as the peer shouldn't change.  (TFTP is an exception.)
	 * By calling connect, the caller can call send() and recv().
	 */
	if (dontconn == 0) 
	{
		if (connect(fd, (struct sockaddr *) &udp_srv_addr,sizeof(udp_srv_addr)) < 0) 
		{
			//SetFieldText(MainStatusField,"udp_open: connect error",79, true);
			return(-6);
		}
	}

	return(fd);
}

/***********************************************************************
 *
 * FUNCTION:       tcp_open        
 *
 * DESCRIPTION:    called by Echo    
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
	
	bzero((char *) &tcp_srv_addr, sizeof(tcp_srv_addr));
	tcp_srv_addr.sin_family = AF_INET;
	
  SetFieldText(MainStatusField, "At start of tcp_open", 79, true);
	if (service != NULL) 
	{
		if ((sp = getservbyname(service, "tcp")) == NULL) 
		{
			//sprintf(msg, "tcp_open: unknown service: %s/tcp", service);
			//SetFieldText(MainStatusField,msg,79, true);
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
			//SetFieldText(MainStatusField,"tcp_open: must specify either service or port",79, true);
			return(-2);
		}
		tcp_srv_addr.sin_port = htons(port);
	}

	/*
	 * First try to convert the host name as a dotted-decimal number.
	 * Only if that fails do we call gethostbyname().
	 */
	 
	sprintf (msg,"Lookup on %s", host);
	SetFieldText(MainStatusField, msg,79, true);
	 
	if ((inaddr = inet_addr(host)) != INADDR_NONE) /* it's dotted-decimal */
	{
		bcopy((char *) &inaddr, (char *) &tcp_srv_addr.sin_addr,sizeof(inaddr));
		tcp_host_info.h_name = NULL;
	} 
	else /* its a name */
	{
		if ((hp = gethostbyname(host)) == NULL) 
		{
			//sprintf(msg, "tcp_open: host name error: %s %s",host, host_err_str());
			//SetFieldText(MainStatusField,msg,79, true);
			return(-3);
		}
		tcp_host_info = *hp;	/* found it by name, structure copy */
		bcopy(hp->h_addr, (char *) &tcp_srv_addr.sin_addr,hp->h_length);
	}

	if (port >= 0) 
	{
		if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		{
			//SetFieldText(MainStatusField,"tcp_open: can't create TCP socket",79, true);
			return(-4);
		}
	} 
	else if (port < 0) 
	{
		resvport = IPPORT_RESERVED - 1;
		if ((fd = -1 /*rresvport(&resvport)*/) < 0) 
		{
			//SetFieldText(MainStatusField,"tcp_open: can't get a reserved TCP port",79, true);
			return(-5);
		}
	}

	/*
	 * Connect to the server.
	 */

	if (connect(fd, (struct sockaddr *) &tcp_srv_addr, sizeof(tcp_srv_addr)) < 0) 
	{
		//SetFieldText(MainStatusField,"tcp_open: can't connect to server",79, true);
		close(fd);
		return(-6);
	}

	return(fd);	/* all OK */
}

/***********************************************************************
 *
 * FUNCTION:    ReadN    
 *
 * DESCRIPTION: called by Echo() to read "n" bytes from a descriptor.
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
 * DESCRIPTION: called by Echo() to write "n" bytes to a descriptor.    
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
 * FUNCTION: Echo       
 *
 * DESCRIPTION: Called by cmdStart
 * This program is a client of the Echo service. It connects to
 * a server running the standard echo service. 
 *
 * Loop sending data to server, getting responses, compare the data.
 * If we're sending UDP, don't send more than a 1000 bytes at a time
 * Theoretically, we should be able to send the Epilogue max size
 * (about 1500) worth at a time but the Sun Workstation's echo server
 * seems to max out at 1024.		
 *
 * PARAMETERS:  None
 *
 * RETURNED:    None
 *
 * REVISION HISTORY:
 *         Name   Date       Description
 *         -----  ---------- -----------
 *         dcat   06/14/1999 Initial Release
 *             
 ***********************************************************************/

void Echo(char *host, int sendBytes, int Delay, int useTCP)
{
	char 	*host_err_str();
	char	szDate[longDateStrLength];
	char	szTime[timeStringLength];	
	
	int		sentBytes, rcvBytes;
	ULong	CurrTime, RunTime, StartTime, loop=0;
	UInt	portNum = 7;	
	CharPtr	serviceNameP = 0;
	DateTimeType	dateTime;	

	static	EventType	event;
	DateFormatType	fmtDate;
	TimeFormatType	fmtTime;
	SystemPreferencesType sysPrefs;	
	
	//__get the date and time formats.
	PrefGetPreferences (&sysPrefs);

	//__get the time formats from the system preferences.
	fmtDate   = sysPrefs.dateFormat;
	fmtTime   = sysPrefs.timeFormat;	
	StartTime = TimGetSeconds();
	
	//__get a socket now and connect to the host	
	if (useTCP)
	{
		SetFieldText(MainStatusField, "Tcp Selected", 79, true);				
		if ((fd = tcp_open(host, serviceNameP, portNum)) < 0) 
		{
			sprintf (msg,"tcp_open error: %d: %s", fd, host_err_str());
			SetFieldText (MainStatusField,msg, 79, true);
			goto Exit;
		}
	}	
	else 
	{
		SetFieldText(MainStatusField, "Udp Selected", 79, true);
		if ( (fd = udp_open(host, serviceNameP, portNum, false)) < 0) 
		{
		  	sprintf (msg,"udp_open error: %d: %s", fd, host_err_str());
			SetFieldText (MainStatusField, msg, 79, true);
			goto Exit;
		}
	}              
	
	SetFieldText (MainStatusField, "Open Succeded", 79, true);
			
	TimeOut = SysSetAutoOffTime(0);          	// Make sure we don't turn off
	
	if (!useTCP && sendBytes > 1024) // limit UDP to 1024 bytes 
		sendBytes = 1024;	 
	
	//-------------------------------------------------------------------
	do 
	{
		sprintf (msg,"%lu", loop++);
		SetFieldText (MainCountField, msg, 79, true);	
		 	
		//___read the current data and time then store in the buffer
		CurrTime = TimGetSeconds();
		RunTime = CurrTime - StartTime; // elapse time in seconds
		
		TimSecondsToDateTime (RunTime, &dateTime);
		TimeToAscii (dateTime.hour, dateTime.minute, fmtTime, szTime);
		DateToAscii (dateTime.month, dateTime.day, dateTime.year, fmtDate, szDate);		
		
		sprintf (msg,"%s %s", szDate, szTime);
		StrCopy (send_buf, msg);		

		sprintf (msg,"RunTime: %s %s", szDate, szTime);
		SetFieldText (MainTimeField, msg, 39, true);	
		
		//___write the data to the echo host
		RefreshNetwork (true);
		sentBytes = WriteN (fd, (BytePtr)send_buf, sendBytes);			
		if ( sentBytes != sendBytes) 
		{
			sprintf (msg, "Write_err: %d, %s", sentBytes, host_err_str());
			SetFieldText(MainStatusField, msg, 79, true);
			SndPlaySystemSound (sndError); // see page 155 in ref2.pdf
			SndPlaySystemSound (sndAlarm);
			SndPlaySystemSound (sndWarning);
			goto Exit;
		}				
  		SetFieldText (MainStatusField, "Send Complete...", 79, true);			
		
		//___read available data from host
		RefreshNetwork (true);
		rcvBytes = ReadN (fd, (BytePtr) recv_buf, sendBytes);		
		if (rcvBytes != sendBytes) 
		{
			sprintf (msg, "Read_err: %d, %s", rcvBytes, host_err_str());
			SetFieldText (MainStatusField, msg, 79, true);
			SndPlaySystemSound (sndError); // see page 155 in ref2.pdf
			SndPlaySystemSound (sndAlarm);
			SndPlaySystemSound (sndWarning);
			goto Exit;
		}
		SetFieldText (MainStatusField, "Read Complete...", 79, true);
		
		//___compare the data
		if (MemCmp (send_buf, recv_buf, sendBytes))
		{
			SetFieldText (MainStatusField, "Data-Mismatch", 79, true);
			SndPlaySystemSound (sndError); // see page 155 in ref2.pdf
			SndPlaySystemSound (sndAlarm);
			SndPlaySystemSound (sndWarning);
			goto Exit;		
		}  			
		
		//___delay the required seconds 
		if (Delay) // a zero value delays forever
		{			
			SetFieldText (MainStatusField,"Delay...",79, true);
			sleep (Delay);
		}	
			
		//___allow the user to exit
		do 
		{
			EvtGetEvent (&event, 100);
			if( !SysHandleEvent (&event))
			{
				if (event.eType == ctlSelectEvent &&
				    event.data.ctlEnter.controlID == MainStopButton)
				{
					SetFieldText(MainStatusField, "Loop Canceled",39, true);
					goto Exit;
				}
				else FrmDispatchEvent (&event);
			}
		}	 while (event.eType != nilEvent);
		
		//___clear the receive buffer for next loop
		MemSet (recv_buf, sendBytes, 0);
				
	} while (true);
	
	SetFieldText (MainStatusField,"Loop complete...",79, true);	
	sprintf (msg,"%d", loop++);
	SetFieldText (MainCountField, msg,79, true);	 	

Exit: // close the socket		
	Disconnect();
	return;
}





















