#include <PalmOS.h>
#include <PalmOSGlue.h>

/* MathLib headers */
#include "MathLib.h"

#include "PlotterZ.h"
#include "PlotterZ_Rsc.h"

/*********************************************************************
 * Entry Points
 *********************************************************************/

/*********************************************************************
 * Global variables
 *********************************************************************/
/* 
 * g_prefs
 * cache for application preferences during program execution 
 */
PlotterZPreferenceType g_prefs;

/*********************************************************************
 * Internal Constants
 *********************************************************************/

/* Define the minimum OS version we support */
#define ourMinVersion    sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0)
#define kPalmOS20Version sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0)


static void * GetObjectPtr(UInt16 objectID)
{
	FormType * frmP;

	frmP = FrmGetActiveForm();
	return FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID));
}

static void MainFormInit(FormType *frmP)
{
	FieldType *field;
	const char *wizardDescription;
	UInt16 fieldIndex;

	fieldIndex = FrmGetObjectIndex(frmP, MainDescriptionField);
	field = (FieldType *)FrmGetObjectPtr(frmP, fieldIndex);
	FrmSetFocus(frmP, fieldIndex);

	wizardDescription =
		"C application\n"
		"Creator Code: KUKI\n"
		"\n"
		"Other SDKs:\n"
		
"    MathLib (floating-point math)\n";
				
	/* dont stack FldInsert calls, since each one generates a
	 * fldChangedEvent, and multiple uses can overflow the event queue */
	FldInsert(field, wizardDescription, StrLen(wizardDescription));
}

static Boolean MainFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
		case OptionsAboutPlotterZ:
		{
			FormType * frmP;

			/* Clear the menu status from the display */
			MenuEraseStatus(0);

			/* Display the About Box. */
			frmP = FrmInitForm (AboutForm);
			FrmDoDialog (frmP);                    
			FrmDeleteForm (frmP);

			handled = true;
			break;
		}
		case OptionsPreferences:
		{
			FormType * frmP;
			ControlType * cbox = NULL;
			FieldType * field = NULL;			
			UInt16 controlID = 0;
			MemHandle handle = 0;

			/* Clear the menu status from the display */
			MenuEraseStatus(0);

			/* Initialize the preference form. */
			frmP = FrmInitForm (PrefsForm);

			/* 
			 * Set the controls in the preference dialog to reflect our 
			 * preference data structure
			 */
			cbox = (ControlType *)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, PrefsSetting1Checkbox));
			if (cbox)
				CtlSetValue(cbox, g_prefs.pref1);
			
			field = (FieldType *)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, PrefsSetting2Field));
			if (field && (g_prefs.pref2[0] != '\0'))
			{
				MemPtr text = NULL;
				UInt32 len = StrLen(g_prefs.pref2);
				
				handle = FldGetTextHandle(field);
				if (!handle)
					handle = MemHandleNew(len + 1);
				else
					MemHandleResize(handle, len + 1);

				text = MemHandleLock(handle);
				if (text)
					StrCopy((char *)text, g_prefs.pref2);

				MemHandleUnlock(handle);
				FldSetTextHandle(field, handle);
				
				handle = 0;
			}

			/* 
			 * Display the preferences dialog. The call will block until 
			 * the dialog is dismissed
			 */
			controlID = FrmDoDialog (frmP);
			
			/* controlID contains the ID of the button used to dismiss the dialog */
			if (controlID == PrefsOKButton)
			{
				/* 
				 * The user hit the OK button. Get the value of the controls 
				 * and store them in our pref struct 
				 */
				if (cbox)
					g_prefs.pref1 = CtlGetValue(cbox);
					
				if (field)
				{
					handle = FldGetTextHandle(field);
					if (handle)
					{				
						MemPtr text = MemHandleLock(handle);
						if (text)
						{
							/* Guard against the field text being longer than our pref's buffer */
							UInt32 len = StrLen((const char *)text);
							UInt32 count = (len > (sizeof(g_prefs.pref2) - 1)) ? (sizeof(g_prefs.pref2) - 1) : len;
							MemMove(g_prefs.pref2, text, count);
							g_prefs.pref2[count] = '\0';
						}

						MemHandleUnlock(handle);
					}
				}
			}
			
			/* Clean up */
			FrmDeleteForm (frmP);

			handled = true;
			break;
		}

	}

	return handled;
}

