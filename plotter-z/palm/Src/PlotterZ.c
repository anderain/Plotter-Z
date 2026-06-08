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

#define FOV_DRAG_THRESHOLD 15
#define DRAW_ZOOM_THRESHOLD 8
#define CURRENT_FONT_WIDTH  6
#define CURRENT_FONT_HEIGHT 8
#define CORNER_SIZE         10
#define CORNER_TEXT_X      	2
#define CORNER_TEXT_Y      	1
#define ORTHOGRAPHIC    	0
#define PERSPECTIVE     	1
#define MAIN_FORM_IDLE		0
#define MAIN_FORM_ERROR		1
#define MAIN_FORM_READY		2

/*********************************************************************
 * Global variables
 *********************************************************************/
PlotterZPreferenceType g_prefs;
BmpBuffer*             g_pBmpBufFormula;
BmpBuffer*             g_pBmpCanvas;

/*====================================================
 * Numeric
 *====================================================*/
#define USE_FIXED_POINT

#ifdef USE_FIXED_POINT
#   define NUMERIC          PZ_FIXED
#   define NUM_VAL(v)       (PZ_FLOAT_TO_FIXED(v))
#   define TO_FLOAT(v)      (PZ_FIXED_TO_FLOAT(v))
#   define OrthoProject     PzCamera_OrthoProjectFixed
#   define PerspProject     PzCamera_PerspProjectFixed
#else
#   define NUMERIC          PZ_FLOAT
#   define NUM_VAL(v)       (v)
#   define TO_FLOAT(v)      (v)
#   define OrthoProject     PzCamera_OrthoProjectFloat
#   define PerspProject     PzCamera_PerspProjectFloat
#endif

/*====================================================
 * Camera and surface data
 *====================================================*/
typedef struct { NUMERIC x, y, z; } Vertex;
typedef struct { int i0, i1; } Edge;

#define CART_EXPR_LENGTH	150
#define PARM_EXPR_LENGTH	80

#define X_GRID_MAX      25
#define Y_GRID_MAX      25
#define U_GRID_MAX      15
#define V_GRID_MAX      15
#define BUFFER_MAX(a,b) ((a) > (b) ? (a) : (b))
#define VERTEX_BUFFER_SIZE  BUFFER_MAX(X_GRID_MAX + Y_GRID_MAX + X_GRID_MAX * Y_GRID_MAX, U_GRID_MAX * V_GRID_MAX * 3)

char    g_szCartExpr[CART_EXPR_LENGTH] = "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))";
char    g_szParmExpr[3][PARM_EXPR_LENGTH] = { "5*sin(u)*cos(v)", "5*sin(u)*sin(v)", "5/2*cos(u)" };
char    g_szErrMsgLine1[EZ_ERROR_CONTENT_LENGTH] = "";
char    g_szErrMsgLine2[EZ_ERROR_CONTENT_LENGTH] = "";

int             g_iFuncType     = FUNC_TYPE_CARTESIAN;
FzAstNode*      g_pAstCartZ     = NULL;
EzMachine*      g_pVmCartZ      = NULL;

FzAstNode*      g_pAstParm[]    = { NULL, NULL, NULL };
EzMachine*      g_pVmParm[]     = { NULL, NULL, NULL };

RenderNode*     g_pRenderNode   = NULL;
RenderConfig    g_rzConfig;

int             g_iProjection = ORTHOGRAPHIC;

static const char* szFuncTypeLabel[] = { "CARTESIAN", "PARAMETRIC" };

/* Vertex Buffer */
NUMERIC g_fVertexBuf[VERTEX_BUFFER_SIZE];

/* Cartesian */
NUMERIC *g_fCartXBuf = g_fVertexBuf;
NUMERIC *g_fCartYBuf = g_fVertexBuf + X_GRID_MAX;
NUMERIC *g_fCartZBuf = g_fVertexBuf + X_GRID_MAX + Y_GRID_MAX;

#define CART_Z_BUF(x,y) (g_fCartZBuf[(x) + (y) * Camera.xGrid])

/* Parametric */
NUMERIC *g_fParmXBuf = g_fVertexBuf;
NUMERIC *g_fParmYBuf = g_fVertexBuf + U_GRID_MAX * V_GRID_MAX;
NUMERIC *g_fParmZBuf = g_fVertexBuf + U_GRID_MAX * V_GRID_MAX * 2;

#define PARM_X_BUF(iu,iv) (g_fParmXBuf[(iu) + (iv) * Camera.uGrid])
#define PARM_Y_BUF(iu,iv) (g_fParmYBuf[(iu) + (iv) * Camera.uGrid])
#define PARM_Z_BUF(iu,iv) (g_fParmZBuf[(iu) + (iv) * Camera.uGrid])

/* GraphForm interaction */
static Int16 	g_iDrawMode = 0; /* 0=Camera, 1=Pan Move, 2=Zoom, 3=Fov */
static Boolean 	g_bDrawBox = false;
static Boolean 	g_bDrawAxes = false;
static Boolean 	g_bDrawFooter = true;
static Boolean 	g_bDrawPenDown = false;
static Int16 	g_iDrawPrevX, g_iDrawPrevY;
static Int16 	g_iDrawZoomAccum = 0;
static Int16 	g_iFovAccum = 0;
static Int16 	g_iSurfaceDragThreshold = 12;
static Int16 	g_iFormulaDragThreshold = 6;
static UInt32 	g_dwLastDrawUpdate = 0;

