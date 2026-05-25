#include <PalmOS.h>
#include <PalmOSGlue.h>

/* MathLib headers */
#include "MathLib.h"

/* Core modules */
#include "../../formula-z/fz.h"
#include "../../evaluator-z/ez.h"
#include "../../renderer-z/rz.h"
#include "../../common/utils.h"
#include "../../common/constants.h"

#include "PlotterZ.h"
#include "PlotterZ_Rsc.h"
#include "BitmapBuffer.h"

/*********************************************************************
 * Global variables
 *********************************************************************/
PlotterZPreferenceType g_prefs;
BmpBuffer*             g_pBmpBufFormula;
BmpBuffer*             g_pBmpCanvas;

FzAstNode*  g_pAstExpr      = NULL;
EzMachine*  g_pVm           = NULL;
RenderNode* g_pRenderNode   = NULL;
RenderConfig g_RenderConfig;

static char g_szExpr[256] = "sin(sqr(x^2+y^2))";
static char g_szErrorBuf[EZ_ERROR_CONTENT_LENGTH];
static Boolean g_bParseOk = false;

#define CURRENT_FONT_WIDTH 6
#define CURRENT_FONT_HEIGHT 8

/*********************************************************************
 * Internal Constants
 *********************************************************************/

/* Define the minimum OS version we support */
#define ourMinVersion    sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0)
#define kPalmOS20Version sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0)


static void * GetObjectPtr(UInt16 objectID) {
	FormType * frmP;

	frmP = FrmGetActiveForm();
	return FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID));
}

/*====================================================
 * Renderer-Z interface wrappers (draw on formula bmp)
 *====================================================*/
static void rzSetPixel(int x, int y) {
    BmpBuffer_SetPixel(g_pBmpBufFormula, x, y, 1);
}

static void rzPlotLine(int x0, int y0, int x1, int y1) {
    BmpBuffer_PlotLine(g_pBmpBufFormula, x0, y0, x1, y1, 1);
}

static void rzPutChar(int x, int y, UInt8 ch) {
    BmpBuffer_PutChar(g_pBmpBufFormula, x, y, ch, 1);
}

/*====================================================
 * Parse expression and render to formula bitmap
 *====================================================*/
