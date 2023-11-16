/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: CmdStevens.c
 *
 * Description:
 *	  A bunch of Sockets API test routines obtained mostly from Richard
 *	Steven's books. 
 *
 *****************************************************************************/

// Include these for the EvtResetAutoOff() call.
#include <PalmOS.h>

// Standard "Unix" includes
#include <sys_socket.h>

// Stdio Functions
#include "AppStdIO.h"

// Application header
#include "NetSample.h"


/*
 * Recoverable error.  Print a message, and return to caller.
 *
 *	err_ret(str, arg1, arg2, ...)
 *
 * The string "str" must specify the conversion specification for any args.
 */
#define	err_ret	printf
#define	err_quit	printf
#define	err_dump	printf
#define	err_sys	printf

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff	/* should be in <netinet/in.h> */
#endif

/*
 * The following globals are available to the caller, if desired.
 */

struct sockaddr_in	udp_srv_addr;	/* server's Internet socket addr */
struct sockaddr_in	udp_cli_addr;	/* client's Internet socket addr */
struct servent			udp_serv_info;	/* from getservbyname() */
struct hostent			udp_host_info;	/* from gethostbyname() */

static int			/* return socket descriptor if OK, else -1 on error */
udp_open(host, service, port, dontconn)
char	*host;		/* name of other system to communicate with */
char	*service;	/* name of service being requested */
			/* can be NULL, iff port > 0 */
int	port;		/* if == 0, nothing special - use port# of service */
			/* if < 0, bind a local reserved port */
			/* if > 0, it's the port# of server (host-byte-order) */
int	dontconn;	/* if == 0, call connect(), else don't */
{
	int				fd;
	unsigned long	inaddr;
	char *			host_err_str();
	struct servent	*sp;
	struct hostent	*hp;

	/*
	 * Initialize the server's Internet address structure.
	 * We'll store the actual 4-byte Internet address and the
	 * 2-byte port# below.
	 */

	bzero((char *) &udp_srv_addr, sizeof(udp_srv_addr));
	udp_srv_addr.sin_family = AF_INET;

	if (service != NULL) {
		if ( (sp = getservbyname(service, "udp")) == NULL) {
			err_ret("udp_open: unknown service: %s/udp", service);
			return(-1);
		}
		udp_serv_info = *sp;			/* structure copy */
		if (port > 0)
			udp_srv_addr.sin_port = htons(port);
							/* caller's value */
		else
			udp_srv_addr.sin_port = sp->s_port;
							/* service's value */
	} else {
		if (port <= 0) {
			err_ret("udp_open: must specify either service or port");
			return(-1);
		}
		udp_srv_addr.sin_port = htons(port);
	}

	/*
	 * First try to convert the host name as a dotted-decimal number.
	 * Only if that fails do we call gethostbyname().
	 */

	if ( (inaddr = inet_addr(host)) != INADDR_NONE) {
						/* it's dotted-decimal */
		bcopy((char *) &inaddr, (char *) &udp_srv_addr.sin_addr,
			sizeof(inaddr));
		udp_host_info.h_name = NULL;

	} else {
		if ( (hp = gethostbyname(host)) == NULL) {
			err_ret("udp_open: host name error: %s %s",
						host, host_err_str());
			return(-1);
		}
		udp_host_info = *hp;	/* found it by name, structure copy */
		bcopy(hp->h_addr, (char *) &udp_srv_addr.sin_addr,
			hp->h_length);
	}

	if (port < 0)
		err_quit("udp_open: reserved ports not implemeneted yet");

	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		err_ret("udp_open: can't create UDP socket");
		return(-1);
	}

	/*
	 * Bind any local address for us.
	 */

	bzero((char *) &udp_cli_addr, sizeof(udp_cli_addr));
	udp_cli_addr.sin_family      = AF_INET;
	udp_cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	udp_cli_addr.sin_port        = htons(0);
	if (bind(fd, (struct sockaddr *) &udp_cli_addr,
						sizeof(udp_cli_addr)) < 0) {
		err_ret("udp_open: bind error");
		close(fd);
		return(-1);
	}

	/*
	 * Call connect, if desired.  This is used by most caller's,
	 * as the peer shouldn't change.  (TFTP is an exception.)
	 * By calling connect, the caller can call send() and recv().
	 */

	if (dontconn == 0) {
		if (connect(fd, (struct sockaddr *) &udp_srv_addr,
						sizeof(udp_srv_addr)) < 0) {
			err_ret("udp_open: connect error");
			return(-1);
		}
	}

	return(fd);
}

/*
 * The following globals are available to the caller, if desired.
 */

struct sockaddr_in	tcp_srv_addr;	/* server's Internet socket addr */
struct servent		tcp_serv_info;	/* from getservbyname() */
struct hostent		tcp_host_info;	/* from gethostbyname() */

static int			/* return socket descriptor if OK, else -1 on error */
tcp_open(host, service, port)
char	*host;		/* name or dotted-decimal addr of other system */
char	*service;	/* name of service being requested */
			/* can be NULL, iff port > 0 */