/* MainForm interaction */
static Int16 	g_iFormulaX = 0;
static Int16 	g_iFormulaY = 0;
static Int16 	g_iMainFormState = MAIN_FORM_IDLE;
static Boolean	g_bParseSuccess = false;

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
static void RzSetPixel(int x, int y) {
    BmpBuffer_SetPixel(g_pBmpBufFormula, x, y, 1);
}

static void RzPlotLine(int x0, int y0, int x1, int y1) {
    BmpBuffer_PlotLine(g_pBmpBufFormula, x0, y0, x1, y1, 1);
}

static void RzPutChar(int x, int y, UInt8 ch) {
    BmpBuffer_PutChar(g_pBmpBufFormula, x, y, ch, 1);
}

/*====================================================
 * Parse expression (AST, compile, build RenderNode)
 *====================================================*/
void HandleParseError(
    int bIsSyntaxError,
    int iCompileError,
    const char* szErrorBuf,
    int bIsParm,
    int iParamIndex
) {
    static const char* szParmLabel[] = { "x", "y", "z" };

    g_szErrMsgLine1[0] = 0;
    g_szErrMsgLine2[0] = 0;

    if (bIsSyntaxError) {
        if (bIsParm) {
            StrPrintF(g_szErrMsgLine1, "Failed to parse \"%s=\":", szParmLabel[iParamIndex]);
        } else {
            StrCopy(g_szErrMsgLine1, "Failed to parse:");
        }
        StrCopy(g_szErrMsgLine2, "Syntax error.");
    }
    else {
        switch (iCompileError) {
            case EZERR_VARIABLE_UNDEFINED:
                StrCopy(g_szErrMsgLine1, "Undefined variable:");
                StrPrintF(g_szErrMsgLine2, "\"%s\"", szErrorBuf);
                break;
            case EZERR_FUNCTION_UNDEFINED:
                StrCopy(g_szErrMsgLine1, "Undefined function:");
                StrPrintF(g_szErrMsgLine2, "\"%s\"", szErrorBuf);
                break;
            case EZERR_FUNCTION_PARAM_MISMATCH:
                StrCopy(g_szErrMsgLine1, "Function parameter mismatch");
                StrPrintF(g_szErrMsgLine2, "\"%s\"", szErrorBuf);
                break;
        }
    }

	g_iMainFormState = MAIN_FORM_ERROR;
}

int ParseAndRenderExpr(void) {
    int iW = g_pBmpBufFormula->iW, iH = g_pBmpBufFormula->iH - 16, i;
    char szErrorBuf[EZ_ERROR_CONTENT_LENGTH];
    int iCompileError;

	g_bParseSuccess = true;

    /* Destroy previous AST, RenderNode */
    if (g_pRenderNode != NULL) {
        RenderNode_Destroy(g_pRenderNode);
        g_pRenderNode = NULL;
    }
    if (g_pAstCartZ != NULL) {
        FzAstNode_Destroy(g_pAstCartZ);
        g_pAstCartZ = NULL;
    }
    for (i = 0; i < 3; ++i) {
        if (g_pAstParm[i] != NULL) {
            FzAstNode_Destroy(g_pAstParm[i]);
            g_pAstParm[i] = NULL;
        }
    }

    if (g_iFuncType == FUNC_TYPE_CARTESIAN) {
        /* Parse */
        g_pAstCartZ = FzParser_ParseExpression(g_szCartExpr);
        if (g_pAstCartZ == NULL) {
            HandleParseError(1, EZERR_NONE, NULL, 0, 0);
            return 0;
        }

        /* Create VM singleton */
        if (g_pVmCartZ == NULL) {
            g_pVmCartZ = EzMachine_Create();
            if (g_pVmCartZ != NULL) {
                EzMachine_DeclareVariable(g_pVmCartZ, "x");
                EzMachine_DeclareVariable(g_pVmCartZ, "y");
                EzMachine_DeclareVariable(g_pVmCartZ, "pi");
                EzMachine_DeclareVariable(g_pVmCartZ, "t");
                EzMachine_AllocateVariables(g_pVmCartZ);
                EzMachine_SetVariableByIndex(g_pVmCartZ, 2, PZ_PI);
            }
        }

        /* Compile */
        if (g_pVmCartZ != NULL) {
            iCompileError = EzMachine_Compile(g_pVmCartZ, g_pAstCartZ, szErrorBuf);
            if (iCompileError != EZERR_NONE) {
                HandleParseError(0, iCompileError, szErrorBuf, 0, 0);
            }
        }

        /* Build render tree */
        g_pRenderNode = Render_Transform(g_pAstCartZ, "z=");
    }
    else if (g_iFuncType == FUNC_TYPE_PARAMETRIC) {
        const char *szPrefix[] = { "x=", "y=", "z=" };
        /* Parse */
        for (i = 0; i < 3; ++i) {
            g_pAstParm[i] = FzParser_ParseExpression(g_szParmExpr[i]);
            if (g_pAstParm[i] == NULL) {
                HandleParseError(1, EZERR_NONE, NULL, 1, i);
                return 0;
            }
        }

        /* Build render tree */
        g_pRenderNode = RenderNode_Create(RN_VERTICAL);
        for (i = 0; i < 3; ++i) {
            vlPushBack(g_pRenderNode->uData.sVertical.pList, Render_Transform(g_pAstParm[i], szPrefix[i]));
        }

        /* Compile */
        for (i = 0; i < 3; ++i) {
            /* Create VM singleton */
            if (g_pVmParm[i] == NULL) {
                g_pVmParm[i] = EzMachine_Create();
                EzMachine_DeclareVariable(g_pVmParm[i], "u");
                EzMachine_DeclareVariable(g_pVmParm[i], "v");
                EzMachine_DeclareVariable(g_pVmParm[i], "pi");
                EzMachine_DeclareVariable(g_pVmParm[i], "t");
                EzMachine_AllocateVariables(g_pVmParm[i]);
                EzMachine_SetVariableByIndex(g_pVmParm[i], 2, PZ_PI);
            }
            /* Compile expression to VM */
            if (g_pVmParm[i] != NULL) {
                iCompileError = EzMachine_Compile(g_pVmParm[i], g_pAstParm[i], szErrorBuf);
                if (iCompileError != EZERR_NONE) {
                    HandleParseError(0, iCompileError, szErrorBuf, 1, i);
                }
            }
        }
    }

    if (g_pRenderNode == NULL) return 0;

    RenderNode_CalculateSize(g_pRenderNode, &g_rzConfig);

    /* Center formula */
    g_iFormulaX = (iW - g_pRenderNode->sLayout.iWidth) / 2;
    if (g_iFormulaX < 0) g_iFormulaX = 0;
    g_iFormulaY = (iH - g_pRenderNode->sLayout.iAscent - g_pRenderNode->sLayout.iDescent) / 2 + g_pRenderNode->sLayout.iAscent;
    
	g_bParseSuccess = true;
	g_iMainFormState = MAIN_FORM_READY;
    return 1;
}

