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

#define CURRENT_FONT_WIDTH 6
#define CURRENT_FONT_HEIGHT 8

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
 * Configuration structure with defaults
 *====================================================*/
typedef struct {
    int         iScreenWidth;
    int         iScreenHeight;
    int         iScale;
    int         bLcd;
    int         bFullscreen;
    PZ_FLOAT    fXMin, fXMax; int iXGrid;
    PZ_FLOAT    fYMin, fYMax; int iYGrid;
    PZ_FLOAT    fZMin, fZMax;
    PZ_FLOAT    fUMin, fUMax; int iUGrid;
    PZ_FLOAT    fVMin, fVMax; int iVGrid;
    int         bShowBox;
    const char* szExpr;
    int         iFuncType;
} PzSdlConfig;

PzSdlConfig appConfig;

static const PzSdlConfig DEFAULT_CONFIG = {
    640, 480,                                   /* width, height */
    1,                                          /* scale */
    0,                                          /* lcd mode */
    0,                                          /* fullscreen */
    -6.0f, 6.0f, 20,                            /* xMin, xMax, xGrid */
    -6.0f, 6.0f, 20,                            /* yMin, yMax, yGrid */
    -3.0f, 3.0f,                                /* zMin, zMax */
    -PZ_PI, PZ_PI, 20,                          /* uMin, uMax, uGrid */
    -PZ_PI, PZ_PI, 20,                          /* vMin, vMax, vGrid */
    0,                                          /* showBox */
    "exp(-abs(x/pi))*cos(sqr(x^2+y^2))*3",      /* default expression */
    FUNC_TYPE_CARTESIAN                         /* function type */
};

/*====================================================
 * Global state
 *====================================================*/
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
int             bShowAxes = 0;
SDL_Surface*    sfScreen;
SDL_Surface*    sfCanvas;
Uint32          uLcdDarken = 0x111111;

/*====================================================
 * Color palette (grayscale)
 *====================================================*/

#define COLOR_WHITE         0
#define COLOR_LIGHT_GRAY    1
#define COLOR_DARK_GRAY     2
#define COLOR_BLACK         3

static Uint32 uPalette[] = {    
    0x7ba200,   /* COLOR_WHITE       */
    0x437f00,   /* COLOR_LIGHT_GRAY  */
    0x226600,   /* COLOR_DARK_GRAY   */
    0x115000,   /* COLOR_BLACK       */
};

static Uint32 getColor(int iIndex) {
    return uPalette[iIndex];
}

int             g_bError = 0;
int             g_bHelp = 0;
int             g_bInspector = 0;
int             g_iInspectorScroll = 0;
char            g_szErrorText[200] = "";
int             g_bMouseLeftDown = 0;
int             g_bMouseRightDown = 0;
int             g_iMousePrevX = 0;
int             g_iMousePrevY = 0;

FzAstNode*      pAstCartZ   = NULL;
EzMachine*      pVmCartZ    = NULL;

FzAstNode*      pAstParm[]   = { NULL, NULL, NULL };
EzMachine*      pVmParm[]    = { NULL, NULL, NULL };

RenderNode*     pRenderNode = NULL;
RenderConfig    rzConfig;

/*====================================================
 * Vertex
 *====================================================*/

#define X_GRID_MAX      40
#define Y_GRID_MAX      40
#define U_GRID_MAX      20
#define V_GRID_MAX      20
#define BUFFER_MAX(a,b) ((a) > (b) ? (a) : (b))

#define VERTEX_BUFFER_SIZE  BUFFER_MAX(X_GRID_MAX + Y_GRID_MAX + X_GRID_MAX * Y_GRID_MAX, U_GRID_MAX * V_GRID_MAX * 3)

typedef struct { NUMERIC x, y, z; } Vertex;
typedef struct { int i0, i1; } Edge;

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
        "%s%%(%d, %d)",
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
        "%s%%(%d, %d)",
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
    return sscanf(szArg, "%dx%d", pWidth, pHeight) == 2 && *pWidth > 0 && *pHeight > 0;
}

