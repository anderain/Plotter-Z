#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "resource.h"
#include "../utils/hybird_6x8.h"
#include "../utils/samples.h"
#include "../../common/utils.h"
#include "../../common/constants.h"
#include "../../formula-z/fz.h"
#include "../../evaluator-z/ez.h"
#include "../../renderer-z/rz.h"
#include "../../renderer-z/ascii_extended_mapping.h"
#include "../../deps/salvia89/salvia.h"
#include "../../deps/pine89/pine-ini.h"

/*====================================================
 * Constants
 *====================================================*/
#define MAX_LOADSTRING          100
#define CURRENT_FONT_WIDTH      6
#define CURRENT_FONT_HEIGHT     8
#define ZOOM_DRAG_THRESHOLD     15

/* Function type */
#define ORTHOGRAPHIC            0
#define PERSPECTIVE             1

/* Expression buffer size */
#define CART_EXPR_LENGTH        150
#define PARM_EXPR_LENGTH        80

/* Performance test*/
#define PERF_TEST_SECONDS   15
#define PERF_TEST_MS        ((PERF_TEST_SECONDS) * 1000)

static const char* szFuncTypeLabel[] = { "CARTESIAN", "PARAMETRIC" };

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

#define SAMPLE_DEFAULT_GRID 20

/*====================================================
 * Handles
 *====================================================*/
HINSTANCE   hInst;
HWND        hwndCB;

/*====================================================
 * Interfaces
 *====================================================*/
int     RecalcSurface               (HWND hWnd);
void    CharToTChar                 (TCHAR* dst, const char* src, int maxLen);
void    TCharToChar                 (char* dst, const TCHAR* src, int maxLen) ;
void    CenterDialog                (HWND hDlg);
void    LoadApplicationConfig       (void);
void    PerformanceTestRedraw       (HWND hWnd);
void    PerformanceTestCheck        (HWND hWnd);
void    ReCreateCanvasAndBackBuffer (HWND hWnd, int iPrevScale);
int     ParseExpr                   (void);
void    UpdateDisplayMenuCheckStatus(HWND hWnd);
void    UpdateViewMenuCheckStatus   (HWND hWnd);
void    BalanceParen                (char* szNew, int iMax);

/*====================================================
 * Color palette (grayscale, 4 levels)
 *====================================================*/
#define HIGH_CONTRAST_COLOR 1

COLORREF g_rgbPalette[] = {
    RGB(0x00, 0x00, 0x00),      /* COLOR_BLACK       */
    RGB(0x66, 0x66, 0x66),      /* COLOR_DARK_GRAY   */
    RGB(0xaa, 0xaa, 0xaa),      /* COLOR_LIGHT_GRAY  */
    RGB(0xff, 0xff, 0xff),      /* COLOR_WHITE       */
};

/* Pre-computed palette for 16/32 bpp blit */
#define RGB888_TO_WORD_RGB555(r,g,b)    ((WORD)(((r) >> 3 << 10) | ((g) >> 3 << 5) | ((b) >> 3)))

static WORD   g_wPaletteRGB555[4];
static DWORD  g_dwPaletteRGB888[4];

static void RecalcPaletteLUTs(void) {
    int i;
    for (i = 0; i < 4; ++i) {
        BYTE r = GetRValue(g_rgbPalette[i]);
        BYTE g = GetGValue(g_rgbPalette[i]);
        BYTE b = GetBValue(g_rgbPalette[i]);
        g_wPaletteRGB555[i] = RGB888_TO_WORD_RGB555(r, g, b);
        g_dwPaletteRGB888[i] = (DWORD)((r << 16) | (g << 8) | b);
    }
}

#define COLOR_BLACK         0
#define COLOR_DARK_GRAY     1
#define COLOR_LIGHT_GRAY    2
#define COLOR_WHITE         3

/*====================================================
 * Canvas zoom factor
 *   Higher values reduce canvas resolution for
 *   low-performance devices.  e.g. factor=2 means
 *   canvas is half the width/height of the DIB.
 *====================================================*/
static int g_iCanvasScaleFactor = 1;
static int g_iBarHeight      = 0;

/*====================================================
 * Expression
 *====================================================*/
#define DEFAULT_CART_EXPR "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))"

char    g_szCartExpr[CART_EXPR_LENGTH] = DEFAULT_CART_EXPR;
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

/*====================================================
 * Vertex Buffer
 *====================================================*/
typedef struct { NUMERIC x, y, z; } Vertex;
typedef struct { int i0, i1; } Edge;

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

/*====================================================
 * Interactive
 *====================================================*/
/* Application stage */
#define STAGE_IDLE      0
#define STAGE_ERROR     1
#define STAGE_READY     2
static int g_iStage = STAGE_IDLE;

/* Mouse interaction modes */
#define MOUSE_MODE_CAMERA      0
#define MOUSE_MODE_PAN_MOVE    1
#define MOUSE_MODE_ZOOM        2
#define MOUSE_MODE_FORMULA     3
#define MOUSE_MODE_FOV         4

static int  g_iMouseRefreshMs = 120;
DWORD       g_dwLastMouseRefresh;

static int      g_iMouseMode    = MOUSE_MODE_CAMERA;
static int      g_bShowFooter   = 1;
static int      g_bShowAxes     = 0;
static int      g_bShowBox      = 0;
static int      g_bShowFormula     = 1;
static int      g_bMouseDown    = 0;
static int      g_iMousePrevX   = 0;
static int      g_iMousePrevY   = 0;
static int      g_iZoomAccum    = 0;
static int      g_iFovAccum     = 0;
static int      g_iEscTrigger   = 0;
static int      g_bPerformanceTest = 0;
static int      g_nPaintCount      = 0;
static DWORD    g_dwPerfStartTime  = 0;
static int      g_iFormulaX = 0;
static int      g_iFormulaY = 0;

#define ZOOM_DRAG_THRESHOLD     15
#define FOV_DRAG_THRESHOLD      10

/*====================================================
 * 2bpp canvas (4-level grayscale, packed pixels)
 *====================================================*/
static unsigned char*   g_pCanvas       = NULL;
static int              g_iCanvasW      = 0;
static int              g_iCanvasH      = 0;
static int              g_iCanvasPitch  = 0;

#define canvasByteIndex(x, y)   ((y) * g_iCanvasPitch + ((x) >> 2))
#define canvasShift(x)          (6 - (((x) & 3) << 1))
#define canvasMask(x)           (~(0x03 << canvasShift(x)))

/*====================================================
 * Drawing primitives (2bpp canvas)
 *====================================================*/
#define ABS(v)  ((v) < 0 ? -(v) : (v))

static void SetPixelCanvas(int x, int y, int iColor) {
    unsigned char ucMask;
    int iByte;
    if (x < 0 || x >= g_iCanvasW || y < 0 || y >= g_iCanvasH) return;
    iByte = canvasByteIndex(x, y);
    ucMask = canvasMask(x);
    g_pCanvas[iByte] = (unsigned char)((g_pCanvas[iByte] & ucMask)
                     | ((iColor & 0x03) << canvasShift(x)));
}

#define GetPixelCanvas(x, y) \
    ( ((x) < 0 || (x) >= g_iCanvasW || (y) < 0 || (y) >= g_iCanvasH) ? 0 : \
      (g_pCanvas[canvasByteIndex((x), (y))] >> canvasShift((x))) & 0x03 )

static void FillRectCanvas(int dx, int dy, int w, int h, int iColor) {
    int x, y;
    for (y = 0; y < h; ++y)
        for (x = 0; x < w; ++x)
            SetPixelCanvas(dx + x, dy + y, iColor);
}