/*====================================================
 * Recalculate surface geometry
 *====================================================*/

static void RedrawRecalcProgress(Int16 iPct) {
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

static void RecalcCartesian(void) {
    int iX, iY;
    PZ_FLOAT fX[X_GRID_MAX];
    PZ_FLOAT fY[Y_GRID_MAX];
    PZ_FLOAT fZ;

    /* Compute actual x, y sample positions and store in buffers */
    for (iX = 0; iX < Camera.xGrid; ++iX) {
        fX[iX] = Camera.xMin + (Camera.xMax - Camera.xMin) * iX / (Camera.xGrid - 1);
    }
    for (iY = 0; iY < Camera.yGrid; ++iY) {
        fY[iY] = Camera.yMin + (Camera.yMax - Camera.yMin) * iY / (Camera.yGrid - 1);
    }

    /* Evaluate z = f(x, y) for every grid point */
    for (iX = 0; iX < Camera.xGrid; ++iX) {
        for (iY = 0; iY < Camera.yGrid; ++iY) {
            EzMachine_SetVariableByIndex(g_pVmCartZ, 0, fX[iX]);
            EzMachine_SetVariableByIndex(g_pVmCartZ, 1, fY[iY]);
            /* Evaluate the z value */
            fZ = EzMachine_Eval(g_pVmCartZ);
            /* Normalize z into [-1, 1] based on the z-axis bounds */
            CART_Z_BUF(iX, iY) = NUM_VAL(2.0f * (fZ - (Camera.zMax + Camera.zMin) / 2.0f) / (Camera.zMax - Camera.zMin));
        }
        RedrawRecalcProgress((iX + 1) * 100 / Camera.uGrid);
    }

    /* Normalize x and y grid coordinates into [-1, 1] */
    for (iX = 0; iX < Camera.xGrid; ++iX) {
        g_fCartXBuf[iX] = NUM_VAL(2.0f * (fX[iX] - (Camera.xMax + Camera.xMin) / 2.0f) / (Camera.xMax - Camera.xMin));
    }
    for (iY = 0; iY < Camera.yGrid; ++iY) {
        g_fCartYBuf[iY] = NUM_VAL(2.0f * (fY[iY] - (Camera.yMax + Camera.yMin) / 2.0f) / (Camera.yMax - Camera.yMin));
    }
}

void RecalcParametric(void) {
    int iU, iV, i;
    PZ_FLOAT fU, fV;
    PZ_FLOAT fPoint[3];

    if (!g_pVmParm[0] || !g_pVmParm[1] || !g_pVmParm[2]) {
        return;
    }

    for (iU = 0; iU < Camera.uGrid; ++iU) {
        fU = Camera.uMin + (Camera.uMax - Camera.uMin) * iU / (Camera.uGrid - 1);
        for (iV = 0; iV < Camera.vGrid; ++iV) {
            fV = Camera.vMin + (Camera.vMax - Camera.vMin) * iV / (Camera.vGrid - 1);
            /* Evaluate x */
            for (i = 0; i < 3; ++i) {
                EzMachine_SetVariableByIndex(g_pVmParm[i], 0, fU);
                EzMachine_SetVariableByIndex(g_pVmParm[i], 1, fV);
                fPoint[i] = EzMachine_Eval(g_pVmParm[i]);
            }
            /* Normalize */
            PARM_X_BUF(iU, iV) = NUM_VAL(2.0f * (fPoint[0] - (Camera.xMax + Camera.xMin) / 2.0f) / (Camera.xMax - Camera.xMin));
            PARM_Y_BUF(iU, iV) = NUM_VAL(2.0f * (fPoint[1] - (Camera.yMax + Camera.yMin) / 2.0f) / (Camera.yMax - Camera.yMin));
            PARM_Z_BUF(iU, iV) = NUM_VAL(2.0f * (fPoint[2] - (Camera.zMax + Camera.zMin) / 2.0f) / (Camera.zMax - Camera.zMin));
        }
        RedrawRecalcProgress((iU + 1) * 100 / Camera.uGrid);
    }
}

static int RecalcSurface() {
    RedrawRecalcProgress(0);

    switch (g_iFuncType) {
        case FUNC_TYPE_CARTESIAN:
            RecalcCartesian();
            break;
        case FUNC_TYPE_PARAMETRIC:
            RecalcParametric();
            break;
    }
    return 1;
}

/*====================================================
 * Draw wireframe to bitmap
 *====================================================*/
static void DrawCartSurfaceWireframe(void (*xyz2xy)(NUMERIC, NUMERIC, NUMERIC, int*, int *)) {
    int iX, iY, x0, y0, x1, y1;
    NUMERIC z0, z1;

    for (iX = 0; iX < Camera.xGrid; ++iX) {
        iY = 0;
        xyz2xy(g_fCartXBuf[iX], g_fCartYBuf[iY], z0 = CART_Z_BUF(iX, iY), &x0, &y0); 
        for (iY = 0; iY < Camera.yGrid; ++iY) {
            xyz2xy(g_fCartXBuf[iX], g_fCartYBuf[iY], z1 = CART_Z_BUF(iX, iY), &x1, &y1); 
            if (
                z0 <= NUM_VAL(1.0f) &&
                z0 >= NUM_VAL(-1.0f) &&
                z1 <= NUM_VAL(1.0f) &&
                z1 >= NUM_VAL(-1.0f)
            ) {
                BmpBuffer_PlotLine(g_pBmpCanvas, x0, y0, x1, y1, 1);
            }
            x0 = x1, y0 = y1, z0 = z1;
        }
    }

    for (iY = 0; iY < Camera.yGrid; ++iY) {
        iX = 0;
        xyz2xy(g_fCartXBuf[iX], g_fCartYBuf[iY], z0 = CART_Z_BUF(iX, iY), &x0, &y0); 
        for (iX = 0; iX < Camera.xGrid; ++iX) {
            xyz2xy(g_fCartXBuf[iX], g_fCartYBuf[iY], z1 = CART_Z_BUF(iX, iY), &x1, &y1); 
            if (
                z0 <= NUM_VAL(1.0f) &&
                z0 >= NUM_VAL(-1.0f) &&
                z1 <= NUM_VAL(1.0f) &&
                z1 >= NUM_VAL(-1.0f)
            ) {
                BmpBuffer_PlotLine(g_pBmpCanvas, x0, y0, x1, y1, 1);
            }
            x0 = x1, y0 = y1, z0 = z1;
        }
    }
}

static void DrawParmSurfaceWireframe(void (*xyz2xy)(NUMERIC, NUMERIC, NUMERIC, int*, int *)) {
    int iU, iV, x0, y0, x1, y1;
    for (iV = 0; iV < Camera.vGrid; ++iV) {
        iU = 0;
        xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x0, &y0); 
        for (iU = 0; iU < Camera.uGrid; ++iU) {
            xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x1, &y1);
            BmpBuffer_PlotLine(g_pBmpCanvas, x0, y0, x1, y1, 1);
            x0 = x1, y0 = y1;
        }
    }
    for (iU = 0; iU < Camera.uGrid; ++iU) {
        iV = 0;
        xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x0, &y0); 
        for (iV = 0; iV < Camera.vGrid; ++iV) {
            xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x1, &y1); 
            BmpBuffer_PlotLine(g_pBmpCanvas, x0, y0, x1, y1, 1);
            x0 = x1, y0 = y1;
        }
    }
}

