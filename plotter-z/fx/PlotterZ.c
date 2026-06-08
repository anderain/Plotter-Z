#include <fxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bitmaps.h"
#include "keycode.h"
#include "../utils/hybird_6x8.h"
#include "../utils/samples.h"
#include "../../formula-z/fz.h"
#include "../../renderer-z/rz.h"
#include "../../evaluator-z/ez.h"

typedef unsigned int uint;
typedef unsigned char uchar;

#define ORTHOGRAPHIC    0
#define PERSPECTIVE     1

#define CURRENT_FONT_WIDTH  6
#define CURRENT_FONT_HEIGHT 8
#define VRAM_WIDTH          128
#define VRAM_HEIGHT         64

#define B_MENU_LEFT         2
#define B_MENU_ITEM_WIDTH   21
#define B_MENU_TOP          56
#define B_MENU_ITEM_NUM     6

#define EDIT_LEFT           0
#define EDIT_TOP            8
#define EDIT_W              128
#define EDIT_H              48
#define CHARS_PER_LINE      (EDIT_W / CURRENT_FONT_WIDTH)
#define VISIBLE_ROWS        (EDIT_H / CURRENT_FONT_HEIGHT)

#define VM_VAR_T_INDEX       3
#define VM_VAR_T_STEP       ((PZ_FLOAT)1.0f)

static const DISPBOX BoxMenuArea = { 0, 56, VRAM_WIDTH - 1, 63 };
static const DISPBOX BoxTopArea = { 0, 0, VRAM_WIDTH - 1, 7 };
static const DISPBOX BoxEditArea = { 0, 8, VRAM_WIDTH - 1, 55 };

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
 * Global Variables
 *====================================================*/

#define CART_EXPR_LENGTH 80
#define PARM_EXPR_LENGTH 80

char    g_szCartExpr[CART_EXPR_LENGTH] = "sin(sqr(x^2+y^2)+t/5)*cos(sqr(x^2+y^2)+t/5)";
char    g_szParmExpr[3][PARM_EXPR_LENGTH] = { "(5+sin(t/5))*sin(u)*cos(v)", "(5+sin(t/5))*sin(u)*sin(v)", "5/2*cos(u)+cos(t/5)" };

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

/*====================================================
 * Camera and surface data
 *====================================================*/
typedef struct { NUMERIC x, y, z; } Vertex;
typedef struct { int i0, i1; } Edge;

#define GRID_MAX            30

#define CAM_INIT_XMIN   -6.0f
#define CAM_INIT_XMAX   6.0f
#define CAM_INIT_XGRID  12
#define CAM_INIT_YMIN   -6.0f
#define CAM_INIT_YMAX   6.0f
#define CAM_INIT_YGRID  12
#define CAM_INIT_ZMIN   -3.0f
#define CAM_INIT_ZMAX   3.0f

#define X_GRID_MAX      30
#define Y_GRID_MAX      30
#define U_GRID_MAX      20
#define V_GRID_MAX      20
#define BUFFER_MAX(a,b) ((a) > (b) ? (a) : (b))
#define VERTEX_BUFFER_SIZE  BUFFER_MAX(X_GRID_MAX + Y_GRID_MAX + X_GRID_MAX * Y_GRID_MAX, U_GRID_MAX * V_GRID_MAX * 3)

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

int g_iFormulaX = 0;
int g_iFormulaY = 0;
int g_iFormulaOrigX = 0;
int g_iFormulaOrigY = 0;

#define STATE_IDLE  0
#define STATE_ERROR 1
#define STATE_READY 2
static int g_iMainState = STATE_IDLE;

/*====================================================
 * SysCall
 *====================================================*/

#define SCA 0xD201D002
#define SCB 0x422B0009
#define SCE 0x80010070

typedef void* (*pGetVRAMAddress)(void);
const static unsigned int sc0x135[] = { SCA, SCB, SCE, 0x135 };
#define GetVRAMAddress (*(pGetVRAMAddress)sc0x135)

typedef int (*pGetTicks)(void);
const static unsigned int sc003B[] = { SCA, SCB, SCE, 0x03B };
#define RTC_GetTicks (*(pGetTicks)sc003B)

unsigned char* pVRAM = 0;

/*====================================================
 * Graphics
 *====================================================*/

void DrawSprite8x8(int iX, int iY, const uchar* pSprite) {
    int iRowOffset;
    int iStartByte = iX >> 3;
    int iBitOffset = iX & 0x07;
    unsigned char ucSpriteByte;
    int iAddr;
    int iRow, iRowStart, iRowEnd;
    unsigned char ucPart1, ucPart2;

    /* Entirely off-screen */
    if (iX + 8 <= 0 || iX >= VRAM_WIDTH || iY + 8 <= 0 || iY >= VRAM_HEIGHT) {
        return;
    }
    
    /* Visible iRow range */
    iRowStart = iY < 0 ? -iY : 0;
    iRowEnd = iY + 8 >= VRAM_HEIGHT ? VRAM_HEIGHT - 1 - iY : 7;

    /* Left-clipped: only the right-shifted portion is visible */
    if (iX < 0) {
        for (iRow = iRowStart; iRow <= iRowEnd; ++iRow) {
            iRowOffset = (iY + iRow) << 4;
            ucSpriteByte = pSprite[iRow];
            iAddr = iRowOffset + iStartByte;
            ucPart2 = ucSpriteByte << (8 - iBitOffset);
            pVRAM[iAddr + 1] |= ucPart2;
        }
    }
    /* Right-clipped: only the left-shifted portion is visible */
    else if (iX + 8 >= VRAM_WIDTH) {
        for (iRow = iRowStart; iRow <= iRowEnd; ++iRow) {
            iRowOffset = (iY + iRow) << 4;
            ucSpriteByte = pSprite[iRow];
            iAddr = iRowOffset + iStartByte;
            ucPart1 = ucSpriteByte >> iBitOffset;
            pVRAM[iAddr] |= ucPart1;
        }
    }
    /* Full span: writes to 2 bytes per iRow */
    else {
        for (iRow = iRowStart; iRow <= iRowEnd; ++iRow) {
            iRowOffset = (iY + iRow) << 4;
            ucSpriteByte = pSprite[iRow];
            iAddr = iRowOffset + iStartByte;
            ucPart1 = ucSpriteByte >> iBitOffset;
            ucPart2 = ucSpriteByte << (8 - iBitOffset);
            pVRAM[iAddr] |= ucPart1;
            pVRAM[iAddr + 1] |= ucPart2;
        }
    }
}

void DrawBitmap(int iDestX, int iDestY, const uchar* pBitmap) {
    int iW = pBitmap[0];
    int iH = pBitmap[1];
    int iCol = (iW / 8) + (iW % 8 != 0);
    int iRow = (iH / 8) + (iH % 8 != 0);
    int iX, iY;

    pBitmap += 2;

    for (iY = 0; iY < iRow; iY++) {
        for (iX = 0; iX < iCol; iX++) {
            DrawSprite8x8(iDestX + (iX << 3), iDestY + (iY << 3), pBitmap);
            pBitmap += 8;
        }
    }
}

void PutChar(int iDestX, int iDestY, const uchar ucChar) {
    DrawSprite8x8(iDestX, iDestY, FONT_HYBIRD_6x8 + (((int)ucChar) << 3));
}

void PutText(int iDestX, int iDestY, const uchar* szText) {
    uchar* pText;
    for (pText = szText; *pText; ++pText) {
        DrawSprite8x8(iDestX, iDestY, FONT_HYBIRD_6x8 + (((int)*pText) << 3));
        iDestX += CURRENT_FONT_WIDTH;
    }
}
/*====================================================
 * Expr Stage
 *====================================================*/

static int g_iCursor  = 0;
static int g_iScroll  = 0;
static const char* g_szEditTopText = NULL;

/* Convert cursor position to screen (x,y) relative to edit area.
 * Returns true if cursor is on screen, false if scrolled out. */
