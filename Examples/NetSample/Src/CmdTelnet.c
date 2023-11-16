/******************************************************************************
 *
 * Copyright (c) 1994-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: CmdTelnet.c
 *
 * Description:
 * This module provides the "telnet" command to the NetSample application. 
 * This is a very minimalistic Telnet implementation that demonstrates
 * how to use the Net Library. 
 *
 *****************************************************************************/

// Include the PalmOS headers
#include <PalmOS.h>

// Socket Equates
#include <sys_socket.h>

// StdIO header
#include "AppStdIO.h"

// Application headers
#include "NetSample.h"

// Telnet equates
#include "CmdTelnet.h"

// Pilot Equates
//#include	<UIAll.h>

/***********************************************************************
 * Globals used by telnet. 
 ***********************************************************************/
static TnTxStateEnum		TnTxState = tnTxStateRemote;
static TnRxStateEnum		TnRxState = tnRxStateData;
static TnSnStateEnum		TnSnState = tnSnStateStart;
static TnCmdEnum			TnOptionCmd;				// has value tnCmd: Do,Dont,Will,Wont

static Boolean				TnRemoteEcho = false;		// true if local echo.
static Boolean				TnNoGoAhead = false;
static Boolean				TnRcvBinary = false;
static Boolean				TnSndBinary = false;
static Boolean				TnDoTermType = false;
static Boolean				TnSynching = false;


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
		printf("\n-- Telnet --\n");
}


/***********************************************************************
 *
 * FUNCTION:   PrvSnNextState 
 *
 * DESCRIPTION: Calculates actions and next state for the option
 *			sub-negotiation state machine. This state machine takes input 
 *			from the socket and updates local option information. 
 *
 *
 *	CALLED BY:	PrvSnHandleInput  
 *
 * PARAMETERS:
 *			stateP	- pointer to current state, updated on exit
 *			inChar	- input character to process
 *
 * RETURNED:   bitfield of actions to take (tnTxActXXX).
 *
 ***********************************************************************/