static void DrawLineCanvas(int x0, int y0, int x1, int y1, int iColor) {
    int dx = ABS(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -ABS(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        SetPixelCanvas(x0, y0, iColor);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void Draw1bppCanvas(const unsigned char *raw, int dx, int dy, int w, int h, int iColor) {
    int pitch = (w >> 3) + (w % 8 ? 1 : 0);
    int x, y, dot;
    unsigned char ucEight;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            ucEight = raw[y * pitch + (x >> 3)];
            dot = (ucEight >> (7 - (x & 7))) & 1;
            if (dot) SetPixelCanvas(dx + x, dy + y, iColor);
        }
    }
}

/*====================================================
 * Invert colors in a rectangle (0<->3, 1<->2)
 *====================================================*/
static void InvertRectCanvas(int dx, int dy, int w, int h) {
    int x, y, iColor;
    for (y = 0; y < h; ++y)
        for (x = 0; x < w; ++x) {
            iColor = GetPixelCanvas(dx + x, dy + y);
            SetPixelCanvas(dx + x, dy + y, 3 - iColor);
        }
}

static void PutCharCanvas(int x, int y, unsigned char ch, int iColor) {
    Draw1bppCanvas(FONT_HYBIRD_6x8 + 8 * (int)ch, x, y, 8, 8, iColor);
}

static void PutTextCanvas(int x, int y, const unsigned char* usz, int iColor) {
    for (; *usz; ++usz, x += CURRENT_FONT_WIDTH)
        PutCharCanvas(x, y, *usz, iColor);
}

/*====================================================
 * Canvas lifecycle
 *====================================================*/
static int CreateCanvas(int iWidth, int iHeight) {
    int iByteCount;
    g_iCanvasW = iWidth;
    g_iCanvasH = iHeight;
    g_iCanvasPitch = (iWidth + 3) / 4;
    iByteCount = g_iCanvasPitch * iHeight;
    if (iByteCount <= 0) return 0;
    g_pCanvas = (unsigned char *)malloc((size_t)iByteCount);
    if (g_pCanvas == NULL) return 0;
    memset(g_pCanvas, (int)((unsigned char)COLOR_BLACK), (size_t)iByteCount);
    return 1;
}

static void DestroyCanvas(void) {
    if (g_pCanvas != NULL) { free(g_pCanvas); g_pCanvas = NULL; }
    g_iCanvasW = 0; g_iCanvasH = 0; g_iCanvasPitch = 0;
}

/*====================================================
 * Back buffer: memory HDC + DIB section
 *====================================================*/
static int       g_iForceBpp     = -1;
static HDC       g_hdcBuffer     = NULL;
static HBITMAP   g_hBmpBuffer    = NULL;
static HBITMAP   g_hBmpOld       = NULL;
static BYTE*     g_pDibPixels    = NULL;
static int       g_iScreenBpp    = 0;
static int       g_iDibPitch     = 0;
static int       g_iDibW         = 0;
static int       g_iDibH         = 0;

static int CreateBackBuffer(HWND hWnd, int iWidth, int iHeight) {
    HDC hdc;
    BITMAPINFO* pbmi;
    int iPaletteSize = 0, iBitCount, i, iBmiSize;

    hdc = GetDC(hWnd);
    if (hdc == NULL) {
        MessageBox(hWnd, TEXT("Failed to get DC"), TEXT("ERROR"), MB_OK);
        return 0;
    }

    if (g_iForceBpp < 0) {
        HBITMAP hBmpTest;
        BITMAP bmTest;
        hBmpTest = CreateCompatibleBitmap(hdc, 8, 8);
        if (hBmpTest == NULL) {
            MessageBox(hWnd, TEXT("Failed to create test bitmap"), TEXT("ERROR"), MB_OK);
            ReleaseDC(hWnd, hdc);
            return 0;
        }
        GetObject(hBmpTest, sizeof(BITMAP), &bmTest);
        DeleteObject(hBmpTest);  
        g_iScreenBpp = bmTest.bmBitsPixel; 
    }
    else {
        g_iScreenBpp = g_iForceBpp;
    }

    switch (g_iScreenBpp) {
        case 2:  iBitCount = 2;  iPaletteSize = 4;   break;
        case 8:  iBitCount = 8;  iPaletteSize = 256; break;
        case 16: iBitCount = 16; iPaletteSize = 0;   break;
        case 32: iBitCount = 32; iPaletteSize = 0;   break;
        default:
            MessageBox(hWnd, TEXT("Invalid bitmap bpp"), TEXT("ERROR"), MB_OK);
            return 0;
    }

    iBmiSize = sizeof(BITMAPINFO) + iPaletteSize * sizeof(RGBQUAD);
    pbmi = (BITMAPINFO*)malloc((size_t)iBmiSize);
    if (pbmi == NULL) {
        MessageBox(hWnd, TEXT("Failed to allocate BITMAPINFO"), TEXT("ERROR"), MB_OK);
        return 0;
    }
    memset(pbmi, 0, (size_t)iBmiSize);

    if (g_iScreenBpp == 2) {
        static const BYTE byteGrayscale[] = { 0x00, 0x80, 0xc0, 0xff };
        for (i = 0; i < 4; ++i) {
            pbmi->bmiColors[i].rgbRed   = byteGrayscale[i];
            pbmi->bmiColors[i].rgbGreen = byteGrayscale[i];
            pbmi->bmiColors[i].rgbBlue  = byteGrayscale[i];
        }
    } else if (g_iScreenBpp == 8) {
        for (i = 0; i < 4; ++i) {
            pbmi->bmiColors[i].rgbRed   = GetRValue(g_rgbPalette[i]);
            pbmi->bmiColors[i].rgbGreen = GetGValue(g_rgbPalette[i]);
            pbmi->bmiColors[i].rgbBlue  = GetBValue(g_rgbPalette[i]);
        }
    }

    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = iWidth;
    pbmi->bmiHeader.biHeight        = -iHeight;
    pbmi->bmiHeader.biPlanes        = 1;
    pbmi->bmiHeader.biBitCount      = iBitCount;
	pbmi->bmiHeader.biSizeImage		= 0;
	pbmi->bmiHeader.biCompression	= 0;
	pbmi->bmiHeader.biXPelsPerMeter	= 0;
	pbmi->bmiHeader.biYPelsPerMeter	= 0;
	pbmi->bmiHeader.biClrImportant	= 0;
    /* Note: when creating 2bit DIBsection biClrUsed should be set to 0 */
    pbmi->bmiHeader.biClrUsed       = (g_iScreenBpp == 2) ? 0 : iPaletteSize;

    g_hdcBuffer = CreateCompatibleDC(hdc);
    ReleaseDC(hWnd, hdc);

    if (g_hdcBuffer == NULL) {
        free(pbmi);
        MessageBox(hWnd, TEXT("Failed to create hdc buffer"), TEXT("ERROR"), MB_OK);
        return 0;
    }

    g_hBmpBuffer = CreateDIBSection(g_hdcBuffer, pbmi, DIB_RGB_COLORS, (void**)&g_pDibPixels, NULL, 0);
    free(pbmi);

    if (g_hBmpBuffer == NULL || g_pDibPixels == NULL) {
        DeleteDC(g_hdcBuffer);
        g_hdcBuffer = NULL;
        {
            DWORD dwErrorCode;
            TCHAR szBuf[200];
            TCHAR szTitle[] =   TEXT("FAILED: DIB SECTION");
            TCHAR szFormat[] =  TEXT("Error Code = %d.\n")
                                TEXT("Failed to create %dbpp dib section!\n")
                                TEXT("Image Size: %dx%d\n")
                                TEXT("BMP = 0x%x, Pixels = 0x%x");

            dwErrorCode = GetLastError();
            wsprintf(szBuf, szFormat, dwErrorCode, iBitCount, iWidth, iHeight, g_hBmpBuffer, g_pDibPixels);
            MessageBox(hWnd, szBuf, szTitle, MB_OK);
        }
        return 0;
    }
    g_hBmpOld = SelectObject(g_hdcBuffer, g_hBmpBuffer);
    {
        BITMAP bm;
        GetObject(g_hBmpBuffer, sizeof(BITMAP), &bm);
        g_iDibPitch = (int)bm.bmWidthBytes;
    }
    if (g_iDibPitch <= 0) g_iDibPitch = (iWidth * iBitCount + 7) / 8;
    g_iDibW = iWidth;
    g_iDibH = iHeight;
    return 1;
}

static void DestroyBackBuffer(void) {
    if (g_hdcBuffer != NULL) {
        if (g_hBmpOld != NULL) SelectObject(g_hdcBuffer, g_hBmpOld);
        if (g_hBmpBuffer != NULL) DeleteObject(g_hBmpBuffer);
        DeleteDC(g_hdcBuffer);
    }
    g_hdcBuffer = NULL; g_hBmpBuffer = NULL; g_hBmpOld = NULL;
    g_pDibPixels = NULL; g_iScreenBpp = 0; g_iDibPitch = 0;
    g_iDibW = 0; g_iDibH = 0;
}

/*====================================================
 * Paint canvas to window
 *   Scales canvas pixels by g_iCanvasScaleFactor
 *   into the full-resolution DIB, then BitBlt.
 *====================================================*/
static void PaintCanvas2Bpp() {
    int iX, iY, iX2, iY2;
    int iZoom = g_iCanvasScaleFactor;
    BYTE* pRow;
    if (iZoom < 1) iZoom = 1;
    for (iY = 0; iY < g_iCanvasH; ++iY) {
        for (iX = 0; iX < g_iCanvasW; ++iX) {
            int iColor = GetPixelCanvas(iX, iY);
            for (iY2 = 0; iY2 < iZoom; ++iY2) {
                int iDstY = iY * iZoom + iY2;
                int iDstX, iXEnd;
                unsigned char ucColor;
                if (iDstY >= g_iDibH) break;
                pRow = g_pDibPixels + iDstY * g_iDibPitch;
                iDstX = iX * iZoom;  
                iXEnd = iDstX + iZoom;  
                ucColor = (unsigned char)(iColor & 0x03);  
                for (iX2 = iDstX; iX2 < iXEnd && iX2 < g_iDibW; ++iX2) {  
                    int iByteIdx = iX2 >> 2;  
                    int iSrcShift = 6 - ((iX2 & 3) << 1);  
                    unsigned char ucMask = ~(0x03 << iSrcShift);  
                    pRow[iByteIdx] = (pRow[iByteIdx] & ucMask) | (ucColor << iSrcShift);  
                }
            }
        }
    }
}

static void PaintCanvas8Bpp() {
    int iX, iY, iX2, iY2;
    int iZoom = g_iCanvasScaleFactor;
    BYTE* pRow;
    if (iZoom < 1) iZoom = 1;
    for (iY = 0; iY < g_iCanvasH; ++iY) {
        for (iX = 0; iX < g_iCanvasW; ++iX) {
            int iColor = GetPixelCanvas(iX, iY);
            for (iY2 = 0; iY2 < iZoom; ++iY2) {
                int iDstY = iY * iZoom + iY2;
                if (iDstY >= g_iDibH) break;
                pRow = g_pDibPixels + iDstY * g_iDibPitch;
                for (iX2 = iX * iZoom; iX2 < (iX + 1) * iZoom && iX2 < g_iDibW; ++iX2) {
                    pRow[iX2] = (BYTE)iColor;
                }
            }
        }
    }
}

static void PaintCanvas16Bpp() {
    int iX, iY, iX2, iY2;
    int iZoom = g_iCanvasScaleFactor;
    BYTE* pRow;
    if (iZoom < 1) iZoom = 1;
    for (iY = 0; iY < g_iCanvasH; ++iY) {
        for (iX = 0; iX < g_iCanvasW; ++iX) {
            int iColor = GetPixelCanvas(iX, iY);
            for (iY2 = 0; iY2 < iZoom; ++iY2) {
                int iDstY = iY * iZoom + iY2;
                WORD* pDst16;
                WORD wColor;
                if (iDstY >= g_iDibH) break;
                pRow = g_pDibPixels + iDstY * g_iDibPitch;
                pDst16 = (WORD*)pRow;
                wColor = g_wPaletteRGB555[iColor];
                for (iX2 = iX * iZoom; iX2 < (iX + 1) * iZoom && iX2 < g_iDibW; ++iX2) {
                    pDst16[iX2] = wColor;
                }
            }
        }
    }
}

static void PaintCanvas32Bpp() {
    int iX, iY, iX2, iY2;
    int iZoom = g_iCanvasScaleFactor;
    BYTE* pRow;
    if (iZoom < 1) iZoom = 1;
    for (iY = 0; iY < g_iCanvasH; ++iY) {
        for (iX = 0; iX < g_iCanvasW; ++iX) {
            int iColor = GetPixelCanvas(iX, iY);
            for (iY2 = 0; iY2 < iZoom; ++iY2) {
                int iDstY = iY * iZoom + iY2;
                DWORD* pDst32;
                DWORD dwColor;
                if (iDstY >= g_iDibH) break;
                pRow = g_pDibPixels + iDstY * g_iDibPitch;
                pDst32 = (DWORD*)pRow;
                dwColor = g_dwPaletteRGB888[iColor];
                for (iX2 = iX * iZoom; iX2 < (iX + 1) * iZoom && iX2 < g_iDibW; ++iX2) {
                    pDst32[iX2] = dwColor;
                }
            }
        }
    }
}

static void PaintCanvasToWindow(HWND hWnd) {
    HDC hdc;
    PAINTSTRUCT ps;
    int iX, iY;
    hdc = BeginPaint(hWnd, &ps);
    if (g_pCanvas == NULL || g_iCanvasW <= 0 || g_iCanvasH <= 0) {
        EndPaint(hWnd, &ps); return;
    }
    if (g_pDibPixels != NULL && g_iScreenBpp != 0) {
        /* Fill DIB rows with scaled canvas pixels */
        switch (g_iScreenBpp) {
            case 2: {
                PaintCanvas2Bpp();
                break;
            }
            case 8:
                PaintCanvas8Bpp();
                break;
            case 16: {
                PaintCanvas16Bpp();
                break;
            }
            case 32: {
                PaintCanvas32Bpp();
                break;
            }
        }
        BitBlt(hdc, 0, g_iBarHeight, g_iDibW, g_iDibH, g_hdcBuffer, 0, 0, SRCCOPY);
    } else {
        int iZoom = g_iCanvasScaleFactor;
        if (iZoom < 1) iZoom = 1;
        for (iY = 0; iY < g_iDibH; ++iY)
            for (iX = 0; iX < g_iDibW; ++iX)
                SetPixel(hdc, iX, iY + g_iBarHeight,
                    g_rgbPalette[GetPixelCanvas(iX / iZoom, iY / iZoom)]);
    }
    EndPaint(hWnd, &ps);
}

/*====================================================
 * rz interface wrappers
 *====================================================*/
static void RzSetPixel(int x, int y) {
    SetPixelCanvas(x, y, COLOR_BLACK);
}

static void RzPlotLine(int x0, int y0, int x1, int y1) {
    DrawLineCanvas(x0, y0, x1, y1, COLOR_BLACK);
}

static void RzPutChar(int x, int y, unsigned char ch) {
    PutCharCanvas(x, y, ch, COLOR_BLACK);
}

/*====================================================
 * Draw surface wireframe onto the 2bpp canvas
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
                DrawLineCanvas(x0, y0, x1, y1, COLOR_BLACK);
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
                DrawLineCanvas(x0, y0, x1, y1, COLOR_BLACK);
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
            DrawLineCanvas(x0, y0, x1, y1, COLOR_BLACK);
            x0 = x1, y0 = y1;
        }
    }
    for (iU = 0; iU < Camera.uGrid; ++iU) {
        iV = 0;
        xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x0, &y0); 
        for (iV = 0; iV < Camera.vGrid; ++iV) {
            xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x1, &y1); 
            DrawLineCanvas(x0, y0, x1, y1, COLOR_BLACK);
            x0 = x1, y0 = y1;
        }
    }
}

/*====================================================
 * Bounding box / axes
 *====================================================*/
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
        DrawLineCanvas(x0, y0, x1, y1, COLOR_LIGHT_GRAY);
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
        DrawLineCanvas(x0, y0, x1, y1, COLOR_LIGHT_GRAY);
    }
}

