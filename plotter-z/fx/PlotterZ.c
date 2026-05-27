#include <fxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitmaps.h"
#include "../utils/hybird_6x8.h"
#include "../../formula-z/fz.h"
#include "../../renderer-z/rz.h"
#include "../../evaluator-z/ez.h"

typedef unsigned int uint;
typedef unsigned char uchar;

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

static const DISPBOX BoxMenuArea = { 0, 56, VRAM_WIDTH - 1, 63 };
static const DISPBOX BoxTopArea = { 0, 0, VRAM_WIDTH - 1, 7 };
static const DISPBOX BoxEditArea = { 0, 8, VRAM_WIDTH - 1, 55 };

/*====================================================
 * Global Variables
 *====================================================*/

char    g_szExpr[300] = "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))";
char    g_szErrorBuf[EZ_ERROR_CONTENT_LENGTH];
EzError g_iCompileErr = EZERR_NONE;

FzAstNode*      g_pAstExpr      = NULL;
EzMachine*      g_pVm           = NULL;
RenderNode*     g_pRenderNode   = NULL;
RenderConfig    g_RenderConfig;

/*====================================================
 * Camera and surface data
 *====================================================*/
#define GRID_MAX            20
#define ZOOM_LEVEL_DEFAULT  3
#define DEFAULT_VIEW_ALPHA  30
#define DEFAULT_VIEW_BETA   30

struct CameraStruct {
    int         iViewportX, iViewportY, iViewportS;
    int         iAlphaDeg, iBetaDeg;
    PZ_FIXED    cosA, sinA, cosB, sinB;
    PZ_FLOAT    xMin, xMax; int xGrid;
    PZ_FLOAT    yMin, yMax; int yGrid;
    PZ_FLOAT    zMin, zMax;
    int         iZoomLevel;
};

static struct CameraStruct Camera = {
    0, 0, 0,
    DEFAULT_VIEW_ALPHA, DEFAULT_VIEW_BETA,
    0, 0, 0, 0,
    -6.0f, 6.0f, 15,
    -6.0f, 6.0f, 15,
    -3.0f, 3.0f,
    ZOOM_LEVEL_DEFAULT
};

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
    /* Top Banner */
    {
        static const char szTopText[] = "Edit Expression";
        static const int iLeft = (VRAM_WIDTH - (sizeof(szTopText) - 1) * CURRENT_FONT_WIDTH) / 2;
        PutText(iLeft, 0, (const uchar *)szTopText);
        Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, 7);
    }
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

