[InstallSHIELD Silent]
Version=v3.00.00
File=Response File

[DlgOrder]
Dlg0=Welcome-0
Dlg1=SdRegisterUser-0
Dlg2=AskDestPath-0
Dlg3=SdSelectFolder-0
Dlg4=SdFinish-0
Count=5

[Welcome-0]
Result=1

[SdRegisterUser-0]
Result=1
szName=Name
szCompany=Company

[AskDestPath-0]
szPath=<ProgramFilesDir>\Symbol\MSR 3000 Configurator
Result=1

[SdSelectFolder-0]
Result=1
szFolder=MSR 3000 Setup

[SdFinish-0]
Result=1
bOpt1=0
bOpt2=0