static int parseTriple(const char* szArg, PZ_FLOAT* pMin, PZ_FLOAT* pMax, int* pGrid) {
    return sscanf(szArg, "%f,%f,%d", pMin, pMax, pGrid) == 3 && *pGrid > 1;
}

static int parsePair(const char* szArg, PZ_FLOAT* pMin, PZ_FLOAT* pMax) {
    return sscanf(szArg, "%f,%f", pMin, pMax) == 2;
}

static int parsePalette(const char* szArg) {
    int i;
    for (i = 0; i < 4; ++i) {
        int iHex, iR, iG, iB;
        const char* pComma;
        if (sscanf(szArg, "%6x", &iHex) != 1) return 0;
        iR = (iHex >> 16) & 0xFF;
        iG = (iHex >>  8) & 0xFF;
        iB = (iHex      ) & 0xFF;
        uPalette[i] = ((Uint32)iR << 16) | ((Uint32)iG << 8) | (Uint32)iB;
        pComma = strchr(szArg, ',');
        if (pComma == NULL) return (i == 3);
        szArg = pComma + 1;
    }
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
    Camera.uMin  = pCfg->fUMin;
    Camera.uMax  = pCfg->fUMax;
    Camera.uGrid = pCfg->iUGrid;
    Camera.vMin  = pCfg->fVMin;
    Camera.vMax  = pCfg->fVMax;
    Camera.vGrid = pCfg->iVGrid;
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
    setPixelToSurface(sfCanvas, x, y, getColor(COLOR_BLACK));
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
    plotLineColor(x0, y0, x1, y1, getColor(COLOR_BLACK));
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
    putCharColor(x, y, ch, getColor(COLOR_BLACK));
}

static void putText(int x, int y, const unsigned char* usz, Uint32 uColor) {
    for (; *usz; ++usz, x += CURRENT_FONT_WIDTH) {
        putCharColor(x, y, *usz, uColor);
    }
}

/*====================================================
 * Projection
 *====================================================*/
#define ORTHOGRAPHIC    0
#define PERSPECTIVE     1

int g_iProjection = PERSPECTIVE;

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

#define AXES_ARROW_SIZE (0.05f)

static void drawAxes(void (*xyz2xy)(NUMERIC, NUMERIC, NUMERIC, int*, int *)) {
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
        plotLineColor(x0, y0, x1, y1, getColor(COLOR_LIGHT_GRAY));
    }
}

static void drawBoxEdges(void (*xyz2xy)(NUMERIC, NUMERIC, NUMERIC, int*, int *)) {
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
        plotLineColor(x0, y0, x1, y1, getColor(COLOR_LIGHT_GRAY));
    }
}

static void drawCartSurfaceWireframe(void (*xyz2xy)(NUMERIC, NUMERIC, NUMERIC, int*, int *)) {
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
                plotLine(x0, y0, x1, y1);
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
                plotLine(x0, y0, x1, y1);
            }
            x0 = x1, y0 = y1, z0 = z1;
        }
    }
}

static void drawParmSurfaceWireframe(void (*xyz2xy)(NUMERIC, NUMERIC, NUMERIC, int*, int *)) {
    int iU, iV, x0, y0, x1, y1;
    for (iV = 0; iV < Camera.vGrid; ++iV) {
        iU = 0;
        xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x0, &y0); 
        for (iU = 0; iU < Camera.uGrid; ++iU) {
            xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x1, &y1);
            plotLine(x0, y0, x1, y1);
            x0 = x1, y0 = y1;
        }
    }
    for (iU = 0; iU < Camera.uGrid; ++iU) {
        iV = 0;
        xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x0, &y0); 
        for (iV = 0; iV < Camera.vGrid; ++iV) {
            xyz2xy(PARM_X_BUF(iU, iV), PARM_Y_BUF(iU, iV), PARM_Z_BUF(iU, iV), &x1, &y1); 
            plotLine(x0, y0, x1, y1);
            x0 = x1, y0 = y1;
        }
    }
}

