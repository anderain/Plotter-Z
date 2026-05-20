#include "stage-common.h"

/*====================================================
 * Color palette (grayscale)
 *====================================================*/
#define HIGH_CONTRAST_COLOR 1

#ifdef HIGH_CONTRAST_COLOR
const COLORREF g_rgbPalette[] = {
    RGB(0xff, 0xff, 0xff),      /* COLOR_BLACK       */
    RGB(0xaa, 0xaa, 0xaa),      /* COLOR_DARK_GRAY   */
    RGB(0x66, 0x66, 0x66),      /* COLOR_LIGHT_GRAY  */
    RGB(0x00, 0x00, 0x00),      /* COLOR_WHITE       */
};
#else
const COLORREF g_rgbPalette[] = {
    RGB(0x11, 0x50, 0x00),      /* COLOR_BLACK       */
    RGB(0x22, 0x66, 0x00),      /* COLOR_DARK_GRAY   */
    RGB(0x43, 0x7f, 0x00),      /* COLOR_LIGHT_GRAY  */
    RGB(0xaa, 0xcc, 0x00),      /* COLOR_WHITE       */
};
#endif

/*====================================================
 * Expression & Camera
 *====================================================*/
char szExpr[EXPR_MAX] = "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))";

const int arrZoomLevels[] = {
    PZ_FLOAT_TO_FIXED(0.33f),
    PZ_FLOAT_TO_FIXED(0.50f),
    PZ_FLOAT_TO_FIXED(0.75f),
    (int)PZ_FIXED_ONE,
    PZ_FLOAT_TO_FIXED(1.50f),
    (int)PZ_FIXED_ONE * 2,
    (int)PZ_FIXED_ONE * 4,
    (int)PZ_FIXED_ONE * 8,
};
const int iNumZoomLevel = sizeof(arrZoomLevels) / sizeof(arrZoomLevels[0]);

struct CameraStruct Camera = {
    0, 0, 0,
    DEFAULT_VIEW_ALPHA, DEFAULT_VIEW_BETA,
    0, 0, 0, 0,
    -6.0f, 6.0f, 20,
    -6.0f, 6.0f, 20,
    -3.0f, 3.0f,
    ZOOM_LEVEL_DEFAULT
};

PZ_FIXED zBuf[GRID_MAX * GRID_MAX];
PZ_FIXED xBuf[GRID_MAX];
PZ_FIXED yBuf[GRID_MAX];

FzAstNode*      g_pAstExpr = NULL;
EzMachine*      g_pVm = NULL;
RenderNode*     g_pRenderNode = NULL;
RenderConfig    g_RenderConfig;
char            g_szErrorBuf[EZ_ERROR_CONTENT_LENGTH];
int             g_bShowBox      = 1;
int             g_bMouseDown    = 0;
int             g_bPanMode      = 0;
int             g_bExprDrag     = 0;
int             g_iMousePrevX   = 0;
int             g_iMousePrevY   = 0;
int             g_iExprPosX     = 0;
int             g_iExprPosY     = 0;

/*====================================================
 * Stage system
 *====================================================*/
int g_iStage = STAGE_WIREFRAME;
int g_iRecalcProgress = 0;
int g_iRecalcTotal    = 0;

/*====================================================
 * 2bpp canvas
 *====================================================*/
unsigned char*   g_pCanvas       = NULL;
int              g_iCanvasW      = 0;
int              g_iCanvasH      = 0;
int              g_iCanvasPitch  = 0;

static int canvasByteIndex(int x, int y) {
    return y * g_iCanvasPitch + (x >> 2);
}

static int canvasShift(int x) {
    return 6 - ((x & 3) << 1);
}

#define CANVAS_MASK(x)  (~(0x03 << canvasShift(x)))

/*====================================================
 * Drawing primitives (2bpp canvas)
 *====================================================*/
#define ABS(v)  ((v) < 0 ? -(v) : (v))

void setPixelCanvas(int x, int y, int iColor) {
    unsigned char ucMask;
    int iByte;
    if (x < 0 || x >= g_iCanvasW || y < 0 || y >= g_iCanvasH) return;
    iByte = canvasByteIndex(x, y);
    ucMask = CANVAS_MASK(x);
    g_pCanvas[iByte] = (unsigned char)((g_pCanvas[iByte] & ucMask)
                     | ((iColor & 0x03) << canvasShift(x)));
}

