VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   2265
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4455
   LinkTopic       =   "Form1"
   ScaleHeight     =   2265
   ScaleWidth      =   4455
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "Exit"
      Height          =   495
      Left            =   1440
      TabIndex        =   0
      Top             =   1320
      Width           =   1215
   End
   Begin VB.Label Label1 
      Caption         =   "Create the PluDemo.mdb file"
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
      Left            =   360
      TabIndex        =   1
      Top             =   480
      Width           =   4095
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' creates the data base
Option Explicit

Private Sub Command1_Click()
   Unload Form1
   End
End Sub

Private Sub Form_Load()

    Dim MyDB As Database
    Dim MyWs As Workspace
    Dim MyRs As Recordset
    Dim TitTd As TableDef
    Dim TitFlds(4) As Field
    Dim TitIdx As Index
    Dim i As Integer
  
    Set MyWs = DBEngine.Workspaces(0)
    Set MyDB = MyWs.CreateDatabase("h:\projects\vbhost\PluDemo.mdb", dbLangGeneral, dbVersion30)

    ' Create new TableDef for Titles table
    Set TitTd = MyDB.CreateTableDef("Titles")
    
    ' Create fields for Titles Table
    Set TitFlds(0) = TitTd.CreateField("Title", dbText)
    TitFlds(0).Size = 40
    Set TitFlds(1) = TitTd.CreateField("Description", dbText)
    TitFlds(1).Size = 40
    Set TitFlds(2) = TitTd.CreateField("ISBN", dbText)
    TitFlds(2).Size = 40
    Set TitFlds(3) = TitTd.CreateField("Barcode", dbText)
    TitFlds(3).Size = 40
    
    Set TitIdx = TitTd.CreateIndex("ISBN")
    TitIdx.Primary = True
    TitIdx.Unique = True

    ' Append fields to Titles TableDef.
    For i = 0 To 3
        TitTd.Fields.Append TitFlds(i)
    Next i

    ' Save TableDef definition by appending it to TableDefs
    ' collection.
    MyDB.TableDefs.Append TitTd
    MyDB.Close
  
End Sub

Function AddRecord(rstTemp As Recordset, i As Long)
        
    ' Adds a new record to a Recordset using the data passed
    ' by the calling procedure. The new record is then made
    ' the current record.
    With rstTemp
        .AddNew
        !Title = "the title goes here"
        !Description = "the description goes here"
        !ISBN = "1-861000-39-" & Str$(i)
        !Barcode = "1234567890123"
        .Update
        .Bookmark = .LastModified
    End With

End Function