/*====================================================
 * Redraw wireframe on canvas
 *====================================================*/
static void RefreshCameraTrigBuf(void) {
#ifdef USE_FIXED_POINT
    Camera.uTrigBuf.sFixed.sinA = PZ_FLOAT_TO_FIXED(sin(Camera.iAlphaDeg * PZ_PI / 180));
    Camera.uTrigBuf.sFixed.cosA = PZ_FLOAT_TO_FIXED(cos(Camera.iAlphaDeg * PZ_PI / 180));
    Camera.uTrigBuf.sFixed.sinB = PZ_FLOAT_TO_FIXED(sin(Camera.iBetaDeg * PZ_PI / 180));
    Camera.uTrigBuf.sFixed.cosB = PZ_FLOAT_TO_FIXED(cos(Camera.iBetaDeg * PZ_PI / 180));
#else
    Camera.uTrigBuf.sFloat.sinA = (PZ_FLOAT)sin(Camera.iAlphaDeg * PZ_PI / 180);
    Camera.uTrigBuf.sFloat.cosA = (PZ_FLOAT)cos(Camera.iAlphaDeg * PZ_PI / 180);
    Camera.uTrigBuf.sFloat.sinB = (PZ_FLOAT)sin(Camera.iBetaDeg * PZ_PI / 180);
    Camera.uTrigBuf.sFloat.cosB = (PZ_FLOAT)cos(Camera.iBetaDeg * PZ_PI / 180);
#endif
}

static void DrawBoxEdges(void (*xyz2xy)(NUMERIC, NUMERIC, NUMERIC, int*, int *)) {
    static const Vertex BoxVertices[] = {
        { NUM_VAL(1.0f),    NUM_VAL(1.0f),  NUM_VAL(1.0f)   },
        { NUM_VAL(-1.0f),   NUM_VAL(1.0f),  NUM_VAL(1.0f)   },
        { NUM_VAL(-1.0f),   NUM_VAL(-1.0f), NUM_VAL(1.0f)   },
        { NUM_VAL(1.0f),    NUM_VAL(-1.0f), NUM_VAL(1.0f)   },
        { NUM_VAL(1.0f),    NUM_VAL(1.0f),  NUM_VAL(-1.0f)  },
        { NUM_VAL(-1.0f),   NUM_VAL(1.0f),  NUM_VAL(-1.0f)  },
        { NUM_VAL(-1.0f),   NUM_VAL(-1.0f), NUM_VAL(-1.0f)  },
        { NUM_VAL(1.0f),    NUM_VAL(-1.0f), NUM_VAL(-1.0f)  },
    };

    static const Edge BoxEdges[] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
    };

    static const int numEdges = sizeof(BoxEdges) / sizeof(BoxEdges[0]);
    int i;

    for (i = 0; i < numEdges; ++i) {
        const Edge* e = BoxEdges + i;
        const Vertex* v0 = BoxVertices + e->i0;
        const Vertex* v1 = BoxVertices + e->i1;
        int x0, y0, x1, y1;

        xyz2xy(v0->x, v0->y, v0->z, &x0, &y0); 
        xyz2xy(v1->x, v1->y, v1->z, &x1, &y1); 
        BmpBuffer_PlotLine(g_pBmpCanvas, x0, y0, x1, y1, 1);
    }
}