int getPixelCanvas(int x, int y) {
    if (x < 0 || x >= g_iCanvasW || y < 0 || y >= g_iCanvasH) return 0;
    return (g_pCanvas[canvasByteIndex(x, y)] >> canvasShift(x)) & 0x03;
}

void drawLineCanvas(int x0, int y0, int x1, int y1, int iColor) {
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

void fillRectCanvas(int dx, int dy, int w, int h, int iColor) {
    int x, y;
    for (y = 0; y < h; ++y)
        for (x = 0; x < w; ++x)
            setPixelCanvas(dx + x, dy + y, iColor);
}

void draw1bppCanvas(const unsigned char *raw,
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

void putCharCanvas(int x, int y, unsigned char ch, int iColor) {
    draw1bppCanvas(FONT_HYBIRD_6x8 + 8 * (int)ch, x, y, 8, 8, iColor);
}

void putTextCanvas(int x, int y, const unsigned char* usz, int iColor) {
    for (; *usz; ++usz, x += CURRENT_FONT_WIDTH)
        putCharCanvas(x, y, *usz, iColor);
}

void rzSetPixel(int x, int y) {
    setPixelCanvas(x, y, COLOR_BLACK);
}

void rzPlotLine(int x0, int y0, int x1, int y1) {
    drawLineCanvas(x0, y0, x1, y1, COLOR_BLACK);
}

void rzPutChar(int x, int y, unsigned char ch) {
    putCharCanvas(x, y, ch, COLOR_BLACK);
}

/*====================================================
 * 3D projection (fixed-point)
 *====================================================*/
void xyz2xy(PZ_FIXED x, PZ_FIXED y, PZ_FIXED z, int *ox, int *oy) {
    int iZoom = arrZoomLevels[Camera.iZoomLevel];
    int iScale = (int)(((int)Camera.iViewportS * iZoom + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
    int nx, ny;
    nx = PZ_FIXED_MUL(y, Camera.sinB) - PZ_FIXED_MUL(x, Camera.cosB);
    ny = PZ_FIXED_MUL(PZ_FIXED_MUL(x, Camera.sinB) + PZ_FIXED_MUL(y, Camera.cosB), Camera.sinA)
       - PZ_FIXED_MUL(z, Camera.cosA);
    *ox = Camera.iViewportX + (int)(((int)iScale * nx + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
    *oy = Camera.iViewportY + (int)(((int)iScale * ny + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
}

/*====================================================
 * Recalculate surface geometry
 *====================================================*/
int recalc(void) {
    int ix, iy;
    PZ_FLOAT fXbuf[GRID_MAX], fYbuf[GRID_MAX];
    PZ_FLOAT fz, fx, fy;
    PZ_FLOAT fzMid   = (PZ_FLOAT)(Camera.zMax + Camera.zMin) * 0.5f;
    PZ_FLOAT fzRange = (PZ_FLOAT)(Camera.zMax - Camera.zMin);
    PZ_FLOAT fxMid   = (PZ_FLOAT)(Camera.xMax + Camera.xMin) * 0.5f;
    PZ_FLOAT fxRange = (PZ_FLOAT)(Camera.xMax - Camera.xMin);
    PZ_FLOAT fyMid   = (PZ_FLOAT)(Camera.yMax + Camera.yMin) * 0.5f;
    PZ_FLOAT fyRange = (PZ_FLOAT)(Camera.yMax - Camera.yMin);
    EzError iCompileErr;

    if (g_pVm == NULL) return 0;

    if (g_pAstExpr == NULL) {
        g_pAstExpr = FzParser_ParseExpression(szExpr);
        if (g_pAstExpr == NULL) return 0;
        EzMachine_DeclareVariable(g_pVm, "x");
        EzMachine_DeclareVariable(g_pVm, "y");
        EzMachine_DeclareVariable(g_pVm, "pi");
        EzMachine_AllocateVariables(g_pVm);
        EzMachine_SetVariableByIndex(g_pVm, 2, PZ_PI);
        iCompileErr = EzMachine_Compile(g_pVm, g_pAstExpr, g_szErrorBuf);
        if (iCompileErr != EZERR_NONE) return 0;
        if (g_pRenderNode == NULL) {
            g_pRenderNode = Render_Transform(g_pAstExpr);
            RenderNode_EstimateSize(g_pRenderNode, &g_RenderConfig);
        }
    }

    g_iRecalcTotal = Camera.xGrid;
    g_iRecalcProgress = 0;

    for (ix = 0; ix < Camera.xGrid; ++ix)
        fXbuf[ix] = Camera.xMin + (Camera.xMax - Camera.xMin) * ix / (PZ_FLOAT)(Camera.xGrid - 1);
    for (iy = 0; iy < Camera.yGrid; ++iy)
        fYbuf[iy] = Camera.yMin + (Camera.yMax - Camera.yMin) * iy / (PZ_FLOAT)(Camera.yGrid - 1);

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
        g_iRecalcProgress = ix + 1;
    }
    g_iRecalcProgress = g_iRecalcTotal;
    return 1;
}

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
 * Full redraw (clear -> box -> surface)
 *====================================================*/
void redrawCanvas(void) {
    int iBaseline;
    int iStartY;
    char szBuf[48];
    int iLen;

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
        RenderNode_Draw(g_pRenderNode, &g_RenderConfig, g_iExprPosX, g_iExprPosY + iBaseline);
    }

    /* Footer bar */
    iStartY = g_iCanvasH - CURRENT_FONT_HEIGHT - 2;
    if (iStartY < 0) iStartY = 0;
    fillRectCanvas(0, iStartY, g_iCanvasW, CURRENT_FONT_HEIGHT + 2, COLOR_BLACK);

    /* Left: alpha/beta */
    Salvia_Format(szBuf, "%c=%d, %c=%d",
        PZ_AE_GREEK_alpha, Camera.iAlphaDeg,
        PZ_AE_GREEK_beta,  Camera.iBetaDeg);
    putTextCanvas(2, iStartY + 2, (const unsigned char*)szBuf, COLOR_WHITE);

    /* Center: W/E/R */
    {
        const char* szCenter = "Key: W / E / R";
        iLen = (int)strlen(szCenter) * CURRENT_FONT_WIDTH;
        putTextCanvas((g_iCanvasW - iLen) / 2, iStartY + 2,
            (const unsigned char*)szCenter, COLOR_WHITE);
    }

    /* Right: zoom%, (x, y) */
    Salvia_Format(szBuf, "%d%% (%d, %d)",
        (int)(PZ_FIXED_TO_FLOAT(arrZoomLevels[Camera.iZoomLevel]) * 100.0f),
        Camera.iViewportX, Camera.iViewportY);
    iLen = (int)strlen(szBuf) * CURRENT_FONT_WIDTH;
    putTextCanvas(g_iCanvasW - iLen - 2, iStartY + 2,
        (const unsigned char*)szBuf, COLOR_WHITE);
}

/*====================================================
 * Canvas allocation and cleanup
 *====================================================*/
int createCanvas(int iWidth, int iHeight) {
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

void destroyCanvas(void) {
    if (g_pCanvas != NULL) { free(g_pCanvas); g_pCanvas = NULL; }
    g_iCanvasW = 0; g_iCanvasH = 0; g_iCanvasPitch = 0;
}

/*====================================================
 * Back buffer
 *====================================================*/
HDC       g_hdcBuffer     = NULL;
HBITMAP   g_hBmpBuffer    = NULL;
HBITMAP   g_hBmpOld       = NULL;
BYTE*     g_pDibPixels    = NULL;
int       g_iScreenBpp    = 0;
int       g_iDibPitch     = 0;

int createBackBuffer(HWND hWnd, int iWidth, int iHeight) {
    HDC hdc;
    HBITMAP hBmpTest;
    BITMAP bmTest;
    BITMAPINFO* pbmi;
    int iPaletteSize = 0, iBitCount, i, iBmiSize;

    hdc = GetDC(hWnd);
    if (hdc == NULL) return 0;
    hBmpTest = CreateCompatibleBitmap(hdc, 8, 8);
    if (hBmpTest == NULL) { ReleaseDC(hWnd, hdc); return 0; }
    GetObject(hBmpTest, sizeof(BITMAP), &bmTest);
    DeleteObject(hBmpTest);
    ReleaseDC(hWnd, hdc);

    g_iScreenBpp = bmTest.bmBitsPixel;
    switch (g_iScreenBpp) {
        case 2:  iBitCount = 2;  iPaletteSize = 4;   break;
        case 8:  iBitCount = 8;  iPaletteSize = 256; break;
        case 16: iBitCount = 16; iPaletteSize = 0;   break;
        case 32: iBitCount = 32; iPaletteSize = 0;   break;
        default: return 0;
    }

    iBmiSize = sizeof(BITMAPINFO) + iPaletteSize * sizeof(RGBQUAD);
    pbmi = (BITMAPINFO*)malloc((size_t)iBmiSize);
    if (pbmi == NULL) return 0;
    memset(pbmi, 0, (size_t)iBmiSize);
    pbmi->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth       = iWidth;
    pbmi->bmiHeader.biHeight      = -iHeight;
    pbmi->bmiHeader.biPlanes      = 1;
    pbmi->bmiHeader.biBitCount    = iBitCount;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biClrUsed     = (g_iScreenBpp == 2) ? 0 : iPaletteSize;

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

    g_hdcBuffer = CreateCompatibleDC(NULL);
    if (g_hdcBuffer == NULL) { free(pbmi); return 0; }
    g_hBmpBuffer = CreateDIBSection(g_hdcBuffer, pbmi, DIB_RGB_COLORS,
                                     (void**)&g_pDibPixels, NULL, 0);
    free(pbmi);
    if (g_hBmpBuffer == NULL || g_pDibPixels == NULL) {
        DeleteDC(g_hdcBuffer); g_hdcBuffer = NULL; return 0;
    }
    g_hBmpOld = SelectObject(g_hdcBuffer, g_hBmpBuffer);
    {
        BITMAP bm;
        GetObject(g_hBmpBuffer, sizeof(BITMAP), &bm);
        g_iDibPitch = (int)bm.bmWidthBytes;
    }
    if (g_iDibPitch <= 0) g_iDibPitch = (iWidth * iBitCount + 7) / 8;
    return 1;
}

void destroyBackBuffer(void) {
    if (g_hdcBuffer != NULL) {
        if (g_hBmpOld != NULL) SelectObject(g_hdcBuffer, g_hBmpOld);
        if (g_hBmpBuffer != NULL) DeleteObject(g_hBmpBuffer);
        DeleteDC(g_hdcBuffer);
    }
    g_hdcBuffer = NULL; g_hBmpBuffer = NULL; g_hBmpOld = NULL;
    g_pDibPixels = NULL; g_iScreenBpp = 0; g_iDibPitch = 0;
}

void paintCanvasToWindow(HWND hWnd) {
    HDC hdc;
    PAINTSTRUCT ps;
    int iX, iY;
    hdc = BeginPaint(hWnd, &ps);
    if (g_pCanvas == NULL || g_iCanvasW <= 0 || g_iCanvasH <= 0) {
        EndPaint(hWnd, &ps); return;
    }
    if (g_pDibPixels != NULL && g_iScreenBpp != 0) {
        BYTE* pRow = g_pDibPixels;
        for (iY = 0; iY < g_iCanvasH; ++iY) {
            BYTE* pDst = pRow;
            switch (g_iScreenBpp) {
                case 2: {
                    int iSrcRow = iY * g_iCanvasPitch;
                    int iXEnd = g_iCanvasPitch;
                    for (iX = 0; iX < iXEnd; ++iX)
                        pDst[iX] = g_pCanvas[iSrcRow + iX];
                    break;
                }
                case 8:
                    for (iX = 0; iX < g_iCanvasW; ++iX)
                        pDst[iX] = (BYTE)getPixelCanvas(iX, iY);
                    break;
                case 16: {
                    WORD* pDst16 = (WORD*)pDst;
                    for (iX = 0; iX < g_iCanvasW; ++iX) {
                        int iColor = getPixelCanvas(iX, iY);
                        COLORREF rgb = g_rgbPalette[iColor];
                        pDst16[iX] = (WORD)(((GetRValue(rgb) >> 3) << 11)
                                          | ((GetGValue(rgb) >> 2) << 5)
                                          |  (GetBValue(rgb) >> 3));
                    }
                    break;
                }
                case 32: {
                    DWORD* pDst32 = (DWORD*)pDst;
                    for (iX = 0; iX < g_iCanvasW; ++iX) {
                        COLORREF rgb = g_rgbPalette[getPixelCanvas(iX, iY)];
                        pDst32[iX] = (DWORD)((GetRValue(rgb) << 16)
                                           | (GetGValue(rgb) << 8)
                                           |  GetBValue(rgb));
                    }
                    break;
                }
            }
            pRow += g_iDibPitch;
        }
        BitBlt(hdc, 0, 0, g_iCanvasW, g_iCanvasH, g_hdcBuffer, 0, 0, SRCCOPY);
    } else {
        for (iY = 0; iY < g_iCanvasH; ++iY)
            for (iX = 0; iX < g_iCanvasW; ++iX)
                SetPixel(hdc, iX, iY, g_rgbPalette[getPixelCanvas(iX, iY)]);
    }
    EndPaint(hWnd, &ps);
}

/*====================================================
 * Win32 standard boilerplate
 *====================================================*/
HINSTANCE hInst;

BOOL                SetTaskbarVisible   (BOOL bVisible);
ATOM                MyRegisterClass     (HINSTANCE hInstance, LPTSTR szWindowClass);
BOOL                InitInstance        (HINSTANCE, int);
LRESULT CALLBACK    WndProc             (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    About               (HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain( HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR    lpCmdLine,
                    int       nCmdShow) {
    MSG msg;
    HACCEL hAccelTable;
    
    SetTaskbarVisible(FALSE);

    if (!InitInstance(hInstance, nCmdShow)) return FALSE;
    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_PLOTTERZCLASSIC);
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass) {
    WNDCLASS wc;
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = (WNDPROC) WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PLOTTERZCLASSIC));
    wc.hCursor          = 0;
    wc.hbrBackground    = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName     = 0;
    wc.lpszClassName    = szWindowClass;
    return RegisterClass(&wc);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    HWND    hWnd;
    TCHAR   szTitle[MAX_LOADSTRING];
    TCHAR   szWindowClass[MAX_LOADSTRING];
    int     iScrWidth, iScrHeight;
    int     iWinWidth, iWinHeight;
    DWORD   dWindowStyle;
    RECT    rcClient;

    hInst = hInstance;
    LoadString(hInstance, IDC_PLOTTERZCLASSIC, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance, szWindowClass);
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

    rcClient.left   = 0;
    rcClient.top    = 0;
#if !(VER_PLATFORM_WIN32_CE)
    rcClient.right  = 640;
    rcClient.bottom = 480;
    dWindowStyle    = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rcClient, dWindowStyle, FALSE);
#else
    iScrWidth       = GetSystemMetrics(SM_CXSCREEN);
    iScrHeight      = GetSystemMetrics(SM_CYSCREEN);
    rcClient.right  = iScrWidth;
    rcClient.bottom = iScrHeight;
    dWindowStyle    = WS_VISIBLE;
#endif
    iWinWidth   = rcClient.right - rcClient.left;
    iWinHeight  = rcClient.bottom - rcClient.top;
    hWnd = CreateWindow(szWindowClass, szTitle, dWindowStyle,
        0, 0, iWinWidth, iWinHeight, NULL, NULL, hInstance, NULL);
    if (!hWnd) return FALSE;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
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
                case IDM_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default: return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE:
            {
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);
                if (rcClient.right <= 0) rcClient.right = 320;
                if (rcClient.bottom <= 0) rcClient.bottom = 240;
                if (!createCanvas(rcClient.right, rcClient.bottom)) return -1;
                if (!createBackBuffer(hWnd, rcClient.right, rcClient.bottom)) {}
                Camera.iViewportS = (g_iCanvasH < g_iCanvasW ? g_iCanvasH : g_iCanvasW) / 2;
                Camera.iViewportX = g_iCanvasW / 2;
                Camera.iViewportY = g_iCanvasH / 2;
                g_RenderConfig.sInterfaces.setPixel = rzSetPixel;
                g_RenderConfig.sInterfaces.plotLine = rzPlotLine;
                g_RenderConfig.sInterfaces.putChar = rzPutChar;
                RenderConfig_GetDefaultStyle(&g_RenderConfig);
                g_pVm = EzMachine_Create();
                if (recalc()) {
                    redrawCanvas();
                } else {
                    g_iStage = STAGE_ERROR;
                    drawErrorScreen();
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        case WM_KEYDOWN:
            if (wParam == VK_TAB) { DestroyWindow(hWnd); return 0; }
            /* Expression editor handles chars through WM_CHAR */
            if (g_iStage == STAGE_EXPRESSION_EDITOR)
                ExpressionEditor_OnKey(hWnd, wParam);
            else switch (g_iStage) {
                case STAGE_WIREFRAME:        Wireframe_OnKey(hWnd, wParam); break;
                case STAGE_WINDOW_EDITOR:    WindowEditor_OnKey(hWnd, wParam); break;
                case STAGE_RECALC:           Recalc_OnKey(hWnd, wParam); break;
                case STAGE_ERROR:            Error_OnKey(hWnd, wParam); break;
                default: return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CHAR:
            if (g_iStage == STAGE_EXPRESSION_EDITOR)
                ExpressionEditor_OnChar(hWnd, (TCHAR)wParam);
            else if (g_iStage == STAGE_WINDOW_EDITOR)
                WindowEditor_OnChar(hWnd, (TCHAR)wParam);
            break;
        case WM_KEYUP:
            if (g_iStage == STAGE_WIREFRAME && wParam == 'M') g_bPanMode = 0;
            if (g_iStage == STAGE_WIREFRAME && wParam == 'F') g_bExprDrag = 0;
            break;
        case WM_LBUTTONDOWN:
            if (g_iStage != STAGE_WIREFRAME) break;
            g_bMouseDown = 1;
            g_iMousePrevX = LOWORD(lParam);
            g_iMousePrevY = HIWORD(lParam);
            SetCapture(hWnd);
            break;
        case WM_LBUTTONUP:
            g_bMouseDown = 0; ReleaseCapture(); break;
        case WM_MOUSEMOVE:
            if (g_iStage != STAGE_WIREFRAME) break;
            if (g_bMouseDown) {
                int iDeltaX, iDeltaY;
                int iMouseX = LOWORD(lParam);
                int iMouseY = HIWORD(lParam);
                iDeltaX = iMouseX - g_iMousePrevX;
                iDeltaY = iMouseY - g_iMousePrevY;
                g_iMousePrevX = iMouseX; g_iMousePrevY = iMouseY;
                if (iDeltaX == 0 && iDeltaY == 0) break;
                if (g_bPanMode) {
                    Camera.iViewportX += iDeltaX;
                    Camera.iViewportY += iDeltaY;
                } else if (g_bExprDrag) {
                    g_iExprPosX += iDeltaX;
                    g_iExprPosY += iDeltaY;
                } else {
                    Camera.iBetaDeg  -= iDeltaX;
                    Camera.iAlphaDeg -= iDeltaY;
                    Camera.iBetaDeg = Camera.iBetaDeg % 360;
                    if (Camera.iBetaDeg < 0) Camera.iBetaDeg += 360;
                    Camera.iAlphaDeg = Camera.iAlphaDeg % 360;
                    if (Camera.iAlphaDeg < 0) Camera.iAlphaDeg += 360;
                }
                redrawCanvas();
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        case WM_PAINT:
            paintCanvasToWindow(hWnd);
            break;
        case WM_DESTROY:
            if (g_pRenderNode != NULL) RenderNode_Destroy(g_pRenderNode);
            if (g_pVm != NULL) EzMachine_Destroy(g_pVm);
            if (g_pAstExpr != NULL) FzAstNode_Destroy(g_pAstExpr);
            destroyBackBuffer(); destroyCanvas();
            PostQuitMessage(0);
            SetTaskbarVisible(TRUE);
            break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

/*====================================================
 * About box
 *====================================================*/
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    RECT rt, rt1;
    int DlgWidth, DlgHeight, NewPosX, NewPosY;
    switch (message) {
        case WM_INITDIALOG:
            if (GetWindowRect(hDlg, &rt1)) {
                GetClientRect(GetParent(hDlg), &rt);
                DlgWidth  = rt1.right - rt1.left;
                DlgHeight = rt1.bottom - rt1.top;
                NewPosX = (rt.right - rt.left - DlgWidth)/2;
                NewPosY = (rt.bottom - rt.top - DlgHeight)/2;
                if (NewPosX < 0) NewPosX = 0;
                if (NewPosY < 0) NewPosY = 0;
                SetWindowPos(hDlg, 0, NewPosX, NewPosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }
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
	HWND hWndTaskbar = FindWindow(_T("HHTaskBar"), NULL);
	if (!bVisible) {
		SetWindowPos(hWndTaskbar, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
	} else {
		SetWindowPos(hWndTaskbar, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
    return TRUE;
#endif
    return FALSE;
}

/*====================================================
 * Stage implementations (included .c files)
 *====================================================*/
#include "stage-wireframe.c"
#include "stage-window-editor.c"
#include "stage-expression.c"
#include "stage-recalc.c"
#include "stage-error.c"
