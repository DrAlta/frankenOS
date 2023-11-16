/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: CmdTelnet.h
 *
 * Description:
 *	  This module defines equates for the Telnet protocol
 *
 * History:
 *		2/14/96	Created by Ron Marianetti
 *
 *****************************************************************************/

#ifndef _CMDTELNET_H__
#define _CMDTELNET_H__

//====================================================================
// Rx State Machine info. 
// This state machine generally takes input from the server through
//  the socket and sends output to the display.
//====================================================================

// States
typedef enum {
	tnRxStateData=0,					// Normal data processing
	tnRxStateIAC=1,					// Have seen IAC
	tnRxStateWOpt=2,					// Have seen IAC-{WILL/WON't}
	tnRxStateDOpt=3,					// Have seen IAC-{DO/DON'T}
	tnRxStateSubNeg=4,				// Have seen IAC-SB
	tnRxStateSubIAC=5					// Have seen IAC-SB-...-IAC
	} TnRxStateEnum;
#define	tnRxNStates		6			// # of states


// Possible actions from the Rx State Machine
#define	tnRxActRecordOption		0x0001		// Record option type
#define	tnRxActDoEcho				0x0002		// handle will/won't echo option
#define	tnRxActDoNotSupp			0x0004		// handle unsupported will/won't option
#define	tnRxActDoNoGA				0x0008		// don't do telnet Go-Aheads
#define	tnRxActDoTxBinary			0x0010		// handle will/won't transmit binary option
#define	tnRxActWillNotSupp		0x0020		// handle unsupported do/don't option
#define	tnRxActWillTxBinary		0x0040		// handle do/don't transmit binary option
#define	tnRxActWillTermType		0x0080		// handle do/don't terminal type option
#define	tnRxActSubOpt				0x0100		// subnegotiation option
#define	tnRxActSubEnd				0x0200		// subnegotiation end
#define	tnRxActTCDM					0x0400		// TC (??) Data Mark
#define	tnRxActTTPutC				0x0800		// print single character to NVT


//====================================================================
// Tx State Machine info. 
// This state machine generally takes input from the user in the form
//  of keyboard commands and sends output to the server through the
//  socket. 
//====================================================================

// States
typedef enum {
	tnTxStateRemote = 0,								// routing input to server
	tnTxStateLocal = 1,								// in local mode
	tnTxStateCollect = 2								// collecting script 
	} TnTxStateEnum;


// Keyboard commands from user. 
#define	tnKbdCmdEscape  			035			// local escape (^])
#define	tnKbdCmdDiscEsc 			'.'			// disconnect escape 
#define	tnKbdCmdSuspend 			032			// suspend session (^Z)
#define	tnKbdCmdScript  			's'			// begin scripting
#define	tnKbdCmdUnScript  		'u'			// end scripting
#define	tnKbdCmdStatus	 			024			// print status (^T)
#define	tnKbdCmdNewLine  			'\n'			// newline


// Possible actions from the Tx State Machine
#define	tnTxActDiscEscOn			0x0001		// Disconnect escape
#define	tnTxActSktPutC				0x0002		// Socket send character
#define	tnTxActScrGetC				0x0004		// Begin session scripting
#define	tnTxActScrInit				0x0008		// Initialize script file collection
#define	tnTxActScrWrap				0x0010		// Wrap-up script filename collection
#define	tnTxActNotSupported		0x0020		// Un-supported keyboard command
#define	tnTxActStatus				0x0040		// Print connection status
#define	tnTxActSuspend				0x0080		// Suspend client process
#define	tnTxActUnScript			0x0100		// End scripting session


//====================================================================
// Option Sub-negotiation State Machine info. 
// This state machine generally takes input from the server through
//  the socket and sends output to the display.
//====================================================================

// States
typedef enum {
	tnSnStateStart=0,					// start
	tnSnStateTermType=1,				// Have seen TermType option
	tnSnStateEnd=2						// end
	} TnSnStateEnum;
#define	tnSnNStates		3			// # of states


// Possible actions from the Rx State Machine
#define	tnSnActSubTermType		0x0001		// Send termtype




