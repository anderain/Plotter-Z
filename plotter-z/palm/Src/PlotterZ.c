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
#include "../../plotter-z/utils/samples.h"

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

static char g_szExpr[256] = "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))";
static char g_szErrorBuf[EZ_ERROR_CONTENT_LENGTH];
EzError g_iCompileErr;
static Boolean g_bParseOk = false;
static Int16 g_iFormulaX = 0;
static Int16 g_iFormulaY = 0;

/*====================================================
 * Camera and surface data
 *====================================================*/
#define GRID_MAX            25

static PZ_FIXED zBuf[GRID_MAX * GRID_MAX];
static PZ_FIXED xBuf[GRID_MAX];
static PZ_FIXED yBuf[GRID_MAX];
#define Z_BUF(x,y) (zBuf[(x) + (y) * Camera.xGrid])

/* DrawForm interaction */
static Int16 g_iDrawMode = 0; /* 0=Camera, 1=Pan Move, 2=Zoom, 3=Fov */
static Boolean g_bDrawBox = false;
static Boolean g_bDrawFooter = true;
static Boolean g_bDrawPenDown = false;
static Int16 g_iDrawPrevX, g_iDrawPrevY;
static Int16 g_iDrawZoomAccum = 0;
static Int16 g_iFovAccum = 0;
static Int16 g_iSurfaceDragThreshold = 12;
static Int16 g_iFormulaDragThreshold = 6;
static UInt32 g_dwLastDrawUpdate = 0;

#define FOV_DRAG_THRESHOLD 15

#define DRAW_ZOOM_THRESHOLD 8

#define CURRENT_FONT_WIDTH  6
#define CURRENT_FONT_HEIGHT 8
#define CORNER_SIZE         10
#define CORNER_TEXT_X      	2
#define CORNER_TEXT_Y      	1

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
 * Parse expression (AST, compile, build RenderNode)
 *====================================================*/
static void parseFormula(void) {
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
    g_iFormulaX = 0;
    g_iFormulaY = 0;

    /* Parse expression */
    g_pAstExpr = FzParser_ParseExpression(g_szExpr);
    if (g_pAstExpr == NULL) return;

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
        g_iCompileErr = EzMachine_Compile(g_pVm, g_pAstExpr, g_szErrorBuf);
        if (g_iCompileErr != EZERR_NONE) return;
    }

    /* Build render tree */
    g_pRenderNode = Render_Transform(g_pAstExpr);
    if (g_pRenderNode != NULL) {
        RenderNode_CalculateSize(g_pRenderNode, &g_RenderConfig);
        /* Center formula horizontally */
        g_iFormulaX = (Int16)((g_pBmpBufFormula->iW
            - g_pRenderNode->sLayout.iWidth) / 2);
        if (g_iFormulaX < 0) g_iFormulaX = 0;
		g_iFormulaY = (Int16)((g_pBmpBufFormula->iH - g_pRenderNode->sLayout.iAscent - g_pRenderNode->sLayout.iDescent) / 2) + g_pRenderNode->sLayout.iAscent;
		if (g_iFormulaY < 0) g_iFormulaY = 0;
    }
}

/*====================================================
 * Render formula to bitmap
 *====================================================*/
static void renderFormula(void) {
    Int16 x, y;
    Int16 iW, iH;

    iW = g_pBmpBufFormula->iW;
    iH = g_pBmpBufFormula->iH;

    BmpBuffer_AllClear(g_pBmpBufFormula);

    /* Parse error */
    if (g_pAstExpr == NULL) {
        const char* szMsg = "Syntax Error";
        Int16 iLen = (Int16)(StrLen(szMsg) * CURRENT_FONT_WIDTH);
        x = (iW - iLen) / 2;
        y = (iH - CURRENT_FONT_HEIGHT) / 2;
        BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)szMsg, 1);
        return;
    }

    /* Compile error (check g_szErrorBuf non-empty) */
    if (g_szErrorBuf[0] != '\0' && g_pRenderNode == NULL) {
        const char* szErrType = "Compile Error:";
        Int16 iLen;

		switch (g_iCompileErr) {
			case EZERR_VARIABLE_UNDEFINED:
				szErrType = "Undefined Variable:";
				break;
			case EZERR_FUNCTION_UNDEFINED:
				szErrType = "Undefined Function:";
				break;
			case EZERR_FUNCTION_PARAM_MISMATCH:
				szErrType = "Parameters mismatch:";
				break;
		}

        iLen = (Int16)(StrLen(szErrType) * CURRENT_FONT_WIDTH);
        x = (iW - iLen) / 2;
        y = (iH - CURRENT_FONT_HEIGHT * 2) / 2;
        BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)szErrType, 1);
        iLen = (Int16)(StrLen(g_szErrorBuf) * CURRENT_FONT_WIDTH);
        x = (iW - iLen) / 2;
        y += CURRENT_FONT_HEIGHT;
        BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)g_szErrorBuf, 1);
        return;
    }

    /* Render error */
    if (g_pRenderNode == NULL) {
        const char* szMsg = "Render error";
        Int16 iLen = (Int16)(StrLen(szMsg) * CURRENT_FONT_WIDTH);
        x = (iW - iLen) / 2;
        y = (iH - CURRENT_FONT_HEIGHT) / 2;
        BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)szMsg, 1);
        return;
    }

    g_bParseOk = true;

    /* Line 2: formula rendering */
    RenderNode_Draw(g_pRenderNode, &g_RenderConfig, g_iFormulaX, g_iFormulaY);

    /* Line 1: f(x,y)= on filled background */
    {
        const char* szLabel = "f(x,y)=";
        BmpBuffer_FillRect(g_pBmpBufFormula, 0, 0,
            iW, CURRENT_FONT_HEIGHT, 1);
        BmpBuffer_PutText(g_pBmpBufFormula, 0, 0, (UInt8*)szLabel, 0);
    }

    /* Line 3: Ready> on filled background */
    {
        const char* szReady = "Ready\x18";
        Int16 iLen = (Int16)(StrLen(szReady) * CURRENT_FONT_WIDTH);
        x = iW - iLen - 2;
        y = iH - CURRENT_FONT_HEIGHT;
        BmpBuffer_FillRect(g_pBmpBufFormula, 0, y,
            iW, CURRENT_FONT_HEIGHT, 1);
        BmpBuffer_PutText(g_pBmpBufFormula, x, y, (UInt8*)szReady, 0);
    }
}