int ExprStage(void) {
    uint uKey;
    char szBuf[sizeof(g_szExpr)];
    int iLen;

    strcpy(szBuf, g_szExpr);
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

            case KEY_CTRL_F6:
            case KEY_CTRL_EXE:
                strcpy(g_szExpr, szBuf);
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
                if (iLen < (int)(sizeof(szBuf) - 1)) {
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
int ParseAndRenderExpr(void) {
    int iW = VRAM_WIDTH, iH = VRAM_HEIGHT - 16;

    /* Destroy previous state */
    if (g_pRenderNode != NULL) { RenderNode_Destroy(g_pRenderNode); g_pRenderNode = NULL; }
    if (g_pAstExpr != NULL) { FzAstNode_Destroy(g_pAstExpr); g_pAstExpr = NULL; }

    /* Parse */
    g_pAstExpr = FzParser_ParseExpression(g_szExpr);
    if (g_pAstExpr == NULL) return 0;

    /* Create VM singleton */
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

    /* Compile */
    if (g_pVm != NULL) {
        g_iCompileErr = EzMachine_Compile(g_pVm, g_pAstExpr, g_szErrorBuf);
        if (g_iCompileErr != EZERR_NONE) return 0;
    }

    /* Build render tree */
    g_pRenderNode = Render_Transform(g_pAstExpr);
    if (g_pRenderNode == NULL) return 0;

    RenderNode_EstimateSize(g_pRenderNode, &g_RenderConfig);

    /* Center formula */
    g_iFormulaX = (iW - g_pRenderNode->sLayout.iWidth) / 2;
    if (g_iFormulaX < 0) g_iFormulaX = 0;
    g_iFormulaY = (VRAM_HEIGHT - g_pRenderNode->sLayout.iAscent - g_pRenderNode->sLayout.iDescent) / 2 + g_pRenderNode->sLayout.iAscent;
	if (g_iFormulaY < 0) g_iFormulaY = 0;

    g_iFormulaOrigX = g_iFormulaX;
    g_iFormulaOrigY = g_iFormulaY;
    
    return 1;
}

/* Draw formula centered in edit area */
void DrawFormula(void) {
    if (g_pRenderNode == NULL || g_pAstExpr == NULL) {EzError g_iCompileErr;
        const char* szMsg = "Syntax Error";
        int iLeft = (VRAM_WIDTH - (int)strlen(szMsg) * CURRENT_FONT_WIDTH) / 2;
        PutText(iLeft, 8 + 20, (const uchar*)szMsg);
        return;
    }

    /* Formula */
    RenderNode_Draw(g_pRenderNode, &g_RenderConfig, g_iFormulaX, g_iFormulaY);

    /* f(x,y)= banner */
    {
        Bdisp_AreaClr_VRAM(&BoxTopArea);
        PutText(0, 0, (const uchar*)"f(x,y)=");
        Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, CURRENT_FONT_HEIGHT - 1);
    }
}

/*====================================================
 * Value Editor Stage
 *====================================================*/

static char g_szValueEditorBuf[32];

void DrawValueEditor(const char* szTitle) {
    int iLen, iX, iY;
    Bdisp_AllClr_VRAM();

    /* Title banner */
    {
        int iLeft = (VRAM_WIDTH - (int)strlen(szTitle) * CURRENT_FONT_WIDTH) / 2;
        PutText(iLeft, 0, (const uchar*)szTitle);
        Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, 7);
    }

    /* Edit buffer centered with cursor underscore */
    {
        char szDisplay[34];
        strcpy(szDisplay, g_szValueEditorBuf);
        iLen = (int)strlen(szDisplay);
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
    iLen = (int)strlen(g_szValueEditorBuf);

    while (1) {
        DrawValueEditor(szTitle);
        GetKey(&uKey);
        iLen = (int)strlen(g_szValueEditorBuf);

        switch (uKey) {
            case KEY_CTRL_F1:
            case KEY_CTRL_EXIT:
                strcpy(g_szValueEditorBuf, szInitial);
                /* fall through */
            case KEY_CTRL_F6:
            case KEY_CTRL_EXE:
                return (int)strlen(g_szValueEditorBuf);

            case KEY_CTRL_DEL:
                if (iLen > 0)
                    g_szValueEditorBuf[iLen - 1] = '\0';
                break;

            default:
                if (iLen < 30) {
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

#define WIN_EDIT_ROWS           4
#define WIN_EDIT_COLS           2
#define WIN_EDIT_LABEL_X        8
#define WIN_EDIT_FIELD_X        40
#define WIN_EDIT_ROW_START_Y    12
#define WIN_EDIT_ROW_H          10
#define WIN_EDIT_FIELD_W        36

/* Cursor position in grid */
static int g_iWinEditCursorRow = 0;
static int g_iWinEditCursorCol = 0;

/* Row labels */
static const char* g_szWinEditLabels[WIN_EDIT_ROWS] = { "X:", "Y:", "Z:", "Grid:" };

/* String buffers for each field */
static char g_szWinEditValues[WIN_EDIT_ROWS][WIN_EDIT_COLS][20];

void DrawWindowEditorStage(void) {
    int iRow, iCol;

    Bdisp_AllClr_VRAM();

    /* Draw field labels and values */
    for (iRow = 0; iRow < WIN_EDIT_ROWS; ++iRow) {
        for (iCol = 0; iCol < WIN_EDIT_COLS; ++iCol) {
            int iX = WIN_EDIT_FIELD_X + iCol * (WIN_EDIT_FIELD_W + 10);
            int iY = WIN_EDIT_ROW_START_Y + iRow * WIN_EDIT_ROW_H;
            PutText(iX, iY, (const uchar*)g_szWinEditValues[iRow][iCol]);

            /* Highlight cursor field */
            if (iRow == g_iWinEditCursorRow && iCol == g_iWinEditCursorCol) {
                int iLen = (int)strlen(g_szWinEditValues[iRow][iCol]);
                Bdisp_AreaReverseVRAM(iX, iY,
                    iX + iLen * CURRENT_FONT_WIDTH - 1,
                    iY + CURRENT_FONT_HEIGHT - 1);
            }
        }
        /* Arrow between columns for x/y/z rows */
        if (iRow < 3) {
            int iArrowX = WIN_EDIT_FIELD_X + WIN_EDIT_FIELD_W + 2;
            int iY = WIN_EDIT_ROW_START_Y + iRow * WIN_EDIT_ROW_H;
            PutChar(iArrowX, iY, '\x18');
        }
    }

    /* Row label */
    for (iRow = 0; iRow < WIN_EDIT_ROWS; ++iRow) {
        int iY = WIN_EDIT_ROW_START_Y + iRow * WIN_EDIT_ROW_H;
        PutText(WIN_EDIT_LABEL_X, iY, (const uchar*)g_szWinEditLabels[iRow]);
    }

    /* Bottom Menu */
    {
        static const uchar* pMenuBitmap[B_MENU_ITEM_NUM] = { MENU_BACK, 0, 0, MENU_EDIT, 0, MENU_OK };
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
    /* Top Banner */
    {
        static const char szTopText[] = "Window Editor";
        static const int iLeft = (VRAM_WIDTH - (sizeof(szTopText) - 1) * CURRENT_FONT_WIDTH) / 2;
        PutText(iLeft, 0, (const uchar *)szTopText);
        Bdisp_AreaReverseVRAM(0, 0, VRAM_WIDTH - 1, 7);
    }
}

static void WinEdit_LoadCamera(void) {
    int i;
    for (i = 0; i < 4; ++i) {
        if (i == 0) {
            Utils_Ftoa((double)Camera.xMin, g_szWinEditValues[0][0], 2);
            Utils_Ftoa((double)Camera.xMax, g_szWinEditValues[0][1], 2);
        } else if (i == 1) {
            Utils_Ftoa((double)Camera.yMin, g_szWinEditValues[1][0], 2);
            Utils_Ftoa((double)Camera.yMax, g_szWinEditValues[1][1], 2);
        } else if (i == 2) {
            Utils_Ftoa((double)Camera.zMin, g_szWinEditValues[2][0], 2);
            Utils_Ftoa((double)Camera.zMax, g_szWinEditValues[2][1], 2);
        } else {
            Utils_Ftoa(Camera.xGrid, g_szWinEditValues[3][0], 2);
            Utils_Ftoa(Camera.yGrid, g_szWinEditValues[3][1], 2);
        }
    }
}

static void WinEdit_SaveCamera(void) {
    Camera.xMin  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[0][0]);
    Camera.xMax  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[0][1]);
    Camera.yMin  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[1][0]);
    Camera.yMax  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[1][1]);
    Camera.zMin  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[2][0]);
    Camera.zMax  = (PZ_FLOAT)Utils_Atof(g_szWinEditValues[2][1]);

    {
        int iGrid;
        iGrid = (int)Utils_Atoi(g_szWinEditValues[3][0]);
        if (iGrid < 5) iGrid = 5;
        if (iGrid > GRID_MAX) iGrid = GRID_MAX;
        Camera.xGrid = iGrid;

        iGrid = (int)Utils_Atoi(g_szWinEditValues[3][1]);
        if (iGrid < 5) iGrid = 5;
        if (iGrid > GRID_MAX) iGrid = GRID_MAX;
        Camera.yGrid = iGrid;
    }
}

void WindowEditorStage(void) {
    uint uKey;

    WinEdit_LoadCamera();
    g_iWinEditCursorRow = 0;
    g_iWinEditCursorCol = 0;

    while (1) {
        DrawWindowEditorStage();
        GetKey(&uKey);

        switch (uKey) {
            case KEY_CTRL_EXIT:
            case KEY_CTRL_F1:
                return;

            case KEY_CTRL_EXE:
            case KEY_CTRL_F4:
            {
                static const char* szFieldTitles[4][2] = {
                    { "X Min", "X Max" },
                    { "Y Min", "Y Max" },
                    { "Z Min", "Z Max" },
                    { "X Grid", "Y Grid" }
                };
                ValueEditorStage(szFieldTitles[g_iWinEditCursorRow][g_iWinEditCursorCol],
                    g_szWinEditValues[g_iWinEditCursorRow][g_iWinEditCursorCol]);
                strcpy(g_szWinEditValues[g_iWinEditCursorRow][g_iWinEditCursorCol],
                    g_szValueEditorBuf);
                break;
            }

            case KEY_CTRL_F6:
                WinEdit_SaveCamera();
                return;

            case KEY_CTRL_UP:
                if (g_iWinEditCursorRow > 0)
                    g_iWinEditCursorRow--;
                break;

            case KEY_CTRL_DOWN:
                if (g_iWinEditCursorRow < WIN_EDIT_ROWS - 1)
                    g_iWinEditCursorRow++;
                break;

            case KEY_CTRL_LEFT:
                if (g_iWinEditCursorCol > 0)
                    g_iWinEditCursorCol--;
                else if (g_iWinEditCursorRow > 0) {
                    g_iWinEditCursorRow--;
                    g_iWinEditCursorCol = WIN_EDIT_COLS - 1;
                }
                break;

            case KEY_CTRL_RIGHT:
                if (g_iWinEditCursorCol < WIN_EDIT_COLS - 1)
                    g_iWinEditCursorCol++;
                else if (g_iWinEditCursorRow < WIN_EDIT_ROWS - 1) {
                    g_iWinEditCursorRow++;
                    g_iWinEditCursorCol = 0;
                }
                break;

        }
    }
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
    const char* szErrType;
    const char* szDetail;
    int iLen;

    Bdisp_AllClr_VRAM();

    if (g_pAstExpr == NULL) {
        szErrType = "Syntax Error";
        szDetail = NULL;
    } else {
        switch (g_iCompileErr) {
            case EZERR_VARIABLE_UNDEFINED:
                szErrType = "Undefined Variable:"; break;
            case EZERR_FUNCTION_UNDEFINED:
                szErrType = "Undefined Function:"; break;
            case EZERR_FUNCTION_PARAM_MISMATCH:
                szErrType = "Parameters mismatch:"; break;
            default:
                szErrType = "Compile Error:"; break;
        }
        szDetail = g_szErrorBuf;
    }

    iLen = (int)strlen(szErrType) * CURRENT_FONT_WIDTH;
    PutText((VRAM_WIDTH - iLen) / 2, 8 + 12, (const uchar*)szErrType);

    if (szDetail != NULL && szDetail[0] != '\0') {
        iLen = (int)strlen(szDetail) * CURRENT_FONT_WIDTH;
        PutText((VRAM_WIDTH - iLen) / 2, 8 + 22, (const uchar*)szDetail);
    }

    iLen = (int)strlen(szErrType) * CURRENT_FONT_WIDTH;
    PutText((VRAM_WIDTH - iLen) / 2, 8 + 12, (const uchar*)szErrType);

    if (szDetail != NULL && szDetail[0] != '\0') {
        iLen = (int)strlen(szDetail) * CURRENT_FONT_WIDTH;
        PutText((VRAM_WIDTH - iLen) / 2, 8 + 22, (const uchar*)szDetail);
    }
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
        static const uchar* pMenuBitmap[B_MENU_ITEM_NUM] = { MENU_EXPR, MENU_WIN, MENU_SAMPLE, MENU_ABOUT, 0, MENU_PLOT };
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
    uint uKey;
    int iRet;
    while (1) {
        DrawMainStage();
        GetKey(&uKey);
        switch (uKey) {
            case KEY_CTRL_F1:
                iRet = ExprStage();
                if (iRet) {
                    if (ParseAndRenderExpr())
                        g_iMainState = STATE_READY;
                    else
                        g_iMainState = STATE_ERROR;
                }
                break;
            case KEY_CTRL_F2:
                WindowEditorStage();
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

    /* Set up renderer-Z callback interfaces */
    g_RenderConfig.sInterfaces.setPixel = RzSetPixel;
    g_RenderConfig.sInterfaces.plotLine = Bdisp_DrawLineVRAM;
    g_RenderConfig.sInterfaces.putChar  = RzPutChar;
    RenderConfig_GetDefaultStyle(&g_RenderConfig);

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

