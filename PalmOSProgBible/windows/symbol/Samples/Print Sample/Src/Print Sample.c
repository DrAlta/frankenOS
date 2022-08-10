/************************************************************************
* COPYRIGHT:   Copyright  ©  1999 Symbol Technologies, Inc. 
*
* FILE:        Print Sample
*
* SYSTEM:      Symbol Print API for Palm III.
* 
*
* DESCRIPTION: Provides a sample application for the Symbol Print API.*
*
* HISTORY:     03/22/99    MS   Created
*              ...
*************************************************************************/

#include <Pilot.h>
#include <ptPrint.h>
#include "Print Sample.h"
#include "about.h"
#include "info.h"
#include "Print SampleRsc.h"

/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
 
#define BuffSize				256

#define	sleep(x)				SysTaskDelay( (x) * sysTicksPerSecond )
#define FormGetValue(x)			CtlGetValue( GetObjectPtr ((x)) )
#define FormSetValue(x,y)		CtlSetValue( GetObjectPtr ((x)), (y) )

 
/***********************************************************************
 *
 *   Static Global variables
 *
 ***********************************************************************/
#pragma mark -- Global variables --

static	Word		gPrinterValue	= 0;
static	Boolean		gPrinterOpen	= false;

static  PTTransport	gTransport		= PTUnknown;
static	Word		gTransportId	= 0;

static	ULong		gQueryBufLength = BuffSize;
static	char		gQueryBuf[ BuffSize ];
static	char		gQueryBufRes[ BuffSize ];


static	PTConnectSettings gSettings	= { 9600, 
									  	PTDefaultSerTimeout, 
									  	PTDefaultSerFlags,
									  	PTDefaultSerRecvBuf
									  };

static	CharPtr clearLine = "                                                          ";

CharPtr ptStatusSting[] = {

	"Successful and complete",
	"Operation failed",
	"Rom Incompatible",	
	"Bad Parameter",	
	"Transport not Available",
	"Line Error",
	"Transport not Opened",
	"Port already open",
	"No Memory",
	"Printer already connected",		
	"Printer not found",	
	"Time out occurred",	
	"Ir Status Pending",
	"No Peer Addr",
	"Bind failed",
	"No Ir Device Found",
	"Ir Discovery Failed",
	"Ir IrConnectReq Failed",
	"Ir ConnectIrLap Failed",
	"Ir Query Failed",
	"Error Reading Printcap info",
	"                                        "
};

#define PTClear	PTStatusPrintCapFailed + 1

typedef enum {
	cmdOpen = 0,
	cmdClose,
	cmdForm,
	cmdData,
	cmdQueryPrinter,
	cmdInitPrinter,
	cmdResetPrinter,
	cmdPrinterVersion,
	cmdHighLevelAPI
} cmdCommands;


typedef enum {
	queryBaudRate = 0,
	queryStopBit,
	queryParity,
	queryQuery,
	queryInit,
	queryReset
} queryPrintCap;


/***********************************************************************
 *
 *  Shared Global variables
 *
 ***********************************************************************/

Char 	gInfoLabel[134];
CharPtr gInfoLabelPtr = &gInfoLabel[0];

Char 	gInfo[1024];
CharPtr gInfoPtr = &gInfo[0];

/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/
#pragma mark -- Static Functions --

static DWord 	StarterPilotMain(Word cmd, Ptr cmdPBP, Word launchFlags);
static Boolean	AppHandleEvent( EventPtr eventP);
static void 	AppEventLoop(void);
static Err 		AppStart(void);
static void 	AppStop(void);

static PTStatus Connect();
static Boolean	CheckFormReady();
static void		Disconnect();
static void		DoMessage( CharPtr prmMsg );
static void 	DoPrinterCommand(EventPtr event);
static CharPtr	GetPrinterModel();

static Boolean 	MainFormDoCommand(Word command);
static void 	MainFormInit( FormPtr frmP );
static Boolean 	MainFormHandleEvent(EventPtr eventP);

static void		PrinterClose();
static void 	PrinterInit();
static void 	PrinterOpen();
static void 	PrinterQuery();
static void 	PrinterReset();
static void		PrinterVersion();
static void		PrinterHighLevelAPI();

static void 	SendData( Word controlID );
static BytePtr 	SetData();
static BytePtr 	SetForm();
static void		SetPrinter( Word printerId );
static void 	SetStatus( Int status );
static void		SetTrigerLabel(Word list, Word trigger, short index );

/***********************************************************************
 *
 * FUNCTION:    DoMessage
 *
 * DESCRIPTION: Prints a line
 *
 * PARAMETERS:  prmMsg - message
 *
 * RETURNED: 
 *
 *
 ***********************************************************************/
 