int	port;		/* if == 0, nothing special - use port# of service */
			/* if < 0, bind a local reserved port */
			/* if > 0, it's the port# of server (host-byte-order) */
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

	if (service != NULL) {
		if ( (sp = getservbyname(service, "tcp")) == NULL) {
			err_ret("tcp_open: unknown service: %s/tcp", service);
			return(-1);
		}
		tcp_serv_info = *sp;			/* structure copy */
		if (port > 0)
			tcp_srv_addr.sin_port = htons(port);
							/* caller's value */
		else
			tcp_srv_addr.sin_port = sp->s_port;
							/* service's value */
	} else {
		if (port <= 0) {
			err_ret("tcp_open: must specify either service or port");
			return(-1);
		}
		tcp_srv_addr.sin_port = htons(port);
	}

	/*
	 * First try to convert the host name as a dotted-decimal number.
	 * Only if that fails do we call gethostbyname().
	 */

	if ( (inaddr = inet_addr(host)) != INADDR_NONE) {
						/* it's dotted-decimal */
		bcopy((char *) &inaddr, (char *) &tcp_srv_addr.sin_addr,
					sizeof(inaddr));
		tcp_host_info.h_name = NULL;

	} else {
		if ( (hp = gethostbyname(host)) == NULL) {
			err_ret("tcp_open: host name error: %s %s",
						host, host_err_str());
			return(-1);
		}
		tcp_host_info = *hp;	/* found it by name, structure copy */
		bcopy(hp->h_addr, (char *) &tcp_srv_addr.sin_addr,
			hp->h_length);
	}

	if (port >= 0) {
		if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			err_ret("tcp_open: can't create TCP socket");
			return(-1);
		}

	} else if (port < 0) {
		resvport = IPPORT_RESERVED - 1;
		if ( (fd = -1 /*rresvport(&resvport)*/) < 0) {
			err_ret("tcp_open: can't get a reserved TCP port");
			return(-1);
		}
	}

	/*
	 * Connect to the server.
	 */

	if (connect(fd, (struct sockaddr *) &tcp_srv_addr,
						sizeof(tcp_srv_addr)) < 0) {
		err_ret("tcp_open: can't connect to server");
		close(fd);
		return(-1);
	}

	return(fd);	/* all OK */
}

/*
 * Send a datagram to a server, and read a response.
 * Establish a timer and resend as necessary.
 * This function is intended for those applications that send a datagram
 * and expect a response.
 * Returns actual size of received datagram, or -1 if error or no response.
 */
static int
dgsendrecv(fd, outbuff, outbytes, inbuff, inbytes, destaddr, destlen)
int		fd;		/* datagram socket */
char		*outbuff;	/* pointer to buffer to send */
int		outbytes;	/* #bytes to send */
char		*inbuff;	/* pointer to buffer to receive into */
int		inbytes;	/* max #bytes to receive */
struct sockaddr *destaddr;	/* destination address */
				/* can be 0, if datagram socket is connect'ed */
int		destlen;	/* sizeof(destaddr) */
{
	int	n;

	/*
	 * Send the datagram.
	 */

	if (sendto(fd, outbuff, outbytes, 0, destaddr, destlen) != outbytes) {
		err_ret("dgsendrecv: sendto error on socket");
		return(-1);
		}


	n = recvfrom(fd, inbuff, inbytes, 0,
			(struct sockaddr *) 0, (int *) 0);
	if (n < 0) {
		err_ret("dgsendrecv: recvfrom error");
		return(-1);
		}

	return(n);		/* return size of received datagram */
}

/*
 * Return a string containing some additional information after a
 * host name or address lookup error - gethostbyname() or gethostbyaddr().
 */
static char *
host_err_str()
{
	static char	msgstr[100];

	if (h_errno != 0) 
		SysErrString(h_errno, msgstr, 100);
	else 
		msgstr[0] = '\0';

	return(msgstr);
}


/*
 * Go through a list of Internet addresses,
 * printing each one in dotted-decimal notation.
 */

static void pr_inet(listptr, length)
char	**listptr;
int	length;
{
	struct in_addr	*ptr;

	while ( (ptr = (struct in_addr *) *listptr++) != NULL)
		printf("	Internet address: %s\n", inet_ntoa(*ptr));
}

/*
 * Read a line from a descriptor.  Read the line one byte at a time,
 * looking for the newline.  We store the newline in the buffer,
 * then follow it with a null (the same as fgets(3)).
 * We return the number of characters up to, but not including,
 * the null (the same as strlen(3)).
 */

