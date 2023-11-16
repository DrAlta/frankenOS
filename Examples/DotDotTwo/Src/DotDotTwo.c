/******************************************************************************
 *
 * Copyright (c) 1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: DotDotTwo.c
 *
 *****************************************************************************/

#include <PalmOS.h>

#include <SysEvtMgr.h>



static Boolean MainViewHandleEvent(EventPtr event)

{

	Boolean		handled = false;



	if (event->eType == ctlSelectEvent)

	{

   		if (event->data.ctlEnter.controlID == 1005)

		{

			SysReset();

			handled = true;

		}

	}

	else if (event->eType == penDownEvent)

	{

		handled = FrmHandleEvent (FrmGetActiveForm (), event);

	}



	return(handled);

}



static Boolean ApplicationHandleEvent(EventPtr event)

{

	FormPtr	frm;

	Int16		formId;

	Boolean	handled = false;

	char	statusStr[30] = {0};

	short	stringIndex = 0;



	if (event->eType == frmLoadEvent)

	{

		formId = event->data.frmLoad.formID;

		frm = FrmInitForm(formId);

		FrmSetActiveForm(frm);

		switch (formId)

		{

			case 1000:

				FrmSetEventHandler(frm, MainViewHandleEvent);

				break;

		}

		handled = true;

		if (SysLaunchConsole())

		{

			stringIndex++;

		}



		SysStringByIndex(1000, stringIndex, statusStr, sizeof(statusStr));

		FrmCopyLabel (frm, 1008, statusStr);

		FrmDrawForm(frm);

	}

	

	return handled;

}



static void EventLoop(void)

{

	EventType	event;

	UInt16			error;

	

	do

		{

		// Get the next available event.

		EvtGetEvent(&event, evtWaitForever);

		

		// Give the system a chance to handle the event.

		if (! SysHandleEvent(&event))



			// P2. Give the menu bar a chance to update and handle the event.	

			if (! MenuHandleEvent(0, &event, &error))



				// P3. Give the application a chance to handle the event.

				if (! ApplicationHandleEvent(&event))



					// P3. Let the form object provide default handling of the event.

					FrmDispatchEvent(&event);

		}

	while (event.eType != appStopEvent);

}





UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)

{

	if (cmd == sysAppLaunchCmdNormalLaunch)

	{

		FrmGotoForm(1000);

		EventLoop();

	}



	return 0;

}

