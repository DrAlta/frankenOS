
/*                  A WinSock IP Echo program for Windows NT
       Copyright(c) Symbol Technologies Inc. 1998. All rights reserved
This code demonistrates a WinSock v1.1 TCP/IP Echo server. It should support upto
64 users with 5K byte limits on the echo packet size. It's a bare-bones console
application based on the Berkley socket model. All screen io is in printf's.

Compiler: MicroSoft Visual C++ v5.0
Project created within Microsoft Developer Studio by:
	1) FILE/NEW/Win32 CONSOLE APP/ & take defaults
	2) PROJECTS/ADD TO/FILES/ & add this file
	3) PROJECT/SETTINGS/LINK/ & add WSOCK32.LIB to the linking libs

*/

#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <winsock.h> 

#define	MAIN_PORT	7         // the echo port    
#define	MAX_SOCKETS	64        // support 64 users
#define	BUFFER_SIZE	1024*5    // upto 5120 bytes

char buffer   [BUFFER_SIZE];
char dispDate [10];
char dispTime [10];
char datestr  [30];
char logfile  [16];
char msg[80];    

fd_set readfds;

struct timeval  seltime;

WSADATA WsaData;

typedef struct
{
	SOCKET sock_id;
	unsigned int end_octet;

} INUSE_TYPE;

INUSE_TYPE in_use[MAX_SOCKETS];

//-------------------------proto's----------------------------------
void main         (int, char **);
void Read_Echo    (SOCKET s);
void Make_New     (SOCKET s);
void LogStart     (void);
void LogEvent     (char *);
void LogErr       (char *, int, int);
void strmid       (char *,  char *, char, char);
int  getInUseIdx  (SOCKET s);
void setInUseOctet(SOCKET s, unsigned int octet);
unsigned int getInUseOctet (SOCKET s);

void	main( int argc, char **argv )
{
  SOCKET s;
  int x , rc;
  struct sockaddr_in addr;

  for (x = 0; x < MAX_SOCKETS; x++)
  {
    in_use   [x].sock_id = 0;
	in_use	 [x].end_octet = 0;
 
  }

  // Initialize the Windows Sockets DLL v1.1
  rc = WSAStartup( 0x0101, &WsaData );
  if ( rc == SOCKET_ERROR )
  {
     printf( "WSAStartup() failed: %ld\n", GetLastError( ) );
     return;
  }

  // Get a socket for incoming messages
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == INVALID_SOCKET)
  {
    printf( "socket() failed: %ld\n", GetLastError( ) );
    return;
  }

  ZeroMemory (&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(MAIN_PORT);

  rc = bind(s, (struct sockaddr *)&addr, sizeof(addr)); 
  if (rc == SOCKET_ERROR)
  {
    closesocket (s);
    printf( "bind() failed: %ld\n", GetLastError( ) );
    WSACleanup ();
    return;
  }

  // prepare to accept upto 5 pending client connections
  rc = listen (s, 5);
  if (rc == SOCKET_ERROR)
  {
    closesocket (s);
    printf( "listen() failed: %ld\n", GetLastError( ) );
    WSACleanup ();
    return;
  }

   //___if you get here, we're ready to accept incoming messages

   LogStart ();

  for (;;) //----------------main loop--------------------------
    {
     if (kbhit ())
		goto done;
      
      FD_ZERO (&readfds);
      seltime.tv_sec = 1; 
	  seltime.tv_usec = 0;

	  // Always check the main socket for activity
      FD_SET (s, &readfds);
	
	  // Check all in_use sockets for activity.
      for (x = 0; x < MAX_SOCKETS; x++) 
       {
        if (in_use[x].end_octet) 
         {
          FD_SET(in_use[x].sock_id, &readfds);
         }
       }

	  // if there's not any activity, go back to the top of the for loop 
      if (!select(MAX_SOCKETS,&readfds, (fd_set *) 0, (fd_set *) 0,&seltime)) 
       {
		continue;
       }

	  // If you get here, there,s some data on a socket to process.
	  // look for new incomming connections.
	  for (x = 0; x < (int) readfds.fd_count; x++)
	  {
		  // if the readfds structure has a socket that is not in our in_use array,
		  // it must be a new incomming connection.
		  if (getInUseIdx(readfds.fd_array[x]) < 0)
			Make_New(s);
	  }

	  // loop through all the sockets with activity and do a read_echo
      for (x = 0; x < MAX_SOCKETS; x++) 
      {
		// if the socket id is not zero (i.e. if it in use) 
		//   AND it has activity.
		if (in_use[x].sock_id && FD_ISSET(in_use[x].sock_id, &readfds)) 
		{
			// read & echo to the socket iff it is not the main socket.
			if (in_use[x].sock_id != s) 
				Read_Echo (in_use[x].sock_id);
		}
	  }
    } // end for (;;)

done:
  closesocket (s);    // close the main socket for the host
  WSACleanup ();
  printf ("\nShutdown completed....");

} 

// When a client establishs his connect, it comes through the main host 
// port s.  This routine creates a unique socket for output to the new
// client with the accept() call. It then stores the returned new socket
// number ns and the last octet of the ip address in our in_use array.
void Make_New ( SOCKET s)
{
  SOCKET ns;
  int  result;
  struct sockaddr_in peer;
  int  peersize = sizeof(peer);
  char unit [6];
  char ip_addr [20];
  char outline [80];
  char * pdest;

  ns = accept(s, (struct sockaddr *)&peer, &peersize);
  if (ns == INVALID_SOCKET) {
    LogErr ("---Accept failed new on connection ", ns, 0);
    return;
  }

  //___strip off the last byte of the ip address
  strcpy (ip_addr, inet_ntoa(peer.sin_addr));
  strrev (ip_addr);
  pdest = strchr (ip_addr, '.');
  result = pdest - ip_addr;
  strncpy (unit, ip_addr, result);
  unit [result] = '\0';
  strrev (unit);

  sprintf (outline, "Accepted unit %s on port %d", inet_ntoa(peer.sin_addr), ns);
  LogEvent (outline);

  //__Store the last octet of the ip_address in the in_use array. This will
  //  cause it to be picked up for the echo later.
  setInUseOctet(ns, atoi(unit));

}