/*====================================================
 * Full redraw (clear -> box -> surface -> footer)
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

static void Redraw(HWND hWnd) {
    int iBaseline;
    int iStartY, iLen;
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

    FillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);
    
    /* Box */
    if (g_bShowBox) {
        DrawBoxEdges(xyz2xy);
    }
    /* Axes */
    if (g_bShowAxes) {
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
    /* Formula */
    if (g_bShowFormula && g_pRenderNode != NULL) {
        iBaseline = g_pRenderNode->sLayout.iAscent;
        RenderNode_Draw(g_pRenderNode, &g_rzConfig, g_iFormulaX, g_iFormulaY + iBaseline);
    }

    /* Footer bar */
    if (g_bShowFooter) {
        char szBuf[64];
        BOOL bIsWideCanvas = g_iCanvasW >= 240;

        iStartY = g_iCanvasH - CURRENT_FONT_HEIGHT - 2;
        if (iStartY < 0) iStartY = 0;
        FillRectCanvas(0, iStartY, g_iCanvasW, CURRENT_FONT_HEIGHT + 2, COLOR_DARK_GRAY);

        /* Left: alpha / beta */
        if (bIsWideCanvas) {
            Salvia_Format(szBuf, "%c=%d,%c=%d",
                PZ_AE_GREEK_alpha, Camera.iAlphaDeg,
                PZ_AE_GREEK_beta,  Camera.iBetaDeg);
        }
        else {
            Salvia_Format(szBuf, "%d,%d", Camera.iAlphaDeg, Camera.iBetaDeg);
        }
        PutTextCanvas(2, iStartY + 2, (const unsigned char*)szBuf, COLOR_WHITE);

        /* Center: mode display */
        {
            static const char* szWideCenterText[] = { "CAMERA", "PAN MOVE", "ZOOM", "FORMULA", "FOV" };
            static const char* szNarrowCenterText[] = { "C", "P", "Z", "F", "V" };
            static const char* szProjection[] = { "ORTHO", "PERSP" };
            static const char* szNarrowProjection[] = { "O", "P" };
            const char* szMouseMode;
            char szCenter[30];

            if (bIsWideCanvas) {
                switch (g_iMouseMode) {
                    case MOUSE_MODE_CAMERA:
                    case MOUSE_MODE_PAN_MOVE:
                    case MOUSE_MODE_ZOOM:
                    case MOUSE_MODE_FORMULA:
                    case MOUSE_MODE_FOV:
                        szMouseMode = szWideCenterText[g_iMouseMode];
                        break;
                    default:
                        szMouseMode = "?";
                        break;
                }
            }
            else {
                switch (g_iMouseMode) {
                    case MOUSE_MODE_CAMERA:
                    case MOUSE_MODE_PAN_MOVE:
                    case MOUSE_MODE_ZOOM:
                    case MOUSE_MODE_FORMULA:
                    case MOUSE_MODE_FOV:
                        szMouseMode = szNarrowCenterText[g_iMouseMode];
                        break;
                    default:
                        szMouseMode = "?";
                        break;
                }
            }
            Salvia_Format(
                szCenter, "%s,%s",
                szMouseMode,
                (bIsWideCanvas ? szProjection : szNarrowProjection)[g_iProjection]
            );
            iLen = (int)strlen(szCenter) * CURRENT_FONT_WIDTH;
            PutTextCanvas((g_iCanvasW - iLen) / 2, iStartY + 2,
                (const unsigned char*)szCenter, COLOR_WHITE);
        }

        /* Right: zoom%, (viewportX, viewportY) */
        if (bIsWideCanvas) {
            Salvia_Format(szBuf, "%s%%(%d,%d)", szZoomLevels[Camera.iZoomLevel], Camera.iViewportX, Camera.iViewportY);
        }
        else {
            Salvia_Format(szBuf, "%s", szZoomLevels[Camera.iZoomLevel]);
        }
        iLen = (int)strlen(szBuf) * CURRENT_FONT_WIDTH;
        PutTextCanvas(g_iCanvasW - iLen - 2, iStartY + 2,
            (const unsigned char*)szBuf, COLOR_WHITE);
    }
    (void)hWnd;
}

/*====================================================
 * Draw idle screen (centered, multi-line intro)
 *====================================================*/
static void DrawIdleScreen(HWND hWnd) {
    static const char* szLines[] = {
        " \x17 Plotter-Z CE \x18 ",
        "",
        "A 3D function graph plotting tool",
        "Edit > Expression to start",
        "",
        "Maintained by \xC2\xBD\xC1 \xD5\xDC\xC0\xE6"
    };
    static const int iColors[] = {
        COLOR_BLACK,
        COLOR_BLACK,
        COLOR_DARK_GRAY,
        COLOR_DARK_GRAY,
        COLOR_BLACK,
        COLOR_LIGHT_GRAY
    };
    static const BOOL bReverse[] = { TRUE, FALSE, FALSE, FALSE, FALSE, TRUE };
    static const int iCount = sizeof(szLines) / sizeof(szLines[0]);
    static const int iPadding = 2;
    int iLine, iLen, x, y;
    int iTotalH;
    iTotalH = iCount * CURRENT_FONT_HEIGHT + 2 * iPadding;
    y = (g_iCanvasH - iTotalH) / 2;
    if (y < 0) y = 0;
    FillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);
    for (iLine = 0; iLine < iCount; ++iLine) {
        iLen = (int)strlen(szLines[iLine]) * CURRENT_FONT_WIDTH;
        x = (g_iCanvasW - iLen) / 2;
        if (x < 2) x = 2;
        if (bReverse[iLine]) {
            FillRectCanvas(
                x - iPadding,
                y - iPadding / 2,
                iLen + iPadding * 2,
                CURRENT_FONT_HEIGHT + iPadding,
                iColors[iLine]
            );
            PutTextCanvas(x, y, (const unsigned char*)szLines[iLine], COLOR_WHITE);
        } else {
            PutTextCanvas(x, y, (const unsigned char*)szLines[iLine], iColors[iLine]);
        }
        y += CURRENT_FONT_HEIGHT + iPadding;
    }
    InvalidateRect(hWnd, NULL, FALSE);
}

/*====================================================
 * Draw error screen (centered specific message)
 *====================================================*/
static void DrawErrorScreen(HWND hWnd) {
/* TODO: Draw Error */
}

/*====================================================
 * Draw screen
 *====================================================*/

static void DrawScreen(HWND hWnd) {
    switch (g_iStage) {
        case STAGE_ERROR:
            DrawErrorScreen(hWnd);
            break;
        case STAGE_IDLE:
            DrawIdleScreen(hWnd);
            break;
        case STAGE_READY:
            Redraw(hWnd);
            break;
    }
}

/*====================================================
 * Reset camera and formula position to defaults
 *====================================================*/
