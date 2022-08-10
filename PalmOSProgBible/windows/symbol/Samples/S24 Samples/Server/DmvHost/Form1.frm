VERSION 5.00
Object = "{00028C01-0000-0000-0000-000000000046}#1.0#0"; "DBGRID32.OCX"
Object = "{248DD890-BB45-11CF-9ABC-0080C7E7B78D}#1.0#0"; "mswinsck.ocx"
Begin VB.Form Form1 
   BackColor       =   &H8000000E&
   Caption         =   "Spectrum24 California DMV MagStripe Demo v1.00"
   ClientHeight    =   5700
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7590
   LinkTopic       =   "Form1"
   ScaleHeight     =   5700
   ScaleWidth      =   7590
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdRefresh 
      Caption         =   "Refresh Display"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   13.5
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   615
      Left            =   1440
      TabIndex        =   3
      Top             =   4920
      Width           =   2175
   End
   Begin MSWinsockLib.Winsock tcpServer 
      Index           =   0
      Left            =   2760
      Top             =   3480
      _ExtentX        =   741
      _ExtentY        =   741
      _Version        =   327681
   End
   Begin VB.CommandButton cmdExit 
      Caption         =   "Exit Program"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   13.5
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   615
      Left            =   3960
      TabIndex        =   1
      Top             =   4920
      Width           =   2175
   End
   Begin MSDBGrid.DBGrid DBGrid1 
      Bindings        =   "Form1.frx":0000
      Height          =   2535
      Left            =   120
      OleObjectBlob   =   "Form1.frx":0010
      TabIndex        =   0
      Top             =   1920
      Width           =   7335
   End
   Begin VB.Data Data1 
      Caption         =   "Spectrum24 Price Lookup Database"
      Connect         =   "Access"
      DatabaseName    =   "DmvDemo.mdb"
      DefaultCursorType=   0  'DefaultCursor
      DefaultType     =   2  'UseODBC
      Exclusive       =   0   'False
      Height          =   375
      Left            =   720
      Options         =   0
      ReadOnly        =   0   'False
      RecordsetType   =   1  'Dynaset
      RecordSource    =   "DmvTable"
      Top             =   5040
      Visible         =   0   'False
      Width           =   5415
   End
   Begin VB.Label lblStatus 
      Alignment       =   2  'Center
      BackColor       =   &H00FFFFFF&
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   9.75
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   0
      TabIndex        =   4
      Top             =   4560
      Width           =   7575
   End
   Begin VB.Image Image1 
      Height          =   1215
      Left            =   120
      Picture         =   "Form1.frx":09E3
      Stretch         =   -1  'True
      Top             =   720
      Width           =   7380
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      BackColor       =   &H8000000E&
      Caption         =   "MagStripe Host Program"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   24
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H00FF0000&
      Height          =   615
      Left            =   120
      TabIndex        =   2
      Top             =   120
      Width           =   7215
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'This application demonstrates Spectrum24 spt1740 Mag Stripe host support.
'We get a Tcp packet with the DmvId number in a comma-delim'd string and
'use our .MDB file to get the associated image for this driver.  We then
'send this .BIN file to the client.  If this is the first time we've seen
'this driver, we add his DmvId & name to our .MDB file and set his image
'to Default.bin.  Only images of 160x150x2 are currently supported.

Option Explicit
Dim MyWS As Workspace, MyDB As Database, MyRS As Recordset
Dim szDmvId, szName, szAddr1, SzAddr2, szCity, szState, szImageName, szInput As String
Dim intMax As Integer 'for the client port array

Private Sub UpdateGrid() 'repaints the grid for us
    Data1.Refresh
    DBGrid1.Columns(0).Width = 1600
    DBGrid1.Columns(1).Width = 2200
    DBGrid1.Columns(2).Width = 2200
    DBGrid1.Columns(3).Width = 2200
    DBGrid1.Columns(4).Width = 2200
    DBGrid1.Scroll 0, 9999 'this makes the grid scrool up
End Sub

Private Sub cmdRefresh_Click()
   Call UpdateGrid
   lblStatus.Caption = ""
   
End Sub

Private Sub Form_Load()
intMax = 0
    tcpServer(0).Protocol = sckTCPProtocol
    tcpServer(0).LocalPort = 1968
    tcpServer(0).Listen
    
    Call UpdateGrid
    
    Set MyWS = DBEngine.Workspaces(0)
    Set MyDB = MyWS.OpenDatabase("DmvDemo.mdb")
    Set MyRS = MyDB.OpenRecordset("DmvTable")
   
End Sub
'if the user closes the form using the X in the corner
'  you come here instead of cmdExit_Click
Private Sub Form_Unload(Cancel As Integer)
    tcpServer(0).Close
    Unload Form1
    End
End Sub
Private Sub cmdExit_Click()
    tcpServer(0).Close
    Unload Form1
    End
End Sub

Private Sub tcpServer_ConnectionRequest(index As Integer, ByVal requestID As Long)
   If index = 0 Then 'they are calling on the listen port...ie their new
      intMax = intMax + 1
      Load tcpServer(intMax)  'create an new instance of the port
      tcpServer(intMax).LocalPort = 0
      tcpServer(intMax).Accept requestID
   End If
   
End Sub