static int CursorToXY(const char* szBuf, int iPos,
                      int* px, int* py, int iScroll) {
    int i, iCol, iRow;
    iCol = 0; iRow = 0;
    for (i = 0; i < iPos; ++i) {
        ++iCol;
        if (iCol >= CHARS_PER_LINE) {
            iCol = 0;
            ++iRow;
        }
    }
    *px = EDIT_LEFT + iCol * CURRENT_FONT_WIDTH;
    *py = EDIT_TOP  + (iRow - iScroll) * CURRENT_FONT_HEIGHT;
    if (*py < EDIT_TOP || *py >= EDIT_TOP + EDIT_H) return 0;
    return 1;
}

/* Draw cursor at (x,y) as inverted 6x8 block */
static void DrawCursor(int x, int y) {
    Bdisp_AreaReverseVRAM(x, y, x + CURRENT_FONT_WIDTH - 1,
                           y + CURRENT_FONT_HEIGHT - 1);
}

void DrawExprStage(const char* szBuf) {
    int i, iLen;
    int iCol, iRow;
    int iCursorX, iCursorY, iCursorVisible;

    Bdisp_AllClr_VRAM();

    iLen = (int)strlen(szBuf);
    iCol = 0; iRow = 0;

    for (i = 0; i <= iLen; ++i) {
        if (i == g_iCursor)
            CursorToXY(szBuf, g_iCursor, &iCursorX, &iCursorY, g_iScroll);
        if (i == iLen) break;

        if (iRow >= g_iScroll && iRow < g_iScroll + VISIBLE_ROWS) {
            int x = EDIT_LEFT + iCol * CURRENT_FONT_WIDTH;
            int y = EDIT_TOP  + (iRow - g_iScroll) * CURRENT_FONT_HEIGHT;
            PutChar(x, y, (uchar)szBuf[i]);
        }

        ++iCol;
        if (iCol >= CHARS_PER_LINE) {
            iCol = 0;
            ++iRow;
            if (iRow >= g_iScroll + VISIBLE_ROWS) break;
        }
    }

    /* Draw cursor */
    iCursorVisible = CursorToXY(szBuf, g_iCursor, &iCursorX, &iCursorY, g_iScroll);
    if (iCursorVisible)
        DrawCursor(iCursorX, iCursorY);

    /* Bottom Menu */
    {
        static const uchar* pMenuBitmap[B_MENU_ITEM_NUM] = { MENU_BACK, MENU_CLEAR, 0, 0, 0, MENU_OK };
        int i;
        Bdisp_AreaClr_VRAM(&BoxMenuArea);
        for (i = 0; i < B_MENU_ITEM_NUM; ++i) {
            if (pMenuBitmap[i]) {
                DrawBitmap(B_MENU_LEFT + B_MENU_ITEM_WIDTH * i, B_MENU_TOP, pMenuBitmap[i]);
            }
        }
    }
    /* Top Banner */
    if (g_szEditTopText) {
        const int iLeft = (VRAM_WIDTH - strlen(g_szEditTopText) * CURRENT_FONT_WIDTH) / 2;
        PutText(iLeft, 0, (const uchar *)g_szEditTopText);
    }
    Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, 7);
}

/* Get row where cursor currently is */
static int CursorRow(const char* szBuf, int iPos) {
    int i, iCol, iRow;
    iCol = 0; iRow = 0;
    for (i = 0; i < iPos; ++i) {
        ++iCol;
        if (iCol >= CHARS_PER_LINE) { iCol = 0; ++iRow; }
    }
    return iRow;
}

/* Move cursor to same column on target row. Column preserved. */
static int MoveToRow(const char* szBuf, int iCur, int iTargetRow) {
    int iCol, iRow, iLen;
    int iTargetCol;
    iLen = (int)strlen(szBuf);

    /* Find current column */
    iCol = 0; iRow = 0;
    {
        int i;
        for (i = 0; i < iCur; ++i) {
            ++iCol;
            if (iCol >= CHARS_PER_LINE) { iCol = 0; ++iRow; }
        }
    }
    iTargetCol = iCol;

    /* Scan to target row and column */
    iCol = 0; iRow = 0;
    {
        int i;
        for (i = 0; i <= iLen; ++i) {
            if (iRow == iTargetRow && iCol >= iTargetCol) return i;
            if (i == iLen) return iLen;
            ++iCol;
            if (iCol >= CHARS_PER_LINE) { iCol = 0; ++iRow; }
        }
    }
    return iLen;
}

int ExprStage(const int iEditMax, char* szOrigin, const char* szTitle) {
    uint uKey;
    char szBuf[300];
    int iLen;

    g_szEditTopText = szTitle;

    strcpy(szBuf, szOrigin);
    g_iCursor = (int)strlen(szBuf);
    g_iScroll = 0;

    while (1) {
        /* Keep cursor in view */
        {
            int iRow = CursorRow(szBuf, g_iCursor);
            if (iRow < g_iScroll) g_iScroll = iRow;
            if (iRow >= g_iScroll + VISIBLE_ROWS)
                g_iScroll = iRow - VISIBLE_ROWS + 1;
            if (g_iScroll < 0) g_iScroll = 0;
        }

        DrawExprStage(szBuf);
        GetKey(&uKey);
        iLen = (int)strlen(szBuf);

        switch (uKey) {
            case KEY_CTRL_F1:
            case KEY_CTRL_EXIT:
                return 0;

            case KEY_CTRL_F2:
                szBuf[0] = '\0';
                g_iCursor = 0;
                break;

            case KEY_CTRL_F6:
            case KEY_CTRL_EXE:
                strcpy(szOrigin, szBuf);
                return 1;

            case KEY_CTRL_DEL:
                if (iLen > 0 && g_iCursor < iLen) {
                    memmove(szBuf + g_iCursor, szBuf + g_iCursor + 1,
                            (size_t)(iLen - g_iCursor));
                }
                break;

            case KEY_CTRL_LEFT:
                if (g_iCursor > 0) --g_iCursor;
                break;

            case KEY_CTRL_RIGHT:
                if (g_iCursor < iLen) ++g_iCursor;
                break;

            case KEY_CTRL_UP:
            {
                int iRow = CursorRow(szBuf, g_iCursor);
                if (iRow > 0)
                    g_iCursor = MoveToRow(szBuf, g_iCursor, iRow - 1);
                break;
            }

            case KEY_CTRL_DOWN:
            {
                int iRow = CursorRow(szBuf, g_iCursor);
                g_iCursor = MoveToRow(szBuf, g_iCursor, iRow + 1);
                break;
            }

            case KEY_CHAR_SIN:
            case KEY_CHAR_COS:
            case KEY_CHAR_TAN:
            case KEY_CHAR_LN:
            case KEY_CHAR_SQUARE:
            {
                const char* szIns;
                int iInsLen;
                if (iLen + 4 >= (int)(sizeof(szBuf) - 1)) break;
                if (uKey == KEY_CHAR_SIN)       szIns = "sin(";
                else if (uKey == KEY_CHAR_COS)  szIns = "cos(";
                else if (uKey == KEY_CHAR_TAN)  szIns = "tan(";
                else if (uKey == KEY_CHAR_LN)   szIns = "ln(";
                else                            szIns = "sqr(";
                iInsLen = (int)strlen(szIns);
                memmove(szBuf + g_iCursor + iInsLen, szBuf + g_iCursor,
                        (size_t)(iLen - g_iCursor + 1));
                memcpy(szBuf + g_iCursor, szIns, (size_t)iInsLen);
                g_iCursor += iInsLen;
                break;
            }

            default:
                if (iLen < iEditMax) {
                    char ch = 0;

                    if (uKey >= KEY_CHAR_0  && uKey <= KEY_CHAR_9)  ch = (char)uKey;
                    else if (uKey >= KEY_CHAR_A && uKey <= KEY_CHAR_Z) ch = (char)uKey + ('a' - 'A');
                    else if (uKey == KEY_CHAR_DP)      ch = '.';
                    else if (uKey == KEY_CHAR_MINUS)   ch = '-';
                    else if (uKey == KEY_CHAR_PLUS)    ch = '+';
                    else if (uKey == KEY_CHAR_MULT)    ch = '*';
                    else if (uKey == KEY_CHAR_DIV)     ch = '/';
                    else if (uKey == KEY_CHAR_LPAR)    ch = '(';
                    else if (uKey == KEY_CHAR_RPAR)    ch = ')';
                    else if (uKey == KEY_CHAR_COMMA)   ch = ',';
                    else if (uKey == KEY_CHAR_POW)     ch = '^';
                    else if (uKey == KEY_CHAR_EQUAL)   ch = '=';
                    else if (uKey == KEY_CHAR_SPACE)   ch = ' ';

                    if (ch) {
                        memmove(szBuf + g_iCursor + 1, szBuf + g_iCursor,
                                (size_t)(iLen - g_iCursor + 1));
                        szBuf[g_iCursor] = ch;
                        ++g_iCursor;
                    }
                }
                break;
        }
    }
}