static void parseAndRender(void) {
    Int16 x, y;
    Int16 iW, iH;

    iW = g_pBmpBufFormula->iW;
    iH = g_pBmpBufFormula->iH;

    /* Clear formula bitmap */
    MemSet(g_pBmpBufFormula->pRaw,
        (UInt32)(g_pBmpBufFormula->iPitch * g_pBmpBufFormula->iH), 0);

    /* Destroy previous state (keep VM singleton) */
    if (g_pRenderNode != NULL) {
        RenderNode_Destroy(g_pRenderNode);
        g_pRenderNode = NULL;
    }
    if (g_pAstExpr != NULL) {
        FzAstNode_Destroy(g_pAstExpr);
        g_pAstExpr = NULL;
    }

    g_bParseOk = false;

    /* Parse expression */
    g_pAstExpr = FzParser_ParseExpression(g_szExpr);
    if (g_pAstExpr == NULL) {
        const char* szMsg = "Syntax Error";
        Int16 iLen = (Int16)(StrLen(szMsg) * CURRENT_FONT_WIDTH);
        x = (iW - iLen) / 2;
        y = (iH - CURRENT_FONT_HEIGHT) / 2;
        BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)szMsg, 1);
        return;
    }

    /* Create VM singleton and compile */
    if (g_pVm == NULL) {
        g_pVm = EzMachine_Create();
        if (g_pVm != NULL) {
            EzMachine_DeclareVariable(g_pVm, "x");
            EzMachine_DeclareVariable(g_pVm, "y");
            EzMachine_DeclareVariable(g_pVm, "pi");
            EzMachine_AllocateVariables(g_pVm);
            EzMachine_SetVariableByIndex(g_pVm, 2, PZ_PI);
        }
    }
    if (g_pVm != NULL) {
        EzError iCompileErr;
        iCompileErr = EzMachine_Compile(g_pVm, g_pAstExpr, g_szErrorBuf);
        if (iCompileErr != EZERR_NONE) {
            /* Line 1: error type */
            const char* szErrType;
            Int16 iLen;
            switch (iCompileErr) {
                case EZERR_VARIABLE_UNDEFINED:
                    szErrType = "Undefined Variable:"; break;
                case EZERR_FUNCTION_UNDEFINED:
                    szErrType = "Undefined Function:"; break;
                case EZERR_FUNCTION_PARAM_MISMATCH:
                    szErrType = "Parameter Mismatch:"; break;
                default:
                    szErrType = "Compile Error:"; break;
            }
            iLen = (Int16)(StrLen(szErrType) * CURRENT_FONT_WIDTH);
            x = (iW - iLen) / 2;
            y = (iH - CURRENT_FONT_HEIGHT * 2) / 2;
            BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)szErrType, 1);

            /* Line 2: detail */
            iLen = (Int16)(StrLen(g_szErrorBuf) * CURRENT_FONT_WIDTH);
            x = (iW - iLen) / 2;
            y += CURRENT_FONT_HEIGHT;
            BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)g_szErrorBuf, 1);
            return;
        }
    }

    /* Build render tree */
    g_pRenderNode = Render_Transform(g_pAstExpr);
    if (g_pRenderNode == NULL) {
        const char* szMsg = "Render error";
        Int16 iLen = (Int16)(StrLen(szMsg) * CURRENT_FONT_WIDTH);
        x = (iW - iLen) / 2;
        y = (iH - CURRENT_FONT_HEIGHT) / 2;
        BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)szMsg, 1);
        return;
    }

    /* Estimate size and draw */
    RenderNode_EstimateSize(g_pRenderNode, &g_RenderConfig);

    /* Line 1: f(x,y)= on filled background */
    {
        const char* szLabel = "f(x,y)=";
        BmpBuffer_FillRect(g_pBmpBufFormula, 0, 0,
            iW, CURRENT_FONT_HEIGHT, 1);
        BmpBuffer_PutText(g_pBmpBufFormula, 0, 0, (UInt8*)szLabel, 0);
    }

    /* Line 2: formula rendering */
    y = CURRENT_FONT_HEIGHT + 2;
    RenderNode_Draw(g_pRenderNode, &g_RenderConfig,
        0, y + g_pRenderNode->sLayout.iAscent);

    /* Line 3: Ready> on filled background */
    {
        const char* szReady = "Ready>";
        Int16 iLen = (Int16)(StrLen(szReady) * CURRENT_FONT_WIDTH);
        x = iW - iLen - 2;
        y = iH - CURRENT_FONT_HEIGHT;
        BmpBuffer_FillRect(g_pBmpBufFormula, 0, y,
            iW, CURRENT_FONT_HEIGHT, 1);
        BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)szReady, 0);
    }

    g_bParseOk = true;
}

