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

/*====================================================
 * Camera and surface data
 *====================================================*/
#define GRID_MAX            20
#define ZOOM_LEVEL_DEFAULT  3
#define DEFAULT_VIEW_ALPHA  30
#define DEFAULT_VIEW_BETA   30

struct CameraStruct {
    Int16   iViewportX, iViewportY, iViewportS;
    Int16   iAlphaDeg, iBetaDeg;
    PZ_FIXED cosA, sinA, cosB, sinB;
    PZ_FLOAT xMin, xMax; Int16 xGrid;
    PZ_FLOAT yMin, yMax; Int16 yGrid;
    PZ_FLOAT zMin, zMax;
    Int16   iZoomLevel;
};

static const int arrZoomLevels[] = {
    PZ_FLOAT_TO_FIXED(0.33f),
    PZ_FLOAT_TO_FIXED(0.50f),
    PZ_FLOAT_TO_FIXED(0.75f),
    (int)PZ_FIXED_ONE,
    PZ_FLOAT_TO_FIXED(1.50f),
    (int)PZ_FIXED_ONE * 2
};
static const int iNumZoomLevel = sizeof(arrZoomLevels) / sizeof(arrZoomLevels[0]);

static struct CameraStruct Camera = {
    0, 0, 0,
    DEFAULT_VIEW_ALPHA, DEFAULT_VIEW_BETA,
    0, 0, 0, 0,
    -6.0f, 6.0f, 20,
    -6.0f, 6.0f, 20,
    -3.0f, 3.0f,
    ZOOM_LEVEL_DEFAULT
};
static PZ_FIXED zBuf[GRID_MAX * GRID_MAX];
static PZ_FIXED xBuf[GRID_MAX];
static PZ_FIXED yBuf[GRID_MAX];
#define Z_BUF(x,y) (zBuf[(x) + (y) * Camera.xGrid])

/* DrawForm interaction */
static Int16 g_iDrawMode = 0; /* 0=Camera, 1=Position, 2=Zoom */
static Boolean g_bDrawBox = true;
static Boolean g_bDrawPenDown = false;
static Int16 g_iDrawPrevX, g_iDrawPrevY;
static Int16 g_iDrawZoomAccum = 0;
#define DRAW_ZOOM_THRESHOLD 15

#define CURRENT_FONT_WIDTH  6
#define CURRENT_FONT_HEIGHT 8
#define CORNER_SIZE         10
#define CORNER_TEXT_X        2
#define CORNER_TEXT_Y        1

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
    BmpBuffer_AllClear(g_pBmpBufFormula);

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

/*====================================================
 * 3D projection (fixed-point)
 *====================================================*/
static void xyz2xy(PZ_FIXED x, PZ_FIXED y, PZ_FIXED z, Int16 *ox, Int16 *oy) {
    long iZoom = arrZoomLevels[Camera.iZoomLevel];
    long iScale = (long)(((long)Camera.iViewportS * iZoom + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
    long nx, ny;
    nx = PZ_FIXED_MUL(x, Camera.cosB) - PZ_FIXED_MUL(y, Camera.sinB);
    ny = PZ_FIXED_MUL(PZ_FIXED_MUL(x, Camera.sinB) + PZ_FIXED_MUL(y, Camera.cosB), Camera.sinA)
       - PZ_FIXED_MUL(z, Camera.cosA);
    *ox = (Int16)(Camera.iViewportX + (long)(((long)iScale * nx + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT));
    *oy = (Int16)(Camera.iViewportY + (long)(((long)iScale * ny + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT));
}

/*====================================================
 * Recalculate surface geometry
 *====================================================*/
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
            char szBuf[32];
            Int16 iLen;

            BmpBuffer_AllClear(g_pBmpCanvas);
            BmpBuffer_FillRect(g_pBmpCanvas, 0, 0,
                g_pBmpCanvas->iW, g_pBmpCanvas->iH, 0);

            StrPrintF(szBuf, "Recalc %d%%", iPct);
            iLen = (Int16)(StrLen(szBuf) * CURRENT_FONT_WIDTH);
            BmpBuffer_PutText(g_pBmpCanvas,
                (g_pBmpCanvas->iW - iLen) / 2,
                (g_pBmpCanvas->iH - CURRENT_FONT_HEIGHT) / 2,
                (UInt8*)szBuf, 0);

            WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 1);
            iLastPct = iPct;
        }
    }
    return true;
}

/*====================================================
 * Redraw wireframe on canvas
 *====================================================*/
static void redrawCanvas(BmpBuffer* pBuf) {
    Int16 ix, iy;
    PZ_FIXED z0, z1;
    Int16 x0, y0, x1, y1;

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
	}

	return handled;
}

/*====================================================
 * Draw UI indicators on canvas
 *====================================================*/