static void ResetView(HWND hWnd) {
    PzCamera_Reset(g_iCanvasW / 2, g_iCanvasH / 2);
    g_iFormulaX = 0;
    g_iFormulaY = 0;
    g_iZoomAccum = 0;
    Redraw(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
}

/*====================================================
 * Expression edit dialog procedure
 *   Auto-completes unbalanced parentheses on confirm.
 *   Re-parses and re-evaluates the expression.
 *====================================================*/
static LRESULT CALLBACK CartExprDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG: {
            /* Assign expression to edit control */
            TCHAR wszTemp[CART_EXPR_LENGTH];
            int i;
            for (i = 0; g_szCartExpr[i]; ++i) {
                wszTemp[i] = g_szCartExpr[i];
            }
            wszTemp[i] = 0;
            SetDlgItemText(hDlg, IDC_EXPR_EDIT, wszTemp);
            /* Center the dialog */
            CenterDialog(hDlg);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    TCHAR szBufT[CART_EXPR_LENGTH];
                    char szNew[CART_EXPR_LENGTH];

                    GetDlgItemText(hDlg, IDC_EXPR_EDIT, szBufT, CART_EXPR_LENGTH);

                    TCharToChar(szNew, szBufT, CART_EXPR_LENGTH);
                    BalanceParen(szNew, CART_EXPR_LENGTH);

                    /* Update expression and re-evaluate */
                    Utils_StringCopy(g_szCartExpr, CART_EXPR_LENGTH, szNew);

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/*====================================================
 * Expression edit dialog procedure
 *   Re-parses and re-evaluates the expression.
 *====================================================*/
static LRESULT CALLBACK ParmExprDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG: {
            /* Assign expression to edit control */
            TCHAR wszTemp[PARM_EXPR_LENGTH];
            CharToTChar(wszTemp, g_szParmExpr[0], PARM_EXPR_LENGTH);
            SetDlgItemText(hDlg, IDC_X_EDIT, wszTemp);
            CharToTChar(wszTemp, g_szParmExpr[1], PARM_EXPR_LENGTH);
            SetDlgItemText(hDlg, IDC_Y_EDIT, wszTemp);
            CharToTChar(wszTemp, g_szParmExpr[2], PARM_EXPR_LENGTH);
            SetDlgItemText(hDlg, IDC_Z_EDIT, wszTemp);
            /* Center the dialog */
            CenterDialog(hDlg);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    TCHAR szBufT[PARM_EXPR_LENGTH];
                    char szNew[PARM_EXPR_LENGTH];
                    static const int iControl[] = { IDC_X_EDIT, IDC_Y_EDIT, IDC_Z_EDIT };
                    int i;

                    for (i = 0; i < 3; ++i) {
                        GetDlgItemText(hDlg, iControl[i], szBufT, PARM_EXPR_LENGTH);
                        TCharToChar(szNew, szBufT, PARM_EXPR_LENGTH);
                        BalanceParen(szNew, PARM_EXPR_LENGTH);
                        Utils_StringCopy(g_szParmExpr[i], PARM_EXPR_LENGTH, szNew);
                    }

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/*====================================================
 * Window Editor dialog helpers
 *====================================================*/
static void SetDlgFloat(HWND hDlg, int id, PZ_FLOAT f) {
    TCHAR szBufT[32];
    char szBuf[32];
    int i;
    Utils_Ftoa((double)f, szBuf, 4);
    for (i = 0; szBuf[i] != '\0' && i < 31; ++i)
        szBufT[i] = (TCHAR)szBuf[i];
    szBufT[i] = TEXT('\0');
    SetDlgItemText(hDlg, id, szBufT);
}

static PZ_FLOAT getDlgFloat(HWND hDlg, int id) {
    TCHAR szBufT[64];
    char szBuf[64];
    int i;
    GetDlgItemText(hDlg, id, szBufT, 64);
    for (i = 0; szBufT[i] != TEXT('\0') && i < 63; ++i)
        szBuf[i] = (char)szBufT[i];
    szBuf[i] = '\0';
    return (PZ_FLOAT)Utils_Atof(szBuf);
}

static int getDlgInt(HWND hDlg, int id) {
    TCHAR szBufT[64];
    char szBuf[64];
    int i;
    GetDlgItemText(hDlg, id, szBufT, 64);
    for (i = 0; szBufT[i] != TEXT('\0') && i < 63; ++i)
        szBuf[i] = (char)szBufT[i];
    szBuf[i] = '\0';
    return (int)Utils_Atoi(szBuf);
}

/*====================================================
 * Window Editor dialog procedure
 *   Edit Camera x/y/z bounds and grid resolution.
 *====================================================*/
static LRESULT CALLBACK WindowEditorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG:
            SetDlgFloat(hDlg, IDC_WIN_XMIN, Camera.xMin);
            SetDlgFloat(hDlg, IDC_WIN_XMAX, Camera.xMax);
            SetDlgFloat(hDlg, IDC_WIN_YMIN, Camera.yMin);
            SetDlgFloat(hDlg, IDC_WIN_YMAX, Camera.yMax);
            SetDlgFloat(hDlg, IDC_WIN_ZMIN, Camera.zMin);
            SetDlgFloat(hDlg, IDC_WIN_ZMAX, Camera.zMax);
            SetDlgFloat(hDlg, IDC_WIN_UMIN, Camera.uMin);
            SetDlgFloat(hDlg, IDC_WIN_UMAX, Camera.uMax);
            SetDlgFloat(hDlg, IDC_WIN_VMIN, Camera.vMin);
            SetDlgFloat(hDlg, IDC_WIN_VMAX, Camera.vMax);
            {
                TCHAR szBufT[16];
                wsprintf(szBufT, TEXT("%d"), Camera.xGrid);
                SetDlgItemText(hDlg, IDC_WIN_XGRID, szBufT);
                wsprintf(szBufT, TEXT("%d"), Camera.yGrid);
                SetDlgItemText(hDlg, IDC_WIN_YGRID, szBufT);
                wsprintf(szBufT, TEXT("%d"), Camera.uGrid);
                SetDlgItemText(hDlg, IDC_WIN_UGRID, szBufT);
                wsprintf(szBufT, TEXT("%d"), Camera.vGrid);
                SetDlgItemText(hDlg, IDC_WIN_VGRID, szBufT);
            }
            CenterDialog(hDlg);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    HWND hWndParent;
                    int iGrid;
                    hWndParent = GetParent(hDlg);

                    Camera.xMin = getDlgFloat(hDlg, IDC_WIN_XMIN);
                    Camera.xMax = getDlgFloat(hDlg, IDC_WIN_XMAX);
                    Camera.yMin = getDlgFloat(hDlg, IDC_WIN_YMIN);
                    Camera.yMax = getDlgFloat(hDlg, IDC_WIN_YMAX);
                    Camera.zMin = getDlgFloat(hDlg, IDC_WIN_ZMIN);
                    Camera.zMax = getDlgFloat(hDlg, IDC_WIN_ZMAX);
                    Camera.uMin = getDlgFloat(hDlg, IDC_WIN_UMIN);
                    Camera.uMax = getDlgFloat(hDlg, IDC_WIN_UMAX);
                    Camera.vMin = getDlgFloat(hDlg, IDC_WIN_VMIN);
                    Camera.vMax = getDlgFloat(hDlg, IDC_WIN_VMAX);

                    iGrid = getDlgInt(hDlg, IDC_WIN_XGRID);
                    if (iGrid < 5) iGrid = 5;
                    if (iGrid > X_GRID_MAX) iGrid = X_GRID_MAX;
                    Camera.xGrid = iGrid;

                    iGrid = getDlgInt(hDlg, IDC_WIN_YGRID);
                    if (iGrid < 5) iGrid = 5;
                    if (iGrid > Y_GRID_MAX) iGrid = Y_GRID_MAX;
                    Camera.yGrid = iGrid;

                    iGrid = getDlgInt(hDlg, IDC_WIN_UGRID);
                    if (iGrid < 5) iGrid = 5;
                    if (iGrid > U_GRID_MAX) iGrid = U_GRID_MAX;
                    Camera.uGrid = iGrid;

                    iGrid = getDlgInt(hDlg, IDC_WIN_VGRID);
                    if (iGrid < 5) iGrid = 5;
                    if (iGrid > V_GRID_MAX) iGrid = V_GRID_MAX;
                    Camera.vGrid = iGrid;

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/*====================================================
 * Samples dialog procedure
 *====================================================*/
static LRESULT CALLBACK SampleDlgProc(HWND hDlg, UINT message,
                                       WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG: {
            int i, iCount;
            iCount = sizeof(PlotterZSamples) / sizeof(PlotterZSamples[0]);
            for (i = 0; i < iCount; ++i) {
                TCHAR szBufT[CART_EXPR_LENGTH];
                const char* szSrc;
                int j;
                szSrc = PlotterZSamples[i].szName;
                for (j = 0; szSrc[j] != '\0' && j < CART_EXPR_LENGTH - 1; ++j)
                    szBufT[j] = (TCHAR)szSrc[j];
                szBufT[j] = TEXT('\0');
                SendDlgItemMessage(hDlg, IDC_SAMPLE_LIST,
                    LB_ADDSTRING, 0, (LPARAM)szBufT);
            }
            CenterDialog(hDlg);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    LRESULT iSel;
                    iSel = SendDlgItemMessage(hDlg, IDC_SAMPLE_LIST, LB_GETCURSEL, 0, 0);
                    if (iSel == LB_ERR)
                        EndDialog(hDlg, -1);
                    else
                        EndDialog(hDlg, (int)iSel);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hDlg, -1);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/*====================================================
 * Hex color helpers for Preferences dialog
 *====================================================*/
static COLORREF ParseHexColor(const char* szHex) {
    const char* p;
    long lVal;
    p = szHex;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
    lVal = 0;
    while (*p != '\0') {
        int d;
        if      (*p >= '0' && *p <= '9') d = *p - '0';
        else if (*p >= 'a' && *p <= 'f') d = *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F') d = *p - 'A' + 10;
        else break;
        lVal = lVal * 16 + d;
        ++p;
    }
    lVal &= 0xFFFFFF;
    /* Convert RRGGBB (input) to COLORREF BBGGRR */
    lVal = ((lVal & 0xFF) << 16) | (lVal & 0x00FF00) | ((lVal >> 16) & 0xFF);
    return (COLORREF)lVal;
}

static void setColorEdit(HWND hDlg, int idCtrl, COLORREF color) {
    TCHAR szBuf[16];
    unsigned long ulVal;
    ulVal = color & 0xFFFFFF;
    /* COLORREF BBGGRR -> RRGGBB for display */
    ulVal = ((ulVal & 0xFF) << 16) | (ulVal & 0x00FF00) | ((ulVal >> 16) & 0xFF);
    wsprintf(szBuf, TEXT("%06lx"), ulVal);
    SetDlgItemText(hDlg, idCtrl, szBuf);
}

/*====================================================
 * Preferences dialog procedure
 *====================================================*/
typedef struct {
    const char*   szName;
    const TCHAR*  szColors[4];
} Theme;

static const Theme g_sThemes[] = {
    { "Plain",      { TEXT("000000"), TEXT("666666"), TEXT("aaaaaa"), TEXT("ffffff") } },
    { "The Matrix", { TEXT("33ff33"), TEXT("229922"), TEXT("00CD00"), TEXT("000000") } },
    { "VT100",      { TEXT("eeeeee"), TEXT("CDCD00"), TEXT("115511"), TEXT("111111") } },
    { "Gameboy",    { TEXT("0d1400"), TEXT("354f00"), TEXT("5c8b00"), TEXT("84c600") } },
    { "GB Orange",  { TEXT("180b00"), TEXT("622c00"), TEXT("ab4c00"), TEXT("f46d00") } },
    { "GB Yellow",  { TEXT("181400"), TEXT("615200"), TEXT("a98f00"), TEXT("f2cc00") } },
    { "GB Dark",    { TEXT("000e00"), TEXT("003800"), TEXT("006220"), TEXT("008c20") } }
};
static const int g_iNumThemes = sizeof(g_sThemes) / sizeof(g_sThemes[0]);

static void applyThemeToEdits(HWND hDlg, const Theme* pTheme) {
    int i;
    for (i = 0; i < 4; ++i)
        SetDlgItemText(hDlg, IDC_PREF_COLOR0 + i, pTheme->szColors[i]);
}

static LRESULT CALLBACK PrefDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG: {
            int i;
            for (i = 0; i < 4; ++i)
                setColorEdit(hDlg, IDC_PREF_COLOR0 + i, g_rgbPalette[i]);
            if (g_iCanvasScaleFactor == 2)
                SendMessage(GetDlgItem(hDlg, IDC_PREF_LOWRES), BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
            CenterDialog(hDlg);
            for (i = 0; i < g_iNumThemes; ++i) {
                TCHAR szBuf[64];
                int j;
                const char* szSrc = g_sThemes[i].szName;
                for (j = 0; szSrc[j] != '\0' && j < 63; ++j)
                    szBuf[j] = (TCHAR)szSrc[j];
                szBuf[j] = TEXT('\0');
                SendDlgItemMessage(hDlg, IDC_THEMES_LIST, LB_ADDSTRING, 0, (LPARAM)szBuf);
            }
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_INVERT_BUTTON: {
                    TCHAR szBuf0[16], szBuf1[16], szBuf2[16], szBuf3[16];
                    GetDlgItemText(hDlg, IDC_PREF_COLOR0, szBuf0, 16);
                    GetDlgItemText(hDlg, IDC_PREF_COLOR1, szBuf1, 16);
                    GetDlgItemText(hDlg, IDC_PREF_COLOR2, szBuf2, 16);
                    GetDlgItemText(hDlg, IDC_PREF_COLOR3, szBuf3, 16);
                    SetDlgItemText(hDlg, IDC_PREF_COLOR0, szBuf3);
                    SetDlgItemText(hDlg, IDC_PREF_COLOR1, szBuf2);
                    SetDlgItemText(hDlg, IDC_PREF_COLOR2, szBuf1);
                    SetDlgItemText(hDlg, IDC_PREF_COLOR3, szBuf0);
                    return TRUE;
                }
                case IDC_THEMES_LIST:
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        LRESULT iSel;
                        iSel = SendDlgItemMessage(hDlg, IDC_THEMES_LIST, LB_GETCURSEL, 0, 0);
                        if (iSel >= 0 && iSel < g_iNumThemes)
                            applyThemeToEdits(hDlg, &g_sThemes[iSel]);
                    }
                    return TRUE;
                case IDOK: {
                    int i, iPrevScale;
                    TCHAR szBuf[16];
                    HWND hWndParent;
                    hWndParent = GetParent(hDlg);
                    for (i = 0; i < 4; ++i) {
                        char szHex[16];
                        int j;
                        GetDlgItemText(hDlg, IDC_PREF_COLOR0 + i, szBuf, 16);
                        for (j = 0; szBuf[j] != TEXT('\0') && j < 15; ++j)
                            szHex[j] = (char)szBuf[j];
                        szHex[j] = '\0';
                        g_rgbPalette[i] = ParseHexColor(szHex);
                    }
                    RecalcPaletteLUTs();
                    iPrevScale = g_iCanvasScaleFactor;
                    g_iCanvasScaleFactor = (SendMessage(GetDlgItem(hDlg, IDC_PREF_LOWRES), BM_GETCHECK, 0, 0)== BST_CHECKED) ? 2 : 1;
                    if (
                        g_iCanvasScaleFactor != iPrevScale ||
                        /* Workaround: force canvas recreation when bpp equals 8 */
                        g_iScreenBpp == 8
                    ) {
                        ReCreateCanvasAndBackBuffer(hWndParent, iPrevScale);
                    }
                    if (g_iStage == STAGE_READY) {
                        Redraw(hWndParent);
                    } else if (g_iStage == STAGE_ERROR) {
                        DrawErrorScreen(hWndParent);
                    } else {
                        DrawIdleScreen(hWndParent);
                    }
                    InvalidateRect(hWndParent, NULL, FALSE);
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/*====================================================
 * Refresh threshold dialog procedure
 *====================================================*/
static LRESULT CALLBACK RefreshDlgProc(HWND hDlg, UINT message,
                                        WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG: {
            TCHAR szBuf[16];
            SendDlgItemMessage(hDlg, IDC_THRESHOLD_SLIDER, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(20, 300));
            SendDlgItemMessage(hDlg, IDC_THRESHOLD_SLIDER, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)g_iMouseRefreshMs);
            wsprintf(szBuf, TEXT("%d"), g_iMouseRefreshMs);
            SetDlgItemText(hDlg, IDC_THRESHOLD_EDIT, szBuf);
            return TRUE;
        }
        case WM_HSCROLL: {
            int iPos;
            TCHAR szBuf[16];
            iPos = (int)SendDlgItemMessage(hDlg, IDC_THRESHOLD_SLIDER, TBM_GETPOS, 0, 0);
            wsprintf(szBuf, TEXT("%d"), iPos);
            SetDlgItemText(hDlg, IDC_THRESHOLD_EDIT, szBuf);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_THRESHOLD_EDIT:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        TCHAR szBuf[16];
                        char szAsc[16];
                        int iVal, j;
                        GetDlgItemText(hDlg, IDC_THRESHOLD_EDIT, szBuf, 16);
                        for (j = 0; szBuf[j] != TEXT('\0') && j < 15; ++j)
                            szAsc[j] = (char)szBuf[j];
                        szAsc[j] = '\0';
                        iVal = (int)Utils_Atoi(szAsc);
                        if (iVal < 20) iVal = 20;
                        if (iVal > 300) iVal = 300;
                        SendDlgItemMessage(hDlg, IDC_THRESHOLD_SLIDER,
                            TBM_SETPOS, (WPARAM)TRUE, (LPARAM)iVal);
                    }
                    return TRUE;
                case IDOK: {
                    TCHAR szBuf[16];
                    char szAsc[16];
                    int iVal, j;
                    GetDlgItemText(hDlg, IDC_THRESHOLD_EDIT, szBuf, 16);
                    for (j = 0; szBuf[j] != TEXT('\0') && j < 15; ++j)
                        szAsc[j] = (char)szBuf[j];
                    szAsc[j] = '\0';
                    iVal = (int)Utils_Atoi(szAsc);
                    if (iVal < 20) iVal = 20;
                    if (iVal > 300) iVal = 300;
                    g_iMouseRefreshMs = iVal;
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/*====================================================
 * Debug Mode dialog procedure
 *====================================================*/
static LRESULT CALLBACK DebugDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG: {
            char szBuf[256];
            TCHAR szBufT[256];
            int i;

            /* Debug info */
            wsprintf(
                szBufT,
                TEXT("Resolution: %dx%d\r\nScale: %d\r\nBPP: %d\r\nBarHeight: %d\r\nCanvas: 0x%x\r\nBuffer: 0x%x"),
                g_iDibW,
                g_iDibH,
                g_iCanvasScaleFactor,
                g_iScreenBpp,
                g_iBarHeight,
                g_pCanvas,
                g_hdcBuffer
            );
            SetDlgItemText(hDlg, IDC_DEBUG_INFO, szBufT);

            /* VM instruction list */
            if (g_pVmCartZ != NULL) {
                for (i = 0; i < g_pVmCartZ->iInstructionLength; ++i) {
                    EzInstruction* pInst = g_pVmCartZ->pInstructions + i;
                    switch (pInst->iOpCode) {
                        case EZOP_PUSH_IMD:
                            Salvia_Format(szBuf, "[%3d] %s %.3f", i, EzOpCode_GetName(pInst->iOpCode), (double)pInst->uData.fImmediate);
                            break;
                        case EZOP_PUSH_VAR:
                            Salvia_Format(szBuf, "[%3d] %s %d", i, EzOpCode_GetName(pInst->iOpCode), pInst->uData.iVarIndex);
                            break;
                        case EZOP_FUNC: {
                            const PzFuncMeta* pMeta;
                            pMeta = Constant_GetFunctionMetadataByIndex((PzFuncIndex)pInst->uData.iFuncIndex);
                            Salvia_Format(szBuf, "[%3d] %s %s", i, EzOpCode_GetName(pInst->iOpCode), pMeta ? pMeta->szName : "?");
                            break;
                        }
                        default:
                            Salvia_Format(szBuf, "[%3d] %s", i, EzOpCode_GetName(pInst->iOpCode));
                            break;
                    }
                    CharToTChar(szBufT, szBuf, 256);
                    SendDlgItemMessage(hDlg, IDC_DEBUG_VMLIST, LB_ADDSTRING, 0, (LPARAM)szBufT);
                }
            }
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                case IDC_DEBUG_PERFTEST:
                    EndDialog(hDlg, IDC_DEBUG_PERFTEST);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

static void StartDebugDialog(HWND hWnd) {
    int iRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_DEBUG), hWnd, (DLGPROC)DebugDlgProc);
    if (iRet == IDC_DEBUG_PERFTEST) {
        /* Reset to defaults */
        PzCamera_Reset(g_iCanvasW / 2, g_iCanvasH / 2);
        Camera.iViewportS = (g_iCanvasH < g_iCanvasW ? g_iCanvasH : g_iCanvasW) / 2;
        Utils_StringCopy(g_szCartExpr, CART_EXPR_LENGTH, DEFAULT_CART_EXPR);
        g_iFormulaX = 0;
        g_iFormulaY = 0;
        g_iFuncType = FUNC_TYPE_CARTESIAN;
        ParseExpr();
        if (RecalcSurface(hWnd)) {
            /* Start performance test */
            g_bPerformanceTest = TRUE;
            g_nPaintCount = 0;
            g_dwPerfStartTime = GetTickCount();
        }
        DrawScreen(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

/*====================================================
 * Draw progress bar
 *   barY   - top Y coordinate of the bar
 *   iPct   - progress percentage (0-100)
 *====================================================*/
static void DrawProgressBar(int barY, int iPct) {
    int barW, barH, barX;
    int iFillW;
    char szBuf[16];
    int iLen, tx, ty;

    barW = g_iCanvasW / 2;
    barH = CURRENT_FONT_HEIGHT * 2;
    barX = (g_iCanvasW - barW) / 2;
    if (barX < 0) barX = 0;

    /* Border */
    DrawLineCanvas(barX, barY, barX + barW - 1, barY, COLOR_BLACK);
    DrawLineCanvas(barX, barY + barH - 1, barX + barW - 1,
                   barY + barH - 1, COLOR_BLACK);
    DrawLineCanvas(barX, barY, barX, barY + barH - 1, COLOR_BLACK);
    DrawLineCanvas(barX + barW - 1, barY, barX + barW - 1,
                   barY + barH - 1, COLOR_BLACK);

    /* Interior fill WHITE */
    FillRectCanvas(barX + 1, barY + 1, barW - 2, barH - 2, COLOR_WHITE);

    /* Percentage text centered */
    Salvia_Format(szBuf, "%d%%", iPct);
    iLen = (int)strlen(szBuf) * CURRENT_FONT_WIDTH;
    tx = barX + (barW - iLen) / 2;
    ty = barY + (barH - CURRENT_FONT_HEIGHT) / 2;
    PutTextCanvas(tx, ty, (const unsigned char*)szBuf, COLOR_BLACK);

    /* Invert filled portion */
    iFillW = barW * iPct / 100;
    if (iFillW > barW - 2) iFillW = barW - 2;
    if (iFillW > 0)
        InvertRectCanvas(barX + 1, barY + 1, iFillW, barH - 2);
}

/*====================================================
 * Recalculate surface geometry
 *====================================================*/
static void DrawRecalcProgressFirstFrame(HWND hWnd) {
    int y;
    FillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);
    y = g_iCanvasH / 2 - CURRENT_FONT_HEIGHT - 4;
    if (y < 0) y = 0;
    {
        int iLen = 10 * CURRENT_FONT_WIDTH;
        int x = (g_iCanvasW - iLen) / 2;
        if (x < 2) x = 2;
        PutTextCanvas(x, y, (const unsigned char*)"Recalc ...", COLOR_BLACK);
    }
    DrawProgressBar(y + CURRENT_FONT_HEIGHT + 4, 0);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static void DrawRecalcProgress(HWND hWnd, int iPct) {
    MSG msg;
    int y;
    y = g_iCanvasH / 2 - CURRENT_FONT_HEIGHT - 4;
    if (y < 0) y = 0;
    DrawProgressBar(y + CURRENT_FONT_HEIGHT + 4, iPct);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void RecalcCartesian(HWND hWnd) {
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
        DrawRecalcProgress(hWnd, (iX + 1) * 100 / Camera.xGrid);
    }

    /* Normalize x and y grid coordinates into [-1, 1] */
    for (iX = 0; iX < Camera.xGrid; ++iX) {
        g_fCartXBuf[iX] = NUM_VAL(2.0f * (fX[iX] - (Camera.xMax + Camera.xMin) / 2.0f) / (Camera.xMax - Camera.xMin));
    }
    for (iY = 0; iY < Camera.yGrid; ++iY) {
        g_fCartYBuf[iY] = NUM_VAL(2.0f * (fY[iY] - (Camera.yMax + Camera.yMin) / 2.0f) / (Camera.yMax - Camera.yMin));
    }
}

void RecalcParametric(HWND hWnd) {
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
        DrawRecalcProgress(hWnd, (iU + 1) * 100 / Camera.uGrid);
    }
}

int RecalcSurface(HWND hWnd) {
    DrawRecalcProgressFirstFrame(hWnd);
    switch (g_iFuncType) {
        case FUNC_TYPE_CARTESIAN:
            RecalcCartesian(hWnd);
            break;
        case FUNC_TYPE_PARAMETRIC:
            RecalcParametric(hWnd);
            break;
    }
    g_iStage = STAGE_READY;
    return 1;
}

/*====================================================
 * Save session to INI file
 *====================================================*/
static BOOL SaveSession(HWND hWnd) {
    OPENFILENAME ofn;
    TCHAR szFile[MAX_PATH];
    char szIni[4096];
    int iPos;
    HANDLE hFile;
    DWORD dwWritten;

    memset(&ofn, 0, sizeof(ofn));
    szFile[0] = TEXT('\0');
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = hWnd;
    ofn.lpstrFilter = TEXT("INI Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0");
    ofn.lpstrFile   = szFile;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = TEXT("ini");
    if (!GetSaveFileName(&ofn)) return FALSE;

    iPos = 0;
    iPos += Salvia_Format(szIni + iPos,
        "[function]\r\ntype=%d\r\n", g_iFuncType);
    iPos += Salvia_Format(szIni + iPos,
        "[cart_expr]\r\nz=%s\r\n", g_szCartExpr);
    iPos += Salvia_Format(szIni + iPos, "[parm_expr]\r\n");
    iPos += Salvia_Format(szIni + iPos, "x=%s\r\n", g_szParmExpr[0]);
    iPos += Salvia_Format(szIni + iPos, "y=%s\r\n", g_szParmExpr[1]);
    iPos += Salvia_Format(szIni + iPos, "z=%s\r\n", g_szParmExpr[2]);
    iPos += Salvia_Format(szIni + iPos,
        "[camera]\r\n"
        "alpha=%d\r\nbeta=%d\r\n"
        "xmin=%.4f\r\nxmax=%.4f\r\n"
        "ymin=%.4f\r\nymax=%.4f\r\n"
        "zmin=%.4f\r\nzmax=%.4f\r\n"
        "umin=%.4f\r\numax=%.4f\r\n"
        "vmin=%.4f\r\nvmax=%.4f\r\n"
        "xgrid=%d\r\nygrid=%d\r\n"
        "ugrid=%d\r\nvgrid=%d\r\n"
        "zoom=%d\r\n"
        "fov=%d\r\n"
        "viewx=%d\r\nviewy=%d\r\n"
        "exprx=%d\r\nexpry=%d\r\n",
        Camera.iAlphaDeg, Camera.iBetaDeg,
        Camera.xMin, Camera.xMax,
        Camera.yMin, Camera.yMax,
        Camera.zMin, Camera.zMax,
        Camera.uMin, Camera.uMax,
        Camera.vMin, Camera.vMax,
        Camera.xGrid, Camera.yGrid,
        Camera.uGrid, Camera.vGrid,
        Camera.iZoomLevel,
        Camera.iFovLevel,
        Camera.iViewportX * g_iCanvasScaleFactor,
        Camera.iViewportY * g_iCanvasScaleFactor,
        g_iFormulaX * g_iCanvasScaleFactor,
        g_iFormulaY * g_iCanvasScaleFactor);
    iPos += Salvia_Format(szIni + iPos,
        "[palette]\r\n"
        "black=%d\r\n"
        "darkgray=%d\r\n"
        "lightgray=%d\r\n"
        "white=%d\r\n",
        (unsigned long)(g_rgbPalette[COLOR_BLACK] & 0xFFFFFF),
        (unsigned long)(g_rgbPalette[COLOR_DARK_GRAY] & 0xFFFFFF),
        (unsigned long)(g_rgbPalette[COLOR_LIGHT_GRAY] & 0xFFFFFF),
        (unsigned long)(g_rgbPalette[COLOR_WHITE] & 0xFFFFFF));

    iPos += Salvia_Format(szIni + iPos,
        "[display]\r\n"
        "projection=%d\r\n"
        "formula=%d\r\n"
        "box=%d\r\n"
        "axes=%d\r\n"
        "footer=%d\r\n",
        g_iProjection,
        g_bShowFormula,
        g_bShowBox,
        g_bShowAxes,
        g_bShowFooter
    );

    hFile = CreateFile(szFile, GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    WriteFile(hFile, szIni, (DWORD)strlen(szIni), &dwWritten, NULL);
    CloseHandle(hFile);

    return TRUE;
}

/*====================================================
 * Load session from INI file
 *====================================================*/
static BOOL LoadSession(HWND hWnd) {
    OPENFILENAME ofn;
    TCHAR szFile[MAX_PATH];
    HANDLE hFile;
    DWORD dwSize, dwRead;
    char* pBuf;
    PineIniFile* pIni;
    PineIniError iniErr;
    PineIniSection* pSec;

    memset(&ofn, 0, sizeof(ofn));
    szFile[0] = TEXT('\0');
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = hWnd;
    ofn.lpstrFilter = TEXT("INI Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0");
    ofn.lpstrFile   = szFile;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = TEXT("ini");
    if (!GetOpenFileName(&ofn)) return FALSE;

    hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    dwSize = GetFileSize(hFile, NULL);
    if (dwSize == INVALID_FILE_SIZE || dwSize > 8191)
        { CloseHandle(hFile); return FALSE; }

    pBuf = (char*)malloc((size_t)(dwSize + 1));
    if (pBuf == NULL) { CloseHandle(hFile); return FALSE; }

    if (!ReadFile(hFile, pBuf, dwSize, &dwRead, NULL) || dwRead != dwSize) {
        free(pBuf); CloseHandle(hFile); return FALSE;
    }
    CloseHandle(hFile);
    pBuf[dwSize] = '\0';

    pIni = PineIni_Parse(pBuf, &iniErr);
    free(pBuf);
    if (pIni == NULL) return FALSE;

    /* Parse [function] */
    pSec = PineIni_Find(pIni, "function");
    if (pSec != NULL) {
        g_iFuncType = PineIni_Section_GetInt(pSec, "type", g_iFuncType);
    }
    
    /* Parse [cart_expr] */
    pSec = PineIni_Find(pIni, "cart_expr");
    if (pSec != NULL) {
        Utils_StringCopy(g_szCartExpr, CART_EXPR_LENGTH, PineIni_Section_GetString(pSec, "z", g_szCartExpr));
    }

    /* Parse [parm_expr] */
    pSec = PineIni_Find(pIni, "parm_expr");
    if (pSec != NULL) {
        Utils_StringCopy(g_szParmExpr[0], PARM_EXPR_LENGTH, PineIni_Section_GetString(pSec, "x", g_szParmExpr[0]));
        Utils_StringCopy(g_szParmExpr[1], PARM_EXPR_LENGTH, PineIni_Section_GetString(pSec, "y", g_szParmExpr[1]));
        Utils_StringCopy(g_szParmExpr[2], PARM_EXPR_LENGTH, PineIni_Section_GetString(pSec, "z", g_szParmExpr[2]));
    }

    /* Parse [camera] */
    pSec = PineIni_Find(pIni, "camera");
    if (pSec != NULL) {
        Camera.iAlphaDeg  = PineIni_Section_GetInt(pSec, "alpha", Camera.iAlphaDeg);
        Camera.iBetaDeg   = PineIni_Section_GetInt(pSec, "beta", Camera.iBetaDeg);
        Camera.xMin = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "xmin", "-6"));
        Camera.xMax = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "xmax", "6"));
        Camera.yMin = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "ymin", "-6"));
        Camera.yMax = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "ymax", "6"));
        Camera.zMin = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "zmin", "-3"));
        Camera.zMax = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "zmax", "3"));
        Camera.uMin = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "umin", "0"));
        Camera.uMax = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "umax", "6.2832"));
        Camera.vMin = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "vmin", "0"));
        Camera.vMax = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "vmax", "6.2832"));
        Camera.xGrid = PineIni_Section_GetInt(pSec, "xgrid", SAMPLE_DEFAULT_GRID);
        Camera.yGrid = PineIni_Section_GetInt(pSec, "ygrid", SAMPLE_DEFAULT_GRID);
        Camera.xGrid = PineIni_Section_GetInt(pSec, "ugrid", SAMPLE_DEFAULT_GRID);
        Camera.yGrid = PineIni_Section_GetInt(pSec, "vgrid", SAMPLE_DEFAULT_GRID);
        Camera.iZoomLevel = PineIni_Section_GetInt(pSec, "zoom", Camera.iZoomLevel);
        Camera.iFovLevel = PineIni_Section_GetInt(pSec, "fov", Camera.iFovLevel);
        Camera.iViewportX = PineIni_Section_GetInt(pSec, "viewx", Camera.iViewportX);
        Camera.iViewportY = PineIni_Section_GetInt(pSec, "viewy", Camera.iViewportY);
        g_iFormulaX = PineIni_Section_GetInt(pSec, "exprx", g_iFormulaX);
        g_iFormulaY = PineIni_Section_GetInt(pSec, "expry", g_iFormulaY);
        if (g_iCanvasScaleFactor > 1) {
            Camera.iViewportX /= g_iCanvasScaleFactor;
            Camera.iViewportY /= g_iCanvasScaleFactor;
            g_iFormulaX      /= g_iCanvasScaleFactor;
            g_iFormulaY      /= g_iCanvasScaleFactor;
        }
    }

    /* Parse [palette] */
    pSec = PineIni_Find(pIni, "palette");
    if (pSec != NULL) {
        static const char* szKeys[] = { "black", "darkgray", "lightgray", "white" };
        int i;
        for (i = 0; i < 4; ++i) {
            const char* szVal;
            szVal = PineIni_Section_GetString(pSec, szKeys[i], NULL);
            if (szVal != NULL)
                g_rgbPalette[i] = (COLORREF)((int)Utils_Atoi(szVal) & 0xFFFFFF);
        }
    }

    /* Parse [display] */
    pSec = PineIni_Find(pIni, "display");
    if (pSec != NULL) {
        g_iProjection = PineIni_Section_GetInt(pSec, "projection", g_iProjection);
        g_bShowFormula = PineIni_Section_GetInt(pSec, "formula", g_bShowFormula);
        g_bShowBox = PineIni_Section_GetInt(pSec, "box", g_bShowBox);
        g_bShowAxes = PineIni_Section_GetInt(pSec, "axes", g_bShowAxes);
        g_bShowFooter = PineIni_Section_GetInt(pSec, "footer", g_bShowFooter);
    }

    RecalcPaletteLUTs();
    /* Workaround: force canvas recreation when bpp equals 8 */
    if (g_iScreenBpp == 8) {
        ReCreateCanvasAndBackBuffer(hWnd, g_iCanvasScaleFactor);
    };
    
    PineIni_Destroy(pIni);

    return TRUE;
}