static void refreshCameraTrigBuf(void) {
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

static void redraw(void) {
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

    SDL_FillRect(sfCanvas, NULL, getColor(COLOR_WHITE));

    refreshCameraTrigBuf();

    if (bShowBox) {
        drawBoxEdges(xyz2xy);
    }
    
    if (bShowAxes) {
        drawAxes(xyz2xy);
    }

    switch (appConfig.iFuncType) {
        case FUNC_TYPE_CARTESIAN:
            drawCartSurfaceWireframe(xyz2xy);
            break;
        case FUNC_TYPE_PARAMETRIC:
            drawParmSurfaceWireframe(xyz2xy);
            break;
    }

    /* Display title text */
    {
        const char* szTitle = I18N(I18N_TITLE);
        int iTitleWidth = (int)strlen(szTitle) * 6;
        fillRect(0, 0, iCanvasW, 10, getColor(COLOR_DARK_GRAY));
        putText((iCanvasW - iTitleWidth) / 2, 2, (const unsigned char *)szTitle, getColor(COLOR_WHITE));
    }
    
    /* Render expression */
    if (pRenderNode) {
        int iStartX = 2;
        int iStartY = 12;
        int iNodeWidth = pRenderNode->sLayout.iWidth;
        int iNodeHeight = pRenderNode->sLayout.iAscent + pRenderNode->sLayout.iDescent;
        int iBaseline = iStartY + pRenderNode->sLayout.iAscent;
        fillRect(iStartX - 2, iStartY - 2, iNodeWidth + 3, iNodeHeight + 3, getColor(COLOR_WHITE));
        RenderNode_Draw(pRenderNode, &rzConfig, iStartX, iBaseline);
    }

    /* Display footer */
    {
        int iStartY = iCanvasH - 10;
        int iLeft;
        char szBuf[200];

        fillRect(0, iStartY, iCanvasW, 10, getColor(COLOR_DARK_GRAY));

        sprintf(
            szBuf,
            I18N(I18N_VIEW_FMT),
            PZ_AE_GREEK_alpha,
            Camera.iAlphaDeg,
            PZ_AE_GREEK_beta,
            Camera.iBetaDeg
        );
        putText(2, iStartY + 2, (const unsigned char *)szBuf, getColor(COLOR_WHITE));

        sprintf(
            szBuf,
            I18N(I18N_ZOOM_FMT),
            szZoomLevels[Camera.iZoomLevel],
            Camera.iViewportX,
            Camera.iViewportY
        );
        iLeft = iCanvasW - (int)strlen(szBuf) * 6 - 2;
        putText(iLeft, iStartY + 2, (const unsigned char *)szBuf, getColor(COLOR_WHITE));

    }

    SDL_FillRect(sfScreen, NULL, getColor(COLOR_WHITE));
    scaleBlit(sfCanvas, sfScreen, iScale, bLcd, uLcdDarken, iBlitOffX, iBlitOffY);
    SDL_Flip(sfScreen);
}

/*====================================================
 * Recalculate surface geometry
 *====================================================*/

static void recalcCartesian(void) {
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
            EzMachine_SetVariableByIndex(pVmCartZ, 0, fX[iX]);
            EzMachine_SetVariableByIndex(pVmCartZ, 1, fY[iY]);
            /* Evaluate the z value */
            fZ = EzMachine_Eval(pVmCartZ);
            /* Normalize z into [-1, 1] based on the z-axis bounds */
            CART_Z_BUF(iX, iY) = NUM_VAL(2.0f * (fZ - (Camera.zMax + Camera.zMin) / 2.0f) / (Camera.zMax - Camera.zMin));
        }
    }

    /* Normalize x and y grid coordinates into [-1, 1] */
    for (iX = 0; iX < Camera.xGrid; ++iX) {
        g_fCartXBuf[iX] = NUM_VAL(2.0f * (fX[iX] - (Camera.xMax + Camera.xMin) / 2.0f) / (Camera.xMax - Camera.xMin));
    }
    for (iY = 0; iY < Camera.yGrid; ++iY) {
        g_fCartYBuf[iY] = NUM_VAL(2.0f * (fY[iY] - (Camera.yMax + Camera.yMin) / 2.0f) / (Camera.yMax - Camera.yMin));
    }
}

