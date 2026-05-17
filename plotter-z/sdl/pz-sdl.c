#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include "../utils/hybird_6x8.h"
#include "../../renderer-z/ascii_extended_mapping.h"
#include "../../renderer-z/rz.h"
#include "../../evaluator-z/ez.h"
#include "../../common/constants.h"

#define ABS(v)  ((v) < 0 ? -(v) : (v))
#define PZ_PI   3.14159265

#define CURRENT_FONT_WIDTH 6
#define CURRENT_FONT_HEIGHT 8

/*====================================================
 * Configuration structure with defaults
 *====================================================*/
typedef struct {
    int         iScreenWidth;
    int         iScreenHeight;
    int         iScale;
    int         bLcd;
    int         bFullscreen;
    PZ_FLOAT    fXMin, fXMax;
    int         iXGrid;
    PZ_FLOAT    fYMin, fYMax;
    int         iYGrid;
    PZ_FLOAT    fZMin, fZMax;
    int         bShowBox;
    const char* szExpr;
} PzSdlConfig;

static const PzSdlConfig DEFAULT_CONFIG = {
    640, 480,                                   /* width, height */
    1,                                          /* scale */
    0,                                          /* lcd mode */
    0,                                          /* fullscreen */
    -6.0f, 6.0f, 20,                            /* xMin, xMax, xGrid */
    -6.0f, 6.0f, 20,                            /* yMin, yMax, yGrid */
    -3.0f, 3.0f,                                /* zMin, zMax */
    0,                                          /* showBox */
    "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))"       /* default expression */
};

/*====================================================
 * Global state
 *====================================================*/
#define DEFAULT_VIEW_ALPHA 30
#define DEFAULT_VIEW_BETA 30

int             bMainLoop = 1;
int             iScreenWidth = 640;
int             iScreenHeight = 480;
int             iScale = 1;
int             bLcd = 0;
int             iCanvasW = 640;
int             iCanvasH = 480;
int             iBlitOffX = 0;
int             iBlitOffY = 0;
int             bShowBox = 1;
SDL_Surface*    sfScreen;
SDL_Surface*    sfCanvas;
Uint32          uBgColor = 0x99bb00;
Uint32          uSolidColor = 0x226600;
Uint32          uLcdDarken = 0x222222;
int             g_bError = 0;
int             g_bHelp = 0;
int             g_bInspector = 0;
int             g_iInspectorScroll = 0;
char            g_szErrorText[200] = "";
int             g_bMouseLeftDown = 0;
int             g_bMouseRightDown = 0;
int             g_iMousePrevX = 0;
int             g_iMousePrevY = 0;

typedef struct { PZ_FLOAT x, y, z; } Vertex;
typedef struct { int i0, i1; } Edge;

const PZ_FLOAT arrZoomLevels[] = { 0.33, 0.5, 0.75, 1, 1.5, 2, 4, 8 };
const int iNumZoomLevel = sizeof(arrZoomLevels) / sizeof(arrZoomLevels[0]);
#define ZOOM_LEVEL_DEFAULT 3

struct {
    int iViewportX;
    int iViewportY;
    int iViewportS;
    int iAlphaDeg;
    int iBetaDeg;
    PZ_FLOAT cosA;  PZ_FLOAT sinA;  PZ_FLOAT cosB;  PZ_FLOAT sinB; 
    PZ_FLOAT xMin;  PZ_FLOAT xMax;  int xGrid;
    PZ_FLOAT yMin;  PZ_FLOAT yMax;  int yGrid;
    PZ_FLOAT zMin;  PZ_FLOAT zMax;
    int iZoomLevel;
} Camera = {
    0, 0, 0,
    DEFAULT_VIEW_ALPHA, DEFAULT_VIEW_BETA,
    0.0f, 0.0f, 0.0f, 0.0f,
    -6.0f, 6.0f, 20,
    -6.0f, 6.0f, 20,
    -3.0f, 3.0f,
    ZOOM_LEVEL_DEFAULT
};

PZ_FLOAT zBuf[2000];
PZ_FLOAT xBuf[50];
PZ_FLOAT yBuf[50];

#define Z_BUF(x,y) (zBuf[(x) + (y) * Camera.xGrid])

FzAstNode*      pAstExpr = NULL;
EzMachine*      pVm;
RenderNode*     pRenderNode;
RenderConfig    config;

/*====================================================
 * I18N - Internationalization
 *====================================================*/

#define I18N_LANG_EN    0
#define I18N_LANG_JA    1
#define I18N_LANG_COUNT 2

/* Text key indices */
#define I18N_TITLE                  0
#define I18N_ERROR                  1
#define I18N_EXPRESSION_LABEL       2
#define I18N_SYNTAX_ERROR           3
#define I18N_PRESS_TAB_EXIT         4
#define I18N_VIEW_FMT               5
#define I18N_ZOOM_FMT               6
#define I18N_VARIABLE_UNDEFINED_FMT 7
#define I18N_FUNCTION_UNDEFINED_FMT 8
#define I18N_FUNCTION_PARAM_FMT     9
#define I18N_HELP_TITLE             10
#define I18N_HELP_TAB               11
#define I18N_HELP_UP_DOWN           12
#define I18N_HELP_LEFT_RIGHT        13
#define I18N_HELP_R                 14
#define I18N_HELP_Q                 15
#define I18N_HELP_W                 16
#define I18N_HELP_E                 17
#define I18N_HELP_I                 18
#define I18N_HELP_MOUSE_LEFT        19
#define I18N_HELP_MOUSE_RIGHT       20
#define I18N_HELP_ANY_KEY           21
#define I18N_EZ_INSPECTOR           22
#define I18N_EZ_INSTRUCTIONS        23
#define I18N_EZ_PREDEF_VARS         24
#define I18N_COUNT                  25

static int g_iLang = I18N_LANG_EN;