/*====================================================
 * Win32 standard boilerplate
 *====================================================*/
BOOL                SetTaskbarVisible   (BOOL bVisible);
void                ToggleFullscreen    (HWND hWnd);
ATOM                MyRegisterClass     (HINSTANCE hInstance, LPTSTR szWindowClass);
BOOL                InitInstance        (HINSTANCE, int);
LRESULT CALLBACK    WndProc             (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    About               (HWND, UINT, WPARAM, LPARAM);

struct {
    BOOL bFullScreen;
    int iX, iY, iW, iH;
    int iBarHeight;
} PrevWindowSize;

int WINAPI WinMain( HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR    lpCmdLine,
                    int       nCmdShow) {
    MSG msg;
    HACCEL hAccelTable;

    LoadApplicationConfig();

    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_PLOTTERZNATIVE);

    while (GetMessage(&msg, NULL, 0, 0))  {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass) {
#if VER_PLATFORM_WIN32_CE
    WNDCLASS    wc;

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc        = (WNDPROC) WndProc;
    wc.cbClsExtra        = 0;
    wc.cbWndExtra        = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PLOTTERZNATIVE));
    wc.hCursor            = 0;
    wc.hbrBackground    = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName        = 0;
    wc.lpszClassName    = szWindowClass;

    return RegisterClass(&wc);