void recalcParametric(void) {
    int iU, iV, i;
    PZ_FLOAT fU, fV;
    PZ_FLOAT fPoint[3];

    if (!pVmParm[0] || !pVmParm[1] || !pVmParm[2]) {
        return;
    }

    for (iU = 0; iU < Camera.uGrid; ++iU) {
        fU = Camera.uMin + (Camera.uMax - Camera.uMin) * iU / (Camera.uGrid - 1);
        for (iV = 0; iV < Camera.vGrid; ++iV) {
            fV = Camera.vMin + (Camera.vMax - Camera.vMin) * iV / (Camera.vGrid - 1);
            /* Evaluate x */
            for (i = 0; i < 3; ++i) {
                EzMachine_SetVariableByIndex(pVmParm[i], 0, fU);
                EzMachine_SetVariableByIndex(pVmParm[i], 1, fV);
                fPoint[i] = EzMachine_Eval(pVmParm[i]);
            }
            /* Normalize */
            PARM_X_BUF(iU, iV) = NUM_VAL(2.0f * (fPoint[0] - (Camera.xMax + Camera.xMin) / 2.0f) / (Camera.xMax - Camera.xMin));
            PARM_Y_BUF(iU, iV) = NUM_VAL(2.0f * (fPoint[1] - (Camera.yMax + Camera.yMin) / 2.0f) / (Camera.yMax - Camera.yMin));
            PARM_Z_BUF(iU, iV) = NUM_VAL(2.0f * (fPoint[2] - (Camera.zMax + Camera.zMin) / 2.0f) / (Camera.zMax - Camera.zMin));
        }
    }
}

/*====================================================
 * Error screen
 *====================================================*/