static const char* TextI18n[I18N_LANG_COUNT][I18N_COUNT] = {
    /* [I18N_LANG_EN] English */
    {
        "Plotter-Z",
        "ERROR:",
        "  Expression: ",
        "Syntax Error - could not parse expression",
        "Press [Tab] to exit",
        "VIEW: %c=%d, %c=%d",
        "%d%%(%d, %d)",
        "VARIABLE UNDEFINED: '%s'",
        "FUNCTION UNDEFINED: '%s'",
        "FUNCTION PARAM MISMATCH: '%s'",
        "Operation Guide",
        "Tab - Exit",
        "\x15/\x16  - Horizontal rotation",
        "\x17/\x18  - Vertical rotation",
        "R - Reset viewport to origin",
        "Q - Zoom out",
        "W - Zoom in",
        "E - Toggle bounding box",
        "I - Toggle inspector",
        "Left drag  - Rotate view",
        "Right drag - Pan view",
        "Press any key to continue",
        "Evaluator-Z Inspector",
        "VM Instructions:",
        "Variables:",
    },
    /* [I18N_LANG_JA] Japanese */
    {
        "\xD6\xE9\xE5\xB9\xCA\xBAZ",
        "\xBE\xE1\xBA:",
        "  \xBE\xC2\xC7\xD6\xE9\xE4\xB9\xC6\xB8\xE7:",
        "\xC6\xE7\xCA\xB9\xC2\xC7\xBE\xE1\xBA,\xBE\xC2\xC7\xD6\xE9\xE4\xB9\xC6\xB8\xE7 \xB0 \xD4\xE9\xBA\xC7 \xCD\xE8\xC1\xD9\xC8\xE7",
        "[\xCA\xD6\xE8\xC1\xBA] \xB0 \xBF\xC6\xCD \xC6\xB7\xBD\xE2\xB8\xBD",
        "\xD5\xE8\xB7\xBA: %c=%d, %c=%d",
        "%d%%(%d, %d)",
        "\xD4\xE8\xE2\xBB\xD6\xE8\xE3 \xBB\xE7\xCD\xE8\xD6\xB1\xBC\xE7\xCE\xE8: '%s'",
        "\xD6\xB1\xE7\xC2\xC6\xB8\xE7 \xBB\xE7\xCD\xE8\xD6\xB1\xBC\xE7\xCE\xE8: '%s'",
        "\xD6\xB1\xE7\xC2\xC6\xB8\xE7 \xD4\xE9\xE1\xDB \xDA\xC7\xD9\xB9\xCB: '%s'",
        "\xC9\xBD\xC5 \xC0\xE8\xBC\xCE\xE8",
        "\xCA\xD6\xE8 - \xC6\xB7\xBD\xE2\xB8\xBD",
        "\x15/\x16 - \xC7\xBC\xD7\xBC \xC0\xBC\xCD\xE7",
        "\x17/\x18 - \xC7\xBC\xCB\xB8\xC2 \xC0\xBC\xCD\xE7",
        "R - \xD5\xE8\xB7\xBA\xD8\xE9\xBA\xCE \xBF \xC3\xE8\xE7\xCD\xE7 \xD7 \xE2\xC8\xB9\xCE",
        "Q - \xC7\xE8\xBA\xDB \xBB\xBD\xCE",
        "W - \xC7\xE8\xBA\xDB \xBC\xE7",
        "E - \xD4\xE8\xBD\xE7\xCD\xE8\xB2\xE7\xC2\xE8 \xD8\xE8\xB9\xC2\xC7 \xD3 \xCE\xC2\xE8\xE3",
        "I - \xD4\xE8\xBD\xE7\xCD\xE8\xB2\xE7\xC2\xE8 \xBC\xE7\xC7\xD7\xE9\xC2\xCA\xBA",
        "\xE4\xD6\xCE \xCE\xE8\xE1\xB9\xC2\xE8 - \xD5\xE8\xB7\xBA \xC0\xBC\xCD\xE7",
        "\xE1\xBC\xCE \xCE\xE8\xE1\xB9\xC2\xE8 - \xD5\xE8\xB7\xBA \xD4\xE9\xE7",
        "\xCE\xE8\xE7\xCF \xC1\xBA \xCD\xE8\xDD \xBF\xC6\xCD \xC8\xE7\xC9\xE8\xC2",
        "\xBE\xD4\xE8\xE2\xB7\xBE\xBA\xCA\xBAZ \xBC\xE7\xC7\xD7\xE9\xC2\xCA\xBA",
        "VM \xBC\xE7\xC7\xCE\xE1\xC2\xC6\xB8\xE7\xC7\xE8:",
        "\xD4\xE8\xE2\xBB\xD6\xE8\xE3\xC7\xE8:",
    },
};

#define I18N(k)  (TextI18n[g_iLang][k])

/*====================================================
 * Command-line parsing helpers
 *====================================================*/

static int parseResolution(const char* szArg, int* pWidth, int* pHeight) {
    const char* pX;
    *pWidth = atoi(szArg);
    pX = strchr(szArg, 'x');
    if (pX == NULL) return 0;
    *pHeight = atoi(pX + 1);
    return (*pWidth > 0 && *pHeight > 0);
}

static int parseTriple(const char* szArg, PZ_FLOAT* pMin, PZ_FLOAT* pMax, int* pGrid) {
    char*  pEnd;
    const char* pComma;
    *pMin = (PZ_FLOAT)atof(szArg);
    pComma = strchr(szArg, ',');
    if (pComma == NULL) return 0;
    *pMax = (PZ_FLOAT)atof(pComma + 1);
    pComma = strchr(pComma + 1, ',');
    if (pComma == NULL) return 0;
    *pGrid = (int)strtol(pComma + 1, &pEnd, 10);
    (void)pEnd;
    return (*pGrid > 1);
}