Private Sub ParseIt()
    Dim ComPos1, ComPos2, WordLen As Integer ' used to parse input data
'typedef struct {...the expected data from the client
    'char LicId[18];
    'char Name1[30];
    'char Name2[30];
    'char Addr1[30];
    'char Addr2[30];
    'char City [16];
    'char State[4];
    'char Sex  [4];
    'char Hght [4];
    'char Wght [4];
    'char Hair [4];
    'char Eyes [4];
'} DmvType;

    ComPos1 = 1 'strip out LicId
    ComPos2 = InStr(ComPos1, szInput, ",", 1) 'find the next comma
    WordLen = (ComPos2 - ComPos1) 'Calculate the model number length
    szDmvId = Mid(szInput, ComPos1, WordLen) 'Set the value
    
    ComPos1 = ComPos2 + 1 'strip out the Name1
    ComPos2 = InStr(ComPos1, szInput, ",") 'find the next comma
    WordLen = (ComPos2 - ComPos1) 'Calculate the length
    szName = Mid(szInput, ComPos1, WordLen)  'Set the value
    
    ComPos1 = ComPos2 + 1 'strip out Name2...ignore
    ComPos2 = InStr(ComPos1, szInput, ",") 'find the next comma
    'WordLen = (ComPos2 - ComPos1) 'Calculate the length
    'szName = Mid(szInput, ComPos1, WordLen)  'Set the value
    
    ComPos1 = ComPos2 + 1 'strip out Addr1
    ComPos2 = InStr(ComPos1, szInput, ",") 'find the next comma
    WordLen = (ComPos2 - ComPos1) 'Calculate the length
    szAddr1 = Mid(szInput, ComPos1, WordLen)  'Set the value
    
    ComPos1 = ComPos2 + 1 'strip out Addr2.ignore
    ComPos2 = InStr(ComPos1, szInput, ",") 'find the next comma
    'WordLen = (ComPos2 - ComPos1) 'Calculate the length
    'SzAddr2 = Mid(szInput, ComPos1, WordLen)  'Set the value
    
    ComPos1 = ComPos2 + 1 'strip out City
    ComPos2 = InStr(ComPos1, szInput, ",") 'find the next comma
    WordLen = (ComPos2 - ComPos1) 'Calculate the length
    szCity = Mid(szInput, ComPos1, WordLen)  'Set the value
    
    ComPos1 = ComPos2 + 1 'strip out State
    ComPos2 = InStr(ComPos1, szInput, ",") 'find the next comma
    WordLen = (ComPos2 - ComPos1) 'Calculate the length
    szState = Mid(szInput, ComPos1, WordLen)  'Set the value

End Sub

Private Function HashId(szInput) As String 'return next to last digit for now
    HashId = Mid(szInput, Len(szInput) - 1, 1)
End Function

Private Sub tcpServer_DataArrival(index As Integer, ByVal bytesTotal As Long)
    Dim i As Integer
    Dim sqlRequest As String   'temp area used to build response
    Dim MyPath, BitMap, DmvBitMap As String 'used for image .bmp lookup
      
    On Error GoTo 0         ' Turn off error trapping.
    On Error Resume Next    ' Defer error trapping.
    
    'read the socket data from the client into szInput
    tcpServer(index).GetData szInput
    Call ParseIt
    
    'do a lookup on the key
    sqlRequest = "SELECT * FROM DmvTable WHERE DmvId = '" & szDmvId & "'"
    Set MyRS = MyDB.OpenRecordset(sqlRequest)
    
    'lookup this drivers DMV photo file name
    If MyRS.EOF Then ' then we've got a new client
        szImageName = "Tbmp000" + HashId(szDmvId) + ".bin"
        With MyRS
            .AddNew
            !DmvId = szDmvId
            !DmvName = szName
            !DmvPhoto = szImageName
            !DmvAddr1 = szAddr1
            !DmvAddr2 = szCity + "," + szState
            .Update
            .Bookmark = .LastModified
        End With
        
        Call UpdateGrid 'repaint the grid to show updates
        
    Else ' I've seen this one before...
        szImageName = MyRS!DmvPhoto
    End If
  
    'szImageName now contains the drivers .BMP file name
    MyPath = CurDir: DmvBitMap = MyPath + "\" + szImageName
    
    Open DmvBitMap For Binary Access Read Lock Read As #1  'open a 160x150x2 .bin file
    i = Err.Number
    If Err.Number = 53 Then
        i = MsgBox("I Can't find : " + DmvBitMap, vbCritical, ".BIN lookup failed")
        Err.Clear    ' Clear Err object fields
    End If
    
    'now read the palm formated bitmap into our internal buffer
    i = LOF(1)
    BitMap = Input(i, #1)
    Close #1
    
    'now send tne BitMap to the client as binary data
    tcpServer(index).SendData BitMap

    lblStatus.Caption = szImageName + "->" + Str(i) _
                        + " bytes sent to " + tcpServer(0).RemoteHostIP

End Sub

Private Sub cmdDelete_Click()
    'remove the record from the Phone table
    With MyRS
        .index = "PrimaryKey"
        '.Seek "=", txtExtension.Text
        If .NoMatch Then
            'do nothing
        Else
            .Delete
        End If
    End With

End Sub