/*====================================================
 * Renderer-Z VRAM callbacks
 *====================================================*/
static void RzSetPixel(int x, int y) {
    int iAddr;
    if (x < 0 || x >= VRAM_WIDTH || y < 0 || y >= VRAM_HEIGHT) return;
    iAddr = (y << 4) + (x >> 3);
    pVRAM[iAddr] |= (unsigned char)(0x80 >> (x & 7));
}

static void RzPutChar(int x, int y, unsigned char ch) {
    DrawSprite8x8(x, y, FONT_HYBIRD_6x8 + ((int)ch << 3));
}

/*====================================================
 * Parse and render formula to VRAM
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
            sprintf(g_szErrMsgLine1, "Failed to parse \"%s=\":", szParmLabel[iParamIndex]);
        } else {
            strcpy(g_szErrMsgLine1, "Failed to parse:");
        }
        strcpy(g_szErrMsgLine2, "Syntax error.");
    }
    else {
        switch (iCompileError) {
            case EZERR_VARIABLE_UNDEFINED:
                strcpy(g_szErrMsgLine1, "Undefined variable:");
                sprintf(g_szErrMsgLine2, "\"%s\"", szErrorBuf);
                break;
            case EZERR_FUNCTION_UNDEFINED:
                strcpy(g_szErrMsgLine1, "Undefined function:");
                sprintf(g_szErrMsgLine2, "\"%s\"", szErrorBuf);
                break;
            case EZERR_FUNCTION_PARAM_MISMATCH:
                strcpy(g_szErrMsgLine1, "Function parameter mismatch");
                sprintf(g_szErrMsgLine2, "\"%s\"", szErrorBuf);
                break;
        }
    }
}

int ParseAndRenderExpr(void) {
    int iW = VRAM_WIDTH, iH = VRAM_HEIGHT - 16, i;
    char szErrorBuf[EZ_ERROR_CONTENT_LENGTH];
    int iCompileError;

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
    g_iFormulaY = (VRAM_HEIGHT - g_pRenderNode->sLayout.iAscent - g_pRenderNode->sLayout.iDescent) / 2 + g_pRenderNode->sLayout.iAscent;

    g_iFormulaOrigX = g_iFormulaX;
    g_iFormulaOrigY = g_iFormulaY;
    
    return 1;
}

/* Draw formula centered in edit area */
void DrawFormula(void) {
    int iX;
    if (g_pRenderNode == NULL) {
        return;
    }

    /* Formula */
    RenderNode_Draw(g_pRenderNode, &g_rzConfig, g_iFormulaX, g_iFormulaY);

    /* f(x,y)= banner */
    Bdisp_AreaClr_VRAM(&BoxTopArea);
    iX = (VRAM_WIDTH - CURRENT_FONT_WIDTH * strlen(szFuncTypeLabel[g_iFuncType])) / 2;
    PutText(iX, 0, (const uchar*)szFuncTypeLabel[g_iFuncType]);
    Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, CURRENT_FONT_HEIGHT - 1);
}

/*====================================================
 * 3D Projection & Recalc
 *====================================================*/
#define BAR_WIDTH   10
#define BAR_HEIGHT  1
#define BAR_BORDER  1

static const DISPBOX TopRightProgressBox = {
    VRAM_WIDTH - BAR_WIDTH - BAR_BORDER * 2,
    0,
    VRAM_WIDTH - 1,
    BAR_BORDER * 2
};

static void RecalcInitTopRight(void) {
    static const DISPBOX* pBox = &TopRightProgressBox;
    Bdisp_AreaClr_VRAM(pBox);
    /* Border */
    Bdisp_DrawLineVRAM(pBox->left, pBox->top, pBox->right, pBox->top);
    Bdisp_DrawLineVRAM(pBox->right, pBox->top, pBox->right, pBox->bottom);
    Bdisp_DrawLineVRAM(pBox->right, pBox->bottom, pBox->left, pBox->bottom);
    Bdisp_DrawLineVRAM(pBox->left, pBox->bottom, pBox->left, pBox->top);
}

static void RecalcRedrawTopRight(int iPct) {
    static const DISPBOX* pBox = &TopRightProgressBox;
    /* Progress */
    Bdisp_DrawLineVRAM(
        pBox->left + BAR_BORDER,
        pBox->top + BAR_BORDER,
        pBox->left + BAR_BORDER + BAR_WIDTH * iPct / 100,
        pBox->top + BAR_BORDER
    );
    Bdisp_PutDisp_DD();
}

#undef BAR_WIDTH
#undef BAR_HEIGHT
#undef BAR_BORDER

#define BAR_W (VRAM_WIDTH * 2 / 3)
#define BAR_H (CURRENT_FONT_HEIGHT * 3 / 2)
#define BAR_X ((VRAM_WIDTH - BAR_W) / 2)
#define BAR_Y ((VRAM_HEIGHT - BAR_H) / 2)

static const DISPBOX CenterProgressBox = {
    BAR_X + 1,
    BAR_Y + 1,
    BAR_X + BAR_W - 2,
    BAR_Y + BAR_H - 2
};

static void RecalcRedrawFullscreen(int iPct) {
    char szBuf[16];
    int iLen, iFillW;

    /* Percentage text */
    sprintf(szBuf, "%d%%", iPct);
    iLen = (int)strlen(szBuf) * CURRENT_FONT_WIDTH;
    PutText(BAR_X + (BAR_W - iLen) / 2,
        BAR_Y + (BAR_H - CURRENT_FONT_HEIGHT) / 2,
        (const uchar*)szBuf);

    /* Clear bar content */
    Bdisp_AreaClr_VRAM(&CenterProgressBox);

    /* Filled portion invert */
    iFillW = BAR_W * iPct / 100;
    if (iFillW > BAR_W) iFillW = BAR_W;
    if (iFillW > 0)
        Bdisp_AreaReverseVRAM(BAR_X + 1, BAR_Y + 1,
            BAR_X + iFillW - 1, BAR_Y + BAR_H - 2);

    Bdisp_PutDisp_DD();
}

static void RecalcInitFullscreen(void) {
    int iLen;

    Bdisp_AllClr_VRAM();

    /* "Recalc ..." label above bar */
    iLen = (int)strlen("Recalc ...") * CURRENT_FONT_WIDTH;
    PutText((VRAM_WIDTH - iLen) / 2,
        BAR_Y - CURRENT_FONT_HEIGHT - 2,
        (const uchar*)"Recalc ...");

    /* Bar border */
    Bdisp_DrawLineVRAM(BAR_X, BAR_Y, BAR_X + BAR_W - 1, BAR_Y);
    Bdisp_DrawLineVRAM(BAR_X, BAR_Y + BAR_H - 1,
        BAR_X + BAR_W - 1, BAR_Y + BAR_H - 1);
    Bdisp_DrawLineVRAM(BAR_X, BAR_Y, BAR_X, BAR_Y + BAR_H - 1);
    Bdisp_DrawLineVRAM(BAR_X + BAR_W - 1, BAR_Y,
        BAR_X + BAR_W - 1, BAR_Y + BAR_H - 1);
}

