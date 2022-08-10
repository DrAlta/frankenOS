#include <PalmOS.h>

#ifdef __GNUC__
#include "callback.h"
#endif

#include "helloRsc.h"


static Err StartApplication(void)
{
    FrmGotoForm(MainForm);
    return 0;
}


static void StopApplication(void)
{
}


static void SaySomething(UInt16 alertID)
{
    FormType   *form = FrmGetActiveForm();
    FieldType  *field;
    MemHandle  h;
    

    field = FrmGetObjectPtr(form, FrmGetObjectIndex(form,
                            MainNameField));
    h = FldGetTextHandle(field);
    if (h) {
        Char  *s;
        
        s = MemHandleLock((void *)h);
        if (*s != '\0') {
            FrmCustomAlert(alertID, s, NULL, NULL);
        } else {
            FrmCustomAlert(alertID, "whoever you are", NULL,
                           NULL);
        }
        MemHandleUnlock((void *)h);
    } else {
        FrmCustomAlert(alertID, "whoever you are", NULL, NULL);
    }
}
        

static Boolean MainMenuHandleEvent(UInt16 menuID)
{
    Boolean    handled = false;
    FormType   *form;
    FieldType  *field;
    

    form = FrmGetActiveForm();
    field = FrmGetObjectPtr(form,
        FrmGetObjectIndex(form, MainNameField));

    switch (menuID) {
        case MainEditUndo:
            FldUndo(field);
            handled = true;
            break;
        case MainEditCut:
            FldCut(field);
            handled = true;
            break;
        case MainEditCopy:
            FldCopy(field);
            handled = true;
            break;
        case MainEditPaste:
            FldPaste(field);
            handled = true;
            break;
        case MainEditSelectAll:
            FldSetSelection(field, 0,
                            FldGetTextLength(field));
            handled = true;
            break;
            
        case MainEditKeyboard:
            SysKeyboardDialog(kbdDefault);
            handled = true;
            break;
            
        case MainEditGraffitiHelp:
            SysGraffitiReferenceDialog(referenceDefault);
            handled = true;
            break;
            
        case MainOptionsAboutHelloWorld:
            FrmAlert(AboutAlert);
            handled = true;
            break;
            
        default:
            break;
    }

    return handled;
}


static Boolean MainFormHandleEvent(EventPtr event)
{
    Boolean  handled = false;
    
#ifdef __GNUC__
    CALLBACK_PROLOGUE;
#endif

    switch (event->eType) {
        case frmOpenEvent:
        {
            FormType  *form = FrmGetActiveForm();

            FrmDrawForm(form);
            FrmSetFocus(form, FrmGetObjectIndex(form,
                MainNameField));
            handled = true;
        }
            break;

        case ctlSelectEvent:
            switch (event->data.ctlSelect.controlID) {
                case MainHelloButton:
                    SaySomething(HelloAlert);
                    handled = true;
                    break;
                
                case MainGoodbyeButton:
                    SaySomething(GoodbyeAlert);
                    handled = true;
                    break;

                default:
                    break;
            }
            break;

        case menuEvent:
            handled =
                MainMenuHandleEvent(event->data.menu.itemID);
            break;

        default:
            break;
    }

#ifdef __GNUC__
    CALLBACK_EPILOGUE;
#endif

    return handled;
}


static Boolean ApplicationHandleEvent(EventPtr event)
{
    FormType  *form;
    UInt16    formID;
    Boolean   handled = false;
    

    if (event->eType == frmLoadEvent) {
        formID = event->data.frmLoad.formID;
        form = FrmInitForm(formID);
        FrmSetActiveForm(form);
        
        switch (formID) {
            case MainForm:
                FrmSetEventHandler(form, MainFormHandleEvent);
                break;

            default:
                break;
        }
        handled = true;
    }

    return handled;
}


static void EventLoop(void)
{
    EventType  event;
    UInt16     error;

    
    do {
        EvtGetEvent(&event, evtWaitForever);
        if (! SysHandleEvent(&event))
            if (! MenuHandleEvent(0, &event, &error))
                if (! ApplicationHandleEvent(&event))
                    FrmDispatchEvent(&event);
    } while (event.eType != appStopEvent);
}


UInt32 PilotMain(UInt16 launchCode, MemPtr cmdPBP,
                 UInt16 launchFlags)
{
    Err  err;


    switch (launchCode) {
        case sysAppLaunchCmdNormalLaunch:
            if ((err = StartApplication()) == 0) {
                EventLoop();
                StopApplication();
            }
            break;

        default:
            break;
    }

    return err;
}