static void MainFormInit(FormType *frmP) {
	/* Set default expression text */
	{
		FieldType *fieldInput;
		UInt16 fieldIndex;

		fieldIndex = FrmGetObjectIndex(frmP, MainFormulaField);
		fieldInput = (FieldType *)FrmGetObjectPtr(frmP, fieldIndex);
		FrmSetFocus(frmP, fieldIndex);

		FldInsert(fieldInput, g_szExpr, StrLen(g_szExpr));
	}

	/* Idle screen on formula bitmap */
	{
		Int16 iW = g_pBmpBufFormula->iW;
		Int16 iH = g_pBmpBufFormula->iH;
		Int16 y, iLen;
		static const char* szLines[] = {
			"\x17 Plotter-Z \x18",
			"3D Graphing Tool",
			"Input and Parse to start"
		};
		const int iCount = sizeof(szLines) / sizeof(szLines[0]);
		int i;

		MemSet(g_pBmpBufFormula->pRaw,
			(UInt32)(g_pBmpBufFormula->iPitch * g_pBmpBufFormula->iH), 0);

		BmpBuffer_FillRect(g_pBmpBufFormula, 0, 0, iW, iH, 0);

		y = (iH - iCount * CURRENT_FONT_HEIGHT) / 2;
		for (i = 0; i < iCount; ++i) {
			iLen = (Int16)(StrLen(szLines[i]) * CURRENT_FONT_WIDTH);
			BmpBuffer_PutText(g_pBmpBufFormula,
				(iW - iLen) / 2, y, (UInt8*)szLines[i], 1);
			y += CURRENT_FONT_HEIGHT;
		}

		/* Trigger redraw */
		FrmUpdateForm(MainForm, frmRedrawUpdateCode);
	}

	/* Draw button starts disabled */
	{
		ControlType *ctlDraw;
		ctlDraw = (ControlType*)FrmGetObjectPtr(frmP,
			FrmGetObjectIndex(frmP, MainDrawButton));
		if (ctlDraw != NULL) {
			CtlHideControl(ctlDraw);
		}
	}
}

static Boolean MainFormDoCommand(UInt16 command) {
	Boolean handled = false;

	switch (command) {
		case OptionsAboutPlotterZ: {
			FormType * frmP;

			MenuEraseStatus(0);

			frmP = FrmInitForm (AboutForm);
			FrmDoDialog (frmP);
			FrmDeleteForm (frmP);

			handled = true;
			break;
		}
	}

	return handled;
}

static Boolean DrawFormHandleEvent(EventType * eventP) {
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) {
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			handled = true;
			break;

		case penDownEvent:
			FrmGotoForm(MainForm);
			handled = true;
			break;
	}

	return handled;
}

static Boolean MainFormHandleEvent(EventType * eventP) {
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) {
		case menuEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			{
				/* Restore field text */
				FieldType *fieldInput;
				UInt16 fieldIndex;
				fieldIndex = FrmGetObjectIndex(frmP, MainFormulaField);
				fieldInput = (FieldType *)FrmGetObjectPtr(frmP, fieldIndex);
				FldDelete(fieldInput, 0, 0xFFFF);
				FldInsert(fieldInput, g_szExpr, StrLen(g_szExpr));
			}
			FrmUpdateForm(MainForm, frmRedrawUpdateCode);
			handled = true;
			break;

		case frmUpdateEvent:
			WinDrawBitmap(g_pBmpBufFormula->pBmp, 0, 15);
			handled = true;
			break;

		case ctlSelectEvent: {
			if (eventP->data.ctlSelect.controlID == MainParseButton) {
				FieldType *field;
				MemHandle handle;
				MemPtr text;
				UInt32 len, count;
				ControlType *ctlDraw;

				/* Read text from formula field */
				field = (FieldType*)GetObjectPtr(MainFormulaField);
				if (field != NULL) {
					handle = FldGetTextHandle(field);
					if (handle) {
						text = MemHandleLock(handle);
						if (text) {
							len = StrLen((const char *)text);
							count = (len > (sizeof(g_szExpr) - 1))
							      ? (sizeof(g_szExpr) - 1) : len;
							MemMove(g_szExpr, text, count);
							g_szExpr[count] = '\0';
						}
						MemHandleUnlock(handle);
					}
				}

				parseAndRender();

				/* Enable or disable Draw button */
				ctlDraw = (ControlType*)GetObjectPtr(MainDrawButton);
				if (ctlDraw != NULL) {
					if (g_bParseOk)
						CtlShowControl(ctlDraw);
					else
						CtlHideControl(ctlDraw);
				}

				/* Trigger redraw */
				FrmUpdateForm(MainForm, frmRedrawUpdateCode);

				handled = true;
				break;
			}

			if (eventP->data.ctlSelect.controlID == MainDrawButton) {
				FrmGotoForm(DrawForm);
				handled = true;
				break;
			}

			break;
		}
	}

	return handled;
}