static UInt16 PrvSnNextState(TnSnStateEnum* stateP, UInt8	inChar)
{

	switch (*stateP) {
	
		case tnSnStateStart:
			switch (inChar) {
				case tnOptTType:		*stateP = tnSnStateTermType; return 0;
				default: 				*stateP = tnSnStateEnd; return 0;
				}
			break;
			
		case tnSnStateTermType:
			switch (inChar) {
				case tnQualSend:		*stateP = tnSnStateEnd; return tnSnActSubTermType;
				default:					*stateP = tnSnStateEnd; return 0;
				}
			break;
			
		case tnSnStateEnd:
			return 0;
			break;
		}
			
	ErrDisplay("bad state machine");
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:   PrvSnHandleInput 
 *
 * DESCRIPTION: Gets called by the PrvRxHandleInput routine when it must
 *		process a sub-negotiated option like terminal type. 
 *
 *		Updates the global variable TnSubnegState with new state.
 *
 *	CALLED BY:	CmdTelnet
 *
 * PARAMETERS:
 *			sock	- socket refNum of socket connected to Telnet server
 *			bufP	- buffer of data to process
 *			cc		- number of characters in bufP
 *
 * RETURNED:   void
 *
 ***********************************************************************/
static void PrvSnHandleInput(NetSocketRef sock,  UInt8 * bufP, UInt16 cc)
{
	UInt16	i;
	UInt16	actions;
	UInt8		sendBuf[16];
	Char *	termType = "VT52";

	// For each character in the input stream...
	for (i=0; i<cc; i++) {
		UInt8	c = bufP[i];
		
		// Calculate the next state
		actions = PrvSnNextState(&TnSnState, c);
		
		// Have received, tnCmdIAC.tnCmdSB.tnOptTType.tnQualSend. Send
		//  back our terminal type
		if (actions & tnSnActSubTermType) {
			sendBuf[0] = tnCmdIAC;
			sendBuf[1] = tnCmdSB;
			sendBuf[2] = tnOptTType;
			sendBuf[3] = tnQualIs;
			NetUWriteN(sock, sendBuf, 4);
			NetUWriteN(sock, (UInt8 *)termType, StrLen(termType));
			sendBuf[0] = tnCmdIAC;
			sendBuf[1] = tnCmdSE;
			NetUWriteN(sock, sendBuf, 2);
			}
			
		}
}




/***********************************************************************
 *
 * FUNCTION:   PrvTxNextState 
 *
 * DESCRIPTION: Calculates actions and next state for the socket transmit
 *			state machine. This state machine takes input from the user and
 *			generally sends output to the server through the socket.
 *
 *	CALLED BY:	
 *
 * PARAMETERS:
 *			stateP	- pointer to current state, updated on exit
 *			inChar	- input character to process
 *
 * RETURNED:   bitfield of actions to take (tnTxActXXX).
 *
 ***********************************************************************/
static UInt16 PrvTxNextState(TnTxStateEnum* stateP, UInt8	inChar)
{

	switch (*stateP) {
	
		case tnTxStateRemote:
			switch (inChar) {
				case tnKbdCmdEscape:		*stateP = tnTxStateLocal; return 0;
				default: 														return tnTxActSktPutC;
				}
			break;
			
		case tnTxStateLocal:
			switch (inChar) {
				case tnKbdCmdScript:		*stateP = tnTxStateCollect; return tnTxActScrInit;
				case tnKbdCmdSuspend:	*stateP = tnTxStateRemote; return tnTxActSuspend;
				case tnKbdCmdDiscEsc:	*stateP = tnTxStateRemote; return tnTxActDiscEscOn;
				case tnKbdCmdStatus:		*stateP = tnTxStateRemote; return tnTxActStatus;
				case tnKbdCmdEscape:		*stateP = tnTxStateRemote; return tnTxActSktPutC;
				case tnKbdCmdUnScript:	*stateP = tnTxStateRemote; return tnTxActUnScript;
				default:						*stateP = tnTxStateRemote; return tnTxActNotSupported;
				}
			break;
			
		case tnTxStateCollect:
			switch (inChar) {
				case tnKbdCmdNewLine:	*stateP = tnTxStateRemote; return tnTxActScrWrap;
				default:															return tnTxActScrGetC;
				}
			break;
		}
			
	ErrDisplay("bad state machine");
	return 0;
}

/***********************************************************************
 *
 * FUNCTION:   PrvTxHandleInput 
 *
 * DESCRIPTION: Gets called by the main loop of Telnet when we
 *		get input from the user that we need to process. This routine
 *		will feed the input into the Tx State Machine, calculate the
 *		next state, and usually send data to the server through the
 *		socket. 
 *
 *		Updates the global variable TnTxState with new state.
 *
 *	CALLED BY:	CmdTelnet
 *
 * PARAMETERS:
 *			sock	- socket refNum of socket connected to Telnet server
 *			bufP	- buffer of data to process
 *			cc		- number of characters in bufP
 *
 * RETURNED:   void
 *
 ***********************************************************************/
static void PrvTxHandleInput(NetSocketRef sock,  UInt8 * bufP, UInt16 cc)
{
	UInt16	i;
	UInt16	actions;


	// For each character in the input stream...
	for (i=0; i<cc; i++) {
		UInt8	c = bufP[i];
		
		// Calculate the next state
		actions = PrvTxNextState(&TnTxState, c);
		
		if (actions & tnTxActDiscEscOn) 
			;
			
			
		//---------------------------------------------------------------------
		// Send character to server
		//---------------------------------------------------------------------
		if (actions & tnTxActSktPutC) {
		
			// BINARY mode
			if (TnSndBinary) {
				// UInt8-stuff IAC
				if (c == tnCmdIAC) write(sock, &c, 1);
				
				// Precede line-feed with carriage return
				if (c == '\n') {
					UInt8 cr = '\r';
					write(sock, &cr, 1);
					}
				write(sock, &c, 1);
				
				}
				
			// 7-bit ASCII 
			else {
				c &= 0x7F;						// 7-bit ASCII only
				
				// Look for special characters that we must escape
				if (c == 8) {
					c = tnCmdIAC;
					write(sock, &c, 1);
					c = tnCmdEC;
					}
				write(sock, &c, 1);
				}


			// If local echo, display locally too
			if (!TnRemoteEcho) {
				Char	str[2];
				str[0] = c;
				str[1] = 0;
				StdPutS(str);
				}
			}
			
			
		if (actions & tnTxActScrGetC) 
			;
		if (actions & tnTxActScrInit) 
			;
		if (actions & tnTxActScrWrap) 
			;
		if (actions & tnTxActNotSupported) 
			;
		if (actions & tnTxActStatus) 
			;
		if (actions & tnTxActSuspend) 
			;
		if (actions & tnTxActUnScript) 
			;
		}
}




/***********************************************************************
 *
 * FUNCTION:   PrvRxNextState 
 *
 * DESCRIPTION: Calculates actions and next state for the socket receive
 *			state machine. This state machine receives input from the telnet
 *			server through the socket and usually sends output to the
 *			local display. 
 *
 *	CALLED BY:	
 *
 * PARAMETERS:
 *			stateP	- pointer to current state, updated on exit
 *			inChar	- input character to process
 *
 * RETURNED:   bitfield of actions to take (tnRxActXXX).
 *
 ***********************************************************************/
static UInt16 PrvRxNextState(TnRxStateEnum* stateP, UInt8	inChar)
{

	switch (*stateP) {
	
		case tnRxStateData:
			switch (inChar) {
				case tnCmdIAC:		*stateP = tnRxStateIAC; return 0;
				default: 											return tnRxActTTPutC;
				}
			break;
			
		case tnRxStateIAC:
			switch (inChar) {
				case tnCmdIAC: 	*stateP = tnRxStateData; return tnRxActTTPutC;
				case tnCmdSB:		*stateP = tnRxStateSubNeg; return 0;
				case tnCmdNOP: 	*stateP = tnRxStateData; return 0;
				case tnCmdDM:  	*stateP = tnRxStateData; return tnRxActTCDM;
				case tnCmdWill:	*stateP = tnRxStateWOpt; return tnRxActRecordOption;
				case tnCmdWont:	*stateP = tnRxStateWOpt; return tnRxActRecordOption;
				case tnCmdDo:		*stateP = tnRxStateDOpt; return tnRxActRecordOption;
				case tnCmdDont:	*stateP = tnRxStateDOpt; return tnRxActRecordOption;
				default: 			*stateP = tnRxStateData; return 0;
				}
			break;
			
		case tnRxStateWOpt:
			switch (inChar) {
				case tnOptEcho:	*stateP = tnRxStateData; return tnRxActDoEcho;
				case tnOptNoGA: 	*stateP = tnRxStateData; return tnRxActDoNoGA;
				case tnOptTxBinary:*stateP = tnRxStateData; return tnRxActDoTxBinary;
				default:				*stateP = tnRxStateData; return tnRxActDoNotSupp;
				}
			break;
			
		case tnRxStateDOpt:
			switch (inChar) {
				//DEBUG!!case tnOptTType:	*stateP = tnRxStateData; return tnRxActWillTermType;
				case tnOptTxBinary: 	*stateP = tnRxStateData; return tnRxActWillTxBinary;
				default:					*stateP = tnRxStateData; return tnRxActWillNotSupp;
				}
			break;

		case tnRxStateSubNeg:
			switch (inChar) {
				case tnCmdIAC:		*stateP = tnRxStateSubIAC; return 0;
				default:				*stateP = tnRxStateSubNeg; return tnRxActSubOpt;
				}
			break;
			
		case tnRxStateSubIAC:
			switch (inChar) {
				case tnCmdSE:		*stateP = tnRxStateData; return tnRxActSubEnd;
				default:				*stateP = tnRxStateSubNeg; return tnRxActSubOpt;
				}
			break;
			
		}
		
	ErrDisplay("bad state machine");
	return 0;
}




/***********************************************************************
 *
 * FUNCTION:   PrvRxDoTxBinary 
 *
 * DESCRIPTION: Gets called by the PrvRxHandleInput in order to process
 *		the tnRxActDoTxBinary action code. This routine will send
 *		a DO or DONT as appropriate to the server. 
 *
 *	CALLED BY:	PrvRxHandleInput
 *
 * PARAMETERS:
 *			sock	- socket refNum of socket connected to Telnet server
 *			c		- last character received from server.
 *
 * RETURNED:   void
 *
 ***********************************************************************/
static void PrvRxDoTxBinary(NetSocketRef sock, Int16 c)
{
	UInt8	sendBuf[8];
	
	
	if (TnRcvBinary) {
		if (TnOptionCmd == tnCmdWill) return;
		}
	else if (TnOptionCmd == tnCmdWont) return;
	
	// Change our  mode
	sendBuf[0] = tnCmdIAC;
	if (TnOptionCmd == tnCmdWill) {
		TnRcvBinary = true;
		sendBuf[1] = tnCmdDo;
		}
	else {
		TnRcvBinary = false;
		sendBuf[1] = tnCmdDont;
		}
	sendBuf[2] = tnOptTxBinary;
	
	// Send the command to server
	NetUWriteN(sock, sendBuf, 3);
}


/***********************************************************************
 *
 * FUNCTION:   PrvRxWillTxBinary 
 *
 * DESCRIPTION: Gets called by the PrvRxHandleInput in order to process
 *		the tnRxActWillTxBinary action code. This routine will send
 *		a WILL or WONT as appropriate to the server given the last
 *		command verb (DO or DONT). 
 *
 *	CALLED BY:	PrvRxHandleInput
 *
 * PARAMETERS:
 *			sock	- socket refNum of socket connected to Telnet server
 *			c		- last character received from server.
 *
 * RETURNED:   void
 *
 ***********************************************************************/
static void PrvRxWillTxBinary(NetSocketRef sock, Int16 c)
{
	UInt8	sendBuf[8];
	
	if (TnSndBinary) {
		if (TnOptionCmd == tnCmdDo) return;
		}
	else if (TnOptionCmd == tnCmdDont) return;
	
	// Change our  mode
	sendBuf[0] = tnCmdIAC;
	if (TnOptionCmd == tnCmdDo) {
		TnSndBinary = true;
		sendBuf[1] = tnCmdWill;
		}
	else {
		TnRcvBinary = false;
		sendBuf[1] = tnCmdWont;
		}
	sendBuf[2] = tnOptTxBinary;
	
	// Send the command to server
	NetUWriteN(sock, sendBuf, 3);
}



/***********************************************************************
 *
 * FUNCTION:   PrvTermFlush
 *
 * DESCRIPTION: Called by PrvRxHandleInput to flush any data
 *		that's been buffered up for the display.
 *
 *
 *	CALLED BY:	PrvRxHandleInput to send characters to display
 *
 * PARAMETERS:
 *
 * RETURNED:   void
 *
 ***********************************************************************/
typedef enum {
	trmStateNormal,			// normal mode
	trmStateEscape ,			// encountered escape
	trmStateEscBrkt,			// encountered escape-[
	trmStateEscBrktQuest,	// encountered escape-[-?
	trmStateEscLeftParen,	// encountered escape-<
	trmStateEscRightParen	// encountered escape-)
	} TrmStateEnum;
#define	kEsc 	27				// escape character
#define	kTrmBufSize  0x0FF

static UInt8				TrmBuf[kTrmBufSize+1];
static UInt16				TrmBufBytes = 0;
static TrmStateEnum	TrmState;


static void PrvTermFlush(void)
{
	if (TrmBufBytes) {
		TrmBuf[TrmBufBytes] = 0;
		StdPutS((Char *)TrmBuf);
		TrmBufBytes = 0;
		}
}



/***********************************************************************
 *
 * FUNCTION:   PrvTermPutC 
 *
 * DESCRIPTION: Called by PrvRxHandleInput to output a character, which
 *		may be part of a control escape sequence, to the display.
 *		This routine will process VT52 escape sequences as appropriate.
 *
 *		This call will buffer characters. PrvTermFlush can be called
 *		to flush any buffered data to the display.
 *
 *	CALLED BY:	PrvRxHandleInput to send characters to display
 *
 * PARAMETERS:
 *			c		- character to send
 *
 * RETURNED:   void
 *
 ***********************************************************************/
static void PrvTermPutC(UInt8 c)
{
	
	// If no room left in buffer, flush it
	if (TrmBufBytes >= kTrmBufSize-8) PrvTermFlush();
	
	switch (TrmState) {
		case trmStateNormal:
			if (c == kEsc) {
				TrmBuf[TrmBufBytes++] = '^';
				TrmBuf[TrmBufBytes++] = '[';
				return;
				}
			break;
			
		case trmStateEscape:
			switch (c) {
				case '[': TrmState = trmStateEscBrkt; return;
				}
			break;
		}
				


	// We fall through to here if there is no escape processing to be done
	if (c == 8) {
		PrvTermFlush();
		StdBS();
		}
	else if (c != '\r' && c != 0)
		TrmBuf[TrmBufBytes++] = c;

}



/***********************************************************************
 *
 * FUNCTION:   PrvRxHandleInput 
 *
 * DESCRIPTION: Gets called by the main loop of Telnet when we
 *		get input from the telnet server that we need to process. This routine
 *		will feed the input into the Rx State Machine, calculate the
 *		next state, and usually print to the display.
 *
 *		Updates the global variable TnRxState with new state.
 *
 *	CALLED BY:	CmdTelnet to respond to input from the server.
 *
 * PARAMETERS:
 *			sock	- socket refNum of socket connected to Telnet server
 *			bufP	- buffer of data to process
 *			cc		- number of characters in bufP
 *
 * RETURNED:   void
 *
 ***********************************************************************/
static void PrvRxHandleInput(NetSocketRef sock,  UInt8 * bufP, UInt16 cc)
{
	UInt16	i;
	UInt16	actions;
	UInt8		sendBuf[8];
	
	//===================================================================
	// For each character in the input stream...
	//===================================================================
	for (i=0; i<cc; i++) {
		UInt8	c = bufP[i];
		
		// Calculate the next state
		actions = PrvRxNextState(&TnRxState, c);
		
		//------------------------------------------------------------------
		// Print character to display.
		//------------------------------------------------------------------
		if (actions & tnRxActTTPutC) { 
			
			// No data, if in sync
			if (TnSynching) continue;
			
			// Concatenate data to send to printf to cut down on overhead	
			PrvTermPutC(c);
			}


		//------------------------------------------------------------------
		// Record option. This records the option verb which is either
		//  DO, DONT, WILL, or WONT
		//------------------------------------------------------------------
		if (actions & tnRxActRecordOption) {
			TnOptionCmd = (TnCmdEnum)c;
			}
			
			
		//------------------------------------------------------------------
		// Respond to the Echo option. The server sends WILL or WONT followed
		//  by ECHO option to inform the client that it is willing to echo 
		//  characters or is willing to stop echoing characters. If we
		//  end up changing our mode as a result, we send a DO or DONT. 
		//------------------------------------------------------------------
		if (actions & tnRxActDoEcho) {
			
			if (TnRemoteEcho) {
				if (TnOptionCmd == tnCmdWill) continue;	// already using remote echo
				}
			else 
				if (TnOptionCmd == tnCmdWont) continue;	// already not echoing
				
			
			// Change our echo mode
			sendBuf[0] = tnCmdIAC;
			if (TnOptionCmd == tnCmdWill) {
				TnRemoteEcho = true;
				sendBuf[1] = tnCmdDo;
				}
			else {
				TnRemoteEcho = false;
				sendBuf[1] = tnCmdDont;
				}
			sendBuf[2] = tnOptEcho;
			
			// Send the command to server
			NetUWriteN(sock, sendBuf, 3);
			}
			
		//------------------------------------------------------------------
		// When we receive a WILL or WONT for an option that we don't understand
		//  we respond with a DONT
		//------------------------------------------------------------------
		if (actions & tnRxActDoNotSupp) {
			sendBuf[0] = tnCmdIAC;
			sendBuf[1] = tnCmdDont;
			sendBuf[2] = c;
			NetUWriteN(sock, sendBuf, 3);
			}
			
			
		//------------------------------------------------------------------
		// When we receive a WILL or WONT for the no go-ahead option
		//------------------------------------------------------------------
		if (actions & tnRxActDoNoGA) {

			if (TnNoGoAhead) {
				if (TnOptionCmd == tnCmdWill) continue;
				}
			else if (TnOptionCmd == tnCmdWont) continue;
			
			// Change our  mode
			sendBuf[0] = tnCmdIAC;
			if (TnOptionCmd == tnCmdWill) {
				TnNoGoAhead = true;
				sendBuf[1] = tnCmdDo;
				}
			else {
				TnNoGoAhead = false;
				sendBuf[1] = tnCmdDont;
				}
			sendBuf[2] = tnOptNoGA;
			
			// Send the command to server
			NetUWriteN(sock, sendBuf, 3);
			}
			
		//------------------------------------------------------------------
		// When we receive a WILL or WONT for the Transmit-Binary option.
		// We respond with a DO or DONT. 
		//------------------------------------------------------------------
		if (actions & tnRxActDoTxBinary)  {
			PrvRxDoTxBinary(sock, c);
			}
			
		//------------------------------------------------------------------
		// We take this action when we don't support an option that the
		//  server said to DO or DONT
		//------------------------------------------------------------------
		if (actions & tnRxActWillNotSupp) {

			sendBuf[0] = tnCmdIAC;
			sendBuf[1] = tnCmdWont;
			sendBuf[2] = c;
			NetUWriteN(sock, sendBuf, 3);
			}
			
		//------------------------------------------------------------------
		// When the server sends a DO/DONT the transmit binary option, we
		//  take this action and respond with a WILL/WONT
		//------------------------------------------------------------------
		if (actions & tnRxActWillTxBinary)  {
			PrvRxWillTxBinary( sock, c);
			}

		//------------------------------------------------------------------
		// When the server sends a DO or DONT for the terminal type option,
		//  we take this action.
		//------------------------------------------------------------------
		if (actions & tnRxActWillTermType) {

			if (TnDoTermType) {
				if (TnOptionCmd == tnCmdDo) continue;
				}
			else if (TnOptionCmd == tnCmdDont) continue;
			
			// Change our  mode
			sendBuf[0] = tnCmdIAC;
			if (TnOptionCmd == tnCmdDo) {
				TnDoTermType = true;
				sendBuf[1] = tnCmdWill;
				}
			else {
				TnDoTermType = false;
				sendBuf[1] = tnCmdWont;
				}
			sendBuf[2] = tnOptTType;
			
			// Send the command to server
			NetUWriteN(sock, sendBuf, 3);		
			
			// When we advertise our terminal type, we want to be in
			//  8 -bit mode in order to process the control characters. Because
			// the PrvRxDoTxBinary() and PrvRxWillTxBinary() are designed to
			//  process the verb in TnOptionCmd, we must set that global before
			//  we call them. 
			//TnOptionCmd = tnCmdWill;
			//PrvRxDoTxBinary(sock, tnOptTxBinary);		
			//TnOptionCmd = tnCmdDo;
			//PrvRxWillTxBinary(sock, tnOptTxBinary);		
			}
			
			
		//------------------------------------------------------------------
		// When a client agrees to handle the terminal type option, a server
		//  uses option subnegotiation to request the terminal name. 
		//------------------------------------------------------------------
		if (actions & tnRxActSubOpt) {
			PrvSnHandleInput(sock, &c, 1);
			}

		//------------------------------------------------------------------
		// End of sub-negotiations, re-initialize state machine.
		//------------------------------------------------------------------
		if (actions & tnRxActSubEnd) {
			TnSnState = tnSnStateStart;
			}
			
		if (actions & tnRxActTCDM) {
			ErrDisplay("Not Implemented");
			//DOLATER...
			}
		}


	// Print what we have buffered up for the display up till now
	PrvTermFlush();
}

/***********************************************************************
 *
 * FUNCTION:   CmdTelnet()
 *
 * DESCRIPTION: Telnet Client
 *
 *	CALLED BY:	AppProcessCommand in response to the "telnet" command
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void CmdTelnet(int argc, Char * argv[])
{
	Char *		hostname = "localhost";
	Char *		service = "telnet";
	int			sock = -1;
	int			on = 1;
	int			nfds;
	fd_set		arfds, awfds, rfds, wfds;
	const int	bufSize = 0x100;
	UInt8			buf[bufSize];
	int			cc;
	Boolean		controlSequence = false;
	Int16			port = 0;
	
	// Check for help
	if (argc > 1 && !StrCompare(argv[1], "?")) 	goto 	ShortHelp;
	if (argc > 1 && !StrCompare(argv[1], "??")) 	goto 	FullHelp;

	// Get the host name and service name, if any
	if (argc > 1) 	hostname = argv[1];
	if (argc > 2) {
		service = argv[2];
		port = atoi(service);
		if (port > 0) service = 0;
		}
	
	// Init globals used for Telnet
	TnTxState = tnTxStateRemote;
	TnRxState = tnRxStateData;
	TnSnState = tnSnStateStart;
	TnRemoteEcho = false;		// true if remote is echoing for us
	TnNoGoAhead = false;
	TnRcvBinary = false;
	TnSndBinary = false;
	TnDoTermType = false;
	TnSynching = false;
	
	
	// Open up a TCP connection 
	sock = NetUTCPOpen(hostname, service, port);
	if (sock < 0) {
		printf("\nError connecting to %s: %s", hostname, appErrString(errno));
		goto Exit;
		}
		
	// Set socket to receive out-of-band data inline
	setsockopt(sock, SOL_SOCKET, SO_OOBINLINE, (char*)&on, sizeof(on));
	
	
	// Create our file descriptor sets for checking input from the keyboard
	// and the socket simultaneously
	nfds = 0;
	FD_ZERO(&arfds);
	FD_ZERO(&awfds);
	FD_SET(sock, &arfds);
	FD_SET(STDIN_FILENO,  &arfds);
	nfds = sock+1;
	
	
	// Print exit string
	printf("\nWelcome to Telnet");
	printf("\n<'abcde'>-<letter> for ^characters");
	printf("\n<'12345'> for escape character");
	printf("\n^Z to quit\n");
	
	//==================================================================
	// Loop waiting for activity either from the keyboard or from the socket
	//======================================================================
	while(1) {
		bcopy(&arfds, &rfds, sizeof(rfds));
		bcopy(&awfds, &wfds, sizeof(wfds));
		
		if (select(nfds, &rfds, &wfds, (fd_set*)0, (struct timeval*)0) < 0) {
			// just a signal?
			if (errno == EINTR) continue;
			
			printf("\nSelect error: %s", appErrString(errno));
			goto Exit;
			}
			
		//------------------------------------------------------------------
		// See if it was data from the socket
		//------------------------------------------------------------------
		if (FD_ISSET(sock, &rfds)) {
			cc = read (sock, buf, sizeof(buf));
			if (cc < 0) {
				printf("\nSocket Read Error: %s", appErrString(errno));
				goto Exit;
				}
				
			else if (cc == 0) {
				printf("\nConnection closed.");
				goto Exit;
				}
				
			else 
				PrvRxHandleInput(sock, buf, cc);
			}
			
			
		//------------------------------------------------------------------
		// See if there's user input
		//------------------------------------------------------------------
		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			EventType	event;
			
			// Get the event
			EvtGetEvent(&event, 1);
			
			// Filter out the alpha and num keyboard command characters and
			//  use those to generate control characters
			if (event.eType == keyDownEvent) {
				if (event.data.keyDown.chr == keyboardAlphaChr) {
					controlSequence = true;
					continue;
					}
				if (event.data.keyDown.chr == keyboardNumericChr) {
					buf[0] = escapeChr;		// escape key
					PrvTxHandleInput(sock, buf, 1);
					continue;
					}
				}
					
			
			// If not a system event, handle it ourselves.
			if (!SysHandleEvent(&event)) {
			
			
				// Undo the mapping of escape to appStopEvents here
				if (event.eType == appStopEvent) {
					event.eType = keyDownEvent;
					event.data.keyDown.chr = 27;
					}
				
				// If keydown, handle it
				if (event.eType == keyDownEvent) {
					buf[0] = event.data.keyDown.chr;
					if (controlSequence) {
						if (buf[0] >= 'a') buf[0] -= 'a' - 1;
						else buf[0] -= 'A' - 1;
						controlSequence = false;
						}
					if (buf[0] == 26) {
						goto Exit;
						}
					PrvTxHandleInput(sock, buf, 1);
					}
					
				// Else, let form manager handle it.
				else
					FrmDispatchEvent(&event);
				}
				
			}
	
		}
	
	
	//==================================================================
	// Exit now.
	//======================================================================
Exit:
	// Close the connection, if open
	if (sock >= 0) close(sock);
	
	printf("\nExited Telnet.\n");
	return;
	
ShortHelp:
	printf("%s\t\t# Telnet Client\n", argv[0]);
	return;
	
FullHelp:
	printf("\nTelnet Client");
	printf("\nSyntax: %s <server> [<port>]", argv[0]);
	printf("\n");
}


/***********************************************************************
 *
 * FUNCTION:   	CmdTelnetInstall
 *
 * DESCRIPTION: 	Installs the commands from this module into the
 *		master command table used by AppProcessCommand.
 *
 *	CALLED BY:		AppStart 
 *
 * RETURNED:    	void
 *
 ***********************************************************************/
void CmdTelnetInstall(void)
{
	AppAddCommand("_", CmdDivider);

#if INCLUDE_TELNET
	AppAddCommand("telnet", CmdTelnet);
#endif

}