/*====================================================
 * Projection
 *====================================================*/
#define ORTHOGRAPHIC    0
#define PERSPECTIVE     1

void (*xyz2xy)(PZ_FIXED, PZ_FIXED, PZ_FIXED, int*, int *) = PzCamera_OrthoProjectFixed;
int g_iProjection = ORTHOGRAPHIC;

/*====================================================
 * Recalculate surface geometry
 *====================================================*/

static void redrawRecalc(Int16 iPct) {
	char szBuf[32];
	Int16 iLen, iBarW, iBarH, iBarX, iBarY, iFillW;
	Int16 iTextX, iTextY, iLabelY;
	Int16 iCW, iCH;

	iCW = g_pBmpCanvas->iW;
	iCH = g_pBmpCanvas->iH;
	iBarW = iCW * 2 / 3;
	iBarH = CURRENT_FONT_HEIGHT * 2;
	iBarX = (iCW - iBarW) / 2;
	iBarY = (iCH - iBarH) / 2;

	BmpBuffer_AllClear(g_pBmpCanvas);

	/* "Recalc ..." label above bar */
	iLen = (Int16)(StrLen("Recalc ...") * CURRENT_FONT_WIDTH);
	iLabelY = iBarY - CURRENT_FONT_HEIGHT - 2;
	if (iLabelY < 0) iLabelY = 0;
	BmpBuffer_PutText(g_pBmpCanvas, (iCW - iLen) / 2,
		iLabelY, (UInt8*)"Recalc ...", 1);

	/* Progress bar fill */
	BmpBuffer_FillRect(g_pBmpCanvas, iBarX, iBarY, iBarW, iBarH, 0);

	/* Progress bar border */
	BmpBuffer_PlotLine(g_pBmpCanvas,
		iBarX, iBarY, (Int16)(iBarX + iBarW - 1), iBarY, 1);
	BmpBuffer_PlotLine(g_pBmpCanvas,
		iBarX, (Int16)(iBarY + iBarH - 1),
		(Int16)(iBarX + iBarW - 1), (Int16)(iBarY + iBarH - 1), 1);
	BmpBuffer_PlotLine(g_pBmpCanvas,
		iBarX, iBarY, iBarX, (Int16)(iBarY + iBarH - 1), 1);
	BmpBuffer_PlotLine(g_pBmpCanvas,
		(Int16)(iBarX + iBarW - 1), iBarY,
		(Int16)(iBarX + iBarW - 1), (Int16)(iBarY + iBarH - 1), 1);

	/* Percentage text */
	StrPrintF(szBuf, "%d%%", iPct);
	iLen = (Int16)(StrLen(szBuf) * CURRENT_FONT_WIDTH);
	iTextX = iBarX + (iBarW - iLen) / 2;
	iTextY = iBarY + (iBarH - CURRENT_FONT_HEIGHT) / 2;
	BmpBuffer_PutText(g_pBmpCanvas, iTextX, iTextY, (UInt8*)szBuf, 1);

	/* Filled portion invert */
	iFillW = iBarW * iPct / 100;
	if (iFillW > iBarW) iFillW = iBarW;
	if (iFillW > 0)
		BmpBuffer_InvertRect(g_pBmpCanvas, iBarX + 1, iBarY + 1,
			(Int16)(iFillW - 2), (Int16)(iBarH - 2));

	WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
}