#undef BAR_W
#undef BAR_H
#undef BAR_X
#undef BAR_Y

static void RecalcCartesian(void (*pfnRedraw)(int iPct)) {
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
        if (pfnRedraw) pfnRedraw((iX + 1) * 100 / Camera.xGrid);
    }

    /* Normalize x and y grid coordinates into [-1, 1] */
    for (iX = 0; iX < Camera.xGrid; ++iX) {
        g_fCartXBuf[iX] = NUM_VAL(2.0f * (fX[iX] - (Camera.xMax + Camera.xMin) / 2.0f) / (Camera.xMax - Camera.xMin));
    }
    for (iY = 0; iY < Camera.yGrid; ++iY) {
        g_fCartYBuf[iY] = NUM_VAL(2.0f * (fY[iY] - (Camera.yMax + Camera.yMin) / 2.0f) / (Camera.yMax - Camera.yMin));
    }
}

void RecalcParametric(void (*pfnRedraw)(int iPct)) {
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
        if (pfnRedraw) pfnRedraw((iU + 1) * 100 / Camera.uGrid);
    }
}

static int RecalcSurface(
    void (*pfnDrawFirstFrame)(void),
    void (*pfnRedraw)(int iPct)
) {
    if (pfnDrawFirstFrame) pfnDrawFirstFrame();
    if (pfnRedraw) pfnRedraw(0);
    
    switch (g_iFuncType) {
        case FUNC_TYPE_CARTESIAN:
            RecalcCartesian(pfnRedraw);
            break;
        case FUNC_TYPE_PARAMETRIC:
            RecalcParametric(pfnRedraw);
            break;
    }
    return 1;
}

/*====================================================
 * Draw wireframe to VRAM
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
                Bdisp_DrawLineVRAM(x0, y0, x1, y1);
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
                Bdisp_DrawLineVRAM(x0, y0, x1, y1);
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
            Bdisp_DrawLineVRAM(x0, y0, x1, y1);
            x0 = x1, y0 = y1;
        }
    }
    for (iU = 0; iU < Camera.uGrid; ++iU) {
        iV = 0;
        xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x0, &y0); 
        for (iV = 0; iV < Camera.vGrid; ++iV) {
            xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x1, &y1); 
            Bdisp_DrawLineVRAM(x0, y0, x1, y1);
            x0 = x1, y0 = y1;
        }
    }
}

/*====================================================
 * Graph Stage
 *====================================================*/

#define KEY_REPEAT_INTERVAL_TICKS 64 /* = 500ms */

int g_bGraphMenuVisible = 1;
int g_bDrawBox = 0;
int g_bDrawAxes = 0;
int g_bPlaying = 0;
int g_iPlayKeyTicks = 0;

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
        Bdisp_DrawLineVRAM(x0, y0, x1, y1);
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
        Bdisp_DrawLineVRAM(x0, y0, x1, y1);
    }
}

void DrawGraphStage(void) {
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

    Bdisp_AllClr_VRAM();
    RefreshCameraTrigBuf();
    /* Box */
    if (g_bDrawBox) {
        DrawBoxEdges(xyz2xy);
    }
    /* Aexs */
    if (g_bDrawAxes) {
        DrawAxes(xyz2xy);
    }
    /* Surface */
    switch (g_iFuncType) {
        case FUNC_TYPE_CARTESIAN:
            DrawCartSurfaceWireframe(xyz2xy);
            break;
        case FUNC_TYPE_PARAMETRIC:
            DrawParmSurfaceWireframe(xyz2xy);
            break;
    }
    /* Busy Indicator*/
    if (g_bPlaying) {
        DrawBitmap(VRAM_WIDTH - 8, 0, SPRITE_BUSY);
    }
    /* Bottom Menu */
    if (g_bGraphMenuVisible) {
        const uchar* pMenuBitmap[B_MENU_ITEM_NUM] = { MENU_RESET, MENU_PLAY, MENU_O_P_SWITCH, MENU_AXES, MENU_BOX, MENU_HIDE };
        int i;
        Bdisp_AreaClr_VRAM(&BoxMenuArea);
        if (g_bPlaying) {
            pMenuBitmap[0] = MENU_STOP;
            pMenuBitmap[1] = 0;
        }
        for (i = 0; i < B_MENU_ITEM_NUM; ++i) {
            if (pMenuBitmap[i]) {
                DrawBitmap(B_MENU_LEFT + B_MENU_ITEM_WIDTH * i, B_MENU_TOP, pMenuBitmap[i]);
            }
        }
    }
}

void GraphAnimationUpdateFrame(
    void (*pfnDrawFirstFrame)(void),
    void (*pfnRedraw)(int iPct)
) {
    switch (g_iFuncType) {
        case FUNC_TYPE_CARTESIAN:
            EzMachine_SetVariableByIndex(
                g_pVmCartZ,
                VM_VAR_T_INDEX,
                VM_VAR_T_STEP + EzMachine_GetVariableByIndex(g_pVmCartZ, VM_VAR_T_INDEX)
            );
            break;
        case FUNC_TYPE_PARAMETRIC: {
            int i;
            for (i = 0; i < 3; ++i) {
                EzMachine_SetVariableByIndex(
                    g_pVmParm[i],
                    VM_VAR_T_INDEX,
                    VM_VAR_T_STEP + EzMachine_GetVariableByIndex(g_pVmParm[i], VM_VAR_T_INDEX)
                );
            }
        }
    }
    RecalcSurface(pfnDrawFirstFrame, pfnRedraw);
}

void GraphStageHandleKey(uint uKey) {
    switch (uKey) {
        case KEY_CTRL_F1:
        case KEY_CTRL_OPTN:
            PzCamera_Reset(VRAM_WIDTH / 2, VRAM_HEIGHT / 2);
            break;

        case KEY_CTRL_F2:
            g_bPlaying = 1;
            g_iPlayKeyTicks = RTC_GetTicks();
            break;

        case KEY_CTRL_F3:
            g_iProjection = !g_iProjection;
            break;

        case KEY_CTRL_F4:
            g_bDrawAxes = !g_bDrawAxes;
            break;

        case KEY_CTRL_F5:
            g_bDrawBox = !g_bDrawBox;
            break;

        case KEY_CTRL_F6:
            g_bGraphMenuVisible = !g_bGraphMenuVisible;
            break;

        case KEY_CHAR_PLUS:
            if (Camera.iZoomLevel < iNumZoomLevel - 1)
                Camera.iZoomLevel++;
            break;

        case KEY_CHAR_MINUS:
            if (Camera.iZoomLevel > 0)
                Camera.iZoomLevel--;
            break;

        case KEY_CHAR_MULT:
            if (Camera.iFovLevel > FOV_LEVEL_MIN)
                Camera.iFovLevel--;
            break;

        case KEY_CHAR_DIV:
            if (Camera.iFovLevel < FOV_LEVEL_MAX)
                Camera.iFovLevel++;
            break;

        case KEY_CHAR_2:
            Camera.iViewportY += 10;
            break;

        case KEY_CHAR_8:
            Camera.iViewportY -= 10;
            break;

        case KEY_CHAR_4:
            Camera.iViewportX -= 10;
            break;

        case KEY_CHAR_6:
            Camera.iViewportX += 10;
            break;

        case KEY_CTRL_UP:
            Camera.iAlphaDeg -= 5;
            break;

        case KEY_CTRL_DOWN:
            Camera.iAlphaDeg += 5;
            break;

        case KEY_CTRL_LEFT:
            Camera.iBetaDeg -= 5;
            break;

        case KEY_CTRL_RIGHT:
            Camera.iBetaDeg += 5;
            break;

        case KEY_CTRL_FD: {
            if (!g_bPlaying) {
                GraphAnimationUpdateFrame(RecalcInitTopRight, RecalcRedrawTopRight);
            }
            break;
        }
        default:
            return;
    }
    /* Normalize angles */
    Camera.iBetaDeg  = Camera.iBetaDeg % 360;
    if (Camera.iBetaDeg < 0) Camera.iBetaDeg += 360;
    Camera.iAlphaDeg = Camera.iAlphaDeg % 360;
    if (Camera.iAlphaDeg < 0) Camera.iAlphaDeg += 360;
}