//====================================================================
// Commands Received from server.
//====================================================================
typedef enum {
	tnCmdIAC = 	 	 255,				  /* 0xFF, interpret as command: */
	tnCmdDont =	 	 254,				  /* 0xFE, you are not to use option */
	tnCmdDo = 		 253,				  /* 0xFD, please, you use option */
	tnCmdWont =		 252,				  /* 0xFC, I won't use option */
	tnCmdWill = 	 251,				  /* 0xFB, I will use option */
	tnCmdSB = 		 250,				  /* 0xFA, interpret as subnegotiation */
	tnCmdGA = 		 249,				  /* you may reverse the line */
	tnCmdEL =		 248,				  /* erase the current line */
	tnCmdEC = 		 247,				  /* erase the current character */
	tnCmdAYT =  	 246,				  /* are you there */
	tnCmdAO = 		 245,				  /* abort output--but let prog finish */
	tnCmdIP = 		 244,				  /* interrupt process--permanently */
	tnCmdBreak = 	 243,				  /* break */
	tnCmdDM = 		 242,				  /* data mark--for connect. cleaning */
	tnCmdNOP =  	 241,				  /* nop */
	tnCmdSE = 		 240,				  /* 0xF0, end sub negotiation */
	tnCmdEOR =  	 239,				  /* end of record (transparent mode) */
	tnCmdAbort = 	 238,				  /* Abort process */
	tnCmdSusp = 	 237,				  /* Suspend process */
	tnCmdEOF = 	 	 236,				  /* End of file: EOF is already used... */

	tnCmdSynch = 	 242				  	/* for telfunc calls */
	} TnCmdEnum;

#ifdef TELCMDS
char *tnCmds[] = {
		  "EOF", "SUSP", "ABORT", "EOR",
		  "SE", "NOP", "DMARK", "BRK", "IP", "AO", "AYT", "EC",
		  "EL", "GA", "SB", "WILL", "WONT", "DO", "DONT", "IAC", 0,
};
#else
extern char *tnCmds[];
#endif

#define tnCmdFirst 		tnCmdEOF
#define tnCmdLast			tnCmdIAC
#define tnCmdOK(x) 		((x) <= tnCmdLast && (x) >= tnCmdFirst)
#define tnCmd(x) 			tnCmds[(x)-tnCmdFirst]



//====================================================================
// Options
//====================================================================
typedef enum {
		tnOptTxBinary = 	0,		  /* 8-bit data path */
		tnOptEcho = 		1,		  /* echo */
		tnOptRCP = 			2,		  /* prepare to reconnect */
		tnOptNoGA = 		3,		  /* suppress go ahead */
		tnOptNAMS = 		4,		  /* approximate message size */
		tnOptStatus = 		5,		  /* give status */
		tnOptTM =  			6,		  /* timing mark */
		tnOptRCTE = 		7,		  /* remote controlled transmission and echo */
		tnOptNAOL = 		8,		  /* negotiate about output line width */
		tnOptNAOP = 		9,		  /* negotiate about output page size */
		tnOptNAOCRD = 		10, 	  /* negotiate about CR disposition */
		tnOptNAOHTS = 		11 ,	  /* negotiate about horizontal tabstops */
		tnOptNAOHTD = 		12 ,	  /* negotiate about horizontal tab disposition */
		tnOptNAOFFD = 		13 ,	  /* negotiate about formfeed disposition */
		tnOptNAOVTS = 		14 ,	  /* negotiate about vertical tab stops */
		tnOptNAOVTD = 		15 ,	  /* negotiate about vertical tab disposition */
		tnOptNAOLFD = 		16 ,	  /* negotiate about output LF disposition */
		tnOptXASCII = 		17 ,	  /* extended ascic character set */
		tnOptLOGOUT = 		18, 	  /* force logout */
		tnOptBM =  			19 ,	  /* byte macro */
		tnOptDET = 			20 ,	  /* data entry terminal */
		tnOptSUPDUP = 		21 ,	  /* supdup protocol */
		tnOptSUPDUPOutput =  22, /* supdup output */
		tnOptSndLoc = 		23, 	  /* send location */
		tnOptTType =  		24, 	  /* terminal type */
		tnOptEOR = 			25, 	  /* end or record */
		tnOptTUID = 		26, 	  /* TACACS user identification */
		tnOptOutMrk = 		27, 	  /* output marking */
		tnOptTTYLoc = 		28, 	  /* terminal location number */
		tnOpt3270Regime =  29,	  /* 3270 regime */
		tnOptX3Pad =  		30, 	  /* X.3 PAD */
		tnOptNAWS = 		31, 	  /* window size */
		tnOptTSpeed = 		32, 	  /* terminal speed */
		tnOptLFlow =  		33, 	  /* remote flow control */
		tnOptLineMode =  	34, 	  /* Linemode option */
		tnOptXDispLoc =  	35, 	  /* X Display Location */
		tnOptEnvirom = 	36, 	  /* Environment variables */
		tnOptAuthentication =  37,/* Authenticate */
		tnOptEncrypt = 	38, 	  /* Encryption option */
		tnOptEXOPL =  		255	  /* extended-options-list */
		} TnOptEnum;