static int parsePair(const char* szArg, PZ_FLOAT* pMin, PZ_FLOAT* pMax) {
    const char* pComma;
    *pMin = (PZ_FLOAT)atof(szArg);
    pComma = strchr(szArg, ',');
    if (pComma == NULL) return 0;
    *pMax = (PZ_FLOAT)atof(pComma + 1);
    return 1;
}

static void applyConfig(const PzSdlConfig* pCfg) {
    iScreenWidth   = pCfg->iScreenWidth;
    iScreenHeight  = pCfg->iScreenHeight;
    iScale   = pCfg->iScale;
    bLcd     = pCfg->bLcd;
    bShowBox = pCfg->bShowBox;

    iCanvasW = iScreenWidth / iScale;
    iCanvasH = iScreenHeight / iScale;
    iBlitOffX = (iScreenWidth  - iCanvasW * iScale) / 2;
    iBlitOffY = (iScreenHeight - iCanvasH * iScale) / 2;

    Camera.iViewportS = (iCanvasH < iCanvasW ? iCanvasH : iCanvasW) / 2;
    Camera.iViewportX = iCanvasW / 2;
    Camera.iViewportY = iCanvasH / 2;
    Camera.xMin  = pCfg->fXMin;
    Camera.xMax  = pCfg->fXMax;
    Camera.xGrid = pCfg->iXGrid;
    Camera.yMin  = pCfg->fYMin;
    Camera.yMax  = pCfg->fYMax;
    Camera.yGrid = pCfg->iYGrid;
    Camera.zMin  = pCfg->fZMin;
    Camera.zMax  = pCfg->fZMax;
}

/*====================================================
 * Pixel / line / text drawing
 *====================================================*/

static void setPixelToSurface(SDL_Surface *sf, int x, int y, Uint32 color) {
    unsigned char *row8;
    unsigned short *row16;
    unsigned int *row32;
    int bytesPerPixel = sf->format->BytesPerPixel;
    int pitch = sf->pitch;
    
    if (x < 0 || x >= sf->w || y < 0 || y >= sf->h) {
        return;
    }

    switch (bytesPerPixel) {
    case 1:
        row8 = (unsigned char *)((unsigned char *)sf->pixels + y * pitch + x * bytesPerPixel);
        *row8 = (unsigned char) color;
        break;

    case 2:
        row16 = (unsigned short *)((unsigned char *)sf->pixels + y * pitch + x * bytesPerPixel);
        *row16 = (unsigned short) color;
        break;

    case 4:
        row32 = (unsigned int *)((unsigned char *)sf->pixels + y * pitch + x * bytesPerPixel);
        *row32 = (unsigned int) color;
        break;
    }
}

static void setPixel(int x, int y) {
    setPixelToSurface(sfCanvas, x, y, uSolidColor);
}