int GraphStage(void) {
    uint uKey;
    g_bPlaying = 0;
    while (1) {
        DrawGraphStage();
        if (g_bPlaying) {
            int iCode1, iCode2, iUnused;
            /* EXIT or AC pressed */
            if (Bkey_GetKeyWait(&iCode1, &iCode2, KEYWAIT_HALTOFF_TIMEROFF, 0, 1, &iUnused) == KEYREP_KEYEVENT) {
                int nKey = KEYCODE_COMBINE(iCode1, iCode2);
                /* Stop */
                if (nKey == NKEY_AC || nKey == NKEY_EXIT) {
                    g_bPlaying = 0;
                    continue;
                }
                /* Check key repeat */
                if (RTC_GetTicks() - g_iPlayKeyTicks < KEY_REPEAT_INTERVAL_TICKS) {
                    continue;
                }
                g_iPlayKeyTicks = RTC_GetTicks();
                switch(nKey) {
                    case NKEY_F1: GraphStageHandleKey(KEY_CTRL_F1); break;
                    case NKEY_F2: GraphStageHandleKey(KEY_CTRL_F2); break;
                    case NKEY_F3: GraphStageHandleKey(KEY_CTRL_F3); break;
                    case NKEY_F4: GraphStageHandleKey(KEY_CTRL_F4); break;
                    case NKEY_F5: GraphStageHandleKey(KEY_CTRL_F5); break;
                    case NKEY_F6: GraphStageHandleKey(KEY_CTRL_F6); break;
                    case NKEY_MUL: GraphStageHandleKey(KEY_CHAR_MULT); break;
                    case NKEY_DIV: GraphStageHandleKey(KEY_CHAR_DIV); break;
                    case NKEY_PLUS: GraphStageHandleKey(KEY_CHAR_PLUS); break;
                    case NKEY_MINUS: GraphStageHandleKey(KEY_CHAR_MINUS); break;
                    case NKEY_2: GraphStageHandleKey(KEY_CHAR_2); break;
                    case NKEY_8: GraphStageHandleKey(KEY_CHAR_8); break;
                    case NKEY_4: GraphStageHandleKey(KEY_CHAR_4); break;
                    case NKEY_6: GraphStageHandleKey(KEY_CHAR_6); break;
                    case NKEY_UP: GraphStageHandleKey(KEY_CTRL_UP); break;
                    case NKEY_DOWN: GraphStageHandleKey(KEY_CTRL_DOWN); break;
                    case NKEY_LEFT: GraphStageHandleKey(KEY_CTRL_LEFT); break;
                    case NKEY_RIGHT: GraphStageHandleKey(KEY_CTRL_RIGHT); break;
                    case NKEY_FD: GraphStageHandleKey(KEY_CTRL_FD); break;
                    case NKEY_OPTN: GraphStageHandleKey(KEY_CTRL_OPTN); break;
                    default: break;
                }
            }
            Bdisp_PutDisp_DD();
            GraphAnimationUpdateFrame(NULL, NULL);
            continue;
        }
        GetKey(&uKey);
        switch (uKey) {
            case KEY_CTRL_EXIT:
                return 0;
            default:
                GraphStageHandleKey(uKey);
                break;
        }
    }
}

/*====================================================
 * Value Editor Stage
 *====================================================*/

#define VAL_BUF_SIZE 20
static char g_szValueEditorBuf[VAL_BUF_SIZE];

void DrawValueEditor(const char* szTitle, int iLen) {
    int iX, iY;
    Bdisp_AllClr_VRAM();

    /* Title banner */
    {
        int iLeft = (VRAM_WIDTH - (int)strlen(szTitle) * CURRENT_FONT_WIDTH) / 2;
        PutText(iLeft, 0, (const uchar*)szTitle);
        Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, 7);
    }

    /* Edit buffer centered with cursor underscore */
    {
        char szDisplay[VAL_BUF_SIZE + 2];
        strcpy(szDisplay, g_szValueEditorBuf);
        szDisplay[iLen] = '_';
        szDisplay[iLen + 1] = '\0';
        iX = (VRAM_WIDTH - (iLen + 1) * CURRENT_FONT_WIDTH) / 2;
        iY = (VRAM_HEIGHT - 16 - CURRENT_FONT_HEIGHT) / 2 + 8;
        if (iX < 0) iX = 0;
        PutText(iX, iY, (const uchar*)szDisplay);
    }

    /* Bottom Menu */
    {
        static const uchar* pMenuBitmap[B_MENU_ITEM_NUM] = { MENU_BACK, 0, 0, 0, 0, MENU_OK };
        int bMenuItemVisible[B_MENU_ITEM_NUM];
        int i;
        Bdisp_AreaClr_VRAM(&BoxMenuArea);
        memset(bMenuItemVisible, 1, sizeof(bMenuItemVisible));
        for (i = 0; i < B_MENU_ITEM_NUM; ++i) {
            if (pMenuBitmap[i] && bMenuItemVisible[i]) {
                DrawBitmap(B_MENU_LEFT + B_MENU_ITEM_WIDTH * i, B_MENU_TOP, pMenuBitmap[i]);
            }
        }
    }
}

int ValueEditorStage(const char* szTitle, const char* szInitial) {
    uint uKey;
    int iLen;

    strcpy(g_szValueEditorBuf, szInitial);

    while (1) {
        iLen = (int)strlen(g_szValueEditorBuf);
        DrawValueEditor(szTitle, iLen);
        GetKey(&uKey);

        switch (uKey) {
            case KEY_CTRL_F1:
            case KEY_CTRL_EXIT:
                strcpy(g_szValueEditorBuf, szInitial);
                /* fall through */
            case KEY_CTRL_F6:
            case KEY_CTRL_EXE:
                return (int)strlen(g_szValueEditorBuf);

            case KEY_CTRL_AC:
                g_szValueEditorBuf[0] = '\0';
                break;
    
            case KEY_CTRL_DEL:
                if (iLen > 0)
                    g_szValueEditorBuf[iLen - 1] = '\0';
                break;

            default:
                if (iLen < VAL_BUF_SIZE) {
                    char ch = 0;
                    if (uKey >= KEY_CHAR_0 && uKey <= KEY_CHAR_9) ch = (char)uKey;
                    else if (uKey == KEY_CHAR_DP)      ch = '.';
                    else if (uKey == KEY_CHAR_PMINUS)  ch = '-';
                    else if (uKey == KEY_CHAR_MINUS)   ch = '-';

                    if (ch) {
                        g_szValueEditorBuf[iLen] = ch;
                        g_szValueEditorBuf[iLen + 1] = '\0';
                    }
                }
                break;
        }
    }
}

/*====================================================
 * Window Editor Stage
 *====================================================*/

#define WIN_EDIT_ROWS           14
#define WE_VISIBLE_ROWS          6

/* Cursor */
static int g_iWinEditCursor = 0;
static int g_iWinEditScroll = 0;

/* Row labels */
static const char* g_szWinEditLabels[WIN_EDIT_ROWS] = {
    "X Min", "X Max", "X Grid",
    "Y Min", "Y Max", "Y Grid",
    "Z Min", "Z Max",
    "U Min", "U Max", "U Grid",
    "V Min", "V Max", "V Grid"
};

/* String buffers for each field */
static char g_szWinEditValues[WIN_EDIT_ROWS][20];

