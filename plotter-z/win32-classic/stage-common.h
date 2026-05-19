#ifndef STAGE_COMMON_H
#define STAGE_COMMON_H

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "resource.h"
#include "../utils/hybird_6x8.h"
#include "../../common/utils.h"
#include "../../common/constants.h"
#include "../../formula-z/fz.h"
#include "../../evaluator-z/ez.h"
#include "../../renderer-z/rz.h"
#include "../../renderer-z/ascii_extended_mapping.h"
#include "../../deps/salvia89/salvia.h"

#define MAX_LOADSTRING 100
#define EXPR_MAX 300
#define CURRENT_FONT_WIDTH  6
#define CURRENT_FONT_HEIGHT 8

/* Colors */
#define COLOR_BLACK         0
#define COLOR_DARK_GRAY     1
#define COLOR_LIGHT_GRAY    2
#define COLOR_WHITE         3

extern const COLORREF g_rgbPalette[];

/* Expression */
extern char szExpr[EXPR_MAX];

/* Camera */
#define DEFAULT_VIEW_ALPHA 30
#define DEFAULT_VIEW_BETA 30

extern const int arrZoomLevels[];
extern const int iNumZoomLevel;
#define ZOOM_LEVEL_DEFAULT 3

struct CameraStruct {
    int iViewportX;
    int iViewportY;
    int iViewportS;
    int iAlphaDeg;
    int iBetaDeg;
    PZ_FIXED cosA;  PZ_FIXED sinA;  PZ_FIXED cosB;  PZ_FIXED sinB; 
    PZ_FLOAT xMin;  PZ_FLOAT xMax;  int xGrid;
    PZ_FLOAT yMin;  PZ_FLOAT yMax;  int yGrid;
    PZ_FLOAT zMin;  PZ_FLOAT zMax;
    int iZoomLevel;
};

extern struct CameraStruct Camera;

#define GRID_MAX 30
extern PZ_FIXED zBuf[GRID_MAX * GRID_MAX];
extern PZ_FIXED xBuf[GRID_MAX];
extern PZ_FIXED yBuf[GRID_MAX];
#define Z_BUF(x,y) (zBuf[(x) + (y) * Camera.xGrid])

extern FzAstNode*      g_pAstExpr;
extern EzMachine*      g_pVm;
extern RenderNode*     g_pRenderNode;
extern RenderConfig    g_RenderConfig;
extern char            g_szErrorBuf[EZ_ERROR_CONTENT_LENGTH];

extern int g_bShowBox;
extern int g_bMouseDown;
extern int g_bPanMode;
extern int g_bExprDrag;
extern int g_iMousePrevX;
extern int g_iMousePrevY;
extern int g_iExprPosX;
extern int g_iExprPosY;

/* Stage system */
#define STAGE_WIREFRAME         0
#define STAGE_WINDOW_EDITOR     1
#define STAGE_EXPRESSION_EDITOR 2
#define STAGE_RECALC            3
#define STAGE_ERROR             4

extern int g_iStage;

/* Recalc progress */
extern int g_iRecalcProgress;
extern int g_iRecalcTotal;

/* Canvas */
extern unsigned char*   g_pCanvas;
extern int              g_iCanvasW;
extern int              g_iCanvasH;
extern int              g_iCanvasPitch;

/* Back buffer */
extern HDC       g_hdcBuffer;
extern HBITMAP   g_hBmpBuffer;
extern HBITMAP   g_hBmpOld;
extern BYTE*     g_pDibPixels;
extern int       g_iScreenBpp;
extern int       g_iDibPitch;

/* Canvas drawing primitives */
int  getPixelCanvas(int x, int y);
void setPixelCanvas(int x, int y, int iColor);
void drawLineCanvas(int x0, int y0, int x1, int y1, int iColor);
void fillRectCanvas(int dx, int dy, int w, int h, int iColor);
void draw1bppCanvas(const unsigned char *raw, int dx, int dy, int w, int h, int iColor);
void putCharCanvas(int x, int y, unsigned char ch, int iColor);
void putTextCanvas(int x, int y, const unsigned char* usz, int iColor);

/* Canvas lifecycle */
int  createCanvas(int iWidth, int iHeight);
void destroyCanvas(void);

/* Back buffer */
int  createBackBuffer(HWND hWnd, int iWidth, int iHeight);
void destroyBackBuffer(void);
void paintCanvasToWindow(HWND hWnd);

/* 3D projection */
void xyz2xy(PZ_FIXED x, PZ_FIXED y, PZ_FIXED z, int *ox, int *oy);

/* Recalc */
int recalc(void);

/* Full redraw */
void redrawCanvas(void);

/* Stage draw functions */
void drawWindowEditor(void);
void drawRecalcScreen(HWND hWnd);
void drawErrorScreen(void);
void drawExpressionEditor(void);

/* Stage key handlers */
void Wireframe_OnKey(HWND hWnd, WPARAM wParam);
void WindowEditor_OnKey(HWND hWnd, WPARAM wParam);
void WindowEditor_OnChar(HWND hWnd, TCHAR ch);
void ExpressionEditor_OnKey(HWND hWnd, WPARAM wParam);
void ExpressionEditor_OnChar(HWND hWnd, TCHAR ch);
void Recalc_OnKey(HWND hWnd, WPARAM wParam);
void Error_OnKey(HWND hWnd, WPARAM wParam);

/* rz wrappers */
void rzSetPixel(int x, int y);
void rzPlotLine(int x0, int y0, int x1, int y1);
void rzPutChar(int x, int y, unsigned char ch);

/* HINSTANCE */
extern HINSTANCE hInst;

#endif