// This routine scans the in_use array to see if anyone needs to be processed.
// Given an existing socket with data to process, we read the data and echo it
// back unchanged.
void Read_Echo (SOCKET s)
{
  int    rc,len;

  len = recv (s, buffer, sizeof(buffer), 0);
  
  // if the MU gracefully shuts down, rc will be zero on the above recv()
  if (len == SOCKET_ERROR || len == 0)

	LogErr ("---Closing socket for 157.235.93.", s, len);

   else //___got a good read so process it
   {  
    rc = send (s, buffer, len, 0);

    if (rc == SOCKET_ERROR) 
		LogErr ("---send() error..Closing socket for ", s, rc);
	
	else //___only good reads/writes get here
	{  
		sprintf(msg, "\nEchoed TCP packet length=%d to port %d at: ", rc, s);
		strcat (msg, _strdate (dispDate));
		strcat (msg, " ");
		strcat (msg, _strtime (dispTime));
		printf (msg); 
	}
   }
} 

// Build a unique filename based on the current date & time. Open the file
// and write the title bar with the version number
void LogStart (void)
{
  char      tempstr [10];
  time_t    nowtime;
  char      *nowtime_str;

  FILE *fpLog;

  strcpy(logfile, "sp");
  nowtime = time(NULL);
  nowtime_str = ctime(&nowtime);

  tempstr[0] = nowtime_str[8];   /* get the current date */
  tempstr[1] = nowtime_str[9];
  tempstr[2] = nowtime_str[11];  /* get the current hour */
  tempstr[3] = nowtime_str[12];
  tempstr[4] = nowtime_str[14];  /* get the current minute */
  tempstr[5] = nowtime_str[15];
  tempstr[6] = '.';              /* append ".log" */
  tempstr[7] = 'l';
  tempstr[8] = 'o';
  tempstr[9] = 'g';
  tempstr[10] = '\0';

  strcat(logfile,tempstr);	  /* logfile = "spddhhmm.log" */

  if ( (fpLog = fopen(logfile,"wt") ) == NULL )
   {
    printf ("Error in opening \"%s\"...",logfile);
    exit (1);
   }
   else
   {
    LogEvent ("-----Spectrum24 WinSock TCP Host Program v1.0------");
    LogEvent ("---------Press any key to Shutdown-------------");
   }
}

// Write an entry in the log file and close the socket
void LogErr ( char * msg, int socket, int code)
{
  char szDate [10];
  char szTime [10];
  char outline[80];
  FILE *fpLog;

  if (!(fpLog = fopen (logfile,"at")))
    {
      printf ("Error on open in LogErr\n");
      return;
    }

  sprintf (outline, "\n%s %s %s%u rtn=%u", _strdate (szDate), _strtime (szTime),
                    msg, getInUseOctet(socket), code);

  fprintf (fpLog, outline);
  printf  ("%s", outline);

  //___wipe out it's entry in the in_use array &mark this guy as a goner...
  setInUseOctet(socket, 0); // 0 to wipe out it's entry in the in_use array
  closesocket (socket);

  fflush  (fpLog);
  fclose  (fpLog);
}

// Write a time stamped entry into the log.
void LogEvent ( char *msg)
{
  char szDate [10];
  char szTime [10];
  char outline[80];
  FILE *fpLog;

  if (!(fpLog = fopen (logfile,"at")))
    {
      printf ("Error on open in LogEvent\n");
      return;
    }

  sprintf (outline, "\n%s %s %s", _strdate (szDate), _strtime (szTime), msg);
  fprintf (fpLog, outline);
  printf  ("%s",  outline);

  fflush  (fpLog);
  fclose  (fpLog);
}

//___Given a socket, lookup its index into the in_use array
int getInUseIdx(SOCKET s)
{
	int ii;

	for (ii = 0; ii < MAX_SOCKETS; ii++)
	{
		if (s == in_use[ii].sock_id)
			return ii;
	}
	
	return -1;
}

//___Given a socket, return the last octet of the IP address
unsigned int getInUseOctet(SOCKET s)
{
	int idx = getInUseIdx(s);

	if (idx >= 0)
		return in_use[idx].end_octet;
	else
		return 0;
}

//___Given a Socket & the last octet of the IP address, mark it in_use
//   Called by LogErr with octet=0 to remove a sockets entry in in_use
void setInUseOctet(SOCKET s, unsigned int octet)
{
	int ii;

	// look up the index for this socket
	int idx = getInUseIdx(s);
	
	// 
	if (idx >= 0) // implies it's in_use or active so zero out the entry
	{
		in_use[idx].end_octet = octet;	// will be zero to close a socket
		if (octet == 0)
			in_use[idx].sock_id = 0;
		return;
	}
	else // got a -1 so this is a new socket
	{
		for (ii = 0; ii < MAX_SOCKETS; ii++)
		{
			if (in_use[ii].sock_id == 0)
			{
				in_use[ii].sock_id = s;
				in_use[ii].end_octet = octet;
				return;
			}
		}
	}
}

//___extract a sub-string given a starting point and length
void strmid ( char *input,  char *out, char start, char length )
{  
  char * strptr;
   
  strptr = input + (long)start;
  
  strncpy (out, strptr, length);
  
  out [length] = '\0';
} 