void DrawWindowEditorStage(void) {
    int iRow;
    Bdisp_AllClr_VRAM();

    /* Top Banner */
    {
        static const char szTopText[] = "Window Editor";
        static const int iLeft = (VRAM_WIDTH - (sizeof(szTopText) - 1) * CURRENT_FONT_WIDTH) / 2;
        PutText(iLeft, 0, (const uchar *)szTopText);
        Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, 7);
    }

    /* Field list (6 visible rows) */
    for (iRow = g_iWinEditScroll; iRow < g_iWinEditScroll + WE_VISIBLE_ROWS; ++iRow) {
        if (iRow >= WIN_EDIT_ROWS) break;
        {
            int iY = 8 + (iRow - g_iWinEditScroll) * CURRENT_FONT_HEIGHT;
            char szLine[32];
            sprintf(szLine, "%-7s= %s",
                g_szWinEditLabels[iRow], g_szWinEditValues[iRow]);
            PutText(4, iY, (const uchar*)szLine);

            /* Highlight cursor */
            if (iRow == g_iWinEditCursor) {
                Bdisp_AreaReverseVRAM(0, iY,
                    VRAM_WIDTH - 1,
                    iY + CURRENT_FONT_HEIGHT - 1);
            }
        }
    }

    /* Bottom Menu */
    {
        static const uchar* pMenuBitmap[B_MENU_ITEM_NUM] = { MENU_BACK, 0, 0, MENU_EDIT, MENU_INIT, MENU_OK };
        int bMenuItemVisible[B_MENU_ITEM_NUM];
        int i;
        Bdisp_AreaClr_VRAM(&BoxMenuArea);
        memset(bMenuItemVisible, 1, sizeof(bMenuItemVisible));
        for (i = 0; i < B_MENU_ITEM_NUM; ++i) {
            if (pMenuBitmap[i] && bMenuItemVisible[i]) {
                DrawBitmap(B_MENU_LEFT + B_MENU_ITEM_WIDTH * i, B_MENU_TOP, pMenuBitmap[i]);
            }
        }
    }
}

static void WinEdit_SetField(int iRow, PZ_FLOAT f) {
    Utils_Ftoa((double)f, g_szWinEditValues[iRow], 2);
}

static void WinEdit_SetFieldInt(int iRow, int n) {
    char szBuf[16];
    sprintf(szBuf, "%d", n);
    strcpy(g_szWinEditValues[iRow], szBuf);
}

static void WinEdit_LoadCamera(void) {
    WinEdit_SetField(0,  Camera.xMin);
    WinEdit_SetField(1,  Camera.xMax);
    WinEdit_SetFieldInt(2, Camera.xGrid);
    WinEdit_SetField(3,  Camera.yMin);
    WinEdit_SetField(4,  Camera.yMax);
    WinEdit_SetFieldInt(5, Camera.yGrid);
    WinEdit_SetField(6,  Camera.zMin);
    WinEdit_SetField(7,  Camera.zMax);
    WinEdit_SetField(8,  Camera.uMin);
    WinEdit_SetField(9,  Camera.uMax);
    WinEdit_SetFieldInt(10, Camera.uGrid);
    WinEdit_SetField(11, Camera.vMin);
    WinEdit_SetField(12, Camera.vMax);
    WinEdit_SetFieldInt(13, Camera.vGrid);
}

static void WinEdit_SaveCamera(void) {
    int iGrid;

    Camera.xMin  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[0]);
    Camera.xMax  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[1]);
    iGrid = (int)Utils_Atoi(g_szWinEditValues[2]);
    if (iGrid < 5) iGrid = 5; if (iGrid > GRID_MAX) iGrid = GRID_MAX;
    Camera.xGrid = iGrid;

    Camera.yMin  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[3]);
    Camera.yMax  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[4]);
    iGrid = (int)Utils_Atoi(g_szWinEditValues[5]);
    if (iGrid < 5) iGrid = 5; if (iGrid > GRID_MAX) iGrid = GRID_MAX;
    Camera.yGrid = iGrid;

    Camera.zMin  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[6]);
    Camera.zMax  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[7]);

    Camera.uMin  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[8]);
    Camera.uMax  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[9]);
    iGrid = (int)Utils_Atoi(g_szWinEditValues[10]);
    if (iGrid < 5) iGrid = 5; if (iGrid > GRID_MAX) iGrid = GRID_MAX;
    Camera.uGrid = iGrid;

    Camera.vMin  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[11]);
    Camera.vMax  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[12]);
    iGrid = (int)Utils_Atoi(g_szWinEditValues[13]);
    if (iGrid < 5) iGrid = 5; if (iGrid > GRID_MAX) iGrid = GRID_MAX;
    Camera.vGrid = iGrid;
}

void WinEdit_ApplyDefault() {
    Camera.xMin  = CAM_INIT_XMIN;
    Camera.xMax  = CAM_INIT_XMAX;
    Camera.xGrid = CAM_INIT_XGRID;
    Camera.yMin  = CAM_INIT_YMIN;
    Camera.yMax  = CAM_INIT_YMAX;
    Camera.yGrid = CAM_INIT_YGRID;
    Camera.zMin  = CAM_INIT_ZMIN;
    Camera.zMax  = CAM_INIT_ZMAX;
    Camera.uMin  = -1.0f;
    Camera.uMax  =  1.0f;
    Camera.uGrid = 12;
    Camera.vMin  = -1.0f;
    Camera.vMax  =  1.0f;
    Camera.vGrid = 12;
}

void WindowEditorStage(void) {
    uint uKey;

    WinEdit_LoadCamera();
    g_iWinEditCursor = 0;
    g_iWinEditScroll = 0;

    while (1) {
        /* Keep cursor in view */
        if (g_iWinEditCursor < g_iWinEditScroll)
            g_iWinEditScroll = g_iWinEditCursor;
        if (g_iWinEditCursor >= g_iWinEditScroll + WE_VISIBLE_ROWS)
            g_iWinEditScroll = g_iWinEditCursor - WE_VISIBLE_ROWS + 1;

        DrawWindowEditorStage();
        GetKey(&uKey);

        switch (uKey) {
            case KEY_CTRL_EXIT:
            case KEY_CTRL_F1:
                return;

            case KEY_CTRL_EXE:
            case KEY_CTRL_F4:
                ValueEditorStage(g_szWinEditLabels[g_iWinEditCursor],
                    g_szWinEditValues[g_iWinEditCursor]);
                strcpy(g_szWinEditValues[g_iWinEditCursor],
                    g_szValueEditorBuf);
                break;

            case KEY_CTRL_F5:
                WinEdit_ApplyDefault();
                WinEdit_LoadCamera();
                break;

            case KEY_CTRL_F6:
                WinEdit_SaveCamera();
                return;

            case KEY_CTRL_UP:
                if (g_iWinEditCursor > 0)
                    g_iWinEditCursor--;
                break;

            case KEY_CTRL_DOWN:
                if (g_iWinEditCursor < WIN_EDIT_ROWS - 1)
                    g_iWinEditCursor++;
                break;

        }
    }
}

/*====================================================
 * Samples Stage
 *====================================================*/

static int g_iSampleIndex = 0;
static int g_iSampleScroll = 0;
static const int g_iNumSamples = sizeof(PlotterZSamples) / sizeof(PlotterZSamples[0]);
#define SAMPLE_VISIBLE_ROWS 6

