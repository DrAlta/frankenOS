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
      Caption         =   "Populate the PluDemo.mdb file"
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
' populates the data base with 50 records
Option Explicit

Private Sub Command1_Click()
   Unload Form1
   End
End Sub

Private Sub Form_Load()

    Dim MyDB As Database
    Dim MyRs As Recordset
    Dim i As Long
    
    Set MyDB = OpenDatabase("H:\projects\VBhost\PluDemo.mdb")
    Set MyRs = MyDB.OpenRecordset("Titles", dbOpenDynaset)
    For i = 0 To 50
        AddRecord MyRs, i
    Next i
   
    MyRs.Close
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
        !ISBN = "1-861000-39" & Str(i * -1)
        !Barcode = "1234567890123"
        .Update
        .Bookmark = .LastModified
    End With

End Function


