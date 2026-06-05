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

#define ZOOM_DRAG_THRESHOLD     15

static int  g_iMouseRefreshMs = 120;
DWORD       g_dwLastMouseRefresh;
HINSTANCE   hInst;
HWND        hwndCB;

int         recalc                      (HWND hWnd);
void        CharToTChar                 (TCHAR* dst, const char* src, int maxLen);
void        CenterDialog                (HWND hDlg);
void        LoadApplicationConfig       (void);
void        PerformanceTestRedraw       (HWND hWnd);
void        PerformanceTestCheck        (HWND hWnd);
void        RecreateCanvasAndBackBuffer (HWND hWnd, int iPrevScale);

/*====================================================
 * Constants
 *====================================================*/
#define MAX_LOADSTRING      100
#define EXPR_MAX            300
#define CURRENT_FONT_WIDTH  6
#define CURRENT_FONT_HEIGHT 8

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

static void recalcPaletteLUTs(void) {
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
 * Expression & Camera
 *====================================================*/
#define DEFAULT_EXPR "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))"
char szExpr[EXPR_MAX] = DEFAULT_EXPR;

#define GRID_MAX            30

PZ_FIXED zBuf[GRID_MAX * GRID_MAX];
PZ_FIXED xBuf[GRID_MAX];
PZ_FIXED yBuf[GRID_MAX];
#define Z_BUF(x,y) (zBuf[(x) + (y) * Camera.xGrid])

#define SAMPLE_DEFAULT_GRID 20

FzAstNode*      g_pAstExpr      = NULL;
EzMachine*      g_pVm           = NULL;
RenderNode*     g_pRenderNode   = NULL;
RenderConfig    g_RenderConfig;
char            g_szErrorBuf[EZ_ERROR_CONTENT_LENGTH];
int             g_bShowBox      = 1;
int             g_iExprPosX     = 0;
int             g_iExprPosY     = 0;

#define PERF_TEST_SECONDS   15
#define PERF_TEST_MS        ((PERF_TEST_SECONDS) * 1000)

/* Application stage */
#define STAGE_IDLE      0
#define STAGE_ERROR     1
#define STAGE_READY     2
static int      g_iStage        = STAGE_IDLE;

/* Mouse interaction modes */
#define MOUSE_MODE_CAMERA      0
#define MOUSE_MODE_PAN_MOVE    1
#define MOUSE_MODE_ZOOM        2
#define MOUSE_MODE_FORMULA     3
#define MOUSE_MODE_FOV         4

static int      g_iMouseMode    = MOUSE_MODE_CAMERA;
static int      g_bShowFooter   = 1;
static int      g_bMouseDown    = 0;
static int      g_iMousePrevX   = 0;
static int      g_iMousePrevY   = 0;
static int      g_iZoomAccum    = 0;
static int      g_iFovAccum     = 0;
static int      g_iEscTrigger   = 0;
static int      g_bPerformanceTest = 0;
static int      g_nPaintCount      = 0;
static DWORD    g_dwPerfStartTime  = 0;
#define ZOOM_DRAG_THRESHOLD     15
#define FOV_DRAG_THRESHOLD       10

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

static void setPixelCanvas(int x, int y, int iColor) {
    unsigned char ucMask;
    int iByte;
    if (x < 0 || x >= g_iCanvasW || y < 0 || y >= g_iCanvasH) return;
    iByte = canvasByteIndex(x, y);
    ucMask = canvasMask(x);
    g_pCanvas[iByte] = (unsigned char)((g_pCanvas[iByte] & ucMask)
                     | ((iColor & 0x03) << canvasShift(x)));
}

#define getPixelCanvas(x, y) \
    ( ((x) < 0 || (x) >= g_iCanvasW || (y) < 0 || (y) >= g_iCanvasH) ? 0 : \
      (g_pCanvas[canvasByteIndex((x), (y))] >> canvasShift((x))) & 0x03 )

static void fillRectCanvas(int dx, int dy, int w, int h, int iColor) {
    int x, y;
    for (y = 0; y < h; ++y)
        for (x = 0; x < w; ++x)
            setPixelCanvas(dx + x, dy + y, iColor);
}

static void drawLineCanvas(int x0, int y0, int x1, int y1, int iColor) {
    int dx = ABS(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -ABS(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        setPixelCanvas(x0, y0, iColor);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw1bppCanvas(const unsigned char *raw,
                           int dx, int dy, int w, int h, int iColor) {
    int pitch = (w >> 3) + (w % 8 ? 1 : 0);
    int x, y, dot;
    unsigned char ucEight;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            ucEight = raw[y * pitch + (x >> 3)];
            dot = (ucEight >> (7 - (x & 7))) & 1;
            if (dot) setPixelCanvas(dx + x, dy + y, iColor);
        }
    }
}

/*====================================================
 * Invert colors in a rectangle (0<->3, 1<->2)
 *====================================================*/
static void invertRectCanvas(int dx, int dy, int w, int h) {
    int x, y, iColor;
    for (y = 0; y < h; ++y)
        for (x = 0; x < w; ++x) {
            iColor = getPixelCanvas(dx + x, dy + y);
            setPixelCanvas(dx + x, dy + y, 3 - iColor);
        }
}

static void putCharCanvas(int x, int y, unsigned char ch, int iColor) {
    draw1bppCanvas(FONT_HYBIRD_6x8 + 8 * (int)ch, x, y, 8, 8, iColor);
}

static void putTextCanvas(int x, int y, const unsigned char* usz, int iColor) {
    for (; *usz; ++usz, x += CURRENT_FONT_WIDTH)
        putCharCanvas(x, y, *usz, iColor);
}

/*====================================================
 * Canvas lifecycle
 *====================================================*/
static int createCanvas(int iWidth, int iHeight) {
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

static void destroyCanvas(void) {
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

static int createBackBuffer(HWND hWnd, int iWidth, int iHeight) {
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

static void destroyBackBuffer(void) {
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
static void paintCanvas2Bpp() {
    int iX, iY, iX2, iY2;
    int iZoom = g_iCanvasScaleFactor;
    BYTE* pRow;
    if (iZoom < 1) iZoom = 1;
    for (iY = 0; iY < g_iCanvasH; ++iY) {
        for (iX = 0; iX < g_iCanvasW; ++iX) {
            int iColor = getPixelCanvas(iX, iY);
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

static void paintCanvas8Bpp() {
    int iX, iY, iX2, iY2;
    int iZoom = g_iCanvasScaleFactor;
    BYTE* pRow;
    if (iZoom < 1) iZoom = 1;
    for (iY = 0; iY < g_iCanvasH; ++iY) {
        for (iX = 0; iX < g_iCanvasW; ++iX) {
            int iColor = getPixelCanvas(iX, iY);
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

static void paintCanvas16Bpp() {
    int iX, iY, iX2, iY2;
    int iZoom = g_iCanvasScaleFactor;
    BYTE* pRow;
    if (iZoom < 1) iZoom = 1;
    for (iY = 0; iY < g_iCanvasH; ++iY) {
        for (iX = 0; iX < g_iCanvasW; ++iX) {
            int iColor = getPixelCanvas(iX, iY);
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

static void paintCanvas32Bpp() {
    int iX, iY, iX2, iY2;
    int iZoom = g_iCanvasScaleFactor;
    BYTE* pRow;
    if (iZoom < 1) iZoom = 1;
    for (iY = 0; iY < g_iCanvasH; ++iY) {
        for (iX = 0; iX < g_iCanvasW; ++iX) {
            int iColor = getPixelCanvas(iX, iY);
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

static void paintCanvasToWindow(HWND hWnd) {
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
                paintCanvas2Bpp();
                break;
            }
            case 8:
                paintCanvas8Bpp();
                break;
            case 16: {
                paintCanvas16Bpp();
                break;
            }
            case 32: {
                paintCanvas32Bpp();
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
                    g_rgbPalette[getPixelCanvas(iX / iZoom, iY / iZoom)]);
    }
    EndPaint(hWnd, &ps);
}

/*====================================================
 * rz interface wrappers
 *====================================================*/
static void rzSetPixel(int x, int y) {
    setPixelCanvas(x, y, COLOR_BLACK);
}

static void rzPlotLine(int x0, int y0, int x1, int y1) {
    drawLineCanvas(x0, y0, x1, y1, COLOR_BLACK);
}

static void rzPutChar(int x, int y, unsigned char ch) {
    putCharCanvas(x, y, ch, COLOR_BLACK);
}

/*====================================================
 * Projection
 *====================================================*/
#define ORTHOGRAPHIC    0
#define PERSPECTIVE     1

void (*xyz2xy)(PZ_FIXED, PZ_FIXED, PZ_FIXED, int*, int *) = PzCamera_OrthoProjectFixed;
int g_iProjection = ORTHOGRAPHIC;


/*====================================================
 * Draw surface wireframe onto the 2bpp canvas
 *====================================================*/
static void drawSurfaceWireframeCanvas(void) {
    int ix, iy, x0, y0, x1, y1;
    PZ_FIXED z0, z1;

    for (ix = 0; ix < Camera.xGrid; ++ix) {
        iy = 0;
        xyz2xy(xBuf[ix], yBuf[iy], z0 = Z_BUF(ix, iy), &x0, &y0);
        for (iy = 0; iy < Camera.yGrid; ++iy) {
            xyz2xy(xBuf[ix], yBuf[iy], z1 = Z_BUF(ix, iy), &x1, &y1);
            if (z0 <= PZ_FIXED_ONE && z0 >= PZ_FIXED_NEG_ONE
                && z1 <= PZ_FIXED_ONE && z1 >= PZ_FIXED_NEG_ONE)
                drawLineCanvas(x0, y0, x1, y1, COLOR_BLACK);
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
                drawLineCanvas(x0, y0, x1, y1, COLOR_BLACK);
            x0 = x1; y0 = y1; z0 = z1;
        }
    }
}

/*====================================================
 * Bounding box edges
 *====================================================*/
typedef struct { PZ_FIXED x, y, z; } Vertex;
typedef struct { int i0, i1; } Edge;

static void drawBoxEdgesCanvas(void) {
    static const Vertex BoxVertices[] = {
        {   PZ_FIXED_ONE,       PZ_FIXED_ONE,       PZ_FIXED_ONE },
        {   PZ_FIXED_NEG_ONE,   PZ_FIXED_ONE,       PZ_FIXED_ONE },
        {   PZ_FIXED_NEG_ONE,   PZ_FIXED_NEG_ONE,   PZ_FIXED_ONE },
        {   PZ_FIXED_ONE,       PZ_FIXED_NEG_ONE,   PZ_FIXED_ONE },
        {   PZ_FIXED_ONE,       PZ_FIXED_ONE,       PZ_FIXED_NEG_ONE },
        {   PZ_FIXED_NEG_ONE,   PZ_FIXED_ONE,       PZ_FIXED_NEG_ONE },
        {   PZ_FIXED_NEG_ONE,   PZ_FIXED_NEG_ONE,   PZ_FIXED_NEG_ONE },
        {   PZ_FIXED_ONE,       PZ_FIXED_NEG_ONE,   PZ_FIXED_NEG_ONE },
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
        drawLineCanvas(x0, y0, x1, y1, COLOR_LIGHT_GRAY);
    }
}

/*====================================================
 * Full redraw (clear -> box -> surface -> footer)
 *====================================================*/
static void redrawCanvas(HWND hWnd) {
    int iBaseline;
    int iStartY, iLen;

    Camera.sinA = PZ_FLOAT_TO_FIXED(sin(Camera.iAlphaDeg * PZ_PI / 180));
    Camera.cosA = PZ_FLOAT_TO_FIXED(cos(Camera.iAlphaDeg * PZ_PI / 180));
    Camera.sinB = PZ_FLOAT_TO_FIXED(sin(Camera.iBetaDeg * PZ_PI / 180));
    Camera.cosB = PZ_FLOAT_TO_FIXED(cos(Camera.iBetaDeg * PZ_PI / 180));
    fillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);
    if (g_bShowBox)
        drawBoxEdgesCanvas();
    drawSurfaceWireframeCanvas();
    if (g_pRenderNode != NULL) {
        iBaseline = g_pRenderNode->sLayout.iAscent;
        RenderNode_Draw(g_pRenderNode, &g_RenderConfig,
            g_iExprPosX, g_iExprPosY + iBaseline);
    }

    /* Footer bar */
    if (g_bShowFooter) {
        char szBuf[64];
        BOOL bIsWideCanvas = g_iCanvasW >= 240;

        iStartY = g_iCanvasH - CURRENT_FONT_HEIGHT - 2;
        if (iStartY < 0) iStartY = 0;
        fillRectCanvas(0, iStartY, g_iCanvasW, CURRENT_FONT_HEIGHT + 2, COLOR_DARK_GRAY);

        /* Left: alpha / beta */
        if (bIsWideCanvas) {
            Salvia_Format(szBuf, "%c=%d,%c=%d",
                PZ_AE_GREEK_alpha, Camera.iAlphaDeg,
                PZ_AE_GREEK_beta,  Camera.iBetaDeg);
        }
        else {
            Salvia_Format(szBuf, "%d,%d", Camera.iAlphaDeg, Camera.iBetaDeg);
        }
        putTextCanvas(2, iStartY + 2, (const unsigned char*)szBuf, COLOR_WHITE);

        /* Center: mode display */
        {
            static const char* szWideCenterText[] = { "Camera", "Pan Move", "Zoom", "Formula", "FOV" };
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
            putTextCanvas((g_iCanvasW - iLen) / 2, iStartY + 2,
                (const unsigned char*)szCenter, COLOR_WHITE);
        }

        /* Right: zoom%, (viewportX, viewportY) */
        if (bIsWideCanvas) {
            Salvia_Format(szBuf, "%s(%d,%d)", szZoomLevels[Camera.iZoomLevel], Camera.iViewportX, Camera.iViewportY);
        }
        else {
            Salvia_Format(szBuf, "%s", szZoomLevels[Camera.iZoomLevel]);
        }
        iLen = (int)strlen(szBuf) * CURRENT_FONT_WIDTH;
        putTextCanvas(g_iCanvasW - iLen - 2, iStartY + 2,
            (const unsigned char*)szBuf, COLOR_WHITE);
    }
    (void)hWnd;
}

/*====================================================
 * Draw idle screen (centered, multi-line intro)
 *====================================================*/
static void drawIdleScreen(HWND hWnd) {
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
    fillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);
    for (iLine = 0; iLine < iCount; ++iLine) {
        iLen = (int)strlen(szLines[iLine]) * CURRENT_FONT_WIDTH;
        x = (g_iCanvasW - iLen) / 2;
        if (x < 2) x = 2;
        if (bReverse[iLine]) {
            fillRectCanvas(
                x - iPadding,
                y - iPadding / 2,
                iLen + iPadding * 2,
                CURRENT_FONT_HEIGHT + iPadding,
                iColors[iLine]
            );
            putTextCanvas(x, y, (const unsigned char*)szLines[iLine], COLOR_WHITE);
        } else {
            putTextCanvas(x, y, (const unsigned char*)szLines[iLine], iColors[iLine]);
        }
        y += CURRENT_FONT_HEIGHT + iPadding;
    }
    InvalidateRect(hWnd, NULL, FALSE);
}

/*====================================================
 * Draw error screen (centered specific message)
 *====================================================*/
static void drawErrorScreen(HWND hWnd) {
    const char* szMsg;
    int iMsgLen, x, y;

    if (g_pAstExpr == NULL)
        szMsg = "Syntax Error - could not parse expression";
    else
        szMsg = g_szErrorBuf;

    fillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);

    /* Main error line */
    iMsgLen = (int)strlen(szMsg) * CURRENT_FONT_WIDTH;
    x = (g_iCanvasW - iMsgLen) / 2;
    if (x < 2) x = 2;
    y = g_iCanvasH / 2 - CURRENT_FONT_HEIGHT;
    if (y < 0) y = 0;
    putTextCanvas(x, y, (const unsigned char*)szMsg, COLOR_BLACK);

    /* Hint line */
    {
        const char* szHint = "Edit > Expression to retry";
        iMsgLen = (int)strlen(szHint) * CURRENT_FONT_WIDTH;
        x = (g_iCanvasW - iMsgLen) / 2;
        if (x < 2) x = 2;
        putTextCanvas(x, y + CURRENT_FONT_HEIGHT + 2,
            (const unsigned char*)szHint, COLOR_DARK_GRAY);
    }
    InvalidateRect(hWnd, NULL, FALSE);
}

/*====================================================
 * Reset camera and formula position to defaults
 *====================================================*/
static void resetView(HWND hWnd) {
    PzCamera_Reset(g_iCanvasW / 2, g_iCanvasH / 2);
    g_iExprPosX = 0;
    g_iExprPosY = 0;
    g_iZoomAccum = 0;
    redrawCanvas(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
}

/*====================================================
 * Expression edit dialog procedure
 *   Auto-completes unbalanced parentheses on confirm.
 *   Re-parses and re-evaluates the expression.
 *====================================================*/
static LRESULT CALLBACK ExprDlgProc( HWND hDlg, UINT message,
                                     WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG: {
            /* Assign expression to edit control */
            TCHAR wszTemp[EXPR_MAX];
            int i;
            for (i = 0; szExpr[i]; ++i) {
                wszTemp[i] = szExpr[i];
            }
            wszTemp[i] = 0;
            SetDlgItemText(hDlg, IDC_EXPR_EDIT, wszTemp);
            /* Center the dialog */
            CenterDialog(hDlg);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                {
                    HWND hWndParent;
                    TCHAR szBufT[EXPR_MAX];
                    char szNew[EXPR_MAX];
                    int iLen, iParenCount;

                    hWndParent = GetParent(hDlg);
                    GetDlgItemText(hDlg, IDC_EXPR_EDIT, szBufT, EXPR_MAX);

                    /* Convert TCHAR to char */
                    iLen = 0;
                    while (iLen < EXPR_MAX - 1 && szBufT[iLen] != TEXT('\0')) {
                        szNew[iLen] = (char)szBufT[iLen];
                        ++iLen;
                    }
                    szNew[iLen] = '\0';

                    /* Count and auto-complete parentheses */
                    iParenCount = 0;
                    {
                        int i;
                        for (i = 0; szNew[i] != '\0'; ++i) {
                            if (szNew[i] == '(') ++iParenCount;
                            else if (szNew[i] == ')') --iParenCount;
                        }
                    }
                    while (iParenCount > 0 && iLen < EXPR_MAX - 1) {
                        szNew[iLen++] = ')';
                        --iParenCount;
                    }
                    szNew[iLen] = '\0';

                    /* Destroy old state */
                    if (g_pRenderNode != NULL) {
                        RenderNode_Destroy(g_pRenderNode);
                        g_pRenderNode = NULL;
                    }
                    if (g_pAstExpr != NULL) {
                        FzAstNode_Destroy(g_pAstExpr);
                        g_pAstExpr = NULL;
                    }
                    if (g_pVm != NULL) {
                        EzMachine_Destroy(g_pVm);
                        g_pVm = NULL;
                    }
                    g_pVm = EzMachine_Create();

                    /* Update expression and re-evaluate */
                    Utils_StringCopy(szExpr, EXPR_MAX, szNew);

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
static void setDlgFloat(HWND hDlg, int id, PZ_FLOAT f) {
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
static LRESULT CALLBACK WindowDlgProc(HWND hDlg, UINT message,
                                       WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG:
            setDlgFloat(hDlg, IDC_WIN_XMIN, Camera.xMin);
            setDlgFloat(hDlg, IDC_WIN_XMAX, Camera.xMax);
            setDlgFloat(hDlg, IDC_WIN_YMIN, Camera.yMin);
            setDlgFloat(hDlg, IDC_WIN_YMAX, Camera.yMax);
            setDlgFloat(hDlg, IDC_WIN_ZMIN, Camera.zMin);
            setDlgFloat(hDlg, IDC_WIN_ZMAX, Camera.zMax);
            {
                TCHAR szBufT[16];
                wsprintf(szBufT, TEXT("%d"), Camera.xGrid);
                SetDlgItemText(hDlg, IDC_WIN_XGRID, szBufT);
                wsprintf(szBufT, TEXT("%d"), Camera.yGrid);
                SetDlgItemText(hDlg, IDC_WIN_YGRID, szBufT);
            }
            CenterDialog(hDlg);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                {
                    HWND hWndParent;
                    int iGrid;
                    hWndParent = GetParent(hDlg);

                    Camera.xMin = getDlgFloat(hDlg, IDC_WIN_XMIN);
                    Camera.xMax = getDlgFloat(hDlg, IDC_WIN_XMAX);
                    Camera.yMin = getDlgFloat(hDlg, IDC_WIN_YMIN);
                    Camera.yMax = getDlgFloat(hDlg, IDC_WIN_YMAX);
                    Camera.zMin = getDlgFloat(hDlg, IDC_WIN_ZMIN);
                    Camera.zMax = getDlgFloat(hDlg, IDC_WIN_ZMAX);

                    iGrid = getDlgInt(hDlg, IDC_WIN_XGRID);
                    if (iGrid < 5) iGrid = 5;
                    if (iGrid > GRID_MAX) iGrid = GRID_MAX;
                    Camera.xGrid = iGrid;

                    iGrid = getDlgInt(hDlg, IDC_WIN_YGRID);
                    if (iGrid < 5) iGrid = 5;
                    if (iGrid > GRID_MAX) iGrid = GRID_MAX;
                    Camera.yGrid = iGrid;

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
        case WM_INITDIALOG:
        {
            int i, iCount;
            iCount = sizeof(PlotterZSamples) / sizeof(PlotterZSamples[0]);
            for (i = 0; i < iCount; ++i) {
                TCHAR szBufT[EXPR_MAX];
                const char* szSrc;
                int j;
                szSrc = PlotterZSamples[i].szExpr;
                for (j = 0; szSrc[j] != '\0' && j < EXPR_MAX - 1; ++j)
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
                case IDOK:
                {
                    LRESULT iSel;
                    iSel = SendDlgItemMessage(hDlg, IDC_SAMPLE_LIST,
                        LB_GETCURSEL, 0, 0);
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
static COLORREF parseHexColor(const char* szHex) {
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

static LRESULT CALLBACK PrefDlgProc(HWND hDlg, UINT message,
                                     WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG:
        {
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
                case IDC_INVERT_BUTTON:
                {
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
                case IDOK:
                {
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
                        g_rgbPalette[i] = parseHexColor(szHex);
                    }
                    recalcPaletteLUTs();
                    iPrevScale = g_iCanvasScaleFactor;
                    g_iCanvasScaleFactor = (SendMessage(GetDlgItem(hDlg, IDC_PREF_LOWRES), BM_GETCHECK, 0, 0)== BST_CHECKED) ? 2 : 1;
                    if (
                        g_iCanvasScaleFactor != iPrevScale ||
                        /* Workaround: force canvas recreation when bpp equals 8 */
                        g_iScreenBpp == 8
                    ) {
                        RecreateCanvasAndBackBuffer(hWndParent, iPrevScale);
                    }
                    if (g_iStage == STAGE_READY) {
                        redrawCanvas(hWndParent);
                    } else if (g_iStage == STAGE_ERROR) {
                        drawErrorScreen(hWndParent);
                    } else {
                        drawIdleScreen(hWndParent);
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
        case WM_INITDIALOG:
        {
            TCHAR szBuf[16];
            SendDlgItemMessage(hDlg, IDC_THRESHOLD_SLIDER, TBM_SETRANGE,
                (WPARAM)TRUE, (LPARAM)MAKELONG(20, 300));
            SendDlgItemMessage(hDlg, IDC_THRESHOLD_SLIDER, TBM_SETPOS,
                (WPARAM)TRUE, (LPARAM)g_iMouseRefreshMs);
            wsprintf(szBuf, TEXT("%d"), g_iMouseRefreshMs);
            SetDlgItemText(hDlg, IDC_THRESHOLD_EDIT, szBuf);
            return TRUE;
        }
        case WM_HSCROLL:
        {
            int iPos;
            TCHAR szBuf[16];
            iPos = (int)SendDlgItemMessage(hDlg, IDC_THRESHOLD_SLIDER,
                TBM_GETPOS, 0, 0);
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
                case IDOK:
                {
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
static LRESULT CALLBACK DebugDlgProc(HWND hDlg, UINT message,
                                      WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
        case WM_INITDIALOG:
        {
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
            if (g_pVm != NULL) {
                for (i = 0; i < g_pVm->iInstructionLength; ++i) {
                    EzInstruction* pInst = g_pVm->pInstructions + i;
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
        Utils_StringCopy(szExpr, EXPR_MAX, DEFAULT_EXPR);
        g_iExprPosX = 0;
        g_iExprPosY = 0;

        /* Destroy old state and recalc */
        if (g_pRenderNode != NULL) { RenderNode_Destroy(g_pRenderNode); g_pRenderNode = NULL; }
        if (g_pAstExpr != NULL) { FzAstNode_Destroy(g_pAstExpr); g_pAstExpr = NULL; }
        if (g_pVm != NULL) { EzMachine_Destroy(g_pVm); g_pVm = NULL; }
        g_pVm = EzMachine_Create();

        if (recalc(hWnd)) {
            /* Start performance test */
            g_bPerformanceTest = TRUE;
            g_nPaintCount = 0;
            g_dwPerfStartTime = GetTickCount();
            redrawCanvas(hWnd);
        } else {
            drawErrorScreen(hWnd);
        }
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

/*====================================================
 * Draw progress bar
 *   barY   - top Y coordinate of the bar
 *   iPct   - progress percentage (0-100)
 *====================================================*/
static void drawProgressBar(int barY, int iPct) {
    int barW, barH, barX;
    int iFillW;
    char szBuf[16];
    int iLen, tx, ty;

    barW = g_iCanvasW / 2;
    barH = CURRENT_FONT_HEIGHT * 2;
    barX = (g_iCanvasW - barW) / 2;
    if (barX < 0) barX = 0;

    /* Border */
    drawLineCanvas(barX, barY, barX + barW - 1, barY, COLOR_BLACK);
    drawLineCanvas(barX, barY + barH - 1, barX + barW - 1,
                   barY + barH - 1, COLOR_BLACK);
    drawLineCanvas(barX, barY, barX, barY + barH - 1, COLOR_BLACK);
    drawLineCanvas(barX + barW - 1, barY, barX + barW - 1,
                   barY + barH - 1, COLOR_BLACK);

    /* Interior fill WHITE */
    fillRectCanvas(barX + 1, barY + 1, barW - 2, barH - 2, COLOR_WHITE);

    /* Percentage text centered */
    Salvia_Format(szBuf, "%d%%", iPct);
    iLen = (int)strlen(szBuf) * CURRENT_FONT_WIDTH;
    tx = barX + (barW - iLen) / 2;
    ty = barY + (barH - CURRENT_FONT_HEIGHT) / 2;
    putTextCanvas(tx, ty, (const unsigned char*)szBuf, COLOR_BLACK);

    /* Invert filled portion */
    iFillW = barW * iPct / 100;
    if (iFillW > barW - 2) iFillW = barW - 2;
    if (iFillW > 0)
        invertRectCanvas(barX + 1, barY + 1, iFillW, barH - 2);
}

/*====================================================
 * Recalculate surface geometry
 *====================================================*/
int recalc(HWND hWnd) {
    char szErrorBuf[200];
    int ix, iy;
    int iPct, iLastPct;
    PZ_FLOAT fXbuf[GRID_MAX], fYbuf[GRID_MAX];
    PZ_FLOAT fz, fx, fy;
    PZ_FLOAT fzMid   = (PZ_FLOAT)(Camera.zMax + Camera.zMin) * 0.5f;
    PZ_FLOAT fzRange = (PZ_FLOAT)(Camera.zMax - Camera.zMin);
    PZ_FLOAT fxMid   = (PZ_FLOAT)(Camera.xMax + Camera.xMin) * 0.5f;
    PZ_FLOAT fxRange = (PZ_FLOAT)(Camera.xMax - Camera.xMin);
    PZ_FLOAT fyMid   = (PZ_FLOAT)(Camera.yMax + Camera.yMin) * 0.5f;
    PZ_FLOAT fyRange = (PZ_FLOAT)(Camera.yMax - Camera.yMin);
    EzError iCompileErr;

    if (g_pVm == NULL) { g_iStage = STAGE_ERROR; return 0; }

    if (g_pAstExpr == NULL) {
        g_pAstExpr = FzParser_ParseExpression(szExpr);
        if (g_pAstExpr == NULL) { g_iStage = STAGE_ERROR; return 0; }
        EzMachine_DeclareVariable(g_pVm, "x");
        EzMachine_DeclareVariable(g_pVm, "y");
        EzMachine_DeclareVariable(g_pVm, "pi");
        EzMachine_AllocateVariables(g_pVm);
        EzMachine_SetVariableByIndex(g_pVm, 2, PZ_PI);
        iCompileErr = EzMachine_Compile(g_pVm, g_pAstExpr, szErrorBuf);
        if (iCompileErr != EZERR_NONE) {
            g_iStage = STAGE_ERROR;
            switch (iCompileErr) {
                case EZERR_VARIABLE_UNDEFINED:
                    strcpy(g_szErrorBuf, "Undefined Variable: ");
                    strcpy(strchr(g_szErrorBuf, '\0'), szErrorBuf);
                    break;
                case EZERR_FUNCTION_UNDEFINED:
                    strcpy(g_szErrorBuf, "Undefined Function: ");
                    strcpy(strchr(g_szErrorBuf, '\0'), szErrorBuf);
                    break;
                case EZERR_FUNCTION_PARAM_MISMATCH:
                    strcpy(g_szErrorBuf, "Function parameters mismatch: ");
                    strcpy(strchr(g_szErrorBuf, '\0'), szErrorBuf);
                    break;
            }
            return 0;
        }
        if (g_pRenderNode == NULL) {
            g_pRenderNode = Render_Transform(g_pAstExpr);
            RenderNode_CalculateSize(g_pRenderNode, &g_RenderConfig);
        }
    }

    for (ix = 0; ix < Camera.xGrid; ++ix)
        fXbuf[ix] = Camera.xMin + (Camera.xMax - Camera.xMin) * ix / (PZ_FLOAT)(Camera.xGrid - 1);
    for (iy = 0; iy < Camera.yGrid; ++iy)
        fYbuf[iy] = Camera.yMin + (Camera.yMax - Camera.yMin) * iy / (PZ_FLOAT)(Camera.yGrid - 1);

    /* Show progress on canvas */
    {
        int y;
        fillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);
        y = g_iCanvasH / 2 - CURRENT_FONT_HEIGHT - 4;
        if (y < 0) y = 0;
        {
            int iLen = 10 * CURRENT_FONT_WIDTH;
            int x = (g_iCanvasW - iLen) / 2;
            if (x < 2) x = 2;
            putTextCanvas(x, y, (const unsigned char*)"Recalc ...", COLOR_BLACK);
        }
        drawProgressBar(y + CURRENT_FONT_HEIGHT + 4, 0);
        InvalidateRect(hWnd, NULL, FALSE);
        UpdateWindow(hWnd);
    }

    iLastPct = -1;
    for (ix = 0; ix < Camera.xGrid; ++ix) {
        fx = fXbuf[ix];
        for (iy = 0; iy < Camera.yGrid; ++iy) {
            fy = fYbuf[iy];
            EzMachine_SetVariableByIndex(g_pVm, 0, fx);
            EzMachine_SetVariableByIndex(g_pVm, 1, fy);
            fz = EzMachine_Eval(g_pVm);
            xBuf[ix] = PZ_FLOAT_TO_FIXED(2.0f * (fx - fxMid) / fxRange);
            yBuf[iy] = PZ_FLOAT_TO_FIXED(2.0f * (fy - fyMid) / fyRange);
            Z_BUF(ix, iy) = PZ_FLOAT_TO_FIXED(2.0f * (fz - fzMid) / fzRange);
        }
        iPct = (ix + 1) * 100 / Camera.xGrid;
        if (iPct != iLastPct) {
            MSG msg;
            int y;
            y = g_iCanvasH / 2 - CURRENT_FONT_HEIGHT - 4;
            if (y < 0) y = 0;
            drawProgressBar(y + CURRENT_FONT_HEIGHT + 4, iPct);
            InvalidateRect(hWnd, NULL, FALSE);
            UpdateWindow(hWnd);
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            iLastPct = iPct;
        }
    }
    g_iStage = STAGE_READY;
    return 1;
}

/*====================================================
 * Save session to INI file
 *====================================================*/
static void saveSession(HWND hWnd) {
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
    if (!GetSaveFileName(&ofn)) return;

    iPos = 0;
    iPos += Salvia_Format(szIni + iPos,
        "[expression]\r\nexpr=%s\r\n", szExpr);
    iPos += Salvia_Format(szIni + iPos,
        "[camera]\r\n"
        "alpha=%d\r\nbeta=%d\r\n"
        "xmin=%.4f\r\nxmax=%.4f\r\n"
        "ymin=%.4f\r\nymax=%.4f\r\n"
        "zmin=%.4f\r\nzmax=%.4f\r\n"
        "xgrid=%d\r\nygrid=%d\r\n"
        "zoom=%d\r\n"
        "viewx=%d\r\nviewy=%d\r\n"
        "exprx=%d\r\nexpry=%d\r\n",
        Camera.iAlphaDeg, Camera.iBetaDeg,
        Camera.xMin, Camera.xMax,
        Camera.yMin, Camera.yMax,
        Camera.zMin, Camera.zMax,
        Camera.xGrid, Camera.yGrid,
        Camera.iZoomLevel,
        Camera.iViewportX * g_iCanvasScaleFactor,
        Camera.iViewportY * g_iCanvasScaleFactor,
        g_iExprPosX * g_iCanvasScaleFactor,
        g_iExprPosY * g_iCanvasScaleFactor);
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

    hFile = CreateFile(szFile, GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;
    WriteFile(hFile, szIni, (DWORD)strlen(szIni), &dwWritten, NULL);
    CloseHandle(hFile);
}

/*====================================================
 * Load session from INI file
 *====================================================*/
static void loadSession(HWND hWnd) {
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
    if (!GetOpenFileName(&ofn)) return;

    hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    dwSize = GetFileSize(hFile, NULL);
    if (dwSize == INVALID_FILE_SIZE || dwSize > 8191)
        { CloseHandle(hFile); return; }

    pBuf = (char*)malloc((size_t)(dwSize + 1));
    if (pBuf == NULL) { CloseHandle(hFile); return; }

    if (!ReadFile(hFile, pBuf, dwSize, &dwRead, NULL) || dwRead != dwSize) {
        free(pBuf); CloseHandle(hFile); return;
    }
    CloseHandle(hFile);
    pBuf[dwSize] = '\0';

    pIni = PineIni_Parse(pBuf, &iniErr);
    free(pBuf);
    if (pIni == NULL) return;

    /* Parse [expression] */
    pSec = PineIni_Find(pIni, "expression");
    if (pSec != NULL)
        Utils_StringCopy(szExpr, EXPR_MAX,
            PineIni_Section_GetString(pSec, "expr", szExpr));

    /* Parse [camera] */
    pSec = PineIni_Find(pIni, "camera");
    if (pSec != NULL) {
        Camera.iAlphaDeg  = PineIni_Section_GetInt(pSec, "alpha",  Camera.iAlphaDeg);
        Camera.iBetaDeg   = PineIni_Section_GetInt(pSec, "beta",   Camera.iBetaDeg);
        Camera.xMin = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "xmin", "-6"));
        Camera.xMax = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "xmax", "6"));
        Camera.yMin = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "ymin", "-6"));
        Camera.yMax = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "ymax", "6"));
        Camera.zMin = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "zmin", "-3"));
        Camera.zMax = (PZ_FLOAT)Utils_Atof(PineIni_Section_GetString(pSec, "zmax", "3"));
        Camera.xGrid = PineIni_Section_GetInt(pSec, "xgrid", Camera.xGrid);
        Camera.yGrid = PineIni_Section_GetInt(pSec, "ygrid", Camera.yGrid);
        Camera.iZoomLevel = PineIni_Section_GetInt(pSec, "zoom", Camera.iZoomLevel);
        Camera.iViewportX = PineIni_Section_GetInt(pSec, "viewx", Camera.iViewportX);
        Camera.iViewportY = PineIni_Section_GetInt(pSec, "viewy", Camera.iViewportY);
        g_iExprPosX = PineIni_Section_GetInt(pSec, "exprx", g_iExprPosX);
        g_iExprPosY = PineIni_Section_GetInt(pSec, "expry", g_iExprPosY);
        if (g_iCanvasScaleFactor > 1) {
            Camera.iViewportX /= g_iCanvasScaleFactor;
            Camera.iViewportY /= g_iCanvasScaleFactor;
            g_iExprPosX      /= g_iCanvasScaleFactor;
            g_iExprPosY      /= g_iCanvasScaleFactor;
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
    recalcPaletteLUTs();
    /* Workaround: force canvas recreation when bpp equals 8 */
    if (g_iScreenBpp == 8) {
        RecreateCanvasAndBackBuffer(hWnd, g_iCanvasScaleFactor);
    };
    
    PineIni_Destroy(pIni);

    /* Destroy old state and recalc */
    if (g_pRenderNode != NULL) { RenderNode_Destroy(g_pRenderNode); g_pRenderNode = NULL; }
    if (g_pAstExpr != NULL)   { FzAstNode_Destroy(g_pAstExpr); g_pAstExpr = NULL; }
    if (g_pVm != NULL)        { EzMachine_Destroy(g_pVm); g_pVm = NULL; }
    g_pVm = EzMachine_Create();

    if (recalc(hWnd)) {
        redrawCanvas(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
    } else {
        drawErrorScreen(hWnd);
    }
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
                    saveSession(hWnd);
                    break;
                case IDM_FILE_LOADSESSION:
                    loadSession(hWnd);
                    break;
                case IDM_FILE_SAMPLES:
                {
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
                        Camera.xGrid = SAMPLE_DEFAULT_GRID;
                        Camera.yGrid = SAMPLE_DEFAULT_GRID;
                        Utils_StringCopy(szExpr, EXPR_MAX, p->szExpr);
                        if (g_pRenderNode != NULL) {
                            RenderNode_Destroy(g_pRenderNode);
                            g_pRenderNode = NULL;
                        }
                        if (g_pAstExpr != NULL) {
                            FzAstNode_Destroy(g_pAstExpr);
                            g_pAstExpr = NULL;
                        }
                        if (g_pVm != NULL) {
                            EzMachine_Destroy(g_pVm);
                            g_pVm = NULL;
                        }
                        g_pVm = EzMachine_Create();
                        if (recalc(hWnd)) {
                            redrawCanvas(hWnd);
                            InvalidateRect(hWnd, NULL, FALSE);
                        } else {
                            drawErrorScreen(hWnd);
                        }
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
                case IDM_EDIT_EXPRESSION: {
                    int iRet;
                    iRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_EXPRESSION), hWnd, (DLGPROC)ExprDlgProc);
                    if (iRet == IDOK) {
                        /* Destroy old state and recalc */
                        if (g_pRenderNode != NULL) {
                            RenderNode_Destroy(g_pRenderNode);
                            g_pRenderNode = NULL;
                        }
                        if (g_pAstExpr != NULL) {
                            FzAstNode_Destroy(g_pAstExpr);
                            g_pAstExpr = NULL;
                        }
                        if (g_pVm != NULL) {
                            EzMachine_Destroy(g_pVm);
                            g_pVm = NULL;
                        }
                        g_pVm = EzMachine_Create();
                        if (recalc(hWnd)) {
                            redrawCanvas(hWnd);
                            InvalidateRect(hWnd, NULL, FALSE);
                        } else {
                            drawErrorScreen(hWnd);
                        }
                    }
                    break;
                }
                case IDM_EDIT_WINDOW: {
                    int iRet;
                    iRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_WINDOWEDIT), hWnd, (DLGPROC)WindowDlgProc);
                    if (iRet == IDOK) {
                        if (recalc(hWnd)) {
                            redrawCanvas(hWnd);
                            InvalidateRect(hWnd, NULL, FALSE);
                        } else {
                            drawErrorScreen(hWnd);
                        }
                    }
                    break;
                }
                case IDM_VIEW_CAMERA:
                    g_iMouseMode = MOUSE_MODE_CAMERA;
                    if (g_bShowFooter) { redrawCanvas(hWnd); InvalidateRect(hWnd, NULL, FALSE); }
                    break;
                case IDM_VIEW_PAN_MOVE:
                    g_iMouseMode = MOUSE_MODE_PAN_MOVE;
                    if (g_bShowFooter) { redrawCanvas(hWnd); InvalidateRect(hWnd, NULL, FALSE); }
                    break;
                case IDM_VIEW_ZOOM:
                    g_iMouseMode = MOUSE_MODE_ZOOM;
                    g_iZoomAccum = 0;
                    if (g_bShowFooter) { redrawCanvas(hWnd); InvalidateRect(hWnd, NULL, FALSE); }
                    break;
                case IDM_VIEW_FORMULA:
                    g_iMouseMode = MOUSE_MODE_FORMULA;
                    if (g_bShowFooter) { redrawCanvas(hWnd); InvalidateRect(hWnd, NULL, FALSE); }
                    break;
                case IDM_VIEW_FOV:
                    g_iMouseMode = MOUSE_MODE_FOV;
                    g_iFovAccum = 0;
                    if (g_bShowFooter) { redrawCanvas(hWnd); InvalidateRect(hWnd, NULL, FALSE); }
                    break;
                case IDM_VIEW_ORTHO_PERSP:
                    g_iProjection = !g_iProjection;
                    if (g_iProjection == ORTHOGRAPHIC) {
                        xyz2xy = PzCamera_OrthoProjectFixed;
                        if (g_iMouseMode == MOUSE_MODE_FOV)
                            g_iMouseMode = MOUSE_MODE_CAMERA;
                    } else {
                        xyz2xy = PzCamera_PerspProjectFixed;
                    }
                    redrawCanvas(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;
                case IDM_VIEW_RESET:
                    resetView(hWnd);
                    break;
                case IDM_VIEW_TOGGLEFOOTER:
                    g_bShowFooter = !g_bShowFooter;
                    redrawCanvas(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;
                case IDM_VIEW_TOGGLEBOX:
                    g_bShowBox = !g_bShowBox;
                    redrawCanvas(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE:
            {
                RECT rcClient;
                int iCw, iCh;

                recalcPaletteLUTs();
    
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

                if (!createCanvas(iCw, iCh)) {
                    MessageBox(hWnd, TEXT("Failed to create canvas"), TEXT("ERROR"), MB_OK);
                    return -1;
                }
                if (!createBackBuffer(hWnd, rcClient.right, rcClient.bottom)) {
                    MessageBox(hWnd, TEXT("Failed to create back buffer"), TEXT("ERROR"), MB_OK);
                    return -1;
                }

                PzCamera_Initialize();

                Camera.iViewportS = (g_iCanvasH < g_iCanvasW ? g_iCanvasH : g_iCanvasW) / 2;
                Camera.iViewportX = g_iCanvasW / 2;
                Camera.iViewportY = g_iCanvasH / 2;

                RenderConfig_GetDefaultStyle(&g_RenderConfig);
                RenderConfig_CalculateBigSymbolPoints(&g_RenderConfig);
                g_RenderConfig.sInterfaces.setPixel = rzSetPixel;
                g_RenderConfig.sInterfaces.plotLine = rzPlotLine;
                g_RenderConfig.sInterfaces.putChar  = rzPutChar;
                g_pVm = EzMachine_Create();

                drawIdleScreen(hWnd);
                UpdateWindow(hWnd);
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
            redrawCanvas(hWnd);
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
                        g_iExprPosX += iDeltaX;
                        g_iExprPosY += iDeltaY;
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
                        redrawCanvas(hWnd);
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
                        redrawCanvas(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    case 'P':
                        g_iMouseMode = MOUSE_MODE_PAN_MOVE;
                        redrawCanvas(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    case 'Z':
                        g_iMouseMode = MOUSE_MODE_ZOOM;
                        redrawCanvas(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    case 'F':
                        g_iMouseMode = MOUSE_MODE_FORMULA;
                        redrawCanvas(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    case 'V':
                        g_iMouseMode = MOUSE_MODE_FOV;
                        g_iFovAccum = 0;
                        redrawCanvas(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    case 'R':
                        resetView(hWnd);
                        return 0;
                    case 'T':
                        g_bShowFooter = !g_bShowFooter;
                        redrawCanvas(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
                        return 0;
                    case 'B':
                        g_bShowBox = !g_bShowBox;
                        redrawCanvas(hWnd);
                        InvalidateRect(hWnd, NULL, FALSE);
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
                        if (wParam == VK_LEFT)  { g_iExprPosX -= 10; bRedraw = 1; }
                        if (wParam == VK_RIGHT) { g_iExprPosX += 10; bRedraw = 1; }
                        if (wParam == VK_UP)    { g_iExprPosY -= 10; bRedraw = 1; }
                        if (wParam == VK_DOWN)  { g_iExprPosY += 10; bRedraw = 1; }
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
                    redrawCanvas(hWnd);
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
            paintCanvasToWindow(hWnd);
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
                    destroyBackBuffer();
                    destroyCanvas();
                    createCanvas(iCw, iCh);
                    createBackBuffer(hWnd, iNewW, iNewH);
                    Camera.iViewportS = (g_iCanvasH < g_iCanvasW ? g_iCanvasH : g_iCanvasW) / 2;
                    Camera.iViewportX = g_iCanvasW / 2;
                    Camera.iViewportY = g_iCanvasH / 2;
                    if (g_iStage == STAGE_IDLE)
                        drawIdleScreen(hWnd);
                    else if (g_iStage == STAGE_ERROR)
                        drawErrorScreen(hWnd);
                    else
                        redrawCanvas(hWnd);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
            break;
        case WM_DESTROY:
#if VER_PLATFORM_WIN32_CE
            CommandBar_Destroy(hwndCB);
#endif
            if (g_pRenderNode != NULL) RenderNode_Destroy(g_pRenderNode);
            if (g_pVm != NULL) EzMachine_Destroy(g_pVm);
            if (g_pAstExpr != NULL) FzAstNode_Destroy(g_pAstExpr);
            destroyBackBuffer();
            destroyCanvas();
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
        Utils_StringCopy(szExpr, EXPR_MAX, PineIni_Section_GetString(pSec, "expr", szExpr));
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
    redrawCanvas(hWnd);
    Salvia_Format(szBuf, "Frame = %d", g_nPaintCount);
    iWidth = strlen(szBuf) * CURRENT_FONT_WIDTH;
    iLeft = (g_iCanvasW - iWidth) / 2;
    fillRectCanvas(0, iTop - iPadding, g_iCanvasW, CURRENT_FONT_HEIGHT + iPadding * 2, COLOR_BLACK);
    putTextCanvas(iLeft, iTop, (const unsigned char*)szBuf, COLOR_WHITE);
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
        fillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);
        fillRectCanvas(0, 0, g_iCanvasW, CURRENT_FONT_HEIGHT * 2, COLOR_BLACK);
        putTextCanvas(CURRENT_FONT_WIDTH, CURRENT_FONT_HEIGHT / 2, (const unsigned char *)"Test Result", COLOR_WHITE);
        Salvia_Format(szBuf, "Duration:     %ds", PERF_TEST_SECONDS);
        putTextCanvas(0, CURRENT_FONT_HEIGHT * 2, (const unsigned char *)szBuf, COLOR_BLACK);
        Salvia_Format(szBuf, "Frames drawn: %d", g_nPaintCount);
        putTextCanvas(0, CURRENT_FONT_HEIGHT * 3, (const unsigned char *)szBuf, COLOR_BLACK);
        Salvia_Format(szBuf, "Average FPS:  %.2f", fFps);
        putTextCanvas(0, CURRENT_FONT_HEIGHT * 4, (const unsigned char *)szBuf, COLOR_BLACK);
        Salvia_Format(szBuf, "Recommended Threshold: %d", iThreshold);
        putTextCanvas(0, CURRENT_FONT_HEIGHT * 5, (const unsigned char *)szBuf, COLOR_BLACK);
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

void RecreateCanvasAndBackBuffer(HWND hWnd, int iPrevScale) {
    RECT rc;
    int iCw, iCh;
    int iScaleOld, iScaleNew;
    iScaleOld = iPrevScale;
    iScaleNew = g_iCanvasScaleFactor;
    Camera.iViewportX = Camera.iViewportX * iScaleOld / iScaleNew;
    Camera.iViewportY = Camera.iViewportY * iScaleOld / iScaleNew;
    g_iExprPosX = g_iExprPosX * iScaleOld / iScaleNew;
    g_iExprPosY = g_iExprPosY * iScaleOld / iScaleNew;
    destroyBackBuffer();
    destroyCanvas();
    GetClientRect(hWnd, &rc);
    rc.bottom -= g_iBarHeight;
    if (rc.bottom < 1) rc.bottom = 1;
    iCw = rc.right / g_iCanvasScaleFactor;
    iCh = rc.bottom / g_iCanvasScaleFactor;
    if (iCw < 1) iCw = 1;
    if (iCh < 1) iCh = 1;
    createCanvas(iCw, iCh);
    createBackBuffer(hWnd, rc.right, rc.bottom);
    Camera.iViewportS = (g_iCanvasH < g_iCanvasW ? g_iCanvasH : g_iCanvasW) / 2;
}