static Boolean AppHandleEvent(EventType * eventP) {
	UInt16 formId;
	FormType * frmP;

	if (eventP->eType == frmLoadEvent) {
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		switch (formId) {
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

			case DrawForm:
				FrmSetEventHandler(frmP, DrawFormHandleEvent);
				break;
		}
		return true;
	}

	return false;
}

static void AppEventLoop(void) {
	UInt16 error;
	EventType event;

	do
	{
		EvtGetEvent(&event, evtWaitForever);

		if (! SysHandleEvent(&event)) {
			if (! MenuHandleEvent(0, &event, &error)) {
				if (! AppHandleEvent(&event)) {
					FrmDispatchEvent(&event);
				}
			}
		}
	} while (event.eType != appStopEvent);
}

static Err AppStart(void) {
	UInt16 prefsSize;

	/* load the MathLib library */
	{
		MathLibRef = sysInvalidRefNum;

		if (errNone != SysLibFind(MathLibName, &MathLibRef)) {
			SysLibLoad(LibType, MathLibCreator, &MathLibRef);
		}

		if (MathLibRef != sysInvalidRefNum) {
			if (mlErrNone != MathLibOpen(MathLibRef, MathLibVersion)) {
				MathLibRef = sysInvalidRefNum;
			}
		}
	}

	/* Create bitmap buffers */
	g_pBmpBufFormula = BmpBuffer_Create(160, 100);
	g_pBmpCanvas     = BmpBuffer_Create(160, 160);

	/* Set up renderer-Z config */
	g_RenderConfig.sInterfaces.setPixel = rzSetPixel;
	g_RenderConfig.sInterfaces.plotLine = rzPlotLine;
	g_RenderConfig.sInterfaces.putChar  = rzPutChar;
	RenderConfig_GetDefaultStyle(&g_RenderConfig);

	/* Read saved preferences */
	prefsSize = sizeof(g_prefs);
	if (PrefGetAppPreferences(
		appFileCreator, appPrefID, &g_prefs, &prefsSize, true) ==
		noPreferenceFound) {
		g_prefs.pref1 = false;
		g_prefs.pref2[0] = '\0';
	}

	return errNone;
}

static void AppStop(void) {
	PrefSetAppPreferences(
		appFileCreator, appPrefID, appPrefVersionNum,
		&g_prefs, sizeof(g_prefs), true);

	FrmCloseAllForms();

	/* Destroy renderer-Z objects */
	if (g_pRenderNode != NULL) RenderNode_Destroy(g_pRenderNode);
	if (g_pAstExpr != NULL) FzAstNode_Destroy(g_pAstExpr);
	if (g_pVm != NULL) EzMachine_Destroy(g_pVm);

	/* Destroy bitmap buffers */
	BmpBuffer_Destroy(g_pBmpCanvas);
	BmpBuffer_Destroy(g_pBmpBufFormula);

	/* unload the MathLib library */
	if (MathLibRef != sysInvalidRefNum) {
		UInt16 useCount;

		MathLibClose(MathLibRef, &useCount);
		if (useCount == 0) {
			SysLibRemove(MathLibRef);
		}

		MathLibRef = sysInvalidRefNum;
	}
}

static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags) {
	UInt32 romVersion;

	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion) {
		if ((launchFlags &
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) {
			FrmAlert (RomIncompatibleAlert);

			if (romVersion < kPalmOS20Version) {
				AppLaunchWithCommand(
					sysFileCDefaultApp,
					sysAppLaunchCmdNormalLaunch, NULL);
			}
		}

		return sysErrRomIncompatible;
	}

	return errNone;
}

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags) {
	Err error;

	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) return (error);

	switch (cmd) {
		case sysAppLaunchCmdNormalLaunch:
			error = AppStart();
			if (error)
				return error;

			FrmGotoForm(MainForm);
			AppEventLoop();

			AppStop();
			break;
	}

	return errNone;
}
