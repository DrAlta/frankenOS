VERSION 5.00
Object = "{00028C01-0000-0000-0000-000000000046}#1.0#0"; "DBGRID32.OCX"
Object = "{248DD890-BB45-11CF-9ABC-0080C7E7B78D}#1.0#0"; "mswinsck.ocx"
Begin VB.Form Form1 
   Caption         =   "TCPIP Barcode Lookup into an Access Database for Spectrum24 v1.10"
   ClientHeight    =   8295
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8310
   LinkTopic       =   "Form1"
   ScaleHeight     =   8295
   ScaleWidth      =   8310
   StartUpPosition =   3  'Windows Default
   Begin MSWinsockLib.Winsock tcpServer 
      Index           =   0
      Left            =   7200
      Top             =   3960
      _ExtentX        =   741
      _ExtentY        =   741
      _Version        =   327681
   End
   Begin VB.CommandButton cmdExit 
      Caption         =   "Exit"
      Height          =   375
      Left            =   6720
      TabIndex        =   9
      Top             =   7680
      Width           =   1455
   End
   Begin VB.TextBox txtBarcode 
      DataField       =   "barcode"
      DataSource      =   "Data1"
      Height          =   495
      Left            =   1080
      TabIndex        =   7
      Top             =   7080
      Width           =   5415
   End
   Begin MSDBGrid.DBGrid DBGrid1 
      Bindings        =   "Form1.frx":0000
      Height          =   4815
      Left            =   120
      OleObjectBlob   =   "Form1.frx":0010
      TabIndex        =   6
      Top             =   120
      Width           =   8055
   End
   Begin VB.TextBox txtISBN 
      DataField       =   "ISBN"
      DataSource      =   "Data1"
      Height          =   495
      Left            =   1080
      TabIndex        =   2
      Top             =   6360
      Width           =   5415
   End
   Begin VB.TextBox txtDescription 
      DataField       =   "Description"
      DataSource      =   "Data1"
      Height          =   495
      Left            =   1080
      TabIndex        =   1
      Top             =   5760
      Width           =   5415
   End
   Begin VB.TextBox txtTitle 
      DataField       =   "Title"
      DataSource      =   "Data1"
      Height          =   495
      Left            =   1080
      TabIndex        =   0
      Top             =   5160
      Width           =   5415
   End
   Begin VB.Data Data1 
      Caption         =   "Spectrum24 Price Lookup Database"
      Connect         =   "Access"
      DatabaseName    =   "MdbDemo.mdb"
      DefaultCursorType=   0  'DefaultCursor
      DefaultType     =   2  'UseODBC
      Exclusive       =   0   'False
      Height          =   375
      Left            =   1080
      Options         =   0
      ReadOnly        =   0   'False
      RecordsetType   =   1  'Dynaset
      RecordSource    =   "Titles"
      Top             =   7680
      Width           =   5415
   End
   Begin VB.Label Label5 
      Caption         =   "Barcode:"
      Height          =   375
      Left            =   240
      TabIndex        =   8
      Top             =   7200
      Width           =   735
   End
   Begin VB.Label Label3 
      Caption         =   "ISBN:"
      Height          =   375
      Left            =   360
      TabIndex        =   5
      Top             =   6480
      Width           =   495
   End
   Begin VB.Label Label2 
      Caption         =   "Description:"
      Height          =   375
      Left            =   120
      TabIndex        =   4
      Top             =   5880
      Width           =   975
   End
   Begin VB.Label Label1 
      Caption         =   "Title:"
      Height          =   375
      Left            =   360
      TabIndex        =   3
      Top             =   5280
      Width           =   495
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'This application demonstrates data base connectivity to an Access
'database.  It allows the user to modify the records doing the
'standard add/change/delete cycle.  The applciation also contains
'the Winsock activeX control included in Visual Basic v5.0. We use
'this control to set up an IP server running on port 1950. We then
'receive requests from the clients for inquires into the database.
'The client sends us a barcode and we send him back a page of data
'currently in the form of 4 lines of 20 characters (the lowest
'common denominator of the Spectrum24 product line).

'11-04-98 modified to run with PalmPilot FindMdb.prc
'8-11-97 added logic to read startup menu from MENU.TXT disk file
'8-10-97 added logic to send startup menu to clients
'8-8-97 Copied the code from PLUHOST and removed my TcpEng2 logic
'       Converted over to the Visual Basic WinSock Control
'

Option Explicit
Dim MyDB As Database, MyWS As Workspace, MyRS As Recordset
Dim sline1, sline2, sline3, sline4 As String * 20
Dim intMax As Integer






Private Sub Form_Load()
   intMax = 0
   tcpServer(0).Protocol = sckTCPProtocol
   tcpServer(0).LocalPort = 1950
   tcpServer(0).Listen
End Sub




Private Sub cmdExit_Click()
   tcpServer(0).Close
   Unload Form1
End Sub

Private Sub tcpServer_ConnectionRequest(index As Integer, ByVal requestID As Long)
Dim strline As String

   If index = 0 Then 'their calling on the listen port...ie their new
      intMax = intMax + 1
      Load tcpServer(intMax)  'create an new instance of the port
      tcpServer(intMax).LocalPort = 0
      tcpServer(intMax).Accept requestID
      
      'By putting the startup menu here, we can move the logic of the
      'application on the remote to here. Since we handle the decode
      'of the selection here also.  This means you probably wont have
      'to change the remote code in the S2100 units at all.
      
      'now send him his startup menu
      'strline = sline1 & sline2 & sline3 & sline4
      'tcpServer(intMax).SendData strline
      
   End If
End Sub

Private Sub tcpServer_DataArrival(index As Integer, ByVal bytesTotal As Long)
   Dim strRequest As String   'the data from the client
   Dim szInput As String      'the barcode data
   Dim Rst As Recordset       'our database
   Dim sResponse As String    'the respose to the query
   Dim sLine As String * 40   'temp area used to build response
   Dim sqlRequest As String
   Dim szEnd As String
      
   'read the socket data into our string
   tcpServer(index).GetData strRequest
  
   szInput = strRequest
   sqlRequest = "SELECT * FROM Titles WHERE Titles.Barcode = '" & szInput & "'"
            
   'open the database & lookup the barcode
   Set MyWS = DBEngine.Workspaces(0)
   Set MyDB = MyWS.OpenDatabase("MdbDemo.mdb")
   Set MyRS = MyDB.OpenRecordset("Titles")
    
   'do a lookup on the barcode
   Set Rst = MyDB.OpenRecordset(sqlRequest)
    
  'build the display data and response for client
   If Rst.EOF Then
     sLine = "Can't find: " & szInput & "                    "
     sResponse = sLine & sLine & sLine
     txtBarcode.Text = "No Such Number dude" & "     length: " & Len(sResponse)
   Else  'update the server display
     txtTitle.Text = Rst!Title
     txtDescription.Text = Rst!Description
     txtISBN.Text = Rst!ISBN
       
     'build the response line for the client (3x40 chars)
     sLine = Rst!Title & "                                        "
     sResponse = sLine
     sLine = Rst!Description & "                                        "
     sResponse = sResponse & sLine
     sLine = Rst!ISBN & "                                        "
     sResponse = sResponse & sLine
     txtBarcode.Text = Rst!barcode & "     length: " & Len(sResponse)
  End If
 
  'now send tne response from the query back to the client
  tcpServer(index).SendData sResponse
   
  MyRS.Close
  MyDB.Close
  MyWS.Close
   
End Sub