static Boolean recalcSurface(void) {
    Int16 ix, iy;
    PZ_FLOAT fXbuf[GRID_MAX], fYbuf[GRID_MAX];
    PZ_FLOAT fz;
    PZ_FLOAT fzMid   = (Camera.zMax + Camera.zMin) * 0.5f;
    PZ_FLOAT fzRange = (Camera.zMax - Camera.zMin);
    PZ_FLOAT fxMid   = (Camera.xMax + Camera.xMin) * 0.5f;
    PZ_FLOAT fxRange = (Camera.xMax - Camera.xMin);
    PZ_FLOAT fyMid   = (Camera.yMax + Camera.yMin) * 0.5f;
    PZ_FLOAT fyRange = (Camera.yMax - Camera.yMin);
    Int16 iLastPct = -1;

    if (g_pVm == NULL) return false;

	redrawRecalc(0);

    for (ix = 0; ix < Camera.xGrid; ++ix)
        fXbuf[ix] = Camera.xMin + (Camera.xMax - Camera.xMin) * ix / (PZ_FLOAT)(Camera.xGrid - 1);
    for (iy = 0; iy < Camera.yGrid; ++iy)
        fYbuf[iy] = Camera.yMin + (Camera.yMax - Camera.yMin) * iy / (PZ_FLOAT)(Camera.yGrid - 1);

    for (ix = 0; ix < Camera.xGrid; ++ix) {
        PZ_FLOAT fx = fXbuf[ix];
        Int16 iPct;
        for (iy = 0; iy < Camera.yGrid; ++iy) {
            PZ_FLOAT fy = fYbuf[iy];
            EzMachine_SetVariableByIndex(g_pVm, 0, fx);
            EzMachine_SetVariableByIndex(g_pVm, 1, fy);
            fz = EzMachine_Eval(g_pVm);
            xBuf[ix] = PZ_FLOAT_TO_FIXED(2.0f * (fx - fxMid) / fxRange);
            yBuf[iy] = PZ_FLOAT_TO_FIXED(2.0f * (fy - fyMid) / fyRange);
            Z_BUF(ix, iy) = PZ_FLOAT_TO_FIXED(2.0f * (fz - fzMid) / fzRange);
        }
        iPct = (Int16)((ix + 1) * 100 / Camera.xGrid);
        if (iPct != iLastPct) {
			redrawRecalc(iPct);
			iLastPct = iPct;
        }
    }
    return true;
}

/*====================================================
 * Redraw wireframe on canvas
 *====================================================*/