void DrawSamplesStage(void) {
    int iRow;
    Bdisp_AllClr_VRAM();

    /* Top banner */
    {
        const char szTitle[] = "Samples";
        int iLeft = (VRAM_WIDTH - ((int)sizeof(szTitle) - 1) * CURRENT_FONT_WIDTH) / 2;
        PutText(iLeft, 0, (const uchar*)szTitle);
        Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, 7);
    }

    /* Sample list (6 visible rows) */
    for (iRow = g_iSampleScroll; iRow < g_iSampleScroll + SAMPLE_VISIBLE_ROWS; ++iRow) {
        if (iRow >= g_iNumSamples) break;
        {
            const char* szName = PlotterZSamples[iRow].szName;
            int iY = 8 + (iRow - g_iSampleScroll) * CURRENT_FONT_HEIGHT;
            char szNum[4];
            int iMaxChars = CHARS_PER_LINE - 3;

            sprintf(szNum, "%02d:", iRow + 1);
            PutText(2, iY, (const uchar*)szNum);

            {
                char szLine[22];
                int i;
                for (i = 0; i < iMaxChars && szName[i] != '\0'; ++i)
                    szLine[i] = szName[i];
                szLine[i] = '\0';
                PutText(22, iY, (const uchar*)szLine);
            }

            /* Full row reverse highlight */
            if (iRow == g_iSampleIndex) {
                Bdisp_AreaReverseVRAM(0, iY,
                    VRAM_WIDTH - 1,
                    iY + CURRENT_FONT_HEIGHT - 1);
            }
        }
    }

    /* Bottom Menu */
    {
        static const uchar* pMenuBitmap[B_MENU_ITEM_NUM] = { MENU_BACK, 0, 0, 0, 0, MENU_OK };
        int i;
        Bdisp_AreaClr_VRAM(&BoxMenuArea);
        for (i = 0; i < B_MENU_ITEM_NUM; ++i) {
            if (pMenuBitmap[i]) {
                DrawBitmap(B_MENU_LEFT + B_MENU_ITEM_WIDTH * i, B_MENU_TOP, pMenuBitmap[i]);
            }
        }
    }
}

int SamplesStage(void) {
    uint uKey;
    g_iSampleIndex = 0;
    g_iSampleScroll = 0;

    while (1) {
        /* Keep selected item in view */
        if (g_iSampleIndex < g_iSampleScroll)
            g_iSampleScroll = g_iSampleIndex;
        if (g_iSampleIndex >= g_iSampleScroll + SAMPLE_VISIBLE_ROWS)
            g_iSampleScroll = g_iSampleIndex - SAMPLE_VISIBLE_ROWS + 1;

        DrawSamplesStage();
        GetKey(&uKey);
        switch (uKey) {
            case KEY_CTRL_F1:
            case KEY_CTRL_EXIT:
                return 0;

            case KEY_CTRL_F6:
            case KEY_CTRL_EXE:
                return 1;

            case KEY_CTRL_UP:
                if (g_iSampleIndex > 0)
                    g_iSampleIndex--;
                else
                    g_iSampleIndex = g_iNumSamples - 1;
                break;

            case KEY_CTRL_DOWN:
                if (g_iSampleIndex < g_iNumSamples - 1)
                    g_iSampleIndex++;
                else
                    g_iSampleIndex = 0;
                break;

            default:
                break;
        }
    }
}

/*====================================================
 * Help Stage
 *====================================================*/
#define HELP_PAGE_SIZE 3
int g_iHelpPage;

static const char* szHelpPage0[] = {
    "Arrow: Rotate Camera",
    "Num 2/4/6/8: Pan Move",
    "+/-: Zoom",
    "\x04/\x05: Field of view",
    "F-D: Next Frame",
    "AC: Stop Animation"
};

static const char* szHelpPage1[] = {
    "F1: Reset camera",
    "F2: Play animation",
    "F3: Ortho \x17\x18 Persp",
    "F4: Toggle axes",
    "F5: Boundary box",
    "F6: Toggle menu"
};

void DrawHelpPage() {
    int i, iX = 0, iY = 8, iNum;
    const char** szGuides = NULL;
    const char* szTitle = NULL;

    Bdisp_AllClr_VRAM();
    switch (g_iHelpPage) {
        case 0:
            szGuides = szHelpPage0;
            iNum = sizeof(szHelpPage0) / sizeof(szHelpPage0[0]);
            szTitle = "Page 1/3 - Graph";
            break;
        case 1:
            szGuides = szHelpPage1;
            iNum = sizeof(szHelpPage0) / sizeof(szHelpPage0[0]);
            szTitle = "Page 2/3 - Graph";
            break;
        case 2:
            DrawBitmap(56, 16, ICON_16);
            PutText(38, 34, (const uchar *)"Plotter-Z");
            PrintMini(32, 44, (const uchar *)"By Kuki Himekawa", 0);
            szTitle = "Page 3/3 - About";
            break;
    }
    if (szTitle) {
        int iWidth = strlen(szTitle) * 6;
        PutText((VRAM_WIDTH - iWidth) / 2, 0, (const uchar *)szTitle);
        Bdisp_AreaReverseVRAM(0, 0, 127, 7);
    }
    if (szGuides) {
        for (i = 0; i < iNum; ++i, iY += 8) {
            PutText(iX, iY, (const uchar *)szGuides[i]);
        }
    }
    /* Draw 'next page' icon */
    DrawBitmap(107, B_MENU_TOP, MENU_NEXT);
}

void HelpStage() {
    uint uKey;
    g_iHelpPage = 0;
    while (1) {
        DrawHelpPage();
        GetKey(&uKey);
        if (uKey == KEY_CTRL_EXIT || uKey == KEY_CTRL_EXE) {
            return;
        }
        else if (uKey == KEY_CTRL_F6) {
            g_iHelpPage++;
            if (g_iHelpPage >= HELP_PAGE_SIZE) {
                g_iHelpPage = 0;
            }
        }
    }
}

/*====================================================
 * Parametric Stage
 *====================================================*/

static void DrawParametricStage() {
    int iX, iY = 8, i;
    const char* szTitle = "Edit Parametric";
    const char* szExprLabel[] = { "x(u,v)=", "y(u,v)=", "z(u,v)=" };
    Bdisp_AllClr_VRAM();

    /* Title */
    iX = (VRAM_WIDTH - strlen(szTitle) * CURRENT_FONT_WIDTH) / 2;
    PutText(iX, 0, (const uchar *)szTitle);
    Bdisp_AreaReverseVRAM(0, 0, 127, 7);

    /* Expr */
    for (i = 0; i < 3; ++i) {
        PutText(0, iY, szExprLabel[i]);
        iY += CURRENT_FONT_HEIGHT;
        PutText(12, iY, g_szParmExpr[i]);
        iY += CURRENT_FONT_HEIGHT;
    }

    /* Bottom Menu */
    {
        static const uchar* pMenuBitmap[B_MENU_ITEM_NUM] = { MENU_BACK, MENU_X_EQ, MENU_Y_EQ, MENU_Z_EQ, 0, MENU_OK };
        int i;
        Bdisp_AreaClr_VRAM(&BoxMenuArea);
        for (i = 0; i < B_MENU_ITEM_NUM; ++i) {
            if (pMenuBitmap[i]) {
                DrawBitmap(B_MENU_LEFT + B_MENU_ITEM_WIDTH * i, B_MENU_TOP, pMenuBitmap[i]);
            }
        }
    }
}

static int ParametricStage() {
    uint uKey;
    while (1) {
        DrawParametricStage();
        GetKey(&uKey);
        switch (uKey) {
            case KEY_CTRL_EXIT:
            case KEY_CTRL_F1:
                return 0;
    
            case KEY_CTRL_F2:
                ExprStage(PARM_EXPR_LENGTH, g_szParmExpr[0], "Edit x(u,v)=");
                break;
            
            case KEY_CTRL_F3:
                ExprStage(PARM_EXPR_LENGTH, g_szParmExpr[1], "Edit y(u,v)=");
                break;

            case KEY_CTRL_F4:
                ExprStage(PARM_EXPR_LENGTH, g_szParmExpr[2], "Edit z(u,v)=");
                break;

            case KEY_CTRL_EXE:
            case KEY_CTRL_F6:
                return 1;
        }
    }
    return 0;
}

/*====================================================
 * Main Stage: Ready, Error, Formula
 *====================================================*/