#define AXES_ARROW_SIZE (0.05f)

static void DrawAxes(void (*xyz2xy)(NUMERIC, NUMERIC, NUMERIC, int*, int *)) {
    static const Vertex AxesVertices[] = {
        /* [0] Origin */
        { NUM_VAL(0), NUM_VAL(0),  NUM_VAL(0) },
        /* [1] X-axis direction */
        { NUM_VAL(1), NUM_VAL(0),  NUM_VAL(0) },
        /* [2] X-axis arrow line (upper) */
        { NUM_VAL(1 - AXES_ARROW_SIZE), NUM_VAL(AXES_ARROW_SIZE),  NUM_VAL(0) },
        /* [3] X-axis arrow line (lower) */
        { NUM_VAL(1 - AXES_ARROW_SIZE), NUM_VAL(-AXES_ARROW_SIZE),  NUM_VAL(0) },
        /* [4] Y-axis direction */
        { NUM_VAL(0), NUM_VAL(1),  NUM_VAL(0) },
        /* [5] Y-axis arrow line (upper) */
        { NUM_VAL(AXES_ARROW_SIZE), NUM_VAL(1 - AXES_ARROW_SIZE),  NUM_VAL(0) },
        /* [6] Y-axis arrow line (lower) */
        { NUM_VAL(-AXES_ARROW_SIZE), NUM_VAL(1 - AXES_ARROW_SIZE),  NUM_VAL(0) },
        /* [7] Z-axis direction */
        { NUM_VAL(0), NUM_VAL(0),  NUM_VAL(1) },
        /* [8] Z-axis arrow line (upper) */
        { NUM_VAL(AXES_ARROW_SIZE), NUM_VAL(0),  NUM_VAL(1 - AXES_ARROW_SIZE ) },
        /* [9] Z-axis arrow line (lower) */
        { NUM_VAL(-AXES_ARROW_SIZE), NUM_VAL(-0),  NUM_VAL(1 - AXES_ARROW_SIZE) },
    };

    static const Edge AxesEdges[] = {
        { 0, 1 }, { 1, 2 }, { 1, 3 },   /* X-axis */
        { 0, 4 }, { 4, 5 }, { 4, 6 },   /* Y-axis */
        { 0, 7 }, { 7, 8 }, { 7, 9 }    /* Z-axis */
    };

    static const int numEdges = sizeof(AxesEdges) / sizeof(AxesEdges[0]);
    int i;

    for (i = 0; i < numEdges; ++i) {
        const Edge* e = AxesEdges + i;
        const Vertex* v0 = AxesVertices + e->i0;
        const Vertex* v1 = AxesVertices + e->i1;
        int x0, y0, x1, y1;

        xyz2xy(v0->x, v0->y, v0->z, &x0, &y0); 
        xyz2xy(v1->x, v1->y, v1->z, &x1, &y1); 
        BmpBuffer_PlotLine(g_pBmpCanvas, x0, y0, x1, y1, 1);
    }
}

static void RedrawCanvas() {
    void (*xyz2xy)(NUMERIC, NUMERIC, NUMERIC, int*, int *) = NULL;

    switch (g_iProjection) {
        case PERSPECTIVE:
            xyz2xy = PerspProject;
            break;
        case ORTHOGRAPHIC:
        default:
            xyz2xy = OrthoProject;
            break;
    }

	RefreshCameraTrigBuf();
    BmpBuffer_AllClear(g_pBmpCanvas);

	/* Box */
    if (g_bDrawBox) DrawBoxEdges(xyz2xy);

	/* Aexs */
    if (g_bDrawAxes) DrawAxes(xyz2xy);

	/* Surface */
    switch (g_iFuncType) {
        case FUNC_TYPE_CARTESIAN:
            DrawCartSurfaceWireframe(xyz2xy);
            break;
        case FUNC_TYPE_PARAMETRIC:
            DrawParmSurfaceWireframe(xyz2xy);
            break;
    }
}