#else
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX); 

    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, (LPCTSTR)IDI_PLOTTERZNATIVE);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = (LPCSTR)IDM_MENU;
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm        = NULL;

    return RegisterClassEx(&wcex);
#endif
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    HWND    hWnd;
    TCHAR    szTitle[MAX_LOADSTRING];
    TCHAR    szWindowClass[MAX_LOADSTRING];

    hInst = hInstance;

    LoadString(hInstance, IDC_PLOTTERZNATIVE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance, szWindowClass);

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
#if VER_PLATFORM_WIN32_CE
    hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
        0, 0, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
#else
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        0, 0, 640, 480, NULL, NULL, hInstance, NULL);
#endif

    if (!hWnd) {    
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
#if VER_PLATFORM_WIN32_CE
    if (hwndCB) {
        CommandBar_Show(hwndCB, TRUE);
    }
#else
    {
        int iScrWidth, iScrHeight;
        RECT rectWin;
        iScrWidth = GetSystemMetrics(SM_CXSCREEN);
        iScrHeight = GetSystemMetrics(SM_CYSCREEN);
        GetWindowRect(hWnd, &rectWin);
        SetWindowPos(
            hWnd,
            HWND_TOP,
            (iScrWidth - (rectWin.right - rectWin.left)) / 2,
            (iScrHeight - (rectWin.bottom - rectWin.top)) / 2,
            0, 0,
            SWP_NOSIZE
        );
    }
#endif

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int wmId, wmEvent;
    switch (message) {
        case WM_COMMAND:
            wmId = LOWORD(wParam); wmEvent = HIWORD(wParam);
            switch (wmId) {
                case IDM_HELP_ABOUT:
                    DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
                    break;
                case IDM_HELP_DEBUG:
                    StartDebugDialog(hWnd);
                    break;
                case IDM_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;
                case IDM_FILE_SAVESESSION:
                    if (SaveSession(hWnd)) {
                        MessageBox(hWnd, TEXT("Session Saved"), TEXT("Session"), MB_OK);
                    }
                    break;
                case IDM_FILE_LOADSESSION:
                    if (LoadSession(hWnd)) {
                        ParseExpr();
                        RecalcSurface(hWnd);
                        DrawScreen(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateDisplayMenuCheckStatus(hWnd);
                        UpdateViewMenuCheckStatus(hWnd);
                    }
                    break;
                case IDM_FILE_SAMPLES: {
                    int iSel;
                    iSel = DialogBox(hInst, MAKEINTRESOURCE(IDD_SAMPLES),
                                     hWnd, (DLGPROC)SampleDlgProc);
                    if (iSel >= 0) {
                        const PzSample* p;
                        p = &PlotterZSamples[iSel];
                        Camera.xMin  = p->xMin;
                        Camera.xMax  = p->xMax;
                        Camera.yMin  = p->yMin;
                        Camera.yMax  = p->yMax;
                        Camera.zMin  = p->zMin;
                        Camera.zMax  = p->zMax;
                        Camera.uMin  = p->uMin;
                        Camera.uMax  = p->uMax;
                        Camera.vMin  = p->vMin;
                        Camera.vMax  = p->vMax;
                        Camera.xGrid = SAMPLE_DEFAULT_GRID;
                        Camera.yGrid = SAMPLE_DEFAULT_GRID;
                        Camera.uGrid = SAMPLE_DEFAULT_GRID;
                        Camera.uGrid = SAMPLE_DEFAULT_GRID;
                        switch (p->iFuncType) {
                            case FUNC_TYPE_CARTESIAN:
                                Utils_StringCopy(g_szCartExpr, CART_EXPR_LENGTH, p->szExpr[0]);
                                break;
                            case FUNC_TYPE_PARAMETRIC: {
                                int i;
                                for (i = 0; i < 3; ++i) {
                                    Utils_StringCopy(g_szParmExpr[i], CART_EXPR_LENGTH, p->szExpr[i]);
                                }
                                break;
                            }
                        }
                        g_iFuncType = p->iFuncType;
                        ParseExpr();
                        RecalcSurface(hWnd);
                        DrawScreen(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                }
                break;
                case IDM_FILE_PREFERENCES:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_PREFERENCES),
                              hWnd, (DLGPROC)PrefDlgProc);
                    break;
                case IDM_FILE_REFRESH_THRESHOLD:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_REFRESH_THRESHOLD),
                              hWnd, (DLGPROC)RefreshDlgProc);
                    break;
                case IDM_EDIT_CARTESIAN: {
                    int iRet;
                    iRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_CARTESIAN), hWnd, (DLGPROC)CartExprDlgProc);
                    if (iRet == IDOK) {
                        g_iFuncType = FUNC_TYPE_CARTESIAN;
                        ParseExpr();
                        RecalcSurface(hWnd);
                        DrawScreen(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                    break;
                }
                case IDM_EDIT_PARAMETRIC: {
                    int iRet;
                    iRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_PARAMETRIC), hWnd, (DLGPROC)ParmExprDlgProc);
                    if (iRet == IDOK) {
                        g_iFuncType = FUNC_TYPE_PARAMETRIC;
                        ParseExpr();
                        RecalcSurface(hWnd);
                        DrawScreen(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                    break;
                }
                case IDM_EDIT_WINDOW: {
                    int iRet;
                    iRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_WINDOWEDIT), hWnd, (DLGPROC)WindowEditorDlgProc);
                    if (iRet == IDOK && g_iStage == STAGE_READY) {
                        if (RecalcSurface(hWnd)) {
                            Redraw(hWnd);
                            InvalidateRect(hWnd, NULL, FALSE);
                        } else {
                            DrawErrorScreen(hWnd);
                        }
                    }
                    break;
                }
                case IDM_VIEW_CAMERA:
                    g_iMouseMode = MOUSE_MODE_CAMERA;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateViewMenuCheckStatus(hWnd);
                    break;
                case IDM_VIEW_PAN_MOVE:
                    g_iMouseMode = MOUSE_MODE_PAN_MOVE;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateViewMenuCheckStatus(hWnd);
                    break;
                case IDM_VIEW_ZOOM:
                    g_iMouseMode = MOUSE_MODE_ZOOM;
                    g_iZoomAccum = 0;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateViewMenuCheckStatus(hWnd);
                    break;
                case IDM_VIEW_FORMULA:
                    g_iMouseMode = MOUSE_MODE_FORMULA;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateViewMenuCheckStatus(hWnd);
                    break;
                case IDM_VIEW_FOV:
                    g_iMouseMode = MOUSE_MODE_FOV;
                    g_iFovAccum = 0;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateViewMenuCheckStatus(hWnd);
                    break;
                case IDM_VIEW_ORTHOGRAPHIC:
                    g_iProjection = ORTHOGRAPHIC;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateViewMenuCheckStatus(hWnd);
                    break;
                case IDM_VIEW_PERSPECTIVE:
                    g_iProjection = PERSPECTIVE;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateViewMenuCheckStatus(hWnd);
                    break;
                case IDM_VIEW_RESET:
                    ResetView(hWnd);
                    break;
                case IDM_DISPLAY_FOOTER:
                    g_bShowFooter = !g_bShowFooter;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateDisplayMenuCheckStatus(hWnd);
                    break;
                case IDM_DISPLAY_BOX:
                    g_bShowBox = !g_bShowBox;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateDisplayMenuCheckStatus(hWnd);
                    break;
                case IDM_DISPLAY_AXES:
                    g_bShowAxes = !g_bShowAxes;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateDisplayMenuCheckStatus(hWnd);
                    break;
                case IDM_DISPLAY_FORMULA:
                    g_bShowFormula = !g_bShowFormula;
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    UpdateDisplayMenuCheckStatus(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE: {
                RECT rcClient;
                int iCw, iCh;

                RecalcPaletteLUTs();
    
#if VER_PLATFORM_WIN32_CE
                {
                    RECT rcBar;
                    hwndCB = CommandBar_Create(hInst, hWnd, 1);
                    CommandBar_InsertMenubar(hwndCB, hInst, IDM_MENU, 0);
                    CommandBar_AddAdornments(hwndCB, 0, 0);
                    /* Get command bar height and subtract from client area */
                    GetWindowRect(hwndCB, &rcBar);
                    g_iBarHeight = rcBar.bottom - rcBar.top;
                }
#else
                g_iBarHeight = 0;
#endif

                GetClientRect(hWnd, &rcClient);
                if (rcClient.right <= 0) rcClient.right = 320;
                if (rcClient.bottom <= 0) rcClient.bottom = 240;

                rcClient.bottom -= g_iBarHeight;
                if (rcClient.bottom < 1) rcClient.bottom = 1;

                if (g_iCanvasScaleFactor < 1) g_iCanvasScaleFactor = 1;
                iCw = rcClient.right / g_iCanvasScaleFactor;
                iCh = rcClient.bottom / g_iCanvasScaleFactor;
                if (iCw < 1) iCw = 1;
                if (iCh < 1) iCh = 1;

                if (!CreateCanvas(iCw, iCh)) {
                    MessageBox(hWnd, TEXT("Failed to create canvas"), TEXT("ERROR"), MB_OK);
                    return -1;
                }
                if (!CreateBackBuffer(hWnd, rcClient.right, rcClient.bottom)) {
                    MessageBox(hWnd, TEXT("Failed to create back buffer"), TEXT("ERROR"), MB_OK);
                    return -1;
                }

                PzCamera_Initialize();

                Camera.iViewportS = (g_iCanvasH < g_iCanvasW ? g_iCanvasH : g_iCanvasW) / 2;
                Camera.iViewportX = g_iCanvasW / 2;
                Camera.iViewportY = g_iCanvasH / 2;

                RenderConfig_GetDefaultStyle(&g_rzConfig);
                RenderConfig_CalculateBigSymbolPoints(&g_rzConfig);
                g_rzConfig.sInterfaces.setPixel = RzSetPixel;
                g_rzConfig.sInterfaces.plotLine = RzPlotLine;
                g_rzConfig.sInterfaces.putChar  = RzPutChar;

                DrawIdleScreen(hWnd);
                UpdateWindow(hWnd);

                UpdateDisplayMenuCheckStatus(hWnd);
                UpdateViewMenuCheckStatus(hWnd);
            }
            break;
        case WM_LBUTTONDBLCLK:
            MessageBox(hWnd, TEXT("Hello"), TEXT("WORLD"),MB_OK);
            break;
        case WM_LBUTTONDOWN:
            if (g_iStage != STAGE_READY) break;
            g_bMouseDown = 1;
            g_iMousePrevX = LOWORD(lParam);
            g_iMousePrevY = HIWORD(lParam);
            SetCapture(hWnd);
            break;
        case WM_LBUTTONUP:
            if (g_iStage != STAGE_READY) break;
            g_bMouseDown = 0;
            g_iZoomAccum = 0;
            g_iFovAccum = 0;
            ReleaseCapture();
            Redraw(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
            g_dwLastMouseRefresh = 0;
            break;
        case WM_MOUSEMOVE:
            if (g_iStage != STAGE_READY) break;
            if (g_bMouseDown) {
                int iDeltaX, iDeltaY;
                int iMouseX = LOWORD(lParam);
                int iMouseY = HIWORD(lParam);
                iDeltaX = iMouseX - g_iMousePrevX;
                iDeltaY = iMouseY - g_iMousePrevY;
                g_iMousePrevX = iMouseX;
                g_iMousePrevY = iMouseY;
                if (iDeltaX == 0 && iDeltaY == 0) break;

                switch (g_iMouseMode) {
                    case MOUSE_MODE_CAMERA:
                        Camera.iBetaDeg  -= iDeltaX;
                        Camera.iAlphaDeg -= iDeltaY;
                        Camera.iBetaDeg  = Camera.iBetaDeg % 360;
                        if (Camera.iBetaDeg < 0) Camera.iBetaDeg += 360;
                        Camera.iAlphaDeg = Camera.iAlphaDeg % 360;
                        if (Camera.iAlphaDeg < 0) Camera.iAlphaDeg += 360;
                        break;
                    case MOUSE_MODE_PAN_MOVE:
                        Camera.iViewportX += iDeltaX;
                        Camera.iViewportY += iDeltaY;
                        break;
                    case MOUSE_MODE_ZOOM:
                        g_iZoomAccum += iDeltaY;
                        if (g_iZoomAccum >= ZOOM_DRAG_THRESHOLD) {
                            if (Camera.iZoomLevel > 0) Camera.iZoomLevel--;
                            g_iZoomAccum = 0;
                        } else if (g_iZoomAccum <= -ZOOM_DRAG_THRESHOLD) {
                            if (Camera.iZoomLevel < iNumZoomLevel - 1)
                                Camera.iZoomLevel++;
                            g_iZoomAccum = 0;
                        }
                        break;
                    case MOUSE_MODE_FORMULA:
                        g_iFormulaX += iDeltaX;
                        g_iFormulaY += iDeltaY;
                        break;
                    case MOUSE_MODE_FOV:
                        g_iFovAccum -= iDeltaY;
                        if (g_iFovAccum >= FOV_DRAG_THRESHOLD) {
                            if (Camera.iFovLevel > FOV_LEVEL_MIN)
                                Camera.iFovLevel--;
                            g_iFovAccum = 0;
                        } else if (g_iFovAccum <= -FOV_DRAG_THRESHOLD) {
                            if (Camera.iFovLevel < FOV_LEVEL_MAX)
                                Camera.iFovLevel++;
                            g_iFovAccum = 0;
                        }
                        break;
                    default:
                        break;
                }
                {
                    DWORD dwNow = GetTickCount();
                    if (dwNow - g_dwLastMouseRefresh >= (DWORD)g_iMouseRefreshMs
                        || dwNow < g_dwLastMouseRefresh) {
                        Redraw(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        g_dwLastMouseRefresh = dwNow;
                    }
                }
            }
            break;
        case WM_KEYDOWN:
            /* Toggle Fullscreen */
#if VER_PLATFORM_WIN32_CE
            {
                if (wParam == VK_RETURN) {
                    ToggleFullscreen(hWnd);
                    return 0;
                }
            }
#endif
            /* Esc trigger for Debug Mode */
            if (wParam == VK_ESCAPE) {
                ++g_iEscTrigger;
                if (g_iEscTrigger >= 3) {
                    g_iEscTrigger = 0;
                    StartDebugDialog(hWnd);
                }
                return 0;
            }
            g_iEscTrigger = 0;
            if (g_iStage != STAGE_READY) break;
            {
                int bRedraw = 0;
                /* Mode and toggle shortcuts (work in any stage) */
                switch (wParam) {
                    case 'C':
                        g_iMouseMode = MOUSE_MODE_CAMERA; 
                        Redraw(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateViewMenuCheckStatus(hWnd);
                        break;
                    case 'P':
                        g_iMouseMode = MOUSE_MODE_PAN_MOVE;
                        Redraw(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateViewMenuCheckStatus(hWnd);
                        break;
                    case 'Z':
                        g_iMouseMode = MOUSE_MODE_ZOOM;
                        Redraw(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateViewMenuCheckStatus(hWnd);
                        break;
                    case 'F':
                        g_iMouseMode = MOUSE_MODE_FORMULA;
                        Redraw(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateViewMenuCheckStatus(hWnd);
                        break;
                    case 'V':
                        g_iMouseMode = MOUSE_MODE_FOV;
                        g_iFovAccum = 0;
                        Redraw(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateViewMenuCheckStatus(hWnd);
                        break;
                    case 'R':
                        ResetView(hWnd);
                        return 0;
                    case 'T':
                        g_bShowFooter = !g_bShowFooter;
                        Redraw(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateDisplayMenuCheckStatus(hWnd);
                        return 0;
                    case 'B':
                        g_bShowBox = !g_bShowBox;
                        Redraw(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateDisplayMenuCheckStatus(hWnd);
                        return 0;
                    case 'A':
                        g_bShowAxes = !g_bShowAxes;
                        Redraw(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateDisplayMenuCheckStatus(hWnd);
                        return 0;
                }
                switch (g_iMouseMode) {
                    case MOUSE_MODE_CAMERA:
                        if (wParam == VK_LEFT)  { Camera.iBetaDeg -= 5; bRedraw = 1; }
                        if (wParam == VK_RIGHT) { Camera.iBetaDeg += 5; bRedraw = 1; }
                        if (wParam == VK_UP)    { Camera.iAlphaDeg -= 5; bRedraw = 1; }
                        if (wParam == VK_DOWN)  { Camera.iAlphaDeg += 5; bRedraw = 1; }
                        if (bRedraw) {
                            Camera.iBetaDeg  = Camera.iBetaDeg % 360;
                            if (Camera.iBetaDeg < 0) Camera.iBetaDeg += 360;
                            Camera.iAlphaDeg = Camera.iAlphaDeg % 360;
                            if (Camera.iAlphaDeg < 0) Camera.iAlphaDeg += 360;
                        }
                        break;
                    case MOUSE_MODE_PAN_MOVE:
                        if (wParam == VK_LEFT)  { Camera.iViewportX -= 10; bRedraw = 1; }
                        if (wParam == VK_RIGHT) { Camera.iViewportX += 10; bRedraw = 1; }
                        if (wParam == VK_UP)    { Camera.iViewportY -= 10; bRedraw = 1; }
                        if (wParam == VK_DOWN)  { Camera.iViewportY += 10; bRedraw = 1; }
                        break;
                    case MOUSE_MODE_ZOOM:
                        if (wParam == VK_UP && Camera.iZoomLevel < iNumZoomLevel - 1)
                            { Camera.iZoomLevel++; bRedraw = 1; }
                        if (wParam == VK_DOWN && Camera.iZoomLevel > 0)
                            { Camera.iZoomLevel--; bRedraw = 1; }
                        break;
                    case MOUSE_MODE_FORMULA:
                        if (wParam == VK_LEFT)  { g_iFormulaX -= 10; bRedraw = 1; }
                        if (wParam == VK_RIGHT) { g_iFormulaX += 10; bRedraw = 1; }
                        if (wParam == VK_UP)    { g_iFormulaY -= 10; bRedraw = 1; }
                        if (wParam == VK_DOWN)  { g_iFormulaY += 10; bRedraw = 1; }
                        break;
                    case MOUSE_MODE_FOV:
                        if (wParam == VK_UP && Camera.iFovLevel < FOV_LEVEL_MAX)
                            { Camera.iFovLevel++; bRedraw = 1; }
                        if (wParam == VK_DOWN && Camera.iFovLevel > FOV_LEVEL_MIN)
                            { Camera.iFovLevel--; bRedraw = 1; }
                        break;
                    default:
                        break;
                }
                if (bRedraw) {
                    Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
            break;
        case WM_ERASEBKGND:
            /* Do nothing here to prevent flickering. */
            return TRUE;
        case WM_PAINT:
            /* Testing: redraw the function graph here and print the frame count */
            if (g_bPerformanceTest) {
                PerformanceTestRedraw(hWnd);
            }
            /* Paint canvas to window */
            PaintCanvasToWindow(hWnd);
            /* Determine whether to end the test */
            if (g_bPerformanceTest) {
                PerformanceTestCheck(hWnd);
            }
            break;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED && g_iCanvasW > 0) {
                int iNewW, iNewH, iCw, iCh;
                iNewW = LOWORD(lParam);
                iNewH = HIWORD(lParam);
                iNewH -= g_iBarHeight;
                if (iNewH < 1) iNewH = 1;
                if (iNewW != g_iDibW || iNewH != g_iDibH) {
                    iCw = iNewW / g_iCanvasScaleFactor;
                    iCh = iNewH / g_iCanvasScaleFactor;
                    if (iCw < 1) iCw = 1;
                    if (iCh < 1) iCh = 1;
                    DestroyBackBuffer();
                    DestroyCanvas();
                    CreateCanvas(iCw, iCh);
                    CreateBackBuffer(hWnd, iNewW, iNewH);
                    Camera.iViewportS = (g_iCanvasH < g_iCanvasW ? g_iCanvasH : g_iCanvasW) / 2;
                    Camera.iViewportX = g_iCanvasW / 2;
                    Camera.iViewportY = g_iCanvasH / 2;
                    if (g_iStage == STAGE_IDLE)
                        DrawIdleScreen(hWnd);
                    else if (g_iStage == STAGE_ERROR)
                        DrawErrorScreen(hWnd);
                    else
                        Redraw(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
            break;
        case WM_DESTROY:
#if VER_PLATFORM_WIN32_CE
            CommandBar_Destroy(hwndCB);
#endif
            /* Destroy render nodes, ezvm and ast */
            {
                int i;
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
            }
            DestroyBackBuffer();
            DestroyCanvas();
            PostQuitMessage(0);
            SetTaskbarVisible(TRUE);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            CenterDialog(hDlg);
            return TRUE;
        case WM_COMMAND:
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL)) {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}

BOOL SetTaskbarVisible(BOOL bVisible) {
#if defined(VER_PLATFORM_WIN32_CE)
    HWND hWndTaskbar = FindWindow(TEXT("HHTaskBar"), NULL);
    if (!bVisible) {
        SetWindowPos(hWndTaskbar, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
    } else {
        SetWindowPos(hWndTaskbar, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }
    return TRUE;
#else
    (void)bVisible;
    return FALSE;
#endif
}

void CenterDialog(HWND hDlg) {
    RECT rt, rt1;
    int DlgWidth, DlgHeight;
    int NewPosX, NewPosY;
    if (GetWindowRect(hDlg, &rt1)) {
        GetClientRect(GetParent(hDlg), &rt);
        DlgWidth  = rt1.right - rt1.left;
        DlgHeight = rt1.bottom - rt1.top;
        NewPosX = (rt.right - rt.left - DlgWidth) / 2;
        NewPosY = (rt.bottom - rt.top - DlgHeight) / 2;
        if (NewPosX < 0) NewPosX = 0;
        if (NewPosY < 0) NewPosY = 0;
        SetWindowPos(hDlg, 0, NewPosX, NewPosY,
            0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }
}

#ifdef VER_PLATFORM_WIN32_CE
void ToggleFullscreen(HWND hWnd) {
    HWND hWndTarget;
#if (VER_PLATFORM_WIN32_CE >= 200)
    hWndTarget = HWND_TOPMOST;
#else
	hWndTarget = HWND_TOP;
#endif
    if (PrevWindowSize.bFullScreen) {
        PrevWindowSize.bFullScreen = FALSE;
        SetTaskbarVisible(TRUE);
        g_iBarHeight = PrevWindowSize.iBarHeight;
        CommandBar_Show(hwndCB, TRUE);
        /* Back to normal */
        SetWindowPos(
            hWnd, hWndTarget,
            PrevWindowSize.iX,
            PrevWindowSize.iY,
            PrevWindowSize.iW,
            PrevWindowSize.iH,
            SWP_SHOWWINDOW
        );
    }
    else {
        RECT rect;
        int iScrWidth, iScrHeight;
	    iScrWidth = GetSystemMetrics(SM_CXSCREEN);
	    iScrHeight = GetSystemMetrics(SM_CYSCREEN);
        /* To fullscreen */
        if (GetWindowRect(hWnd, &rect)) {
            SetTaskbarVisible(FALSE);
            PrevWindowSize.bFullScreen = TRUE;
            PrevWindowSize.iX = rect.left;
            PrevWindowSize.iY = rect.top;
            PrevWindowSize.iW = rect.right - rect.left;
            PrevWindowSize.iH = rect.bottom - rect.top;
            PrevWindowSize.iBarHeight = g_iBarHeight;
            g_iBarHeight = 0;
            CommandBar_Show(hwndCB, FALSE);
            SetWindowPos(hWnd, hWndTarget, 0, 0, iScrWidth, iScrHeight, SWP_SHOWWINDOW);
        }
    }
}
#endif

/*====================================================
 * Utility: copy char string to TCHAR buffer
 *====================================================*/
void CharToTChar(TCHAR* dst, const char* src, int maxLen) {
    int i;
    for (i = 0; i < maxLen - 1 && src[i] != '\0'; ++i)
        dst[i] = (TCHAR)(src[i] & 0xFF);
    dst[i] = TEXT('\0');
}

void TCharToChar(char* dst, const TCHAR* src, int maxLen) {
    int i;
    for (i = 0; i < maxLen - 1 && src[i] != '\0'; ++i)
        dst[i] = (char)(src[i] & 0xFF);
    dst[i] = 0;
}

void LoadApplicationConfig() {
    TCHAR szFile[MAX_PATH] = TEXT("plotter-z.ini");
    HANDLE hFile;
    DWORD dwSize, dwRead;
    char* pBuf;
    PineIniFile* pIni;
    PineIniError iniErr;
    const PineIniSection* pSec;

    hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    dwSize = GetFileSize(hFile, NULL);
    if (dwSize == INVALID_FILE_SIZE || dwSize > 8191) {
        CloseHandle(hFile); return;
    }

    pBuf = (char*)malloc((size_t)(dwSize + 1));
    if (pBuf == NULL) {
        CloseHandle(hFile); return;
    }

    if (!ReadFile(hFile, pBuf, dwSize, &dwRead, NULL) || dwRead != dwSize) {
        free(pBuf);
        CloseHandle(hFile);
        return;
    }

    CloseHandle(hFile);
    pBuf[dwSize] = '\0';

    pIni = PineIni_Parse(pBuf, &iniErr);
    free(pBuf);
    if (pIni == NULL) return;

    /* Parse [expression] */
    pSec = PineIni_Find(pIni, "expression");
    if (pSec != NULL) {
        Utils_StringCopy(g_szCartExpr, CART_EXPR_LENGTH, PineIni_Section_GetString(pSec, "expr", g_szCartExpr));
    }

    /* Parse [graphics] */
    pSec = PineIni_Find(pIni, "graphics");
    if (pSec != NULL) {
        g_iForceBpp = PineIni_Section_GetInt(pSec, "force_bpp", -1);
        switch (g_iForceBpp) {
            case 2:
            case 8:
            case 16:
            case 32:
                break;
            default:
                g_iForceBpp = -1;
        }
    }

    PineIni_Destroy(pIni);
}

void PerformanceTestRedraw(HWND hWnd) {
    char szBuf[50];
    int iWidth;
    int iLeft;
    int iTop = (g_iCanvasH - CURRENT_FONT_HEIGHT) / 2;
    static int iPadding = 4;
    ++g_nPaintCount;
    Redraw(hWnd);
    Salvia_Format(szBuf, "Frame = %d", g_nPaintCount);
    iWidth = strlen(szBuf) * CURRENT_FONT_WIDTH;
    iLeft = (g_iCanvasW - iWidth) / 2;
    FillRectCanvas(0, iTop - iPadding, g_iCanvasW, CURRENT_FONT_HEIGHT + iPadding * 2, COLOR_BLACK);
    PutTextCanvas(iLeft, iTop, (const unsigned char*)szBuf, COLOR_WHITE);
}

void PerformanceTestCheck(HWND hWnd) {
    DWORD dwNow = GetTickCount();
    if (dwNow - g_dwPerfStartTime >= PERF_TEST_MS) {
        TCHAR szMsg[256];
        char szBuf[100];
        double fFps = (double)g_nPaintCount / PERF_TEST_SECONDS;
        int iThreshold = (PERF_TEST_MS / g_nPaintCount + 5) / 10 * 10;
        if (iThreshold < 10) iThreshold = 10;
        /* Stop Test */
        g_bPerformanceTest = FALSE;
        /* Print Result on canvas */
        FillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);
        FillRectCanvas(0, 0, g_iCanvasW, CURRENT_FONT_HEIGHT * 2, COLOR_BLACK);
        PutTextCanvas(CURRENT_FONT_WIDTH, CURRENT_FONT_HEIGHT / 2, (const unsigned char *)"Test Result", COLOR_WHITE);
        Salvia_Format(szBuf, "Duration:     %ds", PERF_TEST_SECONDS);
        PutTextCanvas(0, CURRENT_FONT_HEIGHT * 2, (const unsigned char *)szBuf, COLOR_BLACK);
        Salvia_Format(szBuf, "Frames drawn: %d", g_nPaintCount);
        PutTextCanvas(0, CURRENT_FONT_HEIGHT * 3, (const unsigned char *)szBuf, COLOR_BLACK);
        Salvia_Format(szBuf, "Average FPS:  %.2f", fFps);
        PutTextCanvas(0, CURRENT_FONT_HEIGHT * 4, (const unsigned char *)szBuf, COLOR_BLACK);
        Salvia_Format(szBuf, "Recommended Threshold: %d", iThreshold);
        PutTextCanvas(0, CURRENT_FONT_HEIGHT * 5, (const unsigned char *)szBuf, COLOR_BLACK);
        /* Popup Message */
        wsprintf(
            szMsg,
            TEXT("Duration: %ds\r\nFrames drawn: %d\r\nFPS=%d.%d\r\nRecommended Threshold: %d"),
            PERF_TEST_SECONDS,
            g_nPaintCount,
            (int)fFps,
            (int)((fFps - floor(fFps)) * 100),
            iThreshold
        );
        InvalidateRect(hWnd, NULL, TRUE);
        MessageBox(hWnd, szMsg, TEXT("Performance Test Result"), MB_OK);
    } else {
        Camera.iBetaDeg += 10;
        if (Camera.iBetaDeg >= 360) Camera.iBetaDeg = Camera.iBetaDeg % 360;
        Camera.iAlphaDeg += 5;
        if (Camera.iAlphaDeg >= 360) Camera.iAlphaDeg = Camera.iAlphaDeg % 360;
        InvalidateRect(hWnd, NULL, TRUE);
    }
}

void ReCreateCanvasAndBackBuffer(HWND hWnd, int iPrevScale) {
    RECT rc;
    int iCw, iCh;
    int iScaleOld, iScaleNew;
    iScaleOld = iPrevScale;
    iScaleNew = g_iCanvasScaleFactor;
    Camera.iViewportX = Camera.iViewportX * iScaleOld / iScaleNew;
    Camera.iViewportY = Camera.iViewportY * iScaleOld / iScaleNew;
    g_iFormulaX = g_iFormulaX * iScaleOld / iScaleNew;
    g_iFormulaY = g_iFormulaY * iScaleOld / iScaleNew;
    DestroyBackBuffer();
    DestroyCanvas();
    GetClientRect(hWnd, &rc);
    rc.bottom -= g_iBarHeight;
    if (rc.bottom < 1) rc.bottom = 1;
    iCw = rc.right / g_iCanvasScaleFactor;
    iCh = rc.bottom / g_iCanvasScaleFactor;
    if (iCw < 1) iCw = 1;
    if (iCh < 1) iCh = 1;
    CreateCanvas(iCw, iCh);
    CreateBackBuffer(hWnd, rc.right, rc.bottom);
    Camera.iViewportS = (g_iCanvasH < g_iCanvasW ? g_iCanvasH : g_iCanvasW) / 2;
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
            Salvia_Format(g_szErrMsgLine1, "Failed to parse \"%s=\":", szParmLabel[iParamIndex]);
        } else {
            strcpy(g_szErrMsgLine1, "Failed to parse:");
        }
        strcpy(g_szErrMsgLine2, "Syntax error.");
    }
    else {
        switch (iCompileError) {
            case EZERR_VARIABLE_UNDEFINED:
                strcpy(g_szErrMsgLine1, "Undefined variable:");
                Salvia_Format(g_szErrMsgLine2, "\"%s\"", szErrorBuf);
                break;
            case EZERR_FUNCTION_UNDEFINED:
                strcpy(g_szErrMsgLine1, "Undefined function:");
                Salvia_Format(g_szErrMsgLine2, "\"%s\"", szErrorBuf);
                break;
            case EZERR_FUNCTION_PARAM_MISMATCH:
                strcpy(g_szErrMsgLine1, "Function parameter mismatch");
                Salvia_Format(g_szErrMsgLine2, "\"%s\"", szErrorBuf);
                break;
        }
    }
}

int ParseExpr(void) {
    int i;
    char szErrorBuf[EZ_ERROR_CONTENT_LENGTH];
    int iCompileError;

    g_iStage = STAGE_ERROR;

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

    return 1;
}

void UpdateDisplayMenuCheckStatus(HWND hWnd) {
    HMENU hMenu;
#ifdef VER_PLATFORM_WIN32_CE
    (void)hWnd;
    hMenu = CommandBar_GetMenu(hwndCB, 0);
#else
    hMenu = GetMenu(hWnd); 
#endif
    CheckMenuItem(hMenu, IDM_DISPLAY_AXES, g_bShowAxes ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_DISPLAY_BOX, g_bShowBox ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_DISPLAY_FOOTER, g_bShowFooter ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_DISPLAY_FORMULA, g_bShowFormula ? MF_CHECKED : MF_UNCHECKED);
}

void UpdateViewMenuCheckStatus(HWND hWnd) {
    HMENU hMenu;
#ifdef VER_PLATFORM_WIN32_CE
    (void)hWnd;
    hMenu = CommandBar_GetMenu(hwndCB, 0);
#else
    hMenu = GetMenu(hWnd); 
#endif
    CheckMenuItem(hMenu, IDM_VIEW_ORTHOGRAPHIC, g_iProjection == ORTHOGRAPHIC ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_VIEW_PERSPECTIVE, g_iProjection == PERSPECTIVE ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_VIEW_CAMERA, g_iMouseMode == MOUSE_MODE_CAMERA ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_VIEW_PAN_MOVE, g_iMouseMode == MOUSE_MODE_PAN_MOVE ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_VIEW_ZOOM, g_iMouseMode == MOUSE_MODE_ZOOM ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_VIEW_FORMULA, g_iMouseMode == MOUSE_MODE_FORMULA ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, IDM_VIEW_FOV, g_iMouseMode == MOUSE_MODE_FOV ? MF_CHECKED : MF_UNCHECKED);
}

/* Count and auto-complete parentheses */
void BalanceParen(char* szNew, int iMax) {
    int iLen = strlen(szNew), iParenCount, i;       
    iParenCount = 0;
    for (i = 0; szNew[i] != '\0'; ++i) {
        if (szNew[i] == '(') ++iParenCount;
        else if (szNew[i] == ')') --iParenCount;
    }
    while (iParenCount > 0 && iLen < iMax - 1) {
        szNew[iLen++] = ')';
        --iParenCount;
    }
    szNew[iLen] = '\0';
}