static void redrawCanvas(BmpBuffer* pBuf) {
    int ix, iy;
    PZ_FIXED z0, z1;
    int x0, y0, x1, y1;

    BmpBuffer_AllClear(pBuf);

    Camera.sinA = PZ_FLOAT_TO_FIXED(sin(Camera.iAlphaDeg * PZ_PI / 180));
    Camera.cosA = PZ_FLOAT_TO_FIXED(cos(Camera.iAlphaDeg * PZ_PI / 180));
    Camera.sinB = PZ_FLOAT_TO_FIXED(sin(Camera.iBetaDeg * PZ_PI / 180));
    Camera.cosB = PZ_FLOAT_TO_FIXED(cos(Camera.iBetaDeg * PZ_PI / 180));

    for (ix = 0; ix < Camera.xGrid; ++ix) {
        iy = 0;
        xyz2xy(xBuf[ix], yBuf[iy], z0 = Z_BUF(ix, iy), &x0, &y0);
        for (iy = 0; iy < Camera.yGrid; ++iy) {
            xyz2xy(xBuf[ix], yBuf[iy], z1 = Z_BUF(ix, iy), &x1, &y1);
            if (z0 <= PZ_FIXED_ONE && z0 >= PZ_FIXED_NEG_ONE
                && z1 <= PZ_FIXED_ONE && z1 >= PZ_FIXED_NEG_ONE)
                BmpBuffer_PlotLine(pBuf, x0, y0, x1, y1, 1);
            x0 = x1; y0 = y1; z0 = z1;
        }
    }
    for (iy = 0; iy < Camera.yGrid; ++iy) {
        ix = 0;
        xyz2xy(xBuf[ix], yBuf[iy], z0 = Z_BUF(ix, iy), &x0, &y0);
        for (ix = 0; ix < Camera.xGrid; ++ix) {
            xyz2xy(xBuf[ix], yBuf[iy], z1 = Z_BUF(ix, iy), &x1, &y1);
            if (z0 <= PZ_FIXED_ONE && z0 >= PZ_FIXED_NEG_ONE
                && z1 <= PZ_FIXED_ONE && z1 >= PZ_FIXED_NEG_ONE)
                BmpBuffer_PlotLine(pBuf, x0, y0, x1, y1, 1);
            x0 = x1; y0 = y1; z0 = z1;
        }
    }

    /* Bounding box edges */
    if (g_bDrawBox) {
        static const PZ_FIXED bvX[8] = {
             PZ_FIXED_ONE, PZ_FIXED_NEG_ONE, PZ_FIXED_NEG_ONE,  PZ_FIXED_ONE,
             PZ_FIXED_ONE, PZ_FIXED_NEG_ONE, PZ_FIXED_NEG_ONE,  PZ_FIXED_ONE
        };
        static const PZ_FIXED bvY[8] = {
             PZ_FIXED_ONE,  PZ_FIXED_ONE, PZ_FIXED_NEG_ONE, PZ_FIXED_NEG_ONE,
             PZ_FIXED_ONE,  PZ_FIXED_ONE, PZ_FIXED_NEG_ONE, PZ_FIXED_NEG_ONE
        };
        static const PZ_FIXED bvZ[8] = {
             PZ_FIXED_ONE,  PZ_FIXED_ONE,  PZ_FIXED_ONE,  PZ_FIXED_ONE,
            PZ_FIXED_NEG_ONE,PZ_FIXED_NEG_ONE,PZ_FIXED_NEG_ONE,PZ_FIXED_NEG_ONE
        };
        static const UInt8 edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},
            {4,5},{5,6},{6,7},{7,4},
            {0,4},{1,5},{2,6},{3,7}
        };
        Int16 ei;
        for (ei = 0; ei < 12; ++ei) {
            Int16 v0, v1;
            v0 = edges[ei][0]; v1 = edges[ei][1];
            xyz2xy(bvX[v0], bvY[v0], bvZ[v0], &x0, &y0);
            xyz2xy(bvX[v1], bvY[v1], bvZ[v1], &x1, &y1);
            BmpBuffer_PlotLine(pBuf, x0, y0, x1, y1, 1);
        }
    }
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

		BmpBuffer_AllClear(g_pBmpBufFormula);

		BmpBuffer_FillRect(g_pBmpBufFormula, 0, 0, iW, iH, 0);

		y = (iH - iCount * CURRENT_FONT_HEIGHT) / 2;
		for (i = 0; i < iCount; ++i) {
			iLen = (Int16)(StrLen(szLines[i]) * CURRENT_FONT_WIDTH);
			BmpBuffer_PutText(g_pBmpBufFormula,
				(iW - iLen) / 2, y, (UInt8*)szLines[i], 1);
			y += CURRENT_FONT_HEIGHT;
		}
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
		case OptionsWinEditor: {
			FormType * frmP;
			UInt16 controlID;
			Int16 iGrid;

			MenuEraseStatus(0);

			frmP = FrmInitForm(WinEditorForm);

			/* Populate fields from Camera */
			{
				char szBuf[16];
#define WE_FIELD(id) ((FieldType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, id)))
				Utils_Ftoa((double)Camera.xMin, szBuf, 2);
				FldInsert(WE_FIELD(WinEditorFieldXmin), szBuf, StrLen(szBuf));
				Utils_Ftoa((double)Camera.xMax, szBuf, 2);
				FldInsert(WE_FIELD(WinEditorFieldXmax), szBuf, StrLen(szBuf));
				StrIToA(szBuf, Camera.xGrid);
				FldInsert(WE_FIELD(WinEditorFieldXgrid), szBuf, StrLen(szBuf));

				Utils_Ftoa((double)Camera.yMin, szBuf, 2);
				FldInsert(WE_FIELD(WinEditorFieldYmin), szBuf, StrLen(szBuf));
				Utils_Ftoa((double)Camera.yMax, szBuf, 2);
				FldInsert(WE_FIELD(WinEditorFieldYmax), szBuf, StrLen(szBuf));
				StrIToA(szBuf, Camera.yGrid);
				FldInsert(WE_FIELD(WinEditorFieldYgrid), szBuf, StrLen(szBuf));

				Utils_Ftoa((double)Camera.zMin, szBuf, 2);
				FldInsert(WE_FIELD(WinEditorFieldZmin), szBuf, StrLen(szBuf));
				Utils_Ftoa((double)Camera.zMax, szBuf, 2);
				FldInsert(WE_FIELD(WinEditorFieldZmax), szBuf, StrLen(szBuf));
			}

			controlID = FrmDoDialog(frmP);

			if (controlID == WinEditorOKButton) {
				char szBuf[16];
				MemHandle handle;
				MemPtr text;

				/* Read X values */
				handle = FldGetTextHandle(WE_FIELD(WinEditorFieldXmin));
				if (handle) { text = MemHandleLock(handle); StrCopy(szBuf, (const char*)text); MemHandleUnlock(handle);
					Camera.xMin = (PZ_FLOAT)Utils_Atof(szBuf); }
				handle = FldGetTextHandle(WE_FIELD(WinEditorFieldXmax));
				if (handle) { text = MemHandleLock(handle); StrCopy(szBuf, (const char*)text); MemHandleUnlock(handle);
					Camera.xMax = (PZ_FLOAT)Utils_Atof(szBuf); }
				handle = FldGetTextHandle(WE_FIELD(WinEditorFieldXgrid));
				if (handle) { text = MemHandleLock(handle); StrCopy(szBuf, (const char*)text); MemHandleUnlock(handle);
					iGrid = (Int16)Utils_Atoi(szBuf);
					if (iGrid < 5) iGrid = 5;
					if (iGrid > GRID_MAX) iGrid = GRID_MAX;
					Camera.xGrid = iGrid; }

				handle = FldGetTextHandle(WE_FIELD(WinEditorFieldYmin));
				if (handle) { text = MemHandleLock(handle); StrCopy(szBuf, (const char*)text); MemHandleUnlock(handle);
					Camera.yMin = (PZ_FLOAT)Utils_Atof(szBuf); }
				handle = FldGetTextHandle(WE_FIELD(WinEditorFieldYmax));
				if (handle) { text = MemHandleLock(handle); StrCopy(szBuf, (const char*)text); MemHandleUnlock(handle);
					Camera.yMax = (PZ_FLOAT)Utils_Atof(szBuf); }
				handle = FldGetTextHandle(WE_FIELD(WinEditorFieldYgrid));
				if (handle) { text = MemHandleLock(handle); StrCopy(szBuf, (const char*)text); MemHandleUnlock(handle);
					iGrid = (Int16)Utils_Atoi(szBuf);
					if (iGrid < 5) iGrid = 5;
					if (iGrid > GRID_MAX) iGrid = GRID_MAX;
					Camera.yGrid = iGrid; }

				handle = FldGetTextHandle(WE_FIELD(WinEditorFieldZmin));
				if (handle) { text = MemHandleLock(handle); StrCopy(szBuf, (const char*)text); MemHandleUnlock(handle);
					Camera.zMin = (PZ_FLOAT)Utils_Atof(szBuf); }
				handle = FldGetTextHandle(WE_FIELD(WinEditorFieldZmax));
				if (handle) { text = MemHandleLock(handle); StrCopy(szBuf, (const char*)text); MemHandleUnlock(handle);
					Camera.zMax = (PZ_FLOAT)Utils_Atof(szBuf); }
			}

			FrmDeleteForm(frmP);

			handled = true;
			break;
		}