static void drawErrorScreen(void) {
    SDL_FillRect(sfCanvas, NULL, getColor(COLOR_WHITE));
    putText(8, 2, (const unsigned char *)I18N(I18N_TITLE), getColor(COLOR_BLACK));
    putText(8, 20, (const unsigned char *)I18N(I18N_ERROR), getColor(COLOR_BLACK));
    putText(8, 30, (const unsigned char *)I18N(I18N_EXPRESSION_LABEL), getColor(COLOR_BLACK));
    if (pAstCartZ == NULL) {
        putText(8, 38, (const unsigned char *)I18N(I18N_SYNTAX_ERROR), getColor(COLOR_BLACK));
    } else {
        putText(8, 38, (const unsigned char *)g_szErrorText, getColor(COLOR_BLACK));
    }
    putText(8, 54, (const unsigned char *)I18N(I18N_PRESS_TAB_EXIT), getColor(COLOR_BLACK));
    SDL_FillRect(sfScreen, NULL, getColor(COLOR_WHITE));
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
    static const int iBorderSize = 1;
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

    fillRect(iBlockLeft - iBorderSize, iStartY - iBorderSize, iMaxWidth * 6 + 2 * iBorderSize, iTotalHeight + 2 * iBorderSize, getColor(COLOR_LIGHT_GRAY));

    for (i = 0; i < iNumLines; ++i) {
        const char* szLine = I18N(iKeyList[i]);
        int iLen = (int)strlen(szLine);
        int iX, iBlockW, iLineY;
        iBlockW = iMaxWidth * CURRENT_FONT_WIDTH;
        iLineY = iStartY + i * iLineHeight;

        if (i == 0 || i == iNumLines - 1) {
            iX = iBlockLeft + (iMaxWidth - iLen) * CURRENT_FONT_WIDTH / 2;
            fillRect(iBlockLeft, iLineY, iBlockW, iLineHeight, getColor(COLOR_DARK_GRAY));
            putText(iX, iLineY, (const unsigned char *)szLine, getColor(COLOR_WHITE));
        } else {
            iX = iBlockLeft;
            putText(iX, iLineY, (const unsigned char *)szLine, getColor(COLOR_BLACK));
        }
    }

    SDL_FillRect(sfScreen, NULL, getColor(COLOR_WHITE));
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

    SDL_FillRect(sfCanvas, NULL, getColor(COLOR_WHITE));

    if (pAstCartZ == NULL || pRenderNode == NULL || pVmCartZ == NULL) {
        putText(iStartX, iStartY, (const unsigned char *)"N/A", getColor(COLOR_DARK_GRAY));
    } else {
        fillRect(iStartX - 2, iStartY - 2, iCanvasW, CURRENT_FONT_HEIGHT + 2, getColor(COLOR_DARK_GRAY));
        putText(iStartX, iStartY, (const unsigned char *)I18N(I18N_EZ_INSPECTOR), getColor(COLOR_WHITE));
        iStartY += CURRENT_FONT_HEIGHT + iMargin;
        iBaseline = iStartY + pRenderNode->sLayout.iAscent;
        RenderNode_Draw(pRenderNode, &rzConfig, (iCanvasW - pRenderNode->sLayout.iWidth) / 2, iBaseline);

        iVmInfoStartY = iStartY + (pRenderNode->sLayout.iAscent + pRenderNode->sLayout.iDescent) + iMargin * 2;

        /* Render variable table */
        iStartY = iVmInfoStartY;
        iStartX = 8;
        putText(iStartX, iStartY, (const unsigned char *)I18N(I18N_EZ_PREDEF_VARS), getColor(COLOR_DARK_GRAY));
        iStartY += CURRENT_FONT_HEIGHT + iMargin;
        for (
            i = 0, pListNode = pVmCartZ->pListVariableName->pHead;
            pListNode != NULL;
            pListNode = pListNode->pNext, iStartY += CURRENT_FONT_HEIGHT, ++i
        ) {
            int iLeftOffset;
            iLeftOffset = CURRENT_FONT_WIDTH * sprintf(szBuf, "%02d\x18", i);
            putText(iStartX, iStartY, (const unsigned char *)szBuf, getColor(COLOR_LIGHT_GRAY));
            putText(iStartX + iLeftOffset, iStartY, (const unsigned char *)pListNode->pData, getColor(COLOR_BLACK));
        }

        /* Render VM instructions header */
        iStartX = iCanvasW / 2;
        iStartY = iVmInfoStartY;
        putText(iStartX, iStartY, (const unsigned char *)I18N(I18N_EZ_INSTRUCTIONS), getColor(COLOR_DARK_GRAY));
        iStartY += CURRENT_FONT_HEIGHT + iMargin;

        /* Calculate scrolling: visible area = canvasH minus header and safety margin */
        iVisibleLines = (iCanvasH - iStartY - CURRENT_FONT_HEIGHT) / CURRENT_FONT_HEIGHT;
        if (iVisibleLines < 1) iVisibleLines = 1;
        if (pVmCartZ->iInstructionLength > iVisibleLines) {
            bNeedScroll = 1;
            iMaxScroll = pVmCartZ->iInstructionLength - iVisibleLines;
            if (g_iInspectorScroll > iMaxScroll) g_iInspectorScroll = iMaxScroll;
            if (g_iInspectorScroll < 0) g_iInspectorScroll = 0;
        } else {
            g_iInspectorScroll = 0;
        }

        /* Draw VM instructions (visible window) */
        i = g_iInspectorScroll;
        for (; i < pVmCartZ->iInstructionLength && i < g_iInspectorScroll + iVisibleLines; ++i, iStartY += CURRENT_FONT_HEIGHT) {
            EzInstruction* pInst = pVmCartZ->pInstructions + i;
            int iLeftOffset;
            iLeftOffset = CURRENT_FONT_WIDTH * sprintf(szBuf, "%03d\x18", i);
            putText(iStartX, iStartY, (const unsigned char *)szBuf, getColor(COLOR_LIGHT_GRAY));
            switch(pInst->iOpCode) {
                case EZOP_PUSH_IMD: {
                    sprintf(szBuf, "%-9s", EzOpCode_GetName(pInst->iOpCode));
                    Utils_Ftoa(pInst->uData.fImmediate, strchr(szBuf, '\0'), DEFAULT_FTOA_PRECISION);
                    break;
                }
                case EZOP_PUSH_VAR:
                    sprintf(szBuf, "%-9s%d", EzOpCode_GetName(pInst->iOpCode), pInst->uData.iVarIndex);
                    break;
                case EZOP_FUNC: {
                    const PzFuncMeta* pFuncMeta = Constant_GetFunctionMetadataByIndex(pInst->uData.iFuncIndex);
                    sprintf(szBuf, "%s \x18 %s", EzOpCode_GetName(pInst->iOpCode), pFuncMeta->szName);
                    break;
                }
                default:
                    sprintf(szBuf, "%-9s", EzOpCode_GetName(pInst->iOpCode));
                    break;
            }
            putText(iStartX + iLeftOffset, iStartY, (const unsigned char *)szBuf, getColor(COLOR_BLACK));
        }

        /* Draw scroll bar if needed */
        if (bNeedScroll) {
            int iTrackTop, iTrackBottom, iTrackH, iThumbY, iThumbH, iThumbW, iThumbX;

            iTrackTop = iVmInfoStartY + CURRENT_FONT_HEIGHT + iMargin;
            iTrackBottom = iCanvasH - CURRENT_FONT_HEIGHT;
            iTrackH = iTrackBottom - iTrackTop - 1;

            /* Track background */
            fillRect(iScrollBarX, iTrackTop, iScrollBarW, iTrackH, getColor(COLOR_LIGHT_GRAY));

            /* Up arrow */
            if (g_iInspectorScroll > 0) {
                putCharColor(iScrollBarX, iTrackTop - CURRENT_FONT_HEIGHT, (unsigned char)PZ_AE_ARROW_UP, getColor(COLOR_DARK_GRAY));
            }

            /* Down arrow */
            if (g_iInspectorScroll < iMaxScroll) {
                putCharColor(iScrollBarX, iTrackBottom, (unsigned char)PZ_AE_ARROW_DOWN, getColor(COLOR_DARK_GRAY));
            }

            /* Thumb (inset by 1px on each side, horizontally centered) */
            if (iTrackH > 0 && iMaxScroll > 0) {
                iThumbW = iScrollBarW - 2;
                iThumbX = iScrollBarX + 1;
                iThumbH = (iTrackH - iThumbW) / (iMaxScroll + 1) - 2 + iThumbW;
                if (iThumbH < 2) iThumbH = 2;
                iThumbY = iTrackTop + 1 + (iTrackH - 2 - iThumbH) * g_iInspectorScroll / iMaxScroll;
                fillRect(iThumbX, iThumbY, iThumbW, iThumbH, getColor(COLOR_DARK_GRAY));
            }
        }
    }

    SDL_FillRect(sfScreen, NULL, getColor(COLOR_WHITE));
    scaleBlit(sfCanvas, sfScreen, iScale, bLcd, uLcdDarken, iBlitOffX, iBlitOffY);
    SDL_Flip(sfScreen);
}

