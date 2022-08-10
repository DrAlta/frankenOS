
/*                  A WinSock UDP Echo program for Windows NT
       Copyright(c) Symbol Technologies Inc. 1998. All rights reserved
This code demonistrates a WinSock v1.1 UDP Echo server. t's a bare-bones
console application based on the Berkley socket model. All screen io is
in printf's.

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
#define	BUFFER_SIZE	1024      // upto 1024 bytes

char buffer   [BUFFER_SIZE];
char dispDate [10];
char dispTime [10];
char logfile  [16];    
char msg      [80];

int remlen;
struct sockaddr_in peer;

WSADATA WsaData;


//-------------------------proto's----------------------------------
void main         (int, char **);
int Read_Echo			(SOCKET s);
void LogStart     (void);
void LogEvent     (char *);


void	main( int argc, char **argv )
{
  SOCKET s;
  int  rc;
  struct sockaddr_in addr;
	
	remlen = sizeof (peer);

  // Initialize the Windows Sockets DLL v1.1
  rc = WSAStartup( 0x0101, &WsaData );
  if ( rc == SOCKET_ERROR )
  {
     printf( "WSAStartup() failed: %ld\n", GetLastError( ) );
     return;
  }

  // Get a socket for incoming messages
  s = socket(PF_INET, SOCK_DGRAM, 0);
  if (s == INVALID_SOCKET)
  {
    printf( "socket() failed: %ld\n", GetLastError( ) );
    return;
  }

  ZeroMemory (&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(MAIN_PORT);
  addr.sin_addr.s_addr = 0;	

  rc = bind(s, (struct sockaddr *)&addr, sizeof(addr)); 
  if (rc == SOCKET_ERROR)
  {
		closesocket (s);
    printf( "bind() failed: %ld\n", GetLastError( ) );
    WSACleanup ();
    return;
  }


	//___if you get here, we're ready to accept incoming messages
	LogStart ();

  for (;;) //----------------main loop--------------------------
	{
 
		rc = Read_Echo(s);

		if (rc < 0)
			break;

		if (kbhit ())
			break;


	} // end for (;;)

  closesocket (s);    // close the main socket for the host
  WSACleanup ();
  printf ("\nShutdown completed....");
}

 
int Read_Echo (SOCKET s)
{
  int  rc,len;
 
  len = recvfrom (s, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&peer, &remlen);
  if (len == SOCKET_ERROR)
	{

		sprintf (msg,"---recvfrom failed...closing socket for %s err= %s", 
			           inet_ntoa(peer.sin_addr), len);
		LogEvent (msg);
		return -1;
  }

  //___got a good read so echo it
	rc = sendto (s, buffer, len, 0, (struct sockaddr *)&peer, sizeof (peer));
	if (rc == SOCKET_ERROR)
	{
		sprintf (msg,"---sendto failed...closing socket for %s err= %s", 
							inet_ntoa(peer.sin_addr), rc);
		LogEvent (msg);      
		return -2;
	}

  //___only good reads/writes get here
  sprintf(msg, "\nEchoed a UDP datagram length=%d to %s at: ", rc, inet_ntoa(peer.sin_addr));
  strcat (msg, _strdate (dispDate));
  strcat (msg, " ");
  strcat (msg, _strtime (dispTime));
  printf (msg);
  return 0;
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
    LogEvent ("-----Spectrum24 WinSock UDP Echo Program v1.0------");
    LogEvent ("-----------Press any key to Shutdown---------------");
   }
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