static void DoMessage( CharPtr prmMsg )
{
    WinDrawChars( &clearLine[0], StrLen( &clearLine[0] ), 5, 20 );
    WinDrawChars( prmMsg, StrLen( prmMsg ), 5, 20 );
}

/***********************************************************************
 *
 * FUNCTION:    SetStatus
 *
 * DESCRIPTION: Prints a line
 *
 * PARAMETERS:  status - index to messages
 *
 * RETURNED: 
 *
 *
 ***********************************************************************/
 
static void SetStatus( Int status )
{
    WinDrawChars( &clearLine[0], StrLen( &clearLine[0] ), 40, 34 );
    WinDrawChars( ptStatusSting[status], StrLen(ptStatusSting[status]), 40, 34 );
}

/***********************************************************************
 *
 * FUNCTION:    MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
 
static Boolean MainFormDoCommand(Word command)
{
	Boolean handled = false;

	switch (command)
	{
		case MainOptionsAboutSymbolPrintSample:
		{
			MenuEraseStatus( 0 );
			FrmPopupForm( AboutForm );
			handled = true;
			break;
		}
		
		default:
		{
			break;
		}	
	}

	return handled;
}

/***********************************************************************
 *
 * FUNCTION:    SetPrinter
 *
 * DESCRIPTION: Set the printer name
 *
 * PARAMETERS:  controlID - list item value
 *
 * RETURNED:  
 *
 *
 ***********************************************************************/
 
static void SetPrinter( Word controlID )
{
	
	if ( gPrinterOpen ) {
		
		if ( controlID == gPrinterValue ) {
		
			FormSetValue( controlID, true );
		}
		else {
			FormSetValue( controlID, false );
		}
		
		FrmAlert( PrinterOpenAlert);
		return;
	}
	
	gPrinterValue = 0;
	
	if ( FormGetValue ( controlID ) ) {
		gPrinterValue = controlID;
	}
		
	FormSetValue( MainComtecCheckbox,  
				( gPrinterValue == MainComtecCheckbox ) );

	FormSetValue( Main9490Checkbox, 
				( gPrinterValue == Main9490Checkbox ) );

	FormSetValue( MainEltronCheckbox, 
				( gPrinterValue == MainEltronCheckbox ) );
				
	FormSetValue( MainONeilCheckbox, 
				( gPrinterValue == MainONeilCheckbox ) );
				
	FormSetValue( MainPCLCheckbox, 
				( gPrinterValue == MainPCLCheckbox ) );
	
	FormSetValue( MainPostscriptCheckbox, 
				( gPrinterValue == MainPostscriptCheckbox ) );	
}

/***********************************************************************
 *
 * FUNCTION:    SetTransport
 *
 * DESCRIPTION: Set the printer transport
 *
 * PARAMETERS:  controlID - list item value
 *
 * RETURNED:  
 *
 *
 ***********************************************************************/
 