static void plotLineColor(int x0, int y0, int x1, int y1, Uint32 color) {
    int dx = ABS(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -ABS(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        setPixelToSurface(sfCanvas, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void plotLine(int x0, int y0, int x1, int y1) {
    plotLineColor(x0, y0, x1, y1, uSolidColor);
}

static void fillRect(int dx, int dy, int w, int h, Uint32 uColor) {
    int x, y;
    for (x = 0; x < w; ++x) {
        for (y = 0; y < h; ++y) {
            setPixelToSurface(sfCanvas, dx + x, dy + y, uColor);
        }
    }
}

static void draw1bpp(const unsigned char *raw, int dx, int dy, int w, int h, int rev, Uint32 color) {
    int pitch = (w >> 3) + (w % 8 ? 1 : 0);
    int x, y, dot;
    unsigned char eightPixels;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            eightPixels = *(raw + y * pitch + (x >> 3));
            dot = (eightPixels >> (7 - x % 8)) & 1;
            if (dot ^ rev) {
                setPixelToSurface(sfCanvas, dx + x, dy + y, color);
            }
        }
    }
}

static void putCharColor(int x, int y, unsigned char ch, Uint32 uColor) {
    draw1bpp(FONT_HYBIRD_6x8 + 8 * ch, x, y, 8, 8, 0, uColor);
}

static void putChar(int x, int y, unsigned char ch) {
    putCharColor(x, y, ch, uSolidColor);
}

static void putText(int x, int y, const unsigned char* usz, Uint32 uColor) {
    for (; *usz; ++usz, x += CURRENT_FONT_WIDTH) {
        putCharColor(x, y, *usz, uColor);
    }
}

/*====================================================
 * 3D projection
 *====================================================*/

static void xyz2xy(PZ_FLOAT x, PZ_FLOAT y, PZ_FLOAT z, int *ox, int *oy) {
    PZ_FLOAT zoom = arrZoomLevels[Camera.iZoomLevel];
    PZ_FLOAT scale = Camera.iViewportS * zoom;
    PZ_FLOAT nx = x * Camera.cosB - y * Camera.sinB;
    PZ_FLOAT ny = (x * Camera.sinB + y * Camera.cosB) * Camera.sinA - z * Camera.cosA;
    *ox = (int)(Camera.iViewportX + scale * nx);
    *oy = (int)(Camera.iViewportY + scale * ny);
}

/*====================================================
 * Scale blit (integer nearest-neighbour)
 *====================================================*/

static void scaleBlit(SDL_Surface* sfSrc, SDL_Surface* sfDst, int iFactor,
                      int bLcdMode, Uint32 uDarken, int iDstOffX, int iDstOffY) {
    int sx, sy, dx, dy;
    int srcBpp, dstBpp, srcPitch, dstPitch;
    unsigned char* pSrcBase;
    unsigned char* pDstBase;
    int i;

    if (iFactor <= 0) return;

    if (iFactor == 1 && !bLcdMode) {
        SDL_Rect rcDst;
        rcDst.x = (Sint16)iDstOffX;
        rcDst.y = (Sint16)iDstOffY;
        SDL_BlitSurface(sfSrc, NULL, sfDst, &rcDst);
        return;
    }

    if (SDL_LockSurface(sfSrc) < 0) return;
    if (SDL_LockSurface(sfDst) < 0) { SDL_UnlockSurface(sfSrc); return; }

    srcBpp = sfSrc->format->BytesPerPixel;
    dstBpp = sfDst->format->BytesPerPixel;
    srcPitch = sfSrc->pitch;
    dstPitch = sfDst->pitch;
    pSrcBase = (unsigned char*)sfSrc->pixels;
    pDstBase = (unsigned char*)sfDst->pixels;

    for (sy = 0; sy < sfSrc->h; ++sy) {
        for (sx = 0; sx < sfSrc->w; ++sx) {
            unsigned char* pSrc = pSrcBase + sy * srcPitch + sx * srcBpp;
            for (dy = 0; dy < iFactor; ++dy) {
                int dstRow = iDstOffY + sy * iFactor + dy;
                unsigned char* pDst = pDstBase + dstRow * dstPitch
                                    + (iDstOffX + sx * iFactor) * dstBpp;
                for (dx = 0; dx < iFactor; ++dx) {
                    unsigned char* pDstPixel = pDst + dx * dstBpp;
                    if (bLcdMode && (dx == iFactor - 1 || dy == iFactor - 1)) {
                        for (i = 0; i < srcBpp; ++i) {
                            unsigned char ucSrc = pSrc[i];
                            unsigned char ucSub = ((unsigned char*)&uDarken)[i];
                            pDstPixel[i] = (ucSrc > ucSub) ? (unsigned char)(ucSrc - ucSub) : (unsigned char)0;
                        }
                    } else {
                        for (i = 0; i < srcBpp; ++i) {
                            pDstPixel[i] = pSrc[i];
                        }
                    }
                }
            }
        }
    }

    SDL_UnlockSurface(sfDst);
    SDL_UnlockSurface(sfSrc);
}

/*====================================================
 * Redraw
 *====================================================*/

static void drawBoundingBox(void) {
    static const Vertex BoxVertices[] = {
        {  1,  1,  1 },
        { -1,  1,  1 },
        { -1, -1,  1 },
        {  1, -1,  1 },
        {  1,  1, -1 },
        { -1,  1, -1 },
        { -1, -1, -1 },
        {  1, -1, -1 },
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
        plotLine(x0, y0, x1, y1);
    }
}

static void drawSurfaceWireframe(void) {
    int ix, iy, x0, y0, x1, y1;
    PZ_FLOAT z0, z1;

    for (ix = 0; ix < Camera.xGrid; ++ix) {
        iy = 0;
        xyz2xy(xBuf[ix], yBuf[iy], z0 = Z_BUF(ix, iy), &x0, &y0); 
        for (iy = 0; iy < Camera.yGrid; ++iy) {
            xyz2xy(xBuf[ix], yBuf[iy], z1 = Z_BUF(ix, iy), &x1, &y1); 
            if (z0 <= 1 && z0 >= -1 && z1 <= 1 && z1 >= -1) {
                plotLine(x0, y0, x1, y1);
            }
            x0 = x1, y0 = y1, z0 = z1;
        }
    }

    for (iy = 0; iy < Camera.yGrid; ++iy) {
        ix = 0;
        xyz2xy(xBuf[ix], yBuf[iy], z0 = Z_BUF(ix, iy), &x0, &y0); 
        for (ix = 0; ix < Camera.xGrid; ++ix) {
            xyz2xy(xBuf[ix], yBuf[iy], z1 = Z_BUF(ix, iy), &x1, &y1); 
            if (z0 <= 1 && z0 >= -1 && z1 <= 1 && z1 >= -1) {
                plotLine(x0, y0, x1, y1);
            }
            x0 = x1, y0 = y1, z0 = z1;
        }
    }
}

static void redraw(void) {

    SDL_FillRect(sfCanvas, NULL, uBgColor);

    Camera.sinA = (PZ_FLOAT)sin(Camera.iAlphaDeg * PZ_PI / 180);
    Camera.cosA = (PZ_FLOAT)cos(Camera.iAlphaDeg * PZ_PI / 180);
    Camera.sinB = (PZ_FLOAT)sin(Camera.iBetaDeg * PZ_PI / 180);
    Camera.cosB = (PZ_FLOAT)cos(Camera.iBetaDeg * PZ_PI / 180);

    drawSurfaceWireframe();

    if (bShowBox) {
        drawBoundingBox();
    }

    /* Display title text */
    {
        const char* szTitle = I18N(I18N_TITLE);
        int iTitleWidth = (int)strlen(szTitle) * 6;
        fillRect(0, 0, iCanvasW, 10, uSolidColor);
        putText((iCanvasW - iTitleWidth) / 2, 2, (const unsigned char *)szTitle, uBgColor);
    }
    
    /* Render expression */
    {
        int iStartX = 2;
        int iStartY = 12;
        int iNodeWidth = pRenderNode->sLayout.iWidth;
        int iNodeHeight = pRenderNode->sLayout.iAscent + pRenderNode->sLayout.iDescent;
        int iBaseline = iStartY + pRenderNode->sLayout.iAscent;
        fillRect(iStartX - 2, iStartY - 2, iNodeWidth + 2, iNodeHeight + 2, uBgColor);
        RenderNode_Draw(pRenderNode, &config, iStartX, iBaseline);
    }

    /* Display footer */
    {
        int iStartY = iCanvasH - 10;
        int iLeft;
        char szBuf[200];

        fillRect(0, iStartY, iCanvasW, 10, uSolidColor);

        sprintf(
            szBuf,
            I18N(I18N_VIEW_FMT),
            PZ_AE_GREEK_alpha,
            Camera.iAlphaDeg,
            PZ_AE_GREEK_beta,
            Camera.iBetaDeg
        );
        putText(2, iStartY + 2, (const unsigned char *)szBuf, uBgColor);

        sprintf(
            szBuf,
            I18N(I18N_ZOOM_FMT),
            (int)(arrZoomLevels[Camera.iZoomLevel] * 100),
            Camera.iViewportX,
            Camera.iViewportY
        );
        iLeft = iCanvasW - (int)strlen(szBuf) * 6 - 2;
        putText(iLeft, iStartY + 2, (const unsigned char *)szBuf, uBgColor);

    }

    SDL_FillRect(sfScreen, NULL, uBgColor);
    scaleBlit(sfCanvas, sfScreen, iScale, bLcd, uLcdDarken, iBlitOffX, iBlitOffY);
    SDL_Flip(sfScreen);
}

/*====================================================
 * Recalculate surface geometry
 *====================================================*/

static void recalc(void) {
    int ix, iy;
    PZ_FLOAT fz;

    /* Compute actual x, y sample positions and store in buffers */
    for (ix = 0; ix < Camera.xGrid; ++ix) {
        xBuf[ix] = Camera.xMin + (Camera.xMax - Camera.xMin) * ix / (Camera.xGrid - 1);
    }
    for (iy = 0; iy < Camera.yGrid; ++iy) {
        yBuf[iy] = Camera.yMin + (Camera.yMax - Camera.yMin) * iy / (Camera.yGrid - 1);
    }

    /* Evaluate z = f(x, y) for every grid point */
    for (ix = 0; ix < Camera.xGrid; ++ix) {
        for (iy = 0; iy < Camera.yGrid; ++iy) {
            EzMachine_SetVariableByIndex(pVm, 0, xBuf[ix]);
            EzMachine_SetVariableByIndex(pVm, 1, yBuf[iy]);
            /* Evaluate the z value */
            fz = EzMachine_Eval(pVm);
            /* Normalize z into [-1, 1] based on the z-axis bounds */
            Z_BUF(ix, iy) = 2.0f * (fz - (Camera.zMax + Camera.zMin) / 2.0f) / (Camera.zMax - Camera.zMin);
        }
    }

    /* Normalize x and y grid coordinates into [-1, 1] */
    for (ix = 0; ix < Camera.xGrid; ++ix) {
        xBuf[ix] = 2.0f * (xBuf[ix] - (Camera.xMax + Camera.xMin) / 2.0f) / (Camera.xMax - Camera.xMin);
    }
    for (iy = 0; iy < Camera.yGrid; ++iy) {
        yBuf[iy] = 2.0f * (yBuf[iy] - (Camera.yMax + Camera.yMin) / 2.0f) / (Camera.yMax - Camera.yMin);
    }   
}

/*====================================================
 * Error screen
 *====================================================*/

static void drawErrorScreen(void) {
    SDL_FillRect(sfCanvas, NULL, uBgColor);
    putText(8, 2, (const unsigned char *)I18N(I18N_TITLE), uSolidColor);
    putText(8, 20, (const unsigned char *)I18N(I18N_ERROR), uSolidColor);
    putText(8, 30, (const unsigned char *)I18N(I18N_EXPRESSION_LABEL), uSolidColor);
    if (pAstExpr == NULL) {
        putText(8, 38, (const unsigned char *)I18N(I18N_SYNTAX_ERROR), uSolidColor);
    } else {
        putText(8, 38, (const unsigned char *)g_szErrorText, uSolidColor);
    }
    putText(8, 54, (const unsigned char *)I18N(I18N_PRESS_TAB_EXIT), uSolidColor);
    SDL_FillRect(sfScreen, NULL, uBgColor);
    scaleBlit(sfCanvas, sfScreen, iScale, bLcd, uLcdDarken, iBlitOffX, iBlitOffY);
    SDL_Flip(sfScreen);
}

/*====================================================
 * Help screen
 *====================================================*/

static void drawHelpScreen(void) {
    static const int iKeyList[] = {
        I18N_HELP_TITLE,
        I18N_HELP_TAB,
        I18N_HELP_UP_DOWN,
        I18N_HELP_LEFT_RIGHT,
        I18N_HELP_R,
        I18N_HELP_Q,
        I18N_HELP_W,
        I18N_HELP_E,
        I18N_HELP_I,
        I18N_HELP_MOUSE_LEFT,
        I18N_HELP_MOUSE_RIGHT,
        I18N_HELP_ANY_KEY,
    };
    static const int iNumLines = sizeof(iKeyList) / sizeof(iKeyList[0]);
    static const int iLineHeight = 8;
    static const int iBorderSize = 2;
    int i;
    int iTotalHeight = iNumLines * iLineHeight;
    int iMaxWidth = 0;
    int iBlockLeft;
    int iStartY;

    for (i = 0; i < iNumLines; ++i) {
        int iLen = (int)strlen(I18N(iKeyList[i]));
        if (iLen > iMaxWidth) iMaxWidth = iLen;
    }

    iBlockLeft = (iCanvasW - iMaxWidth * 6) / 2;
    if (iBlockLeft < 4) iBlockLeft = 4;

    iStartY = (iCanvasH - iTotalHeight) / 2;
    if (iStartY < 2) iStartY = 2;

    redraw();

    fillRect(iBlockLeft - iBorderSize, iStartY - iBorderSize, iMaxWidth * 6 + 2 * iBorderSize, iTotalHeight + 2 * iBorderSize, uBgColor);

    for (i = 0; i < iNumLines; ++i) {
        const char* szLine = I18N(iKeyList[i]);
        int iLen = (int)strlen(szLine);
        int iX, iBlockW, iLineY;
        iBlockW = iMaxWidth * CURRENT_FONT_WIDTH;
        iLineY = iStartY + i * iLineHeight;

        if (i == 0 || i == iNumLines - 1) {
            iX = iBlockLeft + (iMaxWidth - iLen) * CURRENT_FONT_WIDTH / 2;
            fillRect(iBlockLeft, iLineY, iBlockW, iLineHeight, uSolidColor);
            putText(iX, iLineY, (const unsigned char *)szLine, uBgColor);
        } else {
            iX = iBlockLeft;
            putText(iX, iLineY, (const unsigned char *)szLine, uSolidColor);
        }
    }

    SDL_FillRect(sfScreen, NULL, uBgColor);
    scaleBlit(sfCanvas, sfScreen, iScale, bLcd, uLcdDarken, iBlitOffX, iBlitOffY);
    SDL_Flip(sfScreen);
}

/*====================================================
 * Inspector screen (VM internals)
 *====================================================*/

static void drawInspectorScreen(void) {
    const int iMargin = 4;
    const int iScrollBarX = iCanvasW - 8;
    const int iScrollBarW = 5;
    int iStartX = 2;
    int iStartY = 2;
    int iBaseline;
    int iVmInfoStartY;
    char szBuf[100];
    int i;
    VlistNode* pListNode;
    int bNeedScroll = 0;
    int iVisibleLines = 0;
    int iMaxScroll = 0;

    SDL_FillRect(sfCanvas, NULL, uBgColor);

    if (pAstExpr == NULL || pRenderNode == NULL || pVm == NULL) {
        putText(iStartX, iStartY, (const unsigned char *)"N/A", uSolidColor);
    } else {
        fillRect(iStartX - 2, iStartY - 2, iCanvasW, CURRENT_FONT_HEIGHT + 2, uSolidColor);
        putText(iStartX, iStartY, (const unsigned char *)I18N(I18N_EZ_INSPECTOR), uBgColor);
        iStartY += CURRENT_FONT_HEIGHT + iMargin;
        iBaseline = iStartY + pRenderNode->sLayout.iAscent;
        RenderNode_Draw(pRenderNode, &config, (iCanvasW - pRenderNode->sLayout.iWidth) / 2, iBaseline);

        iVmInfoStartY = iStartY + (pRenderNode->sLayout.iAscent + pRenderNode->sLayout.iDescent) + iMargin * 2;

        /* Render variable table */
        iStartY = iVmInfoStartY;
        iStartX = 0;
        putText(iStartX, iStartY, (const unsigned char *)I18N(I18N_EZ_PREDEF_VARS), uSolidColor);
        iStartY += CURRENT_FONT_HEIGHT + iMargin;
        for (
            i = 0, pListNode = pVm->pListVariableName->pHead;
            pListNode != NULL;
            pListNode = pListNode->pNext, iStartY += CURRENT_FONT_HEIGHT, ++i
        ) {
            sprintf(szBuf, "%02d:%s", i, (const char *)pListNode->pData);
            putText(iStartX, iStartY, (const unsigned char *)szBuf, uSolidColor);
        }

        /* Render VM instructions header */
        iStartX = iCanvasW / 2;
        iStartY = iVmInfoStartY;
        putText(iStartX, iStartY, (const unsigned char *)I18N(I18N_EZ_INSTRUCTIONS), uSolidColor);
        iStartY += CURRENT_FONT_HEIGHT + iMargin;

        /* Calculate scrolling: visible area = canvasH minus header and safety margin */
        iVisibleLines = (iCanvasH - iStartY - CURRENT_FONT_HEIGHT) / CURRENT_FONT_HEIGHT;
        if (iVisibleLines < 1) iVisibleLines = 1;
        if (pVm->iInstructionLength > iVisibleLines) {
            bNeedScroll = 1;
            iMaxScroll = pVm->iInstructionLength - iVisibleLines;
            if (g_iInspectorScroll > iMaxScroll) g_iInspectorScroll = iMaxScroll;
            if (g_iInspectorScroll < 0) g_iInspectorScroll = 0;
        } else {
            g_iInspectorScroll = 0;
        }

        /* Draw VM instructions (visible window) */
        i = g_iInspectorScroll;
        for (; i < pVm->iInstructionLength && i < g_iInspectorScroll + iVisibleLines; ++i, iStartY += CURRENT_FONT_HEIGHT) {
            EzInstruction* pInst = pVm->pInstructions + i;
            switch(pInst->iOpCode) {
                case EZOP_PUSH_IMD: {
                    sprintf(szBuf, "%3d:%-9s", i, EzOpCode_GetName(pInst->iOpCode));
                    Utils_Ftoa(pInst->uData.fImmediate, strchr(szBuf, '\0'), DEFAULT_FTOA_PRECISION);
                    break;
                }
                case EZOP_PUSH_VAR:
                    sprintf(szBuf, "%3d:%-9s%d", i, EzOpCode_GetName(pInst->iOpCode), pInst->uData.iVarIndex);
                    break;
                case EZOP_FUNC: {
                    const PzFuncMeta* pFuncMeta = Constant_GetFunctionMetadataByIndex(pInst->uData.iFuncIndex);
                    sprintf(szBuf, "%3d:%s \x18 %s", i, EzOpCode_GetName(pInst->iOpCode), pFuncMeta->szName);
                    break;
                }
                default:
                    sprintf(szBuf, "%3d:%-9s", i, EzOpCode_GetName(pInst->iOpCode));
                    break;
            }
            putText(iStartX, iStartY, (const unsigned char *)szBuf, uSolidColor);
        }

        /* Draw scroll bar if needed */
        if (bNeedScroll) {
            int iTrackTop, iTrackBottom, iTrackH, iThumbY, iThumbH, iThumbW, iThumbX;

            iTrackTop = iVmInfoStartY + CURRENT_FONT_HEIGHT + iMargin;
            iTrackBottom = iCanvasH - CURRENT_FONT_HEIGHT;
            iTrackH = iTrackBottom - iTrackTop - 1;

            /* Track background */
            fillRect(iScrollBarX, iTrackTop, iScrollBarW, iTrackH, uSolidColor);

            /* Up arrow */
            putCharColor(iScrollBarX, iTrackTop - CURRENT_FONT_HEIGHT, (unsigned char)PZ_AE_ARROW_UP, uSolidColor);

            /* Down arrow */
            putCharColor(iScrollBarX, iTrackBottom, (unsigned char)PZ_AE_ARROW_DOWN, uSolidColor);

            /* Thumb (inset by 1px on each side, horizontally centered) */
            if (iTrackH > 0 && iMaxScroll > 0) {
                iThumbW = iScrollBarW - 2;
                iThumbX = iScrollBarX + 1;
                iThumbH = (iTrackH - iThumbW) / (iMaxScroll + 1) - 2 + iThumbW;
                if (iThumbH < 2) iThumbH = 2;
                iThumbY = iTrackTop + 1 + (iTrackH - 2 - iThumbH) * g_iInspectorScroll / iMaxScroll;
                fillRect(iThumbX, iThumbY, iThumbW, iThumbH, uBgColor);
            }
        }
    }

    SDL_FillRect(sfScreen, NULL, uBgColor);
    scaleBlit(sfCanvas, sfScreen, iScale, bLcd, uLcdDarken, iBlitOffX, iBlitOffY);
    SDL_Flip(sfScreen);
}

/*====================================================
 * Main
 *====================================================*/

int main(int argc, char* argv[]) {
    PzSdlConfig cfg = DEFAULT_CONFIG;
    int i;
    int bFullscreen = 0;
    EzError iCompileError = EZERR_NONE;
    char szErrorBuf[EZ_ERROR_CONTENT_LENGTH];

    /* Parse command-line arguments */
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 >= argc || !parseResolution(argv[i + 1], &cfg.iScreenWidth, &cfg.iScreenHeight)) {
                fprintf(stderr, "Error: -r requires WIDTHxHEIGHT\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-f") == 0) {
            cfg.bFullscreen = 1;
        } else if (strcmp(argv[i], "-x") == 0) {
            if (i + 1 >= argc || !parseTriple(argv[i + 1], &cfg.fXMin, &cfg.fXMax, &cfg.iXGrid)) {
                fprintf(stderr, "Error: -x requires MIN,MAX,GRID\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-y") == 0) {
            if (i + 1 >= argc || !parseTriple(argv[i + 1], &cfg.fYMin, &cfg.fYMax, &cfg.iYGrid)) {
                fprintf(stderr, "Error: -y requires MIN,MAX,GRID\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-z") == 0) {
            if (i + 1 >= argc || !parsePair(argv[i + 1], &cfg.fZMin, &cfg.fZMax)) {
                fprintf(stderr, "Error: -z requires MIN,MAX\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-b") == 0) {
            cfg.bShowBox = !cfg.bShowBox;
        } else if (strcmp(argv[i], "-l") == 0) {
            cfg.bLcd = !cfg.bLcd;
        } else if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -s requires SCALE\n");
                return 2;
            }
            cfg.iScale = atoi(argv[i + 1]);
            if (cfg.iScale < 1) cfg.iScale = 1;
            i++;
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -t requires LANG\n");
                return 2;
            }
            if (strcmp(argv[i + 1], "ja") == 0) {
                g_iLang = I18N_LANG_JA;
            } else {
                g_iLang = I18N_LANG_EN;
            }
            i++;
        } else if (cfg.szExpr == DEFAULT_CONFIG.szExpr) {
            /* First positional argument overrides the default expression */
            cfg.szExpr = argv[i];
        }
    }

    applyConfig(&cfg);
    bFullscreen = cfg.bFullscreen;

    /* Initialize renderer interface */
    config.sInterfaces.setPixel = setPixel;
    config.sInterfaces.plotLine = plotLine;
    config.sInterfaces.putChar = putChar;
    RenderConfig_GetDefaultStyle(&config);

    /* Parse expression */
    pAstExpr = FzParser_ParseExpression(cfg.szExpr);
    if (pAstExpr == NULL) {
        g_bError = 1;
    } else {
        pRenderNode = Render_Transform(pAstExpr);
        RenderNode_EstimateSize(pRenderNode, &config);

        /* Compile expression to VM */
        pVm = EzMachine_Create();
        EzMachine_DeclareVariable(pVm, "x");
        EzMachine_DeclareVariable(pVm, "y");
        EzMachine_DeclareVariable(pVm, "pi");
        EzMachine_AllocateVariables(pVm);
        EzMachine_SetVariableByIndex(pVm, 2, PZ_PI);

        iCompileError = EzMachine_Compile(pVm, pAstExpr, szErrorBuf);

        if (iCompileError != EZERR_NONE) {
            g_bError = 1;
            switch (iCompileError) {
                default:
                case EZERR_NONE:
                    break;
                case EZERR_VARIABLE_UNDEFINED:
                    sprintf(g_szErrorText, I18N(I18N_VARIABLE_UNDEFINED_FMT), szErrorBuf);
                    break;
                case EZERR_FUNCTION_UNDEFINED:
                    sprintf(g_szErrorText, I18N(I18N_FUNCTION_UNDEFINED_FMT), szErrorBuf);
                    break;
                case EZERR_FUNCTION_PARAM_MISMATCH:
                    sprintf(g_szErrorText, I18N(I18N_FUNCTION_PARAM_FMT), szErrorBuf);
                    break;
            }
        } else {
            recalc();
        }
    }

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }

    {
        Uint32 uFlags = SDL_HWSURFACE;
        if (bFullscreen) {
            uFlags |= SDL_FULLSCREEN;
        }
        sfScreen = SDL_SetVideoMode(iScreenWidth, iScreenHeight, 32, uFlags);
    }
    SDL_WM_SetCaption("Plotter-Z | SDL", NULL);

    if (sfScreen == NULL) {
        return 0;
    }

    sfCanvas = SDL_CreateRGBSurface(SDL_HWSURFACE, iCanvasW, iCanvasH, 32,
                                    0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000);
    if (sfCanvas == NULL) {
        return 1;
    }

    if (g_bError) {
        drawErrorScreen();
    } else {
        redraw();
    }

    /* Main event loop */
    while (bMainLoop) {
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            switch (sdlEvent.type) {
                case SDL_QUIT:
                    bMainLoop = 0;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
                        g_bMouseLeftDown = 1;
                        g_iMousePrevX = sdlEvent.button.x;
                        g_iMousePrevY = sdlEvent.button.y;
                    } else if (sdlEvent.button.button == SDL_BUTTON_RIGHT) {
                        g_bMouseRightDown = 1;
                        g_iMousePrevX = sdlEvent.button.x;
                        g_iMousePrevY = sdlEvent.button.y;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
                        g_bMouseLeftDown = 0;
                    } else if (sdlEvent.button.button == SDL_BUTTON_RIGHT) {
                        g_bMouseRightDown = 0;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (!g_bError) {
                        int iDeltaX = sdlEvent.motion.x - g_iMousePrevX;
                        int iDeltaY = sdlEvent.motion.y - g_iMousePrevY;
                        g_iMousePrevX = sdlEvent.motion.x;
                        g_iMousePrevY = sdlEvent.motion.y;
                        if (iDeltaX == 0 && iDeltaY == 0) break;
                        if (g_bMouseLeftDown) {
                            Camera.iBetaDeg  += iDeltaX / iScale;
                            Camera.iAlphaDeg -= iDeltaY / iScale;
                            Camera.iBetaDeg = Camera.iBetaDeg % 360;
                            if (Camera.iBetaDeg < 0) Camera.iBetaDeg += 360;
                            Camera.iAlphaDeg = Camera.iAlphaDeg % 360;
                            if (Camera.iAlphaDeg < 0) Camera.iAlphaDeg += 360;
                            redraw();
                        } else if (g_bMouseRightDown) {
                            Camera.iViewportX += iDeltaX / iScale;
                            Camera.iViewportY += iDeltaY / iScale;
                            redraw();
                        }
                    }
                    break;
                case SDL_KEYDOWN:
                    if (sdlEvent.key.keysym.sym == SDLK_TAB) {
                        bMainLoop = 0;
                        break;
                    }
                    if (!g_bError) {
                        /* Common keys */
                        if (sdlEvent.key.keysym.sym == SDLK_t) {
                            g_iLang++;
                            if (g_iLang >= I18N_LANG_COUNT) g_iLang = 0;
                            /* Redraw screen */
                            if (g_bHelp) drawHelpScreen();
                            else if (g_bInspector) drawInspectorScreen();
                            else redraw();
                            break;
                        }
                        /* Help Screen */
                        if (g_bHelp) {
                            g_bHelp = 0;
                            redraw();
                            break;
                        }
                        /* Inspector Screen */
                        if (g_bInspector) {
                            if (sdlEvent.key.keysym.sym == SDLK_i) {
                                g_bInspector = 0;
                                redraw();
                                break;
                            }
                            if (sdlEvent.key.keysym.sym == SDLK_UP) {
                                g_iInspectorScroll--;
                                if (g_iInspectorScroll < 0) g_iInspectorScroll = 0;
                                drawInspectorScreen();
                                break;
                            }
                            if (sdlEvent.key.keysym.sym == SDLK_DOWN) {
                                g_iInspectorScroll++;
                                drawInspectorScreen();
                                break;
                            }
                            break;
                        }
                        /* Main Screen */
                        if (sdlEvent.key.keysym.sym == SDLK_h) {
                            g_bHelp = 1;
                            drawHelpScreen();
                            break;
                        }
                        if (sdlEvent.key.keysym.sym == SDLK_i) {
                            g_bInspector = 1;
                            drawInspectorScreen();
                            break;
                        }
                        if (sdlEvent.key.keysym.sym == SDLK_LEFT) {
                            Camera.iBetaDeg -= 5;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_RIGHT) {
                            Camera.iBetaDeg += 5;
                        }
                        if (sdlEvent.key.keysym.sym == SDLK_UP) {
                            Camera.iAlphaDeg -= 5;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_DOWN) {
                            Camera.iAlphaDeg += 5;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_r) {
                            Camera.iViewportX = iCanvasW / 2;
                            Camera.iViewportY = iCanvasH / 2;
                            Camera.iZoomLevel = ZOOM_LEVEL_DEFAULT;
                            Camera.iBetaDeg = DEFAULT_VIEW_BETA;
                            Camera.iAlphaDeg = DEFAULT_VIEW_ALPHA;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_q) {
                            Camera.iZoomLevel--;
                            if (Camera.iZoomLevel < 0) Camera.iZoomLevel = 0;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_w) {
                            Camera.iZoomLevel++;
                            if (Camera.iZoomLevel >= iNumZoomLevel) Camera.iZoomLevel = iNumZoomLevel - 1;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_e) {
                            bShowBox = !bShowBox;
                        }

                        Camera.iBetaDeg = Camera.iBetaDeg % 360;
                        if (Camera.iBetaDeg < 0) Camera.iBetaDeg += 360; 
                        Camera.iAlphaDeg = Camera.iAlphaDeg % 360;
                        if (Camera.iAlphaDeg < 0) Camera.iAlphaDeg += 360;

                        redraw();
                    }
                    break;
            }
        }
    }

    if (pRenderNode != NULL) {
        RenderNode_Destroy(pRenderNode);
    }
    if (pVm != NULL) {
        EzMachine_Destroy(pVm);
    }
    if (pAstExpr != NULL) {
        FzAstNode_Destroy(pAstExpr);
    }

    SDL_FreeSurface(sfCanvas);
    SDL_Quit();

    return 0;
}