#undef WE_FIELD
		case OptionsSamples: {
			FormType * frmP;
			UInt16 controlID;
			ListType * lstP;
			Int16 iSel;
			const int iCount = sizeof(PlotterZSamples) / sizeof(PlotterZSamples[0]);
			Char** ppItems;
			Int16 i;

			MenuEraseStatus(0);

			frmP = FrmInitForm(SamplesForm);
			lstP = (ListType*)FrmGetObjectPtr(frmP,
				FrmGetObjectIndex(frmP, SamplesListID));

			/* Allocate string pointer array */
			ppItems = (Char**)MemPtrNew(iCount * sizeof(Char*));
			for (i = 0; i < iCount; ++i)
				ppItems[i] = PlotterZSamples[i].szExpr;

			LstSetListChoices(lstP, ppItems, (UInt16)iCount);
			/* LstDrawList(lstP); */

			controlID = FrmDoDialog(frmP);

			if (controlID == SamplesOKButton) {
				iSel = (Int16)LstGetSelection(lstP);
				if (iSel >= 0 && iSel < iCount) {
					const PzSample* p = &PlotterZSamples[iSel];
					Camera.xMin = p->xMin;
					Camera.xMax = p->xMax;
					Camera.yMin = p->yMin;
					Camera.yMax = p->yMax;
					Camera.zMin = p->zMin;
					Camera.zMax = p->zMax;
					Camera.xGrid = 15;
					Camera.yGrid = 15;
					Utils_StringCopy(g_szExpr, sizeof(g_szExpr), p->szExpr);
					/* Update MainForm field */
					{
						FormType * frmMain;
						FieldType * field;
						frmMain = FrmGetFormPtr(MainForm);
						if (frmMain != NULL) {
							field = (FieldType*)FrmGetObjectPtr(frmMain,
								FrmGetObjectIndex(frmMain, MainFormulaField));
							if (field != NULL) {
								FldDelete(field, 0, 0xFFFF);
								FldInsert(field, p->szExpr, StrLen(p->szExpr));
								FldDrawField(field);
							}
						}
					}
				}
				parseFormula();
				renderFormula();
				/* Draw button starts disabled */
				{
					ControlType *ctlDraw;
					/* Enable or disable Draw button */
					ctlDraw = (ControlType*)GetObjectPtr(MainDrawButton);
					if (ctlDraw != NULL) {
						if (g_bParseOk)
							CtlShowControl(ctlDraw);
						else
							CtlHideControl(ctlDraw);
					}
				}
			}
		
			FrmDeleteForm(frmP);
			MemPtrFree(ppItems);

			/* Force MainForm redraw */
			{
				FormType * frmMain = FrmGetFormPtr(MainForm);
				if (frmMain != NULL) {
					FrmEraseForm(frmMain);
					FrmDrawForm(frmMain);
				}
				FrmUpdateForm(MainForm, frmRedrawUpdateCode);
			}

			handled = true;
			break;
		}
	}

	return handled;
}