static void drawDrawUI(BmpBuffer* pBuf) {
    Int16 iW, iH;
    UInt8 chMode, chBox;

    iW = pBuf->iW;
    iH = pBuf->iH;

    /* Top-left: back icon */
    BmpBuffer_FillRect(pBuf, 0, 0, CORNER_SIZE, CORNER_SIZE, 1);
    BmpBuffer_PutChar(pBuf, CORNER_TEXT_X, CORNER_TEXT_Y, '\x17', 0);

    /* Top-right: mode letter */
    BmpBuffer_FillRect(pBuf, (Int16)(iW - CORNER_SIZE), 0,
        CORNER_SIZE, CORNER_SIZE, 1);
    switch (g_iDrawMode) {
        case 0: chMode = 'C'; break;
        case 1: chMode = 'P'; break;
        case 2: chMode = 'Z'; break;
        default: chMode = '?'; break;
    }
    BmpBuffer_PutChar(pBuf, (Int16)(iW - CORNER_SIZE + CORNER_TEXT_X),
        CORNER_TEXT_Y, chMode, 0);

    /* Bottom-left: bounding box toggle */
    chBox = g_bDrawBox ? 'B' : 'b';
    BmpBuffer_FillRect(pBuf, 0, (Int16)(iH - CORNER_SIZE),
        CORNER_SIZE, CORNER_SIZE, 1);
    BmpBuffer_PutChar(pBuf, CORNER_TEXT_X,
        (Int16)(iH - CORNER_SIZE + CORNER_TEXT_Y), chBox, 0);

    /* Bottom-right: reset */
    BmpBuffer_FillRect(pBuf, (Int16)(iW - CORNER_SIZE),
        (Int16)(iH - CORNER_SIZE), CORNER_SIZE, CORNER_SIZE, 1);
    BmpBuffer_PutChar(pBuf, (Int16)(iW - CORNER_SIZE + CORNER_TEXT_X),
        (Int16)(iH - CORNER_SIZE + CORNER_TEXT_Y), 'R', 0);
}

static Boolean DrawFormHandleEvent(EventType * eventP) {
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) {
		case frmOpenEvent:
		{
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			Camera.iViewportS = 80;
			Camera.iViewportX = 80;
			Camera.iViewportY = 80;

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

			/* Top-left corner (back icon) */
			if (iTapX >= 0 && iTapX < CORNER_SIZE
			    && iTapY >= 0 && iTapY < CORNER_SIZE) {
				FrmGotoForm(MainForm);
				handled = true;
				break;
			}

			/* Top-right corner (mode switch) */
			if (iTapX >= g_pBmpCanvas->iW - CORNER_SIZE
			    && iTapX < g_pBmpCanvas->iW
			    && iTapY >= 0 && iTapY < CORNER_SIZE) {
				g_iDrawMode = (Int16)((g_iDrawMode + 1) % 3);
				redrawCanvas(g_pBmpCanvas);
				drawDrawUI(g_pBmpCanvas);
				WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
				handled = true;
				break;
			}

			/* Bottom-left corner (bounding box toggle) */
			if (iTapX >= 0 && iTapX < CORNER_SIZE
			    && iTapY >= g_pBmpCanvas->iH - CORNER_SIZE
			    && iTapY < g_pBmpCanvas->iH) {
				g_bDrawBox = !g_bDrawBox;
				redrawCanvas(g_pBmpCanvas);
				drawDrawUI(g_pBmpCanvas);
				WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
				handled = true;
				break;
			}

			/* Bottom-right corner (reset view) */
			if (iTapX >= g_pBmpCanvas->iW - CORNER_SIZE
			    && iTapX < g_pBmpCanvas->iW
			    && iTapY >= g_pBmpCanvas->iH - CORNER_SIZE
			    && iTapY < g_pBmpCanvas->iH) {
				Camera.iAlphaDeg  = DEFAULT_VIEW_ALPHA;
				Camera.iBetaDeg   = DEFAULT_VIEW_BETA;
				Camera.iViewportX = (Int16)(g_pBmpCanvas->iW / 2);
				Camera.iViewportY = (Int16)(g_pBmpCanvas->iH / 2);
				Camera.iViewportS = (Int16)(g_pBmpCanvas->iH / 2);
				Camera.iZoomLevel = ZOOM_LEVEL_DEFAULT;
				redrawCanvas(g_pBmpCanvas);
				drawDrawUI(g_pBmpCanvas);
				WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
				handled = true;
				break;
			}

			g_bDrawPenDown = true;
			g_iDrawPrevX = iTapX;
			g_iDrawPrevY = iTapY;
			g_iDrawZoomAccum = 0;
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
					case 1: /* Position */
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
				}

				if (bChanged) {
					redrawCanvas(g_pBmpCanvas);
					drawDrawUI(g_pBmpCanvas);
					WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
				}
			}
			handled = true;
			break;

		case penUpEvent:
			g_bDrawPenDown = false;
			g_iDrawZoomAccum = 0;
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

	/* Initialize Camera defaults */
	MemSet(&Camera, sizeof(Camera), 0);
	Camera.iAlphaDeg  = DEFAULT_VIEW_ALPHA;
	Camera.iBetaDeg   = DEFAULT_VIEW_BETA;
	Camera.xMin = -6.0f;  Camera.xMax = 6.0f;   Camera.xGrid = 20;
	Camera.yMin = -6.0f;  Camera.yMax = 6.0f;   Camera.yGrid = 20;
	Camera.zMin = -3.0f;  Camera.zMax = 3.0f;
	Camera.iZoomLevel = ZOOM_LEVEL_DEFAULT;

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