#define tnNOpts			(1+tnOptEncrypt)
#ifdef TELOPTS
char *tnOpts[tnNOpts+1] = {
		  "BINARY", "ECHO", "RCP", "SUPPRESS GO AHEAD", "NAME",
		  "STATUS", "TIMING MARK", "RCTE", "NAOL", "NAOP",
		  "NAOCRD", "NAOHTS", "NAOHTD", "NAOFFD", "NAOVTS",
		  "NAOVTD", "NAOLFD", "EXTEND ASCII", "LOGOUT", "BYTE MACRO",
		  "DATA ENTRY TERMINAL", "SUPDUP", "SUPDUP OUTPUT",
		  "SEND LOCATION", "TERMINAL TYPE", "END OF RECORD",
		  "TACACS UID", "OUTPUT MARKING", "TTYLOC",
		  "3270 REGIME", "X.3 PAD", "NAWS", "TSPEED", "LFLOW",
		  "LINEMODE", "XDISPLOC", "ENVIRON", "AUTHENTICATION",
		  "ENCRYPT",
		  0,
};
#define tnOptFirst 		tnOptBinary
#define tnOptLast			tnOptEncrypt
#define tnOptOK(x) 		((x) <= tnOptLast && (x) >= tnOptFirst)
#define tnOpt(x) 			tnOpts[(x)-tnOptFirst]
#endif

/* sub-option qualifiers */
typedef enum {
	tnQualIs = 			0,		  	/* option is... */
	tnQualSend	=		1,		  	/* send option */
	tnQualInfo = 		2,		  	/* ENVIRON: informational version of IS */
	tnQualyReply = 	2,		  	/* AUTHENTICATION: client version of IS */
	tnQualName =  		3		  	/* AUTHENTICATION: client version of IS */
	} TnQualEnum;



//====================================================================
// LINEMODE suboptions
//====================================================================
typedef enum {
	lmMode = 1,
	lmForwardMask = 2,
	lmSLC = 3
	} LineModeEnum;

#define lmModeEdit 		0x01
#define lmModeTrapSig 	0x02
#define lmModeAck			0x04
#define lmModeSoftTab	0x08
#define lmModeLitEcho	0x10

#define lmModeMask 		0x1f

/* Not part of protocol, but needed to simplify things... */
#define lmModeFlow 		0x0100
#define lmModeEcho 		0x0200
#define lmModeInBin		0x0400
#define lmModeOutBin		0x0800
#define lmModeForce		0x1000


typedef enum {
	slcSynch = 1,
	slcBrk = 2,
	slcIP = 3,
	slcAO = 4,
	slcAYT = 5,
	slcEOR = 6,
	slcAbort = 7,
	slcEOF = 8,
	slcSusp = 9,
	slcEC = 10,
	slcEL = 11,
	slcEW = 12,
	slcRP = 13,
	slcLNext = 14,
	slcXON = 15,
	slcXOFF = 16,
	slcForw1 = 17,
	slcForw2 = 18
	} SlcEnum;
#define	nSLC 	18


/*
 * For backwards compatability, we define SLC_NAMES to be the
 * list of names if SLC_NAMES is not defined.
 */