static void MainFormDrawIdle(void) {
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

static void MainFormDrawFormula(void) {
    Int16 x, y;
    Int16 iW, iH;

    iW = g_pBmpBufFormula->iW;
    iH = g_pBmpBufFormula->iH;

    BmpBuffer_AllClear(g_pBmpBufFormula);

    /* Line 2: formula rendering */
    RenderNode_Draw(g_pRenderNode, &g_rzConfig, g_iFormulaX, g_iFormulaY);

    /* Line 1: Type label filled background */
    {
        const char* szLabel = szFuncTypeLabel[g_iFuncType];
        BmpBuffer_FillRect(g_pBmpBufFormula, 0, 0, iW, CURRENT_FONT_HEIGHT, 1);
        BmpBuffer_PutText(g_pBmpBufFormula, (iW - CURRENT_FONT_WIDTH * StrLen(szLabel)) / 2, 0, (UInt8*)szLabel, 0);
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

static void MainFormDrawError(void) {
    int iW;
    static const char* szTitle = "Error";

    iW = StrLen(szTitle) * CURRENT_FONT_WIDTH;
	BmpBuffer_FillRect(g_pBmpBufFormula, 0, 0,iW, CURRENT_FONT_HEIGHT, 1);
    BmpBuffer_PutText(g_pBmpBufFormula, (g_pBmpBufFormula->iW - iW) / 2, 0, (const UInt8 *)szTitle, 0);
    
    iW = StrLen(g_szErrMsgLine1) * CURRENT_FONT_WIDTH;
    BmpBuffer_PutText(g_pBmpBufFormula, (g_pBmpBufFormula->iW - iW) / 2, 24, (const UInt8 *)g_szErrMsgLine1, 1);

    iW = StrLen(g_szErrMsgLine2) * CURRENT_FONT_WIDTH;
    BmpBuffer_PutText(g_pBmpBufFormula, (g_pBmpBufFormula->iW - iW) / 2, 32, (const UInt8 *)g_szErrMsgLine2, 1);
}

static void MainFormBitmapRedraw(void) {
	switch (g_iMainFormState) {
		case MAIN_FORM_IDLE:
			MainFormDrawIdle();
			break;
		case MAIN_FORM_ERROR:
			MainFormDrawError();
			break;
		case MAIN_FORM_READY:
			MainFormDrawFormula();
			break;
	}
}

static void MainFormInit(FormType *frmP) {
	MainFormBitmapRedraw();
	/* Plot button starts disabled */
	if (!g_bParseSuccess) {
		ControlType *ctlDraw;
		ctlDraw = (ControlType*)FrmGetObjectPtr(frmP,
			FrmGetObjectIndex(frmP, MainDrawButton));
		if (ctlDraw != NULL) {
			CtlHideControl(ctlDraw);
		}
	}
}

static void MainFormStartParse(void) {
	ControlType *pCtlPlot;

	ParseAndRenderExpr();
	MainFormBitmapRedraw();

	/* Enable or disable Draw button */
	pCtlPlot = (ControlType*)GetObjectPtr(MainDrawButton);
	if (pCtlPlot != NULL) {
		if (g_bParseSuccess)
			CtlShowControl(pCtlPlot);
		else
			CtlHideControl(pCtlPlot);
	}

	/* Trigger redraw */
	FrmUpdateForm(MainForm, frmRedrawUpdateCode);
}

static void WinEditorSetText(FieldType* fieldP, const char* szText) {
    MemHandle newHandle;
    MemHandle oldHandle;
    Char* pText;
    UInt16 iTextLen;

	if (fieldP == NULL) return;

    iTextLen = StrLen(szText) + 1;

    newHandle = MemHandleNew(iTextLen);
    if (newHandle == NULL) {
        return; 
    }

    pText = (Char*)MemHandleLock(newHandle);
    StrCopy(pText, szText);
    
    MemHandleUnlock(newHandle);

    oldHandle = FldGetTextHandle(fieldP);

    FldSetTextHandle(fieldP, newHandle);

    if (oldHandle != NULL) {
        MemHandleFree(oldHandle);
    }
}

static void WinEditorSetFloatValue(FormType * frmP, UInt16 id, double fValue) {
	FieldType* fieldP = (FieldType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, id));
	char szBuf[50];
	Utils_Ftoa((double)fValue, szBuf, 4);
	WinEditorSetText(fieldP, szBuf);
}

static void WinEditorSetIntValue(FormType * frmP, UInt16 id, int iValue) {
	FieldType* fieldP = (FieldType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, id));
	char szBuf[50];
	StrIToA(szBuf, iValue);
	WinEditorSetText(fieldP, szBuf);
}

static char* WinEditorGetText(FieldType* fieldP, char* szBuf) {
	MemHandle handle;
	MemPtr text;
	handle = FldGetTextHandle(fieldP);
	text = MemHandleLock(handle);
	StrCopy(szBuf, (const char*)text);
	MemHandleUnlock(handle);
	return szBuf;
}

static PZ_FLOAT WinEditorGetFloatValue(FormType * frmP, UInt16 id) {
	FieldType* fieldP = (FieldType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, id));
	char szBuf[50];
	return Utils_Atof(WinEditorGetText(fieldP, szBuf));
}