static int
readline(fd, ptr, maxlen)
register int	fd;
register char	*ptr;
register int	maxlen;
{
	int	n, rc;
	char	c;

	for (n = 1; n < maxlen; n++) {
		if ( (rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;
		} else if (rc == 0) {
			if (n == 1)
				return(0);	/* EOF, no data read */
			else
				break;		/* EOF, some data was read */
		} else
			return(-1);	/* error */
	}

	*ptr = 0;
	return(n);
}

/*
 * Read "n" bytes from a descriptor.
 * Use in place of read() when fd is a stream socket.
 */

static int
readn(fd, ptr, nbytes)
register int	fd;
register char	*ptr;
register int	nbytes;
{
	int	nleft, nread;

	nleft = nbytes;
	while (nleft > 0) {
		nread = read(fd, ptr, nleft);
		if (nread < 0)
			return(nread);		/* error, return < 0 */
		else if (nread == 0)
			break;			/* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return(nbytes - nleft);		/* return >= 0 */
}


/*
 * Write "n" bytes to a descriptor.
 * Use in place of write() when fd is a stream socket.
 */

static int
writen(register int fd, register UInt8 *ptr, register int nbytes)
{
	int	nleft, nwritten;

	nleft = nbytes;
	while (nleft > 0) {
		nwritten = write(fd, ptr, nleft);
		if (nwritten <= 0)
			return(nwritten);		/* error */

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(nbytes - nleft);
}



#define	MAXLINE		  128		/* max line length */
#define	TVAL_SIZE	    4		/* 4 bytes in the 32-bit binary timeval */

/*
 * Contact the time server using TCP/IP and print the result.
 */

static void tcp_time(host)
char	*host;
{
	int		fd, i;
	unsigned long	temptime, timeval;

	/*
	 * Initiate the connection and receive the 32-bit time value
	 * (in network byte order) from the server.
	 */

	if ( (fd = tcp_open(host, "time", 0)) < 0) {
		err_ret("tcp_open error");
		return;
	}

	if ( (i = readn(fd, (char *) &temptime, TVAL_SIZE)) != TVAL_SIZE)
		err_dump("received %d bytes from server", i);

	timeval = ntohl(temptime);
	printf("time from host %s using TCP/IP = %lu\n", host, timeval);

	close(fd);
}

/*
 * Contact the daytime server using TCP/IP and print the result.
 */

static void tcp_daytime(host)
char	*host;
{
	int	fd;
	char	*ptr, buff[MAXLINE];

	/*
	 * Initiate the session and receive the netascii daytime value
	 * from the server.
	 */

	if ( (fd = tcp_open(host, "daytime", 0)) < 0) {
		err_ret("tcp_open error");
		return;
	}

	if(readline(fd, buff, MAXLINE) < 0)
		err_dump("readline error");

	if ( (ptr = index(buff, '\n')) != NULL)
		*ptr = 0;
	printf("daytime from host %s using TCP/IP = %s\n", host, buff);

	close(fd);
}

/*
 * Contact the time server using UDP/IP and print the result.
 */

static void udp_time(host)
char	*host;
{
	int		fd, i;
	unsigned long	temptime, timeval;

	/*
	 * Open the socket and send an empty datagram to the server.
	 */

	if ( (fd = udp_open(host, "time", 0, 0)) < 0) {
		err_ret("udp_open error");
		return;
	}

	/*
	 * Send a datagram, and read a response.
	 */

	if ( (i = dgsendrecv(fd, (char *) &temptime, 1, (char *) &temptime,
			TVAL_SIZE, (struct sockaddr *) 0, 0)) != TVAL_SIZE)
		if (errno == EINTR) {
			err_ret("udp_time: no response from server");
			close(fd);
			return;
		} else
			err_dump("received %d bytes from server", i);

	timeval = ntohl(temptime);
	printf("time from host %s using UDP/IP = %lu\n", host, timeval);

	close(fd);
}

/*
 * Contact the daytime server using UDP/IP and print the result.
 */

static void udp_daytime(host)
char	*host;
{
	int	fd, i;
	char	*ptr, buff[MAXLINE];

	/*
	 * Open the socket and send an empty datagram to the server.
	 */

	if ( (fd = udp_open(host, "daytime", 0, 0)) < 0) {
		err_ret("udp_open error");
		return;
	}

	if ( (i = dgsendrecv(fd, buff, 1, buff, MAXLINE,
				(struct sockaddr *) 0, 0)) < 0)
		if (errno == EINTR) {
			err_ret("udp_daytime: no response from server");
			close(fd);
			return;
		} else
			err_dump("read error");

	buff[i] = 0;		/* assure its null terminated */
	if ( (ptr = index(buff, '\n')) != NULL)
		*ptr = 0;
	printf("daytime from host %s using UDP/IP = %s\n", host, buff);

	close(fd);
}

/***********************************************************************
 *
 * FUNCTION:   CmdDivider()
 *
 * DESCRIPTION: This command exists solely to print out a dividing line
 *		when the user does a help all.
 *
 *	CALLED BY:	 
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdDivider(int argc, Char * argv[])
{
	// Check for short help
	if (argc > 1 && !StrCompare(argv[1], "?")) 
		printf("\n-- Stevens' Cmds --\n");
}


/*
 * Print the "hostent" information for every host whose name is
 * specified on the command line.
 */
static void CmdHostEnt(argc, argv)
int	argc;
char	**argv;
{
	register char		*ptr;
	char			*host_err_str();	/* our lib function */
	register struct hostent	*hostptr;


	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;

	while (--argc > 0) {
		ptr = *++argv;
		if ( (hostptr = gethostbyname(ptr)) == NULL) {
			err_ret("gethostbyname error for host: %s %s",
					ptr, host_err_str());
			continue;
		}
		printf("official host name: %s\n", hostptr->h_name);

		/* go through the list of aliases */
		while ( (ptr = *(hostptr->h_aliases)) != NULL) {
			printf("	alias: %s\n", ptr);
			hostptr->h_aliases++;
		}
		printf("	addr type = %d, addr length = %d\n",
				hostptr->h_addrtype, hostptr->h_length);

		switch (hostptr->h_addrtype) {
		case AF_INET:
			pr_inet(hostptr->h_addr_list, hostptr->h_length);
			break;

		default:
			err_ret("unknown address type");
			break;
		}
	}

	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# Lookup host name\n", argv[0]);
	return;
	
FullHelp:
	printf("\nLookup host name");
	printf("\nSyntax: %s <hostname>...", argv[0]);
	printf("\n");

}

/**********************************************************************
 * This program is an example using the DARPA "Daytime Protocol"
 * (see RFC 867 for the details) and the DARPA "Time Protocol"
 * (see RFC 868 for the details).
 *
 * BSD systems provide a server for both of these services,
 * using either UDP/IP or TCP/IP for each service.
 * These services are provided by the "inetd" daemon under 4.3BSD.
 *
 *	inettime  [ -t ]  [ -u ]  hostname ...
 *
 * The -t option says use TCP, and the -u option says use UDP.
 * If neither option is specified, both TCP and UDP are used.
 ********************************************************************/
#define	MAXHOSTNAMELEN	64

char	hostname[MAXHOSTNAMELEN];
char	*pname;

static void CmdDaytime(argc, argv)
int   argc;
char  **argv;
{
	int	dotcp, doudp;
	char	*s;

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;

	pname = argv[0];
	dotcp = doudp = 0;

	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s != '\0'; s++)
			switch (*s) {

			case 't':
				dotcp = 1;
				break;

			case 'u':
				doudp = 1;
				break;

			default:
				err_quit("unknown command line option: %c", *s);
			}

	if (dotcp == 0 && doudp == 0)
		dotcp = doudp = 1;		/* default */

	while (argc-- > 0) {
		strcpy(hostname, *(argv++));

		if (dotcp) {
			tcp_daytime(hostname);
			tcp_time(hostname);
		}

		if (doudp) {
			udp_daytime(hostname);
			udp_time(hostname);
		}
	}


	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# Daytime client\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDaytime client");
	printf("\nSyntax: %s [-t] [-u] <hostname>", argv[0]);
	printf("\n");

}

/**************************************************************
 * Implementing a timer using select
 ***************************************************************/
static void CmdTimer(argc, argv)
int	argc;
char	*argv[];
{
	static struct timeval	timeout;

	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	if (argc != 3)
		err_quit("usage: timer <#seconds> <#microseconds>");
	timeout.tv_sec  = StrAToI(argv[1]);
	timeout.tv_usec = StrAToI(argv[2]);

	if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0)
		err_sys("select error");


	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# Test Select timeout\n", argv[0]);
	return;
	
FullHelp:
	printf("\nTest Select timeout");
	printf("\nSyntax: %s <seconds> <microseconds>", argv[0]);
	printf("\n");

}

/**********************************************************************
 * This program is a client of the Echo service. It connects to
 * a server running the standard echo service and sends data to it
 * then checks for a response.
 *
 * This program can also be used to "torture" test by setting
 * the amount of data to send appropriately on the command line.
 *
 * Syntax: echo -p [u|t] [-b <numBytes>] [-l <numLoops>] <hostname>
 *					-p [u|t]				# protocol, either UDP or TCP
 *					-b <numBytes>		# number of bytes to send before checking response
 *					-l <numLoops>		# number of loops to do, -1 for infinite
 *					-port <portnum>	# port number to use
 *					<hostname>			# name of host to connect to.
 ********************************************************************/
static void CmdEcho(argc, argv)
int   argc;
char  **argv;
{
	Boolean	usageErr = false;
	Boolean	useTCP=false;
	int		sendBytes = 100, sentBytes, chunkBytes;
	int		numLoops = 1;
	Char *	hostNameP=0;
	int		fd=-1;

	UInt8 *	srcP = (UInt8 *)0;					// send from address 0.
	Int32		rcvTotalBytes=0, rcvBytes, sendTotalBytes=0;
	UInt8 *	dstBufP = 0;
	const		int	dstBufSize = 0x400;
	int		i;
	int		seconds, milliseconds;
	UInt32		totalTicks, startTicks;
	UInt16		portNum = 0;
	Char *	serviceNameP;
	UInt8 *	srcBufP = 0;


	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	//------------------------------------------------------------
	// Get Command line arguments
	//------------------------------------------------------------
	for ( i=1; i<argc; i++) {
		if (!StrCompare(argv[i], "-p")) {
			if (argc > i+1) {
				if (argv[i+1][0] == 't') useTCP = true;
				else useTCP = false;
				i++;
				}
			else
				usageErr = true;
			}

		else if (!StrCompare(argv[i], "-b")) {
			if (argc > i+1) {
				sendBytes = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}
			
		else if (!StrCompare(argv[i], "-port")) {
			if (argc > i+1) {
				portNum = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}

		else if (!StrCompare(argv[i], "-l")) {
			if (argc > i+1) {
				numLoops = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}

		else if (!hostNameP)
			hostNameP = argv[i];
		
		else
			usageErr = true;
		}


	// If error, print syntax
	if (!hostNameP || usageErr)  {
		goto FullHelp;
		return;
		}
		
	//-------------------------------------------------------------------
	// Print status
	//------------------------------------------------------------------
	// Allocate space for receive data
	dstBufP = MemPtrNew(dstBufSize);
	if (!dstBufP) {
		printf("\nOut of memory");
		goto Exit;
		}
	printf("\nSending %d bytes, %d times...\n",
			sendBytes, numLoops);
			
			
	//-------------------------------------------------------------------
	// Open up the connection
	//------------------------------------------------------------------
	// If no port number specified, use the echo port
	if (!portNum) serviceNameP = "echo";
	else serviceNameP = 0;
	
	if (useTCP) {
		if ( (fd = tcp_open(hostNameP, serviceNameP, portNum)) < 0) {
			err_ret("tcp_open error");
			goto Exit;
			}
		}
	
	else {
		if ( (fd = udp_open(hostNameP, serviceNameP, portNum, false)) < 0) {
			err_ret("udp_open error");
			goto Exit;
			}
		}

	//-------------------------------------------------------------------
	// Loop sending data to server and getting responses
	//-------------------------------------------------------------------
	// If we're sending UDP, don't send more than a packet's worth at a time


	startTicks = TimGetTicks();
	do {
		// Make sure we don't turn off
		EvtResetAutoOffTimer();
	
	
		// If we're sending UDP, don't send more than a 1000 worth at a time
		// Theoretically, we should be able to send the Epilogue max packet size
		//  (about 1500) worth at a time but the Sun Workstation's echo server
		//  seems to max out at 1024.
		sentBytes = 0;
		do {
			chunkBytes = sendBytes - sentBytes;
			if (!useTCP && chunkBytes > 1000) chunkBytes = 1000;
			
			if (writen(fd, srcP, chunkBytes) != chunkBytes) {
				err_ret("write error");
				goto Exit;
				}
			sentBytes += chunkBytes;
			} while(sentBytes < sendBytes);
			
		
		// Read available data
		rcvBytes = 0;
		while(rcvBytes < sendBytes) {
			chunkBytes = recv(fd, dstBufP, dstBufSize, 0);
			if (chunkBytes <= 0) {
				printf("receive error");
				goto Exit;
				}
			rcvBytes += chunkBytes;
			}
			
		
		// Print status
		sendTotalBytes += sendBytes;
		printf(".");
			
		if (numLoops > 0) numLoops--;
		} while(numLoops);
		
		
	//-------------------------------------------------------------------
	// Print results
	//-------------------------------------------------------------------
	totalTicks = TimGetTicks() - startTicks;
	seconds = totalTicks/sysTicksPerSecond;
	milliseconds = (totalTicks % sysTicksPerSecond * 1000) / sysTicksPerSecond;
	printf("\n%ld bytes in %d.%d sec", sendTotalBytes*2, seconds, milliseconds);
	printf("\n  ==> %ld bytes/sec", sendTotalBytes * 2 * sysTicksPerSecond /totalTicks);
	
				
Exit:
	if (fd >=0) close(fd);
	if (dstBufP) MemPtrFree(dstBufP);

	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t\t# Echo client\n", argv[0]);
	return;
	
FullHelp:
	printf("\nEcho client");
	printf("\nSyntax: %s -p [u|t] [-b <numBytes>] [-l <numLoops>] <hostname>",
						argv[0]);
	printf("\n");


}

/**********************************************************************
 * This program is a client of the Discard service. It connects to
 * a server running the standard discard service and sends data to it
 * as fast as it can.
 *
 * This program can also be used to "torture" test by setting
 * the amount of data to send appropriately on the command line.
 *
 * Syntax: discard -p [u|t] [-b <numBytes>] [-l <numLoops>] <hostname>
 *					-p [u|t]				# protocol, either UDP or TCP
 *					-b <numBytes>		# number of bytes to send in each loop
 *					-l <numLoops>		# number of loops to do, -1 for infinite
 *					-port <portnum>	# port number to use
 *					<hostname>			# name of host to connect to.
 ********************************************************************/
static void CmdDiscard(argc, argv)
int   argc;
char  **argv;
{
	Boolean	usageErr = false;
	Boolean	useTCP=false;
	int		sendBytes = 100, sentBytes, chunkBytes;
	int		numLoops = 1;
	Char *	hostNameP=0;
	int		fd=-1;

	UInt8 *	srcP = (UInt8 *)0;					// send from address 0.
	Int32		sendTotalBytes=0;
	int		i;
	int		seconds, milliseconds;
	UInt32	totalTicks, startTicks;
	UInt16	portNum = 0;
	Char *	serviceNameP;
	UInt8 *	srcBufP = 0;


	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	//------------------------------------------------------------
	// Get Command line arguments
	//------------------------------------------------------------
	for ( i=1; i<argc; i++) {
		if (!StrCompare(argv[i], "-p")) {
			if (argc > i+1) {
				if (argv[i+1][0] == 't') useTCP = true;
				else useTCP = false;
				i++;
				}
			else
				usageErr = true;
			}

		else if (!StrCompare(argv[i], "-b")) {
			if (argc > i+1) {
				sendBytes = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}
			
		else if (!StrCompare(argv[i], "-port")) {
			if (argc > i+1) {
				portNum = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}

		else if (!StrCompare(argv[i], "-l")) {
			if (argc > i+1) {
				numLoops = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}

		else if (!hostNameP)
			hostNameP = argv[i];
		
		else
			usageErr = true;
		}


	// If error, print syntax
	if (!hostNameP || usageErr)  {
		goto FullHelp;
		}
		
	//-------------------------------------------------------------------
	// Print status
	//------------------------------------------------------------------
	printf("\nSending %d bytes, %d times...\n",
			sendBytes, numLoops);
			
	//-------------------------------------------------------------------
	// Open up the connection
	//------------------------------------------------------------------
	// If no port number specified, use the echo port
	if (!portNum) serviceNameP = "discard";
	else serviceNameP = 0;
	
	if (useTCP) {
		if ( (fd = tcp_open(hostNameP, serviceNameP, portNum)) < 0) {
			err_ret("tcp_open error");
			goto Exit;
			}
		}
	
	else {
		if ( (fd = udp_open(hostNameP, serviceNameP, portNum, false)) < 0) {
			err_ret("udp_open error");
			goto Exit;
			}
		}

	//-------------------------------------------------------------------
	// Loop sending data to server  
	//-------------------------------------------------------------------
	// If we're sending UDP, don't send more than a packet's worth at a time


	startTicks = TimGetTicks();
	do {
		// Make sure we don't turn off
		EvtResetAutoOffTimer();
	
		// If we're sending UDP, don't send more than a 1000 worth at a time
		// Theoretically, we should be able to send the Epilogue max packet size
		//  (about 1500) worth at a time but the Sun Workstation's echo server
		//  seems to max out at 1024.
		sentBytes = 0;
		do {
			chunkBytes = sendBytes - sentBytes;
			if (!useTCP && chunkBytes > 1000) chunkBytes = 1000;
			
			if (writen(fd, srcP, chunkBytes) != chunkBytes) {
				err_ret("write error");
				goto Exit;
				}
			sentBytes += chunkBytes;
			} while(sentBytes < sendBytes);
			
		
		// Print status
		sendTotalBytes += sendBytes;
		printf(".");
			
		if (numLoops > 0) numLoops--;
		} while(numLoops);
		
		
	//-------------------------------------------------------------------
	// Print results
	//-------------------------------------------------------------------
	totalTicks = TimGetTicks() - startTicks;
	seconds = totalTicks/sysTicksPerSecond;
	milliseconds = (totalTicks % sysTicksPerSecond * 1000) / sysTicksPerSecond;
	printf("\n%ld bytes in %d.%d sec", sendTotalBytes, seconds, milliseconds);
	printf("\n  ==> %ld bytes/sec", sendTotalBytes * sysTicksPerSecond /totalTicks);
	
Exit:
	if (fd >=0) close(fd);

	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# Discard client\n", argv[0]);
	return;
	
FullHelp:
	printf("\nDiscard client");
	printf("\nSyntax: %s -p [u|t] [-b <numBytes>] [-l <numLoops>] <hostname>",
						argv[0]);
	printf("\n");

}

/**********************************************************************
 * This program is a client of the CharGen service. It connects to
 * a server running the standard chargen service and reads data 
 *
 * This program can also be used to "torture" test by setting
 * the amount of data to send appropriately on the command line.
 *
 * Syntax: chargen -p [u|t] [-b <numBytes>] [-l <numLoops>] <hostname>
 *					-p [u|t]				# protocol, either UDP or TCP
 *					-b <numBytes>		# number of bytes to read 
 *					-l <numLoops>		# number of loops to do, -1 for infinite
 *					-port <portnum>	# port number to use
 *					<hostname>			# name of host to connect to.
 ********************************************************************/
static void CmdCharGen(argc, argv)
int   argc;
char  **argv;
{
	Boolean	usageErr = false;
	Boolean	useTCP=false;
	int		chunkBytes;
	int		numLoops = 1;
	Char *	hostNameP=0;
	int		fd=-1;

	Int32		rcvTotalBytes=0, rcvBytes=100, receivedBytes;
	UInt8 *	dstBufP = 0;
	int		i;
	int		seconds, milliseconds;
	UInt32	totalTicks, startTicks;
	UInt16	portNum = 0;
	Char *	serviceNameP;
	UInt8 *	srcBufP = 0;
	struct	linger	ling;


	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	//------------------------------------------------------------
	// Get Command line arguments
	//------------------------------------------------------------
	for ( i=1; i<argc; i++) {
		if (!StrCompare(argv[i], "-p")) {
			if (argc > i+1) {
				if (argv[i+1][0] == 't') useTCP = true;
				else useTCP = false;
				i++;
				}
			else
				usageErr = true;
			}

		else if (!StrCompare(argv[i], "-b")) {
			if (argc > i+1) {
				rcvBytes = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}
			
		else if (!StrCompare(argv[i], "-port")) {
			if (argc > i+1) {
				portNum = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}

		else if (!StrCompare(argv[i], "-l")) {
			if (argc > i+1) {
				numLoops = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}

		else if (!hostNameP)
			hostNameP = argv[i];
		
		else
			usageErr = true;
		}


	// If error, print syntax
	if (!hostNameP || usageErr)  {
		goto FullHelp;
		return;
		}
		
	//-------------------------------------------------------------------
	// Print status
	//------------------------------------------------------------------
	// Allocate space for receive data
	dstBufP = MemPtrNew(rcvBytes);
	if (!dstBufP) {
		printf("Out of memory");
		goto Exit;
		}
	printf("Receiving %ld bytes, %d times...\n",
			rcvBytes, numLoops);
			
	//-------------------------------------------------------------------
	// Open up the connection
	//------------------------------------------------------------------
	// If no port number specified, use the chargen port
	if (!portNum) serviceNameP = "chargen";
	else serviceNameP = 0;
	
	if (useTCP) {
		if ( (fd = tcp_open(hostNameP, serviceNameP, portNum)) < 0) {
			err_ret("tcp_open error");
			goto Exit;
			}
		}
	
	else {
		if ( (fd = udp_open(hostNameP, serviceNameP, portNum, false)) < 0) {
			err_ret("udp_open error");
			goto Exit;
			}
		}

	// Set linger time to 0. Otherwise, the server will keep trying to transmit
	//  data to us for a period of time AFTER we close the socket.
  ling.l_onoff = 1;
  ling.l_linger = 0;					/* 0 for abortive disconnect */
  if (setsockopt(fd, SOL_SOCKET, SO_LINGER,
						(char *) &ling, sizeof(ling)) < 0)
      err_sys("SO_LINGER setsockopt error");



	//-------------------------------------------------------------------
	// Loop sending data to server and getting responses
	//-------------------------------------------------------------------
	// If we're sending UDP, don't receive more than a packet's worth at a time
	startTicks = TimGetTicks();
	do {
		// Make sure we don't turn off
		EvtResetAutoOffTimer();
	
	

		// Read available data
		receivedBytes = 0;
		while(receivedBytes < rcvBytes) {

			// If UDP, send a "tickle" packet
			if (!useTCP) {
				if (writen(fd, 0, 1) != 1) {
					err_ret("write error");
					goto Exit;
					}
				}

			// Read now.
			chunkBytes = recv(fd, dstBufP, rcvBytes, 0);
			if (chunkBytes <= 0) {
				printf("receive error");
				goto Exit;
				}
			receivedBytes += chunkBytes;
			}
			
		
		// Print status
		rcvTotalBytes += rcvBytes;
		printf(".");
			
		if (numLoops > 0) numLoops--;
		} while(numLoops);
		
		
	//-------------------------------------------------------------------
	// Print results
	//-------------------------------------------------------------------
	totalTicks = TimGetTicks() - startTicks;
	seconds = totalTicks/sysTicksPerSecond;
	milliseconds = (totalTicks % sysTicksPerSecond * 1000) / sysTicksPerSecond;
	printf("\n%ld bytes in %d.%d sec", rcvTotalBytes, seconds, milliseconds);
	printf("\n  ==> %ld bytes/sec", rcvTotalBytes * sysTicksPerSecond /totalTicks);
	
				
Exit:
	if (fd >=0) close(fd);
	if (dstBufP) MemPtrFree(dstBufP);

	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# CharGen client\n", argv[0]);
	return;
	
FullHelp:
	printf("\nCharGen client");
	printf("\nSyntax: %s -p [u|t] [-b <numBytes>] [-l <numLoops>] <hostname>",
						argv[0]);
	printf("\n");


}



/**********************************************************************
 * This program is a client of the CharGen service and Discard
 *  service at the same time. It opens 2 sockets: 1 that connects
 *	 to the CharGen service and 1 that connects to the Discard service.
 * It then writes and reads from both sockets simultaneously.
 *
 * This program can also be used to "torture" test by setting
 * the amount of data to send appropriately on the command line.
 *
 * Syntax: torture -p [u|t] [-b <numBytes>] [-l <numLoops>] <hostname>
 *					-p [u|t]				# protocol, either UDP or TCP
 *					-b <numBytes>		# number of bytes to read 
 *					-l <numLoops>		# number of loops to do, -1 for infinite
 *					-port <portnum>	# port number to use
 *					<hostname>			# name of host to connect to.
 ********************************************************************/
static void CmdTorture(argc, argv)
int   argc;
char  **argv;
{
	Boolean	usageErr = false;
	Boolean	useTCP=false;
	int		chunkBytes;
	int		numLoops = 1;
	Char *	hostNameP=0;
	int		fdTx=-1, fdRx=-1;

	Int32		rcvTotalBytes=0, rcvBytes=100, receivedBytes;
	UInt8 *	dstBufP = 0;
	
	UInt8 *	srcP = (UInt8 *)0;
	Int32		sendTotalBytes = 0;
	int		sendBytes = 100, sentBytes;
	
	
	int		i;
	int		seconds, milliseconds;
	UInt32	totalTicks, startTicks;
	UInt16	portNum = 0;
	Char *	serviceNameP;
	UInt8 *	srcBufP = 0;
	struct	linger	ling;


	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	//------------------------------------------------------------
	// Get Command line arguments
	//------------------------------------------------------------
	for ( i=1; i<argc; i++) {
		if (!StrCompare(argv[i], "-p")) {
			if (argc > i+1) {
				if (argv[i+1][0] == 't') useTCP = true;
				else useTCP = false;
				i++;
				}
			else
				usageErr = true;
			}

		else if (!StrCompare(argv[i], "-b")) {
			if (argc > i+1) {
				rcvBytes = sendBytes = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}
			
		else if (!StrCompare(argv[i], "-port")) {
			if (argc > i+1) {
				portNum = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}

		else if (!StrCompare(argv[i], "-l")) {
			if (argc > i+1) {
				numLoops = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}

		else if (!hostNameP)
			hostNameP = argv[i];
		
		else
			usageErr = true;
		}


	// If error, print syntax
	if (!hostNameP || usageErr)  {
		goto FullHelp;
		return;
		}
		
		
	//-------------------------------------------------------------------
	// Print status
	//------------------------------------------------------------------
	// Allocate space for receive data
	dstBufP = MemPtrNew(rcvBytes);
	if (!dstBufP) {
		printf("Out of memory");
		goto Exit;
		}
	printf("Sending & Receiving %ld bytes, %d times...\n",
			rcvBytes, numLoops);
			
			
	//-------------------------------------------------------------------
	// Open up the connection to the discard service
	//------------------------------------------------------------------
	// If no port number specified, use the chargen port
	if (!portNum) serviceNameP = "discard";
	else serviceNameP = 0;
	
	if (useTCP) {
		if ( (fdTx = tcp_open(hostNameP, serviceNameP, portNum)) < 0) {
			err_ret("tcp_open error");
			goto Exit;
			}
		}
	
	else {
		if ( (fdTx = udp_open(hostNameP, serviceNameP, portNum, false)) < 0) {
			err_ret("udp_open error");
			goto Exit;
			}
		}

	// Set linger time to 0. Otherwise, the server will keep trying to transmit
	//  data to us for a period of time AFTER we close the socket.
	ling.l_onoff = 1;
	ling.l_linger = 0;					/* 0 for abortive disconnect */
	if (setsockopt(fdTx, SOL_SOCKET, SO_LINGER,
						(char *) &ling, sizeof(ling)) < 0)
	   err_sys("SO_LINGER setsockopt error");


	//-------------------------------------------------------------------
	// Open up the connection to the chargen service
	//------------------------------------------------------------------
	// If no port number specified, use the chargen port
	if (!portNum) serviceNameP = "chargen";
	else serviceNameP = 0;
	
	if (useTCP) {
		if ( (fdRx = tcp_open(hostNameP, serviceNameP, portNum)) < 0) {
			err_ret("tcp_open error");
			goto Exit;
			}
		}
	
	else {
		if ( (fdRx = udp_open(hostNameP, serviceNameP, portNum, false)) < 0) {
			err_ret("udp_open error");
			goto Exit;
			}
		}

	// Set linger time to 0. Otherwise, the server will keep trying to transmit
	//  data to us for a period of time AFTER we close the socket.
	ling.l_onoff = 1;
	ling.l_linger = 0;					/* 0 for abortive disconnect */
	if (setsockopt(fdRx, SOL_SOCKET, SO_LINGER,
						(char *) &ling, sizeof(ling)) < 0)
	   err_sys("SO_LINGER setsockopt error");



	//-------------------------------------------------------------------
	// Loop sending data to server and getting responses
	//-------------------------------------------------------------------
	// If we're sending UDP, don't receive more than a packet's worth at a time
	startTicks = TimGetTicks();
	do {
		// Make sure we don't turn off
		EvtResetAutoOffTimer();
	
	
		// If we're sending UDP, don't send more than a 1000 worth at a time
		// Theoretically, we should be able to send the Epilogue max packet size
		//  (about 1500) worth at a time but the Sun Workstation's echo server
		//  seems to max out at 1024.
		sentBytes = 0;
		do {
			chunkBytes = sendBytes - sentBytes;
			if (!useTCP && chunkBytes > 1000) chunkBytes = 1000;
			
			if (writen(fdTx, srcP, chunkBytes) != chunkBytes) {
				err_ret("write error");
				goto Exit;
				}
			sentBytes += chunkBytes;
			} while(sentBytes < sendBytes);
		sendTotalBytes += sendBytes;
		

		// Read available data
		receivedBytes = 0;
		while(receivedBytes < rcvBytes) {

			// If UDP, send a "tickle" packet
			if (!useTCP) {
				if (writen(fdRx, 0, 1) != 1) {
					err_ret("write error");
					goto Exit;
					}
				}

			// Read now.
			chunkBytes = recv(fdRx, dstBufP, rcvBytes, 0);
			if (chunkBytes <= 0) {
				printf("receive error");
				goto Exit;
				}
			receivedBytes += chunkBytes;
			}
		rcvTotalBytes += rcvBytes;
			
		
		// Print status
		printf(".");
			
		if (numLoops > 0) numLoops--;
		} while(numLoops);
		
		
	//-------------------------------------------------------------------
	// Print results
	//-------------------------------------------------------------------
	totalTicks = TimGetTicks() - startTicks;
	seconds = totalTicks/sysTicksPerSecond;
	milliseconds = (totalTicks % sysTicksPerSecond * 1000) / sysTicksPerSecond;
	printf("\n%ld bytes in %d.%d sec", rcvTotalBytes*2, seconds, milliseconds);
	printf("\n  ==> %ld bytes/sec", rcvTotalBytes*2 * sysTicksPerSecond /totalTicks);
	
				
Exit:
	if (fdTx >=0) close(fdTx);
	if (fdRx >=0) close(fdRx);
	if (dstBufP) MemPtrFree(dstBufP);

	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t# CharGen & Discard client\n", argv[0]);
	return;
	
FullHelp:
	printf("\nCharGen & Discard client");
	printf("\nSyntax: %s -p [u|t] [-b <numBytes>] [-l <numLoops>] <hostname>",
						argv[0]);
	printf("\n");


}



/**********************************************************************
 * This program talks to a custom server on the remote host that
 *  simply spits data out till the connection is closed.
 *
 * This program can also be used to "torture" test by setting
 * the amount of data to read appropriately on the command line.
 *
 * Syntax: read -p [u|t] -port <portnum> [-b <numBytes>] <hostname>
 *					-p [u|t]				# protocol, either UDP or TCP
 *					-b <numBytes>		# number of bytes to read at a time.
 *					-port <portnum>	# port number to use
 *					<hostname>			# name of host to connect to.
 ********************************************************************/
static void CmdRead(argc, argv)
int   argc;
char  **argv;
{
	Boolean	usageErr = false;
	Boolean	useTCP=true;
	int		readBytes = 100, chunkBytes;
	Char *	hostNameP=0;
	int		fd=-1;

	UInt8 *	srcP = (UInt8 *)0;					// send from address 0.
	Int32		readTotalBytes=0, rcvBytes;
	const		int dstBufSize = 0x200;
	UInt8 *	dstBufP = 0;
	
	int		i;
	int		seconds, milliseconds;
	UInt32	totalTicks, startTicks;
	UInt16	portNum = 0;
	Char *	serviceNameP;
	UInt8 *	srcBufP = 0;


	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;


	//------------------------------------------------------------
	// Get Command line arguments
	//------------------------------------------------------------
	for ( i=1; i<argc; i++) {
		if (!StrCompare(argv[i], "-p")) {
			if (argc > i+1) {
				if (argv[i+1][0] == 't') useTCP = true;
				else useTCP = false;
				i++;
				}
			else
				usageErr = true;
			}

		else if (!StrCompare(argv[i], "-b")) {
			if (argc > i+1) {
				readBytes = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}
			
		else if (!StrCompare(argv[i], "-port")) {
			if (argc > i+1) {
				portNum = StrAToI(argv[i+1]);
				i++;
				}
			else
				usageErr = true;
			}

		else if (!hostNameP)
			hostNameP = argv[i];
		
		else
			usageErr = true;
		}


	// If error, print syntax
	if (!hostNameP || usageErr || !portNum)  {
		goto FullHelp;
		}
		
	//-------------------------------------------------------------------
	// Print status
	//------------------------------------------------------------------
	
	// Allocate space for receive data
	dstBufP = MemPtrNew(dstBufSize);
	if (!dstBufP) {
		printf("\nOut of memory");
		goto Exit;
		}
			
	//-------------------------------------------------------------------
	// Open up the connection
	//------------------------------------------------------------------
	// If no port number specified, use the echo port
	serviceNameP = 0;
	
	if (useTCP) {

		if ( (fd = tcp_open(hostNameP, serviceNameP, portNum)) < 0) {
			err_ret("tcp_open error");
			goto Exit;
			}

		// Print status...
		printf("\nReading %d bytes at a time\n", readBytes);
		}
	
	else {

		if ( (fd = udp_open(hostNameP, serviceNameP, portNum, false)) < 0) {
			err_ret("udp_open error");
			goto Exit;
			}

		// For UDP, send something to start the server spitting back
		if (writen(fd, dstBufP, 1) != 1) {
			err_ret("udp_write error");
			goto Exit;
			}

		// Print status...
		//printf("\nDelaying 5 sec. to force overflow\n");
		//SysTaskDelay(sysTicksPerSecond*5);
		
		printf("\nReading datagrams till time-out...\n", readBytes);
		}

	//-------------------------------------------------------------------
	// Loop reading data from server 
	//-------------------------------------------------------------------
	// If we're sending UDP, don't send more than a packet's worth at a time

	startTicks = TimGetTicks();
	do {
		// Make sure we don't turn off
		EvtResetAutoOffTimer();
	
		// Read available data
		rcvBytes = 0;
		while(rcvBytes < readBytes) {
			chunkBytes = recv(fd, dstBufP, dstBufSize, 0);
			if (chunkBytes <= 0) {
				if (chunkBytes < 0) printf("receive error");
				break;
				}
			rcvBytes += chunkBytes;
			}
			
		
		// Print status
		readTotalBytes += rcvBytes;
		printf(".");
			
		} while(chunkBytes > 0);
		
		
	//-------------------------------------------------------------------
	// Print results
	//-------------------------------------------------------------------
	totalTicks = TimGetTicks() - startTicks;
	
	// If this was a UDP test, take out the timeout value we used to account
	// for the last read that failed.
	if (!useTCP) totalTicks -= AppNetTimeout;
	
	seconds = totalTicks/sysTicksPerSecond;
	milliseconds = (totalTicks % sysTicksPerSecond * 1000) / sysTicksPerSecond;
	printf("\n%ld bytes in %d.%d sec", readTotalBytes, seconds, milliseconds);
	printf("\n  ==> %ld bytes/sec", readTotalBytes * sysTicksPerSecond /totalTicks);
	
Exit:
	if (fd >=0) close(fd);
	if (dstBufP) MemPtrFree(dstBufP);
	printf("\n");
	return;
	
ShortHelp:
	printf("%s\t\t\t# Read client\n", argv[0]);
	return;
	
FullHelp:
	printf("\nRead client");
	printf("\nSyntax: %s -port <portnum>  [-b <numBytes>] [-l <numLoops>] <hostname>",
						argv[0]);
	printf("\n");
				

}

/***********************************************************************
 *
 * FUNCTION:   	CmdStevensInstall
 *
 * DESCRIPTION: 	Installs the commands from this module into the
 *		master command table used by AppProcessCommand.
 *
 *	CALLED BY:		AppStart 
 *
 * RETURNED:    	void
 *
 ***********************************************************************/
void CmdStevensInstall(void)
{
	AppAddCommand("_", CmdDivider);

#if	INCLUDE_HOSTENT
	AppAddCommand("hostent", CmdHostEnt);
#endif

#if	INCLUDE_DAYTIME
	AppAddCommand("daytime", CmdDaytime);
#endif

#if	INCLUDE_TIMER
	AppAddCommand("timer", CmdTimer);
#endif

#if	INCLUDE_ECHO
	AppAddCommand("echo", CmdEcho);
#endif

#if	INCLUDE_DISCARD
	AppAddCommand("discard", CmdDiscard);
#endif

#if	INCLUDE_READ
	AppAddCommand("read", CmdRead);
#endif

#if	INCLUDE_CHARGEN
	AppAddCommand("chargen", CmdCharGen);
#endif

#if	INCLUDE_TORTURE
	AppAddCommand("torture", CmdTorture);
#endif
}