/*====================================================
 * Main
 *====================================================*/

int main(int argc, char* argv[]) {
    int i;
    int bFullscreen = 0;
    EzError iCompileError = EZERR_NONE;
    char szErrorBuf[EZ_ERROR_CONTENT_LENGTH];

    memcpy(&appConfig, &DEFAULT_CONFIG, sizeof(PzSdlConfig));

    PzCamera_Initialize();
    
    /* Parse command-line arguments */
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--resolution") == 0) {
            if (i + 1 >= argc || !parseResolution(argv[i + 1], &appConfig.iScreenWidth, &appConfig.iScreenHeight)) {
                fprintf(stderr, "Error: -r requires WIDTHxHEIGHT\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fullscreen") == 0) {
            appConfig.bFullscreen = 1;
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--xrange") == 0) {
            if (i + 1 >= argc || !parseTriple(argv[i + 1], &appConfig.fXMin, &appConfig.fXMax, &appConfig.iXGrid)) {
                fprintf(stderr, "Error: -x requires MIN,MAX,GRID\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yrange") == 0) {
            if (i + 1 >= argc || !parseTriple(argv[i + 1], &appConfig.fYMin, &appConfig.fYMax, &appConfig.iYGrid)) {
                fprintf(stderr, "Error: -y requires MIN,MAX,GRID\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-z") == 0 || strcmp(argv[i], "--zrange") == 0) {
            if (i + 1 >= argc || !parsePair(argv[i + 1], &appConfig.fZMin, &appConfig.fZMax)) {
                fprintf(stderr, "Error: -z requires MIN,MAX\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--urange") == 0) {
            if (i + 1 >= argc || !parseTriple(argv[i + 1], &appConfig.fUMin, &appConfig.fUMax, &appConfig.iUGrid)) {
                fprintf(stderr, "Error: -u requires MIN,MAX,GRID\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--vrange") == 0) {
            if (i + 1 >= argc || !parseTriple(argv[i + 1], &appConfig.fVMin, &appConfig.fVMax, &appConfig.iVGrid)) {
                fprintf(stderr, "Error: -v requires MIN,MAX,GRID\n");
                return 2;
            }
            i++;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--box") == 0) {
            appConfig.bShowBox = !appConfig.bShowBox;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--lcd") == 0) {
            appConfig.bLcd = !appConfig.bLcd;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--scale") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -s requires SCALE\n");
                return 2;
            }
            appConfig.iScale = atoi(argv[i + 1]);
            if (appConfig.iScale < 1) appConfig.iScale = 1;
            i++;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--translation") == 0) {
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
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cartesian") == 0) {
            appConfig.iFuncType = FUNC_TYPE_CARTESIAN;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--parametric") == 0) {
            appConfig.iFuncType = FUNC_TYPE_PARAMETRIC;
        } else if (appConfig.szExpr == DEFAULT_CONFIG.szExpr) {
            /* First positional argument overrides the default expression */
            appConfig.szExpr = argv[i];
        }
    }

    applyConfig(&appConfig);
    bFullscreen = appConfig.bFullscreen;

    /* Initialize renderer interface */
    RenderConfig_GetDefaultStyle(&rzConfig);
    RenderConfig_CalculateBigSymbolPoints(&rzConfig);
    rzConfig.sInterfaces.setPixel = setPixel;
    rzConfig.sInterfaces.plotLine = plotLine;
    rzConfig.sInterfaces.putChar = putChar;

    /* Parse expression */
    if (appConfig.iFuncType == FUNC_TYPE_CARTESIAN) {
        pAstCartZ = FzParser_ParseExpression(appConfig.szExpr);
        if (pAstCartZ == NULL) {
            g_bError = 1;
        } else {
            pRenderNode = Render_Transform(pAstCartZ, "z=");
            RenderNode_CalculateSize(pRenderNode, &rzConfig);

            /* Compile expression to VM */
            pVmCartZ = EzMachine_Create();
            EzMachine_DeclareVariable(pVmCartZ, "x");
            EzMachine_DeclareVariable(pVmCartZ, "y");
            EzMachine_DeclareVariable(pVmCartZ, "pi");
            EzMachine_AllocateVariables(pVmCartZ);
            EzMachine_SetVariableByIndex(pVmCartZ, 2, PZ_PI);

            iCompileError = EzMachine_Compile(pVmCartZ, pAstCartZ, szErrorBuf);
            if (iCompileError == EZERR_NONE) {
                recalcCartesian();
            }
        }
    }
    else if (appConfig.iFuncType == FUNC_TYPE_PARAMETRIC) {
        char szExprBuf[300];
        const char *szExprPtr[3];
        int iSemicolonPos[2];
        int iSemicolonCount = 0;
        RenderNode* pRenderNodes[3];
        const char *szPrefix[] = { "x=", "y=", "z=" };

        strcpy(szExprBuf, appConfig.szExpr);

        for (i = 0; szExprBuf[i]; ++i) {
            if (szExprBuf[i] == ';') {
                if (iSemicolonCount >= 2) {
                    g_bError = 1;
                    sprintf(g_szErrorText, "The number of expressions is not 3.");
                    goto parseComplete;
                }
                iSemicolonPos[iSemicolonCount] = i;
                iSemicolonCount++;
                szExprBuf[i] = 0;
            }
        }

        if (iSemicolonCount != 2) {
            g_bError = 1;
            sprintf(g_szErrorText, "The number of expressions is not 3.");
            goto parseComplete;
        }

        szExprPtr[0] = szExprBuf;
        szExprPtr[1] = szExprBuf + iSemicolonPos[0] + 1;
        szExprPtr[2] = szExprBuf + iSemicolonPos[1] + 1;

        for (i = 0; i < 3; ++i) {
            pAstParm[i] = FzParser_ParseExpression(szExprPtr[i]);
            if (pAstParm[i] == NULL) {
                g_bError = 1;
                goto parseComplete;
            }
        }

        pRenderNode = RenderNode_Create(RN_VERTICAL);
        for (i = 0; i < 3; ++i) {
            vlPushBack(pRenderNode->uData.sVertical.pList, Render_Transform(pAstParm[i], szPrefix[i]));
        }
        RenderNode_CalculateSize(pRenderNode, &rzConfig);

        for (i = 0; i < 3; ++i) {
            /* Compile expression to VM */
            pVmParm[i] = EzMachine_Create();
            EzMachine_DeclareVariable(pVmParm[i], "u");
            EzMachine_DeclareVariable(pVmParm[i], "v");
            EzMachine_DeclareVariable(pVmParm[i], "pi");
            EzMachine_AllocateVariables(pVmParm[i]);
            EzMachine_SetVariableByIndex(pVmParm[i], 2, PZ_PI);

            iCompileError = EzMachine_Compile(pVmParm[i], pAstParm[i], szErrorBuf);
            if (iCompileError != EZERR_NONE) {
                goto compileComplete;
            }
        }

        recalcParametric();
    }

compileComplete:

    /* Format error message */
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
    }

parseComplete:

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
#ifdef USE_FIXED_POINT
    SDL_WM_SetCaption("Plotter-Z (Fixed Point) | SDL", NULL);
#else
    SDL_WM_SetCaption("Plotter-Z | SDL", NULL);
#endif
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
                            Camera.iBetaDeg  -= iDeltaX / iScale;
                            Camera.iAlphaDeg += iDeltaY / iScale;
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
                            if (appConfig.iFuncType == FUNC_TYPE_CARTESIAN) {
                                g_bInspector = 1;
                                drawInspectorScreen();
                            }
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
                            PzCamera_Reset(iCanvasW / 2, iCanvasH / 2);
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_z) {
                            Camera.iZoomLevel--;
                            if (Camera.iZoomLevel < 0) Camera.iZoomLevel = 0;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_x) {
                            Camera.iZoomLevel++;
                            if (Camera.iZoomLevel >= iNumZoomLevel) Camera.iZoomLevel = iNumZoomLevel - 1;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_c) {
                            Camera.iFovLevel--;
                            if (Camera.iFovLevel < FOV_LEVEL_MIN) Camera.iFovLevel = FOV_LEVEL_MIN;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_v) {
                            Camera.iFovLevel++;
                            if (Camera.iFovLevel > FOV_LEVEL_MAX) Camera.iFovLevel = FOV_LEVEL_MAX;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_p) {
                            g_iProjection = !g_iProjection;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_b) {
                            bShowBox = !bShowBox;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_o) {
                            bShowAxes = !bShowAxes;
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

    if (pRenderNode != NULL) RenderNode_Destroy(pRenderNode);

    if (pVmCartZ != NULL) EzMachine_Destroy(pVmCartZ);
    if (pAstCartZ != NULL) FzAstNode_Destroy(pAstCartZ);

    for (i = 0; i < 3; ++i) {
        if (pVmParm[i] != NULL) EzMachine_Destroy(pVmParm[i]);
        if (pAstParm[i] != NULL) FzAstNode_Destroy(pAstParm[i]);
    }

    SDL_FreeSurface(sfCanvas);
    SDL_Quit();

    return 0;
}