static PZ_FLOAT WinEditorGetIntValue(FormType * frmP, UInt16 id, int iMin, int iMax) {
	FieldType* fieldP = (FieldType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, id));
	char szBuf[50];
	int iVal = Utils_Atoi(WinEditorGetText(fieldP, szBuf));
	if (iVal < iMin) return iMin;
	if (iVal > iMax) return iMax;
	return iVal;
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

			MenuEraseStatus(0);

			frmP = FrmInitForm(WinEditorForm);

			/* Populate fields from Camera */
			{
				WinEditorSetFloatValue(frmP, WinEditorFieldXmin, (double)Camera.xMin);
				WinEditorSetFloatValue(frmP, WinEditorFieldXmax, (double)Camera.xMax);
				WinEditorSetIntValue(frmP, WinEditorFieldXgrid, Camera.xGrid);
				WinEditorSetFloatValue(frmP, WinEditorFieldYmin, (double)Camera.yMin);
				WinEditorSetFloatValue(frmP, WinEditorFieldYmax, (double)Camera.yMax);
				WinEditorSetIntValue(frmP, WinEditorFieldYgrid, Camera.yGrid);
				WinEditorSetFloatValue(frmP, WinEditorFieldZmin, (double)Camera.zMin);
				WinEditorSetFloatValue(frmP, WinEditorFieldZmax, (double)Camera.zMax);
				WinEditorSetFloatValue(frmP, WinEditorFieldUmin, (double)Camera.uMin);
				WinEditorSetFloatValue(frmP, WinEditorFieldUmax, (double)Camera.uMax);
				WinEditorSetIntValue(frmP, WinEditorFieldUgrid, Camera.uGrid);
				WinEditorSetFloatValue(frmP, WinEditorFieldVmin, (double)Camera.vMin);
				WinEditorSetFloatValue(frmP, WinEditorFieldVmax, (double)Camera.vMax);
				WinEditorSetIntValue(frmP, WinEditorFieldVgrid, Camera.vGrid);
			}

			controlID = FrmDoDialog(frmP);

			if (controlID == WinEditorOKButton) {
				Camera.xMin = WinEditorGetFloatValue(frmP, WinEditorFieldXmin);
				Camera.xMax = WinEditorGetFloatValue(frmP, WinEditorFieldXmax);
				Camera.xGrid = WinEditorGetIntValue(frmP, WinEditorFieldXgrid, 5, X_GRID_MAX);
				Camera.yMin = WinEditorGetFloatValue(frmP, WinEditorFieldYmin);
				Camera.yMax = WinEditorGetFloatValue(frmP, WinEditorFieldYmax);
				Camera.yGrid = WinEditorGetIntValue(frmP, WinEditorFieldYgrid, 5, Y_GRID_MAX);
				Camera.zMin = WinEditorGetFloatValue(frmP, WinEditorFieldZmin);
				Camera.zMax = WinEditorGetFloatValue(frmP, WinEditorFieldZmax);
				Camera.uMin = WinEditorGetFloatValue(frmP, WinEditorFieldUmin);
				Camera.uMax = WinEditorGetFloatValue(frmP, WinEditorFieldUmax);
				Camera.uGrid = WinEditorGetIntValue(frmP, WinEditorFieldUgrid, 5, U_GRID_MAX);
				Camera.vMin = WinEditorGetFloatValue(frmP, WinEditorFieldVmin);
				Camera.vMax = WinEditorGetFloatValue(frmP, WinEditorFieldVmax);
				Camera.vGrid = WinEditorGetIntValue(frmP, WinEditorFieldVgrid, 5, V_GRID_MAX);
			}

			FrmDeleteForm(frmP);

			handled = true;
			break;
		}

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
				ppItems[i] = (char *)PlotterZSamples[i].szName;

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
					Camera.uMin = p->uMin;
					Camera.uMax = p->uMax;
					Camera.vMin = p->vMin;
					Camera.vMax = p->vMax;
					Camera.xGrid = 15;
					Camera.yGrid = 15;
					Camera.uGrid = 15;
					Camera.vGrid = 15;
					g_iFuncType = p->iFuncType;
					switch (p->iFuncType) {
						case FUNC_TYPE_CARTESIAN:
							Utils_StringCopy(g_szCartExpr, sizeof(g_szCartExpr), p->szExpr[0]);
							break;
						case FUNC_TYPE_PARAMETRIC:
							for (i = 0; i < 3; ++i) {
								Utils_StringCopy(g_szParmExpr[i], PARM_EXPR_LENGTH, p->szExpr[i]);
							}
							break;
					}
				}
				MainFormStartParse();
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
static void DrawGraphUI(BmpBuffer* pBuf) {
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

static Boolean GraphFormHandleEvent(EventType * eventP) {
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

			if (RecalcSurface()) {
				RedrawCanvas(g_pBmpCanvas);
				DrawGraphUI(g_pBmpCanvas);
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
					RedrawCanvas(g_pBmpCanvas);
					DrawGraphUI(g_pBmpCanvas);
					WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
					handled = true;
					break;
				}

				/* Middle: ORTHO/PERSP toggle */
				if (iTapX >= 55 && iTapX <= 105) {
					g_iProjection = !g_iProjection;
					RedrawCanvas(g_pBmpCanvas);
					DrawGraphUI(g_pBmpCanvas);
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
						RedrawCanvas(g_pBmpCanvas);
						DrawGraphUI(g_pBmpCanvas);
						WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
						handled = true;
						break;
					}
					if (iTapX >= iRightX + 10 && iTapX < iRightX + 20) {
						/* Toggle bounding box */
						g_bDrawBox = !g_bDrawBox;
						RedrawCanvas(g_pBmpCanvas);
						DrawGraphUI(g_pBmpCanvas);
						WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
						handled = true;
						break;
					}
					if (iTapX >= iRightX + 20 && iTapX < iRightX + 30) {
						/* Toggle footer */
						g_bDrawFooter = !g_bDrawFooter;
						RedrawCanvas(g_pBmpCanvas);
						DrawGraphUI(g_pBmpCanvas);
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
				RedrawCanvas(g_pBmpCanvas);
				DrawGraphUI(g_pBmpCanvas);
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
						RedrawCanvas(g_pBmpCanvas);
						DrawGraphUI(g_pBmpCanvas);
						WinDrawBitmap(g_pBmpCanvas->pBmp, 0, 0);
						g_dwLastDrawUpdate = dwNow;
					}
				}
			}
			handled = true;
			break;

		case penUpEvent:
			if (g_bDrawPenDown) {
				RedrawCanvas(g_pBmpCanvas);
				DrawGraphUI(g_pBmpCanvas);
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
			MainFormInit(frmP);
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
			if (g_iMainFormState == MAIN_FORM_READY && eventP->screenY > 16 && eventP->screenY < 116) {
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
						MainFormBitmapRedraw();
						WinDrawBitmap(g_pBmpBufFormula->pBmp, 0, 15);
						g_dwLastDrawUpdate = dwNow;
					}
				}
				handled = true;
			}
			break;

		case penUpEvent:
			if (g_bDrawPenDown) {
				MainFormBitmapRedraw();
				WinDrawBitmap(g_pBmpBufFormula->pBmp, 0, 15);
				handled = true;
			}
			g_bDrawPenDown = false;
			g_dwLastDrawUpdate = 0;
			break;
			handled = true;
			break;

		case ctlSelectEvent: {
			if (eventP->data.ctlSelect.controlID == MainCartesianButton) {
				FormType * frmP;
				FieldType * field;
				UInt16 controlID;

				frmP = FrmInitForm(CartesianForm);

				/* Populate field */
				field = (FieldType*)FrmGetObjectPtr(frmP,
					FrmGetObjectIndex(frmP, CartesianExprField));
				if (field != NULL)
					FldInsert(field, g_szCartExpr, StrLen(g_szCartExpr));

				controlID = FrmDoDialog(frmP);

				if (controlID == CartesianOKButton) {
					MemHandle handle;
					MemPtr text;
					handle = FldGetTextHandle(field);
					if (handle) {
						text = MemHandleLock(handle);
						if (text) {
							UInt32 len = StrLen((const char*)text);
							UInt32 count = (len > (sizeof(g_szCartExpr) - 1))
							             ? (sizeof(g_szCartExpr) - 1) : len;
							MemMove(g_szCartExpr, text, count);
							g_szCartExpr[count] = '\0';
							
						}
						MemHandleUnlock(handle);
						g_iFuncType = FUNC_TYPE_CARTESIAN;
						MainFormStartParse();
					}
				}

				FrmDeleteForm(frmP);
				handled = true;
				break;
			}

			if (eventP->data.ctlSelect.controlID == MainParametricButton) {
				FormType * frmP;
				FieldType * fields[3];
				UInt16 controlID;
				int i;
				UInt16 fieldIds[3] = { ParmExprField1, ParmExprField2, ParmExprField3 };

				frmP = FrmInitForm(ParametricForm);

				/* Populate fields */
				for (i = 0; i < 3; ++i) {
					fields[i] = (FieldType*)FrmGetObjectPtr(frmP,
						FrmGetObjectIndex(frmP, fieldIds[i]));
					if (fields[i] != NULL)
						FldInsert(fields[i], g_szParmExpr[i], StrLen(g_szParmExpr[i]));
				}

				controlID = FrmDoDialog(frmP);

				if (controlID == ParmOKButton) {
					for (i = 0; i < 3; ++i) {
						if (fields[i] != NULL) {
							MemHandle handle;
							MemPtr text;
							handle = FldGetTextHandle(fields[i]);
							if (handle) {
								text = MemHandleLock(handle);
								if (text) {
									UInt32 len = StrLen((const char*)text);
									UInt32 count = (len > (sizeof(g_szParmExpr[i]) - 1))
									             ? (sizeof(g_szParmExpr[i]) - 1) : len;
									MemMove(g_szParmExpr[i], text, count);
									g_szParmExpr[i][count] = '\0';
								}
								MemHandleUnlock(handle);
								g_iFuncType = FUNC_TYPE_PARAMETRIC;
								MainFormStartParse();
							}
						}
					}
				}

				FrmDeleteForm(frmP);
				handled = true;
				break;
			}

			if (eventP->data.ctlSelect.controlID == MainDrawButton) {
				FrmGotoForm(GraphForm);
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

			case GraphForm:
				FrmSetEventHandler(frmP, GraphFormHandleEvent);
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
	g_pBmpBufFormula = BmpBuffer_Create(160, 130);
	g_pBmpCanvas     = BmpBuffer_Create(160, 160);

	/* Set up renderer-Z config */
	RenderConfig_GetDefaultStyle(&g_rzConfig);
	RenderConfig_CalculateBigSymbolPoints(&g_rzConfig);
	g_rzConfig.sInterfaces.setPixel = RzSetPixel;
	g_rzConfig.sInterfaces.plotLine = RzPlotLine;
	g_rzConfig.sInterfaces.putChar  = RzPutChar;

	/* Initialize Camera defaults */
	PzCamera_Initialize();
	Camera.uGrid = U_GRID_MAX;
	Camera.vGrid = V_GRID_MAX;

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
	int i;

	PrefSetAppPreferences(
		appFileCreator, appPrefID, appPrefVersionNum,
		&g_prefs, sizeof(g_prefs), true);

	FrmCloseAllForms();

	/* Destroy renderer-Z objects */
    if (g_pRenderNode != NULL) {
        RenderNode_Destroy(g_pRenderNode);
        g_pRenderNode = NULL;
    }
    if (g_pAstCartZ != NULL) {
        FzAstNode_Destroy(g_pAstCartZ);
        g_pAstCartZ = NULL;
    }
    for (i = 0; i < 3; ++i) {
        if (g_pAstParm[i] != NULL) {
            FzAstNode_Destroy(g_pAstParm[i]);
            g_pAstParm[i] = NULL;
        }
    }
	if (g_pVmCartZ != NULL) {
		EzMachine_Destroy(g_pVmCartZ);
		g_pVmCartZ = NULL;
	}
	for (i = 0; i < 3; ++i) {
        if (g_pVmParm[i] != NULL) {
            EzMachine_Destroy(g_pVmParm[i]);
            g_pVmParm[i] = NULL;
        }
    }

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