static void DrawIdleScreen(void) {
    static const char* szLines[] = {
        "\x17 Plotter-Z \x18",
        "Enter an expression",
        "or select a sample"
    };
    static const int iCount = sizeof(szLines) / sizeof(szLines[0]);
    int i, iY;

    iY = 0;
    for (i = 0; i < iCount; ++i) {
        int iLeft = (VRAM_WIDTH - (int)strlen(szLines[i]) * CURRENT_FONT_WIDTH) / 2;
        PutText(iLeft, iY, (const uchar*)szLines[i]);
        iY += CURRENT_FONT_HEIGHT;
        if (i == 0) {
            iY += CURRENT_FONT_HEIGHT * 2;
        }
    }

    Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, CURRENT_FONT_HEIGHT - 1);
}

static void DrawErrorScreen(void) {
    int iW;
    static const char* szTitle = "Error";

    iW = strlen(szTitle) * CURRENT_FONT_WIDTH;
    PutText((VRAM_WIDTH - iW) / 2, 0, (const uchar *)szTitle);
    Bdisp_AreaReverseVRAM(0, 0, 127, 7);
    
    iW = strlen(g_szErrMsgLine1) * CURRENT_FONT_WIDTH;
    PutText((VRAM_WIDTH - iW) / 2, 24, (const uchar *)g_szErrMsgLine1);

    iW = strlen(g_szErrMsgLine2) * CURRENT_FONT_WIDTH;
    PutText((VRAM_WIDTH - iW) / 2, 32, (const uchar *)g_szErrMsgLine2);
}

void DrawMainStage(void) {
    Bdisp_AllClr_VRAM();
    switch (g_iMainState) {
        case STATE_IDLE:
            DrawIdleScreen();
            break;
        case STATE_ERROR:
            DrawErrorScreen();
            break;
        case STATE_READY:
            DrawFormula();
            break;
    }
    /* Draw bttom menu */
    {
        static const uchar* pMenuBitmap[B_MENU_ITEM_NUM] = { MENU_CART, MENU_PARM, MENU_WIN, MENU_SAMPLE, MENU_HELP, MENU_PLOT };
        int bMenuItemVisible[B_MENU_ITEM_NUM];
        int i;
        Bdisp_AreaClr_VRAM(&BoxMenuArea);
        memset(bMenuItemVisible, 1, sizeof(bMenuItemVisible));
        bMenuItemVisible[5] = STATE_READY == g_iMainState;
        for (i = 0; i < B_MENU_ITEM_NUM; ++i) {
            if (pMenuBitmap[i] && bMenuItemVisible[i]) {
                DrawBitmap(B_MENU_LEFT + B_MENU_ITEM_WIDTH * i, B_MENU_TOP, pMenuBitmap[i]);
            }
        }
    }
}

int MainStage(void) {
    int i;
    uint uKey;
    int iRet;
    while (1) {
        DrawMainStage();
        GetKey(&uKey);
        switch (uKey) {
            case KEY_CTRL_F1:
                iRet = ExprStage(sizeof(g_szCartExpr), g_szCartExpr, "Edit z(x,y)=");
                if (iRet) {
                    g_iFuncType = FUNC_TYPE_CARTESIAN;
                    if (ParseAndRenderExpr()) 
                        g_iMainState = STATE_READY;
                    else
                        g_iMainState = STATE_ERROR;
                }
                break;
            case KEY_CTRL_F2:
                iRet = ParametricStage();
                if (iRet) {
                    g_iFuncType = FUNC_TYPE_PARAMETRIC;
                    if (ParseAndRenderExpr()) 
                        g_iMainState = STATE_READY;
                    else
                        g_iMainState = STATE_ERROR;
                }
                break;
            case KEY_CTRL_F3:
                WindowEditorStage();
                break;
            case KEY_CTRL_F4:
                iRet = SamplesStage();
                if (iRet) {
                    const PzSample* p = PlotterZSamples + g_iSampleIndex;
                    Camera.xMin = p->xMin;
                    Camera.xMax = p->xMax;
                    Camera.yMin = p->yMin;
                    Camera.yMax = p->yMax;
                    Camera.zMin = p->zMin;
                    Camera.zMax = p->zMax;
                    Camera.xGrid = 15;
                    Camera.yGrid = 15;
                    Camera.uMin = p->uMin;
                    Camera.uMax = p->uMax;
                    Camera.vMin = p->vMin;
                    Camera.vMax = p->vMax;
                    Camera.uGrid = 15;
                    Camera.vGrid = 15;
                    switch (p->iFuncType) {
                        case FUNC_TYPE_CARTESIAN:
                            Utils_StringCopy(g_szCartExpr, sizeof(g_szCartExpr), p->szExpr[0]);
                            break;
                        case FUNC_TYPE_PARAMETRIC:
                            for (i = 0; i < 3; ++i) {
                                Utils_StringCopy(g_szParmExpr[i], sizeof(g_szCartExpr), p->szExpr[i]);
                            }
                            break;
                    }
                    g_iFuncType = p->iFuncType;
                    if (ParseAndRenderExpr())
                        g_iMainState = STATE_READY;
                    else
                        g_iMainState = STATE_ERROR;
                }
                break;
            
            case KEY_CTRL_F5:
                HelpStage();
                break;
    
            case KEY_CTRL_F6:
                if (g_iMainState == STATE_READY) {
                    switch (g_iFuncType) {
                        case FUNC_TYPE_CARTESIAN:
                            EzMachine_SetVariableByIndex(g_pVmCartZ, VM_VAR_T_INDEX, 0);
                            break;
                        case FUNC_TYPE_PARAMETRIC: {
                            int i;
                            for (i = 0; i < 3; ++i) {
                                EzMachine_SetVariableByIndex(g_pVmParm[i], VM_VAR_T_INDEX, 0);
                            }
                            break;
                        }
                    }
                    if (RecalcSurface(RecalcInitFullscreen, RecalcRedrawFullscreen)) {
                        GraphStage();
                    }
                }
                break;
            case KEY_CTRL_UP:
                if (g_iMainState == STATE_READY) g_iFormulaY -= 4;
                break;
            case KEY_CTRL_DOWN:
                if (g_iMainState == STATE_READY) g_iFormulaY += 4;
                break;
            case KEY_CTRL_LEFT:
                if (g_iMainState == STATE_READY) g_iFormulaX -= 4;
                break;
            case KEY_CTRL_RIGHT:
                if (g_iMainState == STATE_READY) g_iFormulaX += 4;
                break;
            case KEY_CTRL_EXIT:
                if (g_iMainState == STATE_READY) {
                    g_iFormulaX = g_iFormulaOrigX;
                    g_iFormulaY = g_iFormulaOrigY;
                }
                break;
            default:
                break;
        }
    }
}

/*====================================================
 * fx-9860G SDK boilerplate
 *====================================================*/
int AddIn_main(int isAppli, unsigned short OptionNum) {
    pVRAM = GetVRAMAddress();
    PzCamera_Initialize();
    Camera.iViewportS = VRAM_HEIGHT / 2;
    Camera.iViewportX = VRAM_WIDTH / 2;
    Camera.iViewportY = VRAM_HEIGHT / 2;
    WinEdit_ApplyDefault();
    
    /* Set up renderer-Z callback interfaces */
    RenderConfig_GetDefaultStyle(&g_rzConfig);
    RenderConfig_CalculateBigSymbolPoints(&g_rzConfig);
    g_rzConfig.sInterfaces.setPixel = RzSetPixel;
    g_rzConfig.sInterfaces.plotLine = Bdisp_DrawLineVRAM;
    g_rzConfig.sInterfaces.putChar  = RzPutChar;

    MainStage();
    return 1;
}

#pragma section _BR_Size
unsigned long BR_Size;
#pragma section

#pragma section _TOP

int InitializeSystem(int isAppli, unsigned short OptionNum) {
    return INIT_ADDIN_APPLICATION(isAppli, OptionNum);
}

#pragma section