#define slcNameList 	"0", "SYNCH", "BRK", "IP", "AO", "AYT", "EOR", \
								"ABORT", "EOF", "SUSP", "EC", "EL", "EW", "RP", \
								"LNEXT", "XON", "XOFF", "FORW1", "FORW2", 0,
#ifdef  SLC_NAMES
char *slc_names[] = {
		  slcNameList
};
#else
extern char *slc_names[];
#define SLC_NAMES slcNameList
#endif

#define slcNameOK	(x)	((x) >= 0 && (x) < nSLC)
#define slcName(x)		slc_names[x]

#define slcNoSupport		0
#define slcCantChange	1
#define slcVariable	 	2
#define slcDefault		3
#define slcLevelBits		0x03

#define slcFunc			0
#define slcFlags 			1
#define slcValue 			2

#define slcAck				0x80
#define slcFlushIn		0x40
#define slcFlushOut	 	0x20

#define envValue	 		0
#define envVar				1
#define envESC				2

/*
 * AUTHENTICATION suboptions
 */

/*
 * Who is authenticating who ...
 */
#define AUTH_WHO_CLIENT 		  0		 /* Client authenticating server */
#define AUTH_WHO_SERVER 		  1		 /* Server authenticating client */
#define AUTH_WHO_MASK			  1

/*
 * amount of authentication done
 */
#define AUTH_HOW_ONE_WAY		  0
#define AUTH_HOW_MUTUAL 		  2
#define AUTH_HOW_MASK			  2

#define AUTHTYPE_NULL			  0
#define AUTHTYPE_KERBEROS_V4	  1
#define AUTHTYPE_KERBEROS_V5	  2
#define AUTHTYPE_SPX 			  3
#define AUTHTYPE_MINK			  4
#define AUTHTYPE_CNT 			  5

#define AUTHTYPE_TEST			  99

#ifdef  AUTH_NAMES
char *authtype_names[] = {
		  "NULL", "KERBEROS_V4", "KERBEROS_V5", "SPX", "MINK", 0,
};
#else
extern char *authtype_names[];
#endif

#define AUTHTYPE_NAME_OK(x)	  ((x) >= 0 && (x) < AUTHTYPE_CNT)
#define AUTHTYPE_NAME(x)		  authtype_names[x]

/*
 * ENCRYPTion suboptions
 */
#define ENCRYPT_IS				  0		 /* I pick encryption type ... */
#define ENCRYPT_SUPPORT 		  1		 /* I support encryption types ... */
#define ENCRYPT_REPLY			  2		 /* Initial setup response */
#define ENCRYPT_START			  3		 /* Am starting to send encrypted */
#define ENCRYPT_END				  4		 /* Am ending encrypted */
#define ENCRYPT_REQSTART		  5		 /* Request you start encrypting */
#define ENCRYPT_REQEND			  6		 /* Request you send encrypting */
#define ENCRYPT_ENC_KEYID		  7
#define ENCRYPT_DEC_KEYID		  8
#define ENCRYPT_CNT				  9

#define ENCTYPE_ANY				  0
#define ENCTYPE_DES_CFB64		  1
#define ENCTYPE_DES_OFB64		  2
#define ENCTYPE_CNT				  3

#ifdef  ENCRYPT_NAMES
char *encrypt_names[] = {
		  "IS", "SUPPORT", "REPLY", "START", "END",
		  "REQUEST-START", "REQUEST-END", "ENC-KEYID", "DEC-KEYID",
		  0,
};
char *enctype_names[] = {
		  "ANY", "DES_CFB64",  "DES_OFB64",  0,
};
#else
extern char *encrypt_names[];
extern char *enctype_names[];
#endif


#define ENCRYPT_NAME_OK(x) 	  ((x) >= 0 && (x) < ENCRYPT_CNT)
#define ENCRYPT_NAME(x) 		  encrypt_names[x]

#define ENCTYPE_NAME_OK(x) 	  ((x) >= 0 && (x) < ENCTYPE_CNT)
#define ENCTYPE_NAME(x) 		  enctype_names[x]
#endif /* _ARPA_TELNET_H */