/*====================================================
 * Draw UI indicators on canvas
 *====================================================*/
static void drawDrawUI(BmpBuffer* pBuf) {
    Int16 iW, iH, iBarY;

    iW = pBuf->iW;
    iH = pBuf->iH;
    iBarY = (Int16)(iH - CURRENT_FONT_HEIGHT);

    /* Top-left: back icon */
    BmpBuffer_FillRect(pBuf, 0, 0, CORNER_SIZE, CORNER_SIZE, 1);
    BmpBuffer_PutChar(pBuf, CORNER_TEXT_X, CORNER_TEXT_Y, '\x17', 0);

    if (!g_bDrawFooter) {
        /* Footer hidden - show only H at bottom-right to re-show */
        BmpBuffer_FillRect(pBuf, (Int16)(iW - CORNER_SIZE),
            (Int16)(iH - CORNER_SIZE), CORNER_SIZE, CORNER_SIZE, 1);
        BmpBuffer_PutChar(pBuf, (Int16)(iW - CORNER_SIZE + CORNER_TEXT_X),
            (Int16)(iH - CORNER_SIZE + CORNER_TEXT_Y), 'H', 0);
        return;
    }

    /* Clear footer area */
    BmpBuffer_FillRect(pBuf, 0, iBarY, iW, CURRENT_FONT_HEIGHT, 0);

    /* Bottom bar: draw text in color 1, then reverse bar */
    {
        /* Mode letters: C P Z [F] */
        BmpBuffer_PutChar(pBuf, 3,  iBarY, 'C', 1);
        BmpBuffer_PutChar(pBuf, 13, iBarY, 'P', 1);
        BmpBuffer_PutChar(pBuf, 23, iBarY, 'Z', 1);
        if (g_iProjection == PERSPECTIVE)
            BmpBuffer_PutChar(pBuf, 33, iBarY, 'F', 1);

        /* Middle: ORTHO/PERSP */
        {
            const char* szProj = (g_iProjection == ORTHOGRAPHIC) ? "ORTHO" : "PERSP";
            Int16 iLen = (Int16)(5 * CURRENT_FONT_WIDTH);
            BmpBuffer_PutText(pBuf, (Int16)((iW - iLen) / 2), iBarY,
                (const UInt8*)szProj, 1);
        }

        /* Right: R B H */
        BmpBuffer_PutChar(pBuf, (Int16)(iW - 30), iBarY, 'R', 1);
        BmpBuffer_PutChar(pBuf, (Int16)(iW - 20), iBarY,
            g_bDrawBox ? 'B' : 'b', 1);
        BmpBuffer_PutChar(pBuf, (Int16)(iW - 10), iBarY, 'H', 1);
    }

    /* Invert entire bar */
    BmpBuffer_InvertRect(pBuf, 0, iBarY, iW, CURRENT_FONT_HEIGHT);

    /* Highlight selected mode */
    {
        Int16 iX = (Int16)(2 + g_iDrawMode * 10);
        BmpBuffer_InvertRect(pBuf, iX, iBarY, 8, CURRENT_FONT_HEIGHT);
    }
}