static void SetTransport( Word controlID )
{
	
	if ( gPrinterOpen ) {
	
		if ( controlID == gTransportId ) {
		
			FormSetValue( controlID, true );
		}
		else {
			FormSetValue( controlID, false );
		}
		
		FrmAlert( PrinterOpenAlert);
		return;
	}
	
	gTransport = PTUnknown;
	
	if ( FormGetValue ( controlID ) ) {
	
		switch( controlID )
		{
			case MainSerialCheckbox:
			{
				FormSetValue( MainIrCheckbox, false );
				gTransport = PTSerial;
				gTransportId = controlID;
				break;
			}

			case MainIrCheckbox:
			{
				FormSetValue( MainSerialCheckbox, false );
				gTransport = PTIr;
				gTransportId = controlID;
				break;
			}
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:    CheckFormReady
 *
 * DESCRIPTION: Determine if all information is availble to open the printer.
 *
 * PARAMETERS:  
 *
 * RETURNED: true if printer and transport are selected. 
 *
 *
 ***********************************************************************/
 
static Boolean CheckFormReady()
{
	if ( 0 == gPrinterValue ) {
	
		FrmAlert( NoPrinterSelectedAlert );
	}
	
	if ( PTUnknown == gTransport) {
	
		FrmAlert( NoTransportSelectedAlert );
	}	
	
	if ( ( 0 == gPrinterValue ) || ( PTUnknown == gTransport ) ) {
		return false;
	}
	
	
	if ( ( Main9490Checkbox == gPrinterValue ) &&
		 ( PTIr == gTransport ) ) {
		 
		FrmAlert( PrinterTansportNotSupportedAlert );
		 
		return false;
	}
	
	return true;
}

/***********************************************************************
 *
 * FUNCTION:    GetPrinterModel
 *
 * DESCRIPTION: Returns the printer model used in the open.
 *
 * PARAMETERS:  
 *
 * RETURNED: true if printer and transport are selected. 
 *
 *
 ***********************************************************************/
 
static CharPtr GetPrinterModel()
{
	switch( gPrinterValue )
	{
		case MainComtecCheckbox:
		{
			return "Comtec";
			break;
		}
		
		case Main9490Checkbox:
		{
			return "9490";
			break;
		}
		
		case MainEltronCheckbox:
		{
			return "Eltron";
			break;
		}
	
		case MainONeilCheckbox:
		{
			return "ONeil";
			break;
		}
		
		case MainPCLCheckbox:
		{
			return "PCL";
			break;
		}
		
		case MainPostscriptCheckbox:
		{
			return "Postscript";
			break;
		}
		
		default:
		{
			return "unknown";
			break;
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:    SetSettings
 *
 * DESCRIPTION: Returns a PTConnectSettings.
 *
 * PARAMETERS:  
 *
 * RETURNED: Returns a PTConnectSettings.
 *
 *
 ***********************************************************************/

static PTConnectSettingsPtr SetSettings()
{
	gSettings.baudRate	= 9600;
	gSettings.recvBufSize = BuffSize;
	gSettings.timeOut	= PTDefaultSerTimeout;
	gSettings.flags		= serDefaultSettings | serSettingsFlagCTSAutoM;
	
	return &gSettings;
}	


/***********************************************************************
 *
 * FUNCTION:    SetData
 *
 * DESCRIPTION: Sets the data for a given printer.
 *
 * PARAMETERS:  
 *
 * RETURNED: BytePtr to the data.
 *
 *
 ***********************************************************************/

static BytePtr SetData()
{
	switch( gPrinterValue )
	{
		case MainComtecCheckbox:
		{
			return (BytePtr)"! UF SHELF.FMT\r\n" \
							"$99.99\r\n" \
							"SWEATSHIRT\r\n" \
							"40123456784\r\n" \
							"40123456784\r\n";
			break;
		}
		
	
		case Main9490Checkbox:
		{
			return (BytePtr)"{ " \
							"B,25,N,1 |" \
							"1,\"2754185285\" |" \
							"2,\"74185245768\" |" \
							"3,\"65\" |" \
							"4,\"PURE SILK\" |" \
							"6,\"35\" |" \
							"7,\"COTTON\" |" \
							"}";
			break;
		}
		
		case MainEltronCheckbox:
		{
			return (BytePtr)"\r\n" \
							"FR\"TPORT\"\r\n" \
							"?\r\n" \
							"Symbol Technologies\r\n" \
							"Printing Solution\r\n" \
							"40123456784\r\n" \
							"P1\r\n" \
							"\r\n";
			break;
		}
		
		
		case MainONeilCheckbox:
		{
		
			return (BytePtr)"\033" \
							"EZ\r\n" \
							"{PRINT: \r\n" \
							"@10,30:MF055|$99.99| \r\n" \
							"@50,30:MF072|SWEATSHIRT| \r\n" \
							"@100,30:UPC-A,HIGH 10,WIDE 2|40123456784| \r\n" \
							"@160,20:MF072|40123456784| \r\n" \
							"} \r\n" \
							"{AHEAD:200} \r\n" \
							"{LP}\r\n";
			break;
			
		}
		
		case MainPostscriptCheckbox:
		{
		
			return (BytePtr)"%!PS-Adobe-1.0\r\n" \
							"/Times-Roman findfont\r\n" \
							"20 scalefont\r\n" \
							"setfont\r\n" \
							"newpath\r\n" \
							"72 72 moveto\r\n" \
							"(Hello, world!) show\r\n" \
							"showpage\r\n";					
			break;
			
		}
		
		case MainPCLCheckbox:
		{
		
			return (BytePtr)"\033\%1A\n" \
							"\033*p20X\n" \
							"\033*p700Y\n" \
							"\033&p10X1234567890\n" \
							"\n";							
			break;
			
		}
		
		default:
		{
			return NULL;
			break;
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:    SetForm
 *
 * DESCRIPTION: Sets the form information for a given printer.
 *
 * PARAMETERS:  
 *
 * RETURNED: BytePtr to the form.
 *
 *
 ***********************************************************************/
 
static BytePtr SetForm()
{
	switch( gPrinterValue )
	{
		case MainComtecCheckbox:
		{
			return (BytePtr)"! DF SHELF.FMT\r\n" \
		    				"! 0 200 200 210 1\r\n" \
		    				"BOX 100 100 480 210 1\r\n" \
		    				"CENTER\r\n" \
		    				"TEXT 4 3 0 15 " "\\\\\r\n" \
		    				"TEXT 4 0 0 95 " "\\\\\r\n" \
		    				"BARCODE UPCA 1 1 40 0 145 " "\\\\\r\n" \
		    				"TEXT 7 0 0 185 " "\\\\\r\n" \
		    				"PRINT\r\n";
			break;
		}
		
		case Main9490Checkbox:
		{
			
			return (BytePtr)"{ " \
    						"F,25,A,R,E,300,200,\"EXAM-05\" |" \
    						"T,1,10,V,250,50,0,1,1,1,B,C,0,0,0 |" \
    						"B,2,12,V,150,40,1,2,80,7,L,0 |" \
    						"D,3,3 |" \
    						"D,4,20 |" \
    						"T,5,25,V,80,10,0,1,1,1,B,L,0,0,0 |" \
    						"R,1,\"   %                    \" |" \
    						"R,4,3,1,2,1,1 |" \
    						"R,4,4,1,20,6,1 |" \
    						"D,6,3 |" \
    						"D,7,20 |" \
    						"T,8,25,V,65,10,0,1,1,1,B,L,0,0,0 |" \
    						"R,1,\"   %                    \" |" \
    						"R,4,6,1,2,1,1 |" \
    						"R,4,7,1,20,6,1 |" \
    						"C,30,10,0,1,1,1,B,L,0,0\"MADE IN USA\",0 |" \
    						"L,S,110,30,110,150,10,\" \" |" \
    						"Q,240,30,270,150,3,\" \" |" \
							"}";
			break;
		}
		
		case MainEltronCheckbox:
		{
			return (BytePtr)"\r\n" \
							"FK\"TPORT\"\r\n" \
							"FS\"TPORT\"\r\n" \
							"V00,20,C,\"Enter company name\"\r\n" \
							"V01,20,C,\"Enter product name\"\r\n" \
							"V02,15,C,\"Enter product code\"\r\n" \
							"D8\r\n" \
							"S2\r\n" \
							"Q609,24\r\n" \
							"q384\r\n" \
							"A10,10,0,4,1,1,N,V00\r\n" \
							"A10,90,0,4,1,1,N,V01\r\n" \
							"B70,290,0,3,2,6,180,B,V02\r\n" \
							"FE\r\n";
								
			break;
		}
		
		case MainONeilCheckbox:
		{
		
			return (BytePtr)"\r\n";
							
			break;
			
		}
		
		default:
		{
			return NULL;
			break;
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:    DoPrintCapQuery
 *
 * DESCRIPTION: Query the Printcap information
 *
 * PARAMETERS:  event - current event
 *
 * RETURNED: Displays the results in a popup form.
 *
 *
 ***********************************************************************/

static void DoPrintCapQuery(EventPtr event)
{
    PTStatus	status;
   
	if ( ! gPrinterOpen ) {
		FrmAlert( PrinterNotOpenAlert);
		return;
	}
	
	gQueryBufLength = BuffSize;	
	MemSet( &gQueryBuf[0], BuffSize, 0x0 );
	MemSet( &gQueryBufRes[0], gQueryBufLength, 0x0 );
	
	
	switch(event->data.popSelect.selection)
	{
		case queryBaudRate:
		{
			gQueryBuf[0] = 'b';
			gQueryBuf[1] = 'r';
			break; 
		}
		
		case queryStopBit:
		{
			gQueryBuf[0] = 's';
			gQueryBuf[1] = 'b';
			break; 
		}
		
		case queryParity:
		{
			gQueryBuf[0] = 'p';
			gQueryBuf[1] = 'r';
			break; 
		}
		
		case queryQuery:
		{
			gQueryBuf[0] = 'q';
			gQueryBuf[1] = 's';
			break; 
		}
		
		case queryInit:
		{
			gQueryBuf[0] = 'i';
			gQueryBuf[1] = 's';
			break; 
		}
		
		case queryReset:
		{
			gQueryBuf[0] = 'r';
			gQueryBuf[1] = 's';
			break; 
		}
		
	}
	
	SetTrigerLabel(MainPrintcapList, MainPrintCapPopTrigger, event->data.popSelect.selection );
	
	DoMessage( "ptQueryPrintCap" );
	SetStatus( PTClear );
	
	MemSet( gInfoPtr, 1024, 0x0 ); 
	MemSet( gInfoLabelPtr, 134, 0x0 ); 
	
	status = ptQueryPrintCap( (VoidPtr)&gQueryBuf[0], (VoidPtr)&gQueryBufRes[0], &gQueryBufLength );
	SetStatus( status );
	
	MemMove( gInfoLabelPtr, &gQueryBuf[0], StrLen( &gQueryBuf[0] ) + 1 );
	MemMove( gInfoPtr, &gQueryBufRes[0], gQueryBufLength );
	
	MemSet( gQueryBuf, BuffSize, 0x0 );
	MemSet( gQueryBufRes, BuffSize, 0x0 );			
		
	FrmPopupForm( InfoForm );	
	
}

/***********************************************************************
 *
 * FUNCTION:    SetTrigerLabel
 *
 * DESCRIPTION: Sets to label to the popup list to last command selected.
 *
 * PARAMETERS:  list - form list id
 *				trigger - object name of the list
 *				index - index of item in list 
 *
 * RETURNED: 
 *
 ***********************************************************************/
 
static void SetTrigerLabel(Word list, Word trigger, short index )
{

	ListPtr		pList;
	CharPtr		pLabel = NULL;
	
	pList = GetObjectPtr( list );
	pLabel = LstGetSelectionText(  pList, index );
	CtlSetLabel ( GetObjectPtr ( trigger ), pLabel );
	
}

/***********************************************************************
 *
 * FUNCTION:    DoPrinterCommand
 *
 * DESCRIPTION: Execute the command form the Command popup list.
 *
 * PARAMETERS:  EventPtr event
 *
 * RETURNED: 
 *
 ***********************************************************************/
 
static void DoPrinterCommand(EventPtr event)
{
	    
	switch(event->data.popSelect.selection)
	{
		case cmdOpen:
		{
			PrinterOpen( );					
			break; 
		}
		
		case cmdClose:
		{
			PrinterClose( );
			break; 
		}
		
		case cmdForm:
		{
			SendData( cmdForm );
			break; 
		}
		
		case cmdData:
		{
			SendData( cmdData );
			break; 
		}
		
		case cmdQueryPrinter:
		{
			
			PrinterQuery();
			break; 
		}
		
		case cmdInitPrinter:
		{
			PrinterInit();
			break; 
		}
		
		
		case cmdResetPrinter:
		{
			PrinterReset();
			break; 		
		}
		
		case cmdPrinterVersion:
		{	
			PrinterVersion();	
			break; 
		}
		
		case cmdHighLevelAPI:
		{
			PrinterHighLevelAPI();
		}
		
		default:
		{
			break;
		}
	}
	
	SetTrigerLabel(MainCommandList, MainCommandPopTrigger, event->data.popSelect.selection );
}

/***********************************************************************
 *
 * FUNCTION:    PrinterClose
 *
 * DESCRIPTION: Execute the command ptClosePrinter API call.
 *
 * PARAMETERS:  EventPtr event
 *
 * RETURNED: 
 *
 ***********************************************************************/

static void PrinterClose( )
{

	PTStatus	status;
		
	DoMessage( "ptClosePrinter" );
	SetStatus( PTClear );
					
	status = ptClosePrinter();
	SetStatus( status );
			
	gPrinterOpen = false;
}

/***********************************************************************
 *
 * FUNCTION:    PrinterInit
 *
 * DESCRIPTION: Execute the command PrinterInit API call.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    none
 *
 ***********************************************************************/

static void PrinterInit()
{
	PTStatus	status;

	status = Connect();
	if ( PTStatusOK != status ) {
		return;
	}
	
	/*
	 * Init the printer
	 */	
	DoMessage( "ptInitPrinter" );
	SetStatus( PTClear );
	
	
	status = ptInitPrinter( NULL, 0 ); 
	SetStatus( PTClear );
							
	Disconnect();
	
}

/***********************************************************************
 *
 * FUNCTION:    PrinterQuery
 *
 * DESCRIPTION: Execute the command ptQueryPrinter API call.
 *
 * PARAMETERS:  
 *
 * RETURNED: 
 *
 ***********************************************************************/

static void PrinterQuery()
{
	PTStatus	status;		

	status = Connect();
	if ( PTStatusOK != status ) {
		return;
	}
	
	/*
	 * Write the form to the printer
	 */	
	DoMessage( "ptQueryPrinter" );
	SetStatus( PTClear );
	
	MemSet( gInfoPtr, 1024, 0x0 ); 
	MemSet( gInfoLabelPtr, 134, 0x0 ); 
	MemSet( gQueryBuf, BuffSize, 0x0 );
	MemSet( gQueryBufRes, BuffSize, 0x0 );

	gQueryBufLength = BuffSize;
		
	status = ptQueryPrinter( NULL, 0, gQueryBufRes, &gQueryBufLength );
	
	if ( gQueryBufLength > 0 && PTStatusOK == status ) {

		MemMove( gInfoLabelPtr, &gQueryBuf[0], StrLen( &gQueryBuf[0] ) + 1 );
		MemMove( gInfoPtr, &gQueryBufRes[0], gQueryBufLength );
		Disconnect();
		
		FrmPopupForm( InfoForm );
		
	}
	else {
	
		Disconnect();
		
		DoMessage( "ptQueryPrinter" );
		SetStatus( PTClear );
		SetStatus( status );
		
		FrmAlert( PrintStatusNoDataAlert );
		
	}

	MemSet( gQueryBuf, BuffSize, 0x0 );
	MemSet( gQueryBufRes, BuffSize, 0x0 );

}

/***********************************************************************
 *
 * FUNCTION:    PrinterOpen
 *
 * DESCRIPTION: Execute the command ptOpenPrinter API call.
 *
 * PARAMETERS:  
 *
 * RETURNED: 
 *
 ***********************************************************************/
static void PrinterOpen()
{
	PTConnectSettingsPtr pcSettings;
    PTStatus	status;

	if ( ! CheckFormReady() ) {
		return;
	}
	
	DoMessage( "ptOpenPrinter" );
	SetStatus( PTClear );
	
	pcSettings = SetSettings();
	
	// status = ptOpenPrinter( GetPrinterModel(), gTransport, pcSettings );
	status = ptOpenPrinter( GetPrinterModel(), gTransport, NULL );
	
	if ( PTStatusOK != status ) {
		
		FrmAlert( OpenFailedAlert);
		
	}
	
	gPrinterOpen = true;
	
	SetStatus( status );

}

/***********************************************************************
 *
 * FUNCTION:    PrinterVersion
 *
 * DESCRIPTION: Execute the command ptPrintApiVersion API call.
 *
 * PARAMETERS:  
 *
 * RETURNED: 
 *
 ***********************************************************************/
 
static void PrinterVersion()
{
	PTStatus	status;

	MemSet( gInfoPtr, 1024, 0x0 ); 
	MemSet( gInfoLabelPtr, 134, 0x0 ); 
	
	DoMessage( " " );
	SetStatus( PTStatusOK );
	
	/*
	 * Version
	 */
	DoMessage( "ptPrintApiVersion" );
	SetStatus( PTClear );

	status = ptPrintApiVersion( gInfoPtr, 1024 );
	
	SetStatus( status );
	
	StrCopy( gInfoLabelPtr, "Version :" );
	if ( status != PTStatusOK ) {
		StrCopy( gInfoPtr, "ptPrintApiVersion failed." );
	}
	
	FrmPopupForm( InfoForm );
}

/***********************************************************************
 *
 * FUNCTION:    PrinterVersion
 *
 * DESCRIPTION: Execute the command ptPrintApiVersion API call.
 *
 * PARAMETERS:  
 *
 * RETURNED: 
 *
 ***********************************************************************/
 
static void PrinterHighLevelAPI()
{
	PTStatus	status;
	
	UInt		i;
	ULong		length;
	
	char		pbuf[40];
	char		fontInfo[256];
	

	 if ( ! CheckFormReady() ) {
		return;
	}
	
	DoMessage( "High Level API" );
	SetStatus( PTClear );
	
	DoMessage( "ptStartPrintBuffer" );
	status = ptStartPrintBuffer( 256 );
	if ( PTStatusOK != status ) {
		SetStatus( status );
		return;
	}

	DoMessage( "Text and Line" );
	for ( i = 1; i <= 4; i++ ) {
	
		StrPrintF( &pbuf[0], "%s : %i", "Loop index ", i );
		DoMessage( "ptTextToBuffer Loop" );
		status = ptTextToBuffer( 30 + ( 5 * i ), i * 30, &pbuf[0] );
		if ( PTStatusOK != status && PTStatusPrintCapFailed != status) {
			ptResetPrintBuffer();
			SetStatus( status );
			return;
		}

		DoMessage( "ptLineToBuffer Loop" );
		status = ptLineToBuffer( 30 + ( 5 * i  ), 
						20 + ( i * 30 ), 
						i * 120, 
						20 + ( i * 30 ), 
						i );
		if ( PTStatusOK != status && PTStatusPrintCapFailed != status) {
			ptResetPrintBuffer();
			SetStatus( status );
			return;
		}
	}

	DoMessage( "ptTextToBuffer 2" );
	status = ptTextToBuffer( 30, 240, "Next to last." );
	if ( PTStatusOK != status && PTStatusPrintCapFailed != status) {
			ptResetPrintBuffer();
			SetStatus( status );
			return;
	}

	
	DoMessage( "ptRectToBuffer 1" );
	status = ptRectToBuffer( 20, 215, 200, 280, 4 );
	if ( PTStatusOK != status && PTStatusPrintCapFailed != status) {
			ptResetPrintBuffer();
			SetStatus( status );
			return;
	}

	
	length = 256;
	status = ptQueryPrintCap( "f1", &fontInfo[0], &length );
	if ( PTStatusOK == status ) {
		
		ptSetFont( &fontInfo[0] );
	}
	
	DoMessage( "ptTextToBuffer 3" );
	status = ptTextToBuffer( 30, 300, "The last line." );
	if ( PTStatusOK != status && PTStatusPrintCapFailed != status) {
			ptResetPrintBuffer();
			SetStatus( status );
			return;
	}

	
	DoMessage( "ptLineToBuffer 1" );
	status = ptLineToBuffer( 30, 340, 500, 340, 11 );
	if ( PTStatusOK != status && PTStatusPrintCapFailed != status) {
			ptResetPrintBuffer();
			SetStatus( status );
			return;
	}
		
	DoMessage( "ptPrintPrintBuffer" );
	status = ptPrintPrintBuffer( NULL );
	if ( PTStatusOK != status ) {
			ptResetPrintBuffer();
	}
	SetStatus( status );

}
/***********************************************************************
 *
 * FUNCTION:    PrinterReset
 *
 * DESCRIPTION: Execute the command ptResetPrinter API call.
 *
 * PARAMETERS:  
 *
 * RETURNED: 
 *
 ***********************************************************************/
 
static void PrinterReset()
{
	PTStatus	status;
	
	status = Connect();
	if ( PTStatusOK != status ) {
		return;
	}
	
	/*
	 * Reset the printer
	 */	
	DoMessage( "ptResetPrinter" );
	SetStatus( PTClear );
	
	status = ptResetPrinter( NULL, 0 ); 
							
	SetStatus( status );
	
	Disconnect();	
}


/***********************************************************************
 *
 * FUNCTION:    SendData
 *
 * DESCRIPTION: Write data (data or form) to the printer.
 *
 * PARAMETERS:  controlID - list item value
 *
 * RETURNED: 
 *
 ***********************************************************************/
 
static void SendData( Word controlID )
{
    BytePtr				 pData;
    PTStatus			 status;
    
    if ( ! CheckFormReady() ) {
		return;
	}
	
	/*
	 * Set the data
	 */
	switch ( controlID )
	{
		case cmdForm:
		{
			pData = SetForm();
			break;
		}
		
		case cmdData:
		{
			pData = SetData();
			break;
		}
		
		default:
		{
			pData = (BytePtr)"debug string                           ";
			break;
		}
	}
	
	status = Connect();
	if ( PTStatusOK != status ) {
		return;
	}
	
	sleep( 1 );
	
	/*
	 * Write the form to the printer
	 */	
	DoMessage( "ptWritePrinter" );
	SetStatus( PTClear );
	
	status = ptWritePrinter( pData, StrLen( (CharPtr)pData ) );		
	SetStatus( status );

	sleep(1);

	Disconnect();
}
	
/***********************************************************************
 *
 * FUNCTION:    Disconnect
 *
 * DESCRIPTION: Execute the command ptDisconnectPrinter() API call.
 *
 * PARAMETERS: 
 *
 * RETURNED: 
 *
 ***********************************************************************/
 	
static void Disconnect()
{	
	/*
	 * Disconnect
	 */
	sleep(1);
	DoMessage( "ptDisconnectPrinter");
	SetStatus(  PTClear );
	SetStatus( ptDisconnectPrinter() );					
}

/***********************************************************************
 *
 * FUNCTION:    Connect
 *
 * DESCRIPTION: Execute the command ptConnectPrinter() API call.
 *
 * PARAMETERS:  
 *
 * RETURNED: 	PTStatus result code
 *
 ***********************************************************************/
 			
static PTStatus Connect()
{

	PTStatus status;

	DoMessage( " " );
	SetStatus( PTStatusOK );
	
	/*
	 * Connect
	 */
	DoMessage( "ptConnectPrinter" );
	SetStatus( PTClear );
				
	status = ptConnectPrinter( NULL );
	SetStatus( status );
	
	return status;
		
}

/***********************************************************************
 *
 * FUNCTION:    MainFormInit
 *
 * DESCRIPTION: This routine initializes the MainForm form.
 *
 * PARAMETERS:  frm - pointer to the MainForm form.
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void MainFormInit(FormPtr frmP)
{
	
}

/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "MainForm" of this application.
 *
 * PARAMETERS:  eventP  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/

static Boolean MainFormHandleEvent(EventPtr eventP)
{
    Boolean		handled = false;
    FormPtr		frmP;
   	
	switch (eventP->eType) 
	{
		case frmOpenEvent:
		{
			frmP = FrmGetActiveForm();
			
			MainFormInit( frmP );
			FrmDrawForm ( frmP );
			
			DoMessage( "Printer Closed" );
			SetStatus( PTStatusOK );
					
			handled = true;
			break;
		}
		
		case popSelectEvent:
		{
			switch( eventP->data.ctlEnter.controlID )
			{
				case MainCommandPopTrigger:
				{
					DoPrinterCommand( eventP );
					handled = true;
					
					break;
				}
				
				case MainPrintCapPopTrigger:
				{
					DoPrintCapQuery( eventP );
					handled = true;
					
					break;
				}
			}
			
			break;
		}
		
		case menuEvent:
		{
			return MainFormDoCommand(eventP->data.menu.itemID);
			handled = true;
			break;
		}

	
		case ctlSelectEvent:
		{
			switch( eventP->data.ctlSelect.controlID )
			{
				/*
				 *	Transports
				 */
				case MainSerialCheckbox:
				case MainIrCheckbox:
				{
					SetTransport( eventP->data.ctlSelect.controlID  );
					handled = true;
					break;
				}
					
				/*
				 * Printers
				 */
				case MainComtecCheckbox:
				case Main9490Checkbox:
				case MainEltronCheckbox:
				case MainONeilCheckbox:
				case MainPCLCheckbox:
				case MainPostscriptCheckbox:
				{
					SetPrinter( eventP->data.ctlSelect.controlID  );
					handled = true;
					break;
				}
				
				default:
				{
					break;
				}
			
			}
			
			break;
		}
		
    	default:
    	{
    		break;
    	}
		
    }     	
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
 
static Boolean AppHandleEvent( EventPtr eventP)
{
	Word formId;
	FormPtr frmP;

	if (eventP->eType == frmLoadEvent) {
	
		/*
		 *	Load the form resource.
		 */
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		/*
		 * Set the event handler for the form.  The handler of the currently
		 * active form is called by FrmHandleEvent each time is receives an
		 * event.
		 */ 
		switch (formId)
		{
			case MainForm:
			{
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;
			}
				
			case AboutForm:
			{
				FrmSetEventHandler( frmP, AboutHandleEvent );
				break;
			}
			
			case InfoForm:
			{
				FrmSetEventHandler( frmP, InfoHandleEvent );
				break;
			}
			
			default:
			{
				break;

			}

			return true;
		}
	}
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
 
static void AppEventLoop(void)
{
	Word error;
	EventType event;


	do {
		EvtGetEvent(&event, evtWaitForever);
		
		
		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(0, &event, &error))
				if (! AppHandleEvent(&event))
					FrmDispatchEvent(&event);

	} while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:     AppStart
 *
 * DESCRIPTION:  Get the current application's preferences.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err value 0 if nothing went wrong
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
 
static Err AppStart(void)
{	
   return 0;
}


/***********************************************************************
 *
 * FUNCTION:    AppStop
 *
 * DESCRIPTION: Save the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
 
static void AppStop(void)
{
  	ptClosePrinter();
}


/***********************************************************************
 *
 * FUNCTION:    StarterPilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 *
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
 
static DWord StarterPilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
	Err error;

	switch (cmd)
		{
		case sysAppLaunchCmdNormalLaunch:
			error = AppStart();
			if (error) 
				return error;
				
			FrmGotoForm(MainForm);
			AppEventLoop();
			AppStop();
			break;

		default:
			break;

		}
	
	return 0;
}

/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: Returns a pointer to an object in the current form.
 *
 * PARAMETERS:  objectID 
 *
 * RETURNED:    VoidPtr
 *
 * REVISION HISTORY:
 *
 ***********************************************************************/
 
VoidPtr GetObjectPtr(Word objectID)
{
	FormPtr frmP;

	frmP = FrmGetActiveForm();
	return ( FrmGetObjectPtr( frmP, FrmGetObjectIndex(frmP, objectID) ) );
}

/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
DWord PilotMain( Word cmd, Ptr cmdPBP, Word launchFlags)
{
    return StarterPilotMain(cmd, cmdPBP, launchFlags);
}