static Boolean MainFormHandleEvent(EventType * eventP)
{
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) 
	{
		case menuEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			MainFormInit(frmP);
			handled = true;
			break;
            
        case frmUpdateEvent:
			/* 
			 * To do any custom drawing here, first call
			 * FrmDrawForm(), then do your drawing, and
			 * then set handled to true. 
			 */
			break;
			
		case ctlSelectEvent:
		{
			if (eventP->data.ctlSelect.controlID == MainClearTextButton)
			{
				/* The "Clear" button was hit. Clear the contents of the field. */
				FieldType * field = (FieldType*)GetObjectPtr(MainDescriptionField);
				if (field)
				{
					FldDelete(field, 0, 0xFFFF);					
					FldDrawField(field);
				}
				break;
			}

			break;
		}
	}
    
	return handled;
}

static Boolean AppHandleEvent(EventType * eventP)
{
	UInt16 formId;
	FormType * frmP;

	if (eventP->eType == frmLoadEvent)
	{
		/* Load the form resource. */
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		/* 
		 * Set the event handler for the form.  The handler of the
		 * currently active form is called by FrmHandleEvent each
		 * time is receives an event. 
		 */
		switch (formId)
		{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

		}
		return true;
	}

	return false;
}

static void AppEventLoop(void)
{
	UInt16 error;
	EventType event;

	do 
	{
		/* change timeout if you need periodic nilEvents */
		EvtGetEvent(&event, evtWaitForever);

		if (! SysHandleEvent(&event))
		{
			if (! MenuHandleEvent(0, &event, &error))
			{
				if (! AppHandleEvent(&event))
				{
					FrmDispatchEvent(&event);
				}
			}
		}
	} while (event.eType != appStopEvent);
}

static Err AppStart(void)
{
	UInt16 prefsSize;

	/* load the MathLib library, save reference in MathLibRef */
	{
		MathLibRef = sysInvalidRefNum;

		if (errNone != SysLibFind(MathLibName, &MathLibRef))
		{
			SysLibLoad(LibType, MathLibCreator, &MathLibRef);
		}
		
		if (MathLibRef != sysInvalidRefNum)
		{
			if (mlErrNone != MathLibOpen(MathLibRef, MathLibVersion))
			{
				/* if error loading library, reset reference */
				MathLibRef = sysInvalidRefNum;
			}
		}
	}
	/* Read the saved preferences / saved-state information. */
	prefsSize = sizeof(g_prefs);
	if (PrefGetAppPreferences(
		appFileCreator, appPrefID, &g_prefs, &prefsSize, true) == 
		noPreferenceFound)
	{
		/* no prefs; initialize pref struct with default values */
		g_prefs.pref1 = false;
		g_prefs.pref2[0] = '\0';
	}

	return errNone;
}

static void AppStop(void)
{
	/* 
	 * Write the saved preferences / saved-state information.  This
	 * data will be saved during a HotSync backup. 
	 */
	PrefSetAppPreferences(
		appFileCreator, appPrefID, appPrefVersionNum, 
		&g_prefs, sizeof(g_prefs), true);
        
	/* Close all the open forms. */
	FrmCloseAllForms();

	/* unload the MathLib library */
	if (MathLibRef != sysInvalidRefNum)
	{
		UInt16 useCount;

		MathLibClose(MathLibRef, &useCount);
		if (useCount == 0)
		{
			SysLibRemove(MathLibRef);
		}

		MathLibRef = sysInvalidRefNum;
	}

}

static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	/* See if we're on in minimum required version of the ROM or later. */
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
	{
		if ((launchFlags & 
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
		{
			FrmAlert (RomIncompatibleAlert);

			/* Palm OS versions before 2.0 will continuously relaunch this
			 * app unless we switch to another safe one. */
			if (romVersion < kPalmOS20Version)
			{
				AppLaunchWithCommand(
					sysFileCDefaultApp, 
					sysAppLaunchCmdNormalLaunch, NULL);
			}
		}

		return sysErrRomIncompatible;
	}

	return errNone;
}

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) return (error);

	switch (cmd)
	{
		case sysAppLaunchCmdNormalLaunch:
			error = AppStart();
			if (error) 
				return error;

			/* 
			 * start application by opening the main form
			 * and then entering the main event loop 
			 */
			FrmGotoForm(MainForm);
			AppEventLoop();

			AppStop();
			break;
	}

	return errNone;
}
