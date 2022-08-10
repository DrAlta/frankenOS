/************************************************************************
* COPYRIGHT:   Copyright  ©  1999 Symbol Technologies, Inc. 
*
* FILE:        about.c
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
#include "Print SampleRsc.h"
#include "about.h"

#pragma mark -- Static Prototypes --
static Word FormInit();

Boolean AboutHandleEvent(EventPtr event )
{
	Boolean bHandled = false;
		
	switch( event->eType )
	{
		case frmOpenEvent:
		{
			FormPtr frm;

			frm = FrmGetActiveForm ();
			FormInit();
			FrmDrawForm (frm);
			bHandled = true;
			break;
		}
		
		case ctlSelectEvent:
		{
			if( event->data.ctlSelect.controlID == AboutOkButton )
			{
				FrmReturnToForm(0);
				bHandled = true;
			}
			break;
		}
	}
	return bHandled;

}

static Word FormInit()
{
	return 0;
}