static Boolean DrawFormHandleEvent(EventType * eventP) {
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) {
		case frmOpenEvent:
		{
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			Camera.iViewportS = g_pBmpCanvas->iH / 2;
			Camera.iViewportX = g_pBmpCanvas->iW / 2;
			Camera.iViewportY = g_pBmpCanvas->iH / 2;

			if (recalcSurface()) {
				redrawCanvas(g_pBmpCanvas);
				drawDrawUI(g_pBmpCanvas);
			}
			WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
			handled = true;
			break;
		}

		case penDownEvent:
		{
			Int16 iTapX = eventP->screenX;
			Int16 iTapY = eventP->screenY;
			Int16 iBarY = (Int16)(g_pBmpCanvas->iH - CURRENT_FONT_HEIGHT);

			/* Top-left corner (back icon) */
			if (iTapX >= 0 && iTapX < CORNER_SIZE
			    && iTapY >= 0 && iTapY < CORNER_SIZE) {
				FrmGotoForm(MainForm);
				handled = true;
				break;
			}

			/* Bottom bar interaction */
			if (g_bDrawFooter && iTapY >= iBarY) {
				/* Left mode buttons: C P Z [F] */
				if (iTapX < 42) {
					Int16 iMode;
					iMode = iTapX / 10;
					if (iMode >= 0 && iMode < 3) {
						g_iDrawMode = iMode;
					} else if (iMode == 3 && g_iProjection == PERSPECTIVE) {
						g_iDrawMode = 3;
					} else {
						handled = true;
						break;
					}
					redrawCanvas(g_pBmpCanvas);
					drawDrawUI(g_pBmpCanvas);
					WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
					handled = true;
					break;
				}

				/* Middle: ORTHO/PERSP toggle */
				if (iTapX >= 55 && iTapX <= 105) {
					g_iProjection = !g_iProjection;
					if (g_iProjection == ORTHOGRAPHIC) {
						xyz2xy = PzCamera_OrthoProjectFixed;
						if (g_iDrawMode == 3) g_iDrawMode = 0;
					} else {
						xyz2xy = PzCamera_PerspProjectFixed;
					}
					redrawCanvas(g_pBmpCanvas);
					drawDrawUI(g_pBmpCanvas);
					WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
					handled = true;
					break;
				}

				/* Right buttons: R B H */
				{
					Int16 iRightX = (Int16)(g_pBmpCanvas->iW - 30);
					if (iTapX >= iRightX && iTapX < iRightX + 10) {
						/* Reset */
						PzCamera_Reset(g_pBmpCanvas->iW / 2, g_pBmpCanvas->iH / 2);
						redrawCanvas(g_pBmpCanvas);
						drawDrawUI(g_pBmpCanvas);
						WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
						handled = true;
						break;
					}
					if (iTapX >= iRightX + 10 && iTapX < iRightX + 20) {
						/* Toggle bounding box */
						g_bDrawBox = !g_bDrawBox;
						redrawCanvas(g_pBmpCanvas);
						drawDrawUI(g_pBmpCanvas);
						WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
						handled = true;
						break;
					}
					if (iTapX >= iRightX + 20 && iTapX < iRightX + 30) {
						/* Toggle footer */
						g_bDrawFooter = !g_bDrawFooter;
						redrawCanvas(g_pBmpCanvas);
						drawDrawUI(g_pBmpCanvas);
						WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
						handled = true;
						break;
					}
				}
				handled = true;
				break;
			}

			/* Footer hidden: tap bottom-right to re-show */
			if (!g_bDrawFooter
			    && iTapX >= g_pBmpCanvas->iW - CORNER_SIZE
			    && iTapY >= g_pBmpCanvas->iH - CORNER_SIZE) {
				g_bDrawFooter = true;
				redrawCanvas(g_pBmpCanvas);
				drawDrawUI(g_pBmpCanvas);
				WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
				handled = true;
				break;
			}

			/* Canvas drag (no footer interference) */
			g_bDrawPenDown = true;
			g_iDrawPrevX = iTapX;
			g_iDrawPrevY = iTapY;
			g_iDrawZoomAccum = 0;
			g_iFovAccum = 0;
			handled = true;
			break;
		}

		case penMoveEvent:
			if (g_bDrawPenDown) {
				Int16 iDeltaX, iDeltaY;
				Int16 iCurX, iCurY;
				Boolean bChanged = false;

				iCurX = eventP->screenX;
				iCurY = eventP->screenY;
				iDeltaX = iCurX - g_iDrawPrevX;
				iDeltaY = iCurY - g_iDrawPrevY;
				g_iDrawPrevX = iCurX;
				g_iDrawPrevY = iCurY;

				if (iDeltaX == 0 && iDeltaY == 0) break;

				switch (g_iDrawMode) {
					case 0: /* Camera */
						Camera.iBetaDeg  = (Int16)(Camera.iBetaDeg - iDeltaX);
						Camera.iAlphaDeg = (Int16)(Camera.iAlphaDeg - iDeltaY);
						Camera.iBetaDeg  = (Int16)(Camera.iBetaDeg % 360);
						if (Camera.iBetaDeg < 0) Camera.iBetaDeg = (Int16)(Camera.iBetaDeg + 360);
						Camera.iAlphaDeg = (Int16)(Camera.iAlphaDeg % 360);
						if (Camera.iAlphaDeg < 0) Camera.iAlphaDeg = (Int16)(Camera.iAlphaDeg + 360);
						bChanged = true;
						break;
					case 1: /* Pan Move */
						Camera.iViewportX = (Int16)(Camera.iViewportX + iDeltaX);
						Camera.iViewportY = (Int16)(Camera.iViewportY + iDeltaY);
						bChanged = true;
						break;
					case 2: /* Zoom */
						g_iDrawZoomAccum = (Int16)(g_iDrawZoomAccum - iDeltaY);
						if (g_iDrawZoomAccum >= DRAW_ZOOM_THRESHOLD) {
							if (Camera.iZoomLevel > 0) Camera.iZoomLevel--;
							g_iDrawZoomAccum = 0;
							bChanged = true;
						} else if (g_iDrawZoomAccum <= -DRAW_ZOOM_THRESHOLD) {
							if (Camera.iZoomLevel < iNumZoomLevel - 1) Camera.iZoomLevel++;
							g_iDrawZoomAccum = 0;
							bChanged = true;
						}
						break;
					case 3: /* Fov */
						g_iFovAccum = (Int16)(g_iFovAccum - iDeltaY);
						if (g_iFovAccum >= FOV_DRAG_THRESHOLD) {
							if (Camera.iFovLevel > FOV_LEVEL_MIN)
								Camera.iFovLevel--;
							g_iFovAccum = 0;
							bChanged = true;
						} else if (g_iFovAccum <= -FOV_DRAG_THRESHOLD) {
							if (Camera.iFovLevel < FOV_LEVEL_MAX)
								Camera.iFovLevel++;
							g_iFovAccum = 0;
							bChanged = true;
						}
						break;
				}

				if (bChanged) {
					UInt32 dwNow;
					dwNow = TimGetTicks();
					if (dwNow - g_dwLastDrawUpdate
					    >= (UInt32)g_iSurfaceDragThreshold) {
						redrawCanvas(g_pBmpCanvas);
						drawDrawUI(g_pBmpCanvas);
						WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
						g_dwLastDrawUpdate = dwNow;
					}
				}
			}
			handled = true;
			break;

		case penUpEvent:
			if (g_bDrawPenDown) {
				redrawCanvas(g_pBmpCanvas);
				drawDrawUI(g_pBmpCanvas);
				WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
			}
			g_bDrawPenDown = false;
			g_iDrawZoomAccum = 0;
			g_iFovAccum = 0;
			g_dwLastDrawUpdate = 0;
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
			if (g_bParseOk) {
				/* Restore field text and re-render */
				FieldType *fieldInput;
				UInt16 fieldIndex;
				fieldIndex = FrmGetObjectIndex(frmP, MainFormulaField);
				fieldInput = (FieldType *)FrmGetObjectPtr(frmP, fieldIndex);
				FldDelete(fieldInput, 0, 0xFFFF);
				FldInsert(fieldInput, g_szExpr, StrLen(g_szExpr));
				renderFormula();
			} else {
				MainFormInit(frmP);
			}
			FrmDrawForm(frmP);
			FrmUpdateForm(MainForm, frmRedrawUpdateCode);
			handled = true;
			break;

		case frmUpdateEvent:
			WinDrawBitmap(g_pBmpBufFormula->pBmp, 0, 15);
			handled = true;
			break;

		case penDownEvent:
			/* Only start drag if parsed OK and on canvas area */
			if (g_bParseOk && eventP->screenY > 16 && eventP->screenY < 116) {
				g_bDrawPenDown = true;
				g_iDrawPrevX = eventP->screenX;
				g_iDrawPrevY = eventP->screenY;
				g_dwLastDrawUpdate = 0;
				handled = true;
			}
			break;

		case penMoveEvent:
			if (g_bDrawPenDown) {
				Int16 iDeltaX, iDeltaY;
				Int16 iCurX, iCurY;

				iCurX = eventP->screenX;
				iCurY = eventP->screenY;
				iDeltaX = iCurX - g_iDrawPrevX;
				iDeltaY = iCurY - g_iDrawPrevY;
				g_iDrawPrevX = iCurX;
				g_iDrawPrevY = iCurY;

				if (iDeltaX != 0 || iDeltaY != 0) {
					UInt32 dwNow;
					g_iFormulaX = (Int16)(g_iFormulaX + iDeltaX);
					g_iFormulaY = (Int16)(g_iFormulaY + iDeltaY);

					dwNow = TimGetTicks();
					if (dwNow - g_dwLastDrawUpdate
					    >= (UInt32)g_iFormulaDragThreshold) {
						renderFormula();
						WinDrawBitmap(g_pBmpBufFormula->pBmp, 0, 15);
						g_dwLastDrawUpdate = dwNow;
					}
				}
				handled = true;
			}
			break;

		case penUpEvent:
			if (g_bDrawPenDown) {
				renderFormula();
				WinDrawBitmap(g_pBmpBufFormula->pBmp, 0, 15);
				handled = true;
			}
			g_bDrawPenDown = false;
			g_dwLastDrawUpdate = 0;
			break;
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

				parseFormula();
				renderFormula();

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
	RenderConfig_GetDefaultStyle(&g_RenderConfig);
	RenderConfig_CalculateBigSymbolPoints(&g_RenderConfig);
	g_RenderConfig.sInterfaces.setPixel = rzSetPixel;
	g_RenderConfig.sInterfaces.plotLine = rzPlotLine;
	g_RenderConfig.sInterfaces.putChar  = rzPutChar;

	/* Initialize Camera defaults */
	PzCamera_Initialize();

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
