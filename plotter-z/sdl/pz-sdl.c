#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include "../utils/vga_index.h"
#include "../utils/vga_palette.h"
#include "../utils/hybird_6x8.h"
#include "../../renderer-z/rz.h"
#include "../../evaluator-z/ez.h"

#define ABS(v) ((v) < 0 ? -(v) : (v))

/*====================================================
 * Configuration structure with defaults
 *====================================================*/
typedef struct {
    int         iWidth;
    int         iHeight;
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
    320, 240,                                   /* width, height */
    0,                                          /* fullscreen */
    -10.0f, 6.0f, 20,                           /* xMin, xMax, xGrid */
    -10.0f, 6.0f, 20,                           /* yMin, yMax, yGrid */
    -3.0f, 3.0f,                                /* zMin, zMax */
    1,                                          /* showBox */
    "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))"      /* default expression */
};

/*====================================================
 * Global state
 *====================================================*/
int             bMainLoop = 1;
int             iWidth = 320;
int             iHeight = 240;
int             bShowBox = 1;
SDL_Surface*    sfScreen;
Uint32          uBgColor = 0x99bb00;
Uint32          uSolidColor = 0x115500;

int             g_bError = 0;
char            g_szErrorText[200] = "";

typedef struct { PZ_FLOAT x, y, z; } Vertex;
typedef struct { int i0, i1; } Edge;

struct {
    PZ_FLOAT alpha; PZ_FLOAT beta;
    PZ_FLOAT cosA;  PZ_FLOAT sinA;  PZ_FLOAT cosB;  PZ_FLOAT sinB; 
    PZ_FLOAT xMin;  PZ_FLOAT xMax;  int xGrid;
    PZ_FLOAT yMin;  PZ_FLOAT yMax;  int yGrid;
    PZ_FLOAT zMin;  PZ_FLOAT zMax;
} Camera = {
    0.5f, 0.5f,
    0.0f, 0.0f, 0.0f, 0.0f,
    -10.0f, 6.0f, 20,
    -10.0f, 6.0f, 20,
    -3.0f, 3.0f
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
    iWidth   = pCfg->iWidth;
    iHeight  = pCfg->iHeight;
    bShowBox = pCfg->bShowBox;

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
    setPixelToSurface(sfScreen, x, y, uSolidColor);
}

static void plotLineColor(int x0, int y0, int x1, int y1, Uint32 color) {
    int dx = ABS(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -ABS(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        setPixelToSurface(sfScreen, x0, y0, color);
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

static void draw1bpp(const unsigned char *raw, int dx, int dy, int w, int h, int rev, Uint32 color) {
    int pitch = (w >> 3) + (w % 8 ? 1 : 0);
    int x, y, dot;
    unsigned char eightPixels;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            eightPixels = *(raw + y * pitch + (x >> 3));
            dot = (eightPixels >> (7 - x % 8)) & 1;
            if (dot ^ rev) {
                setPixelToSurface(sfScreen, dx + x, dy + y, color);
            }
        }
    }
}

static void putChar(int x, int y, unsigned char ch) {
    draw1bpp(FONT_HYBIRD_6x8 + 8 * ch, x, y, 8, 8, 0, uSolidColor);
}

static void putText(int x, int y, const unsigned char* usz) {
    for (; *usz; ++usz, x += 6) {
        putChar(x, y, *usz);
    }
}

/*====================================================
 * 3D projection
 *====================================================*/

static void xyz2xy(PZ_FLOAT x, PZ_FLOAT y, PZ_FLOAT z, int *ox, int *oy) {
    PZ_FLOAT zoom = iHeight / 2.0f;
    PZ_FLOAT nx = (x * Camera.cosB - y * Camera.sinB);
    PZ_FLOAT ny = (-x * Camera.sinB * Camera.sinA - y * Camera.cosB * Camera.sinA + z * Camera.cosA);
    *ox = (int)(iWidth / 2.0f + zoom * nx);
    *oy = (int)(iHeight / 2.0f + zoom * ny);
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

    for (ix = 0; ix < Camera.xGrid; ++ix) {
        iy = 0;
        xyz2xy(xBuf[ix], yBuf[iy], Z_BUF(ix, iy), &x0, &y0); 
        for (iy = 0; iy < Camera.yGrid; ++iy) {
            xyz2xy(xBuf[ix], yBuf[iy], Z_BUF(ix, iy), &x1, &y1); 
            plotLine(x0, y0, x1, y1);
            x0 = x1;
            y0 = y1;
        }
    }

    for (iy = 0; iy < Camera.yGrid; ++iy) {
        ix = 0;
        xyz2xy(xBuf[ix], yBuf[iy], Z_BUF(ix, iy), &x0, &y0); 
        for (ix = 0; ix < Camera.xGrid; ++ix) {
            xyz2xy(xBuf[ix], yBuf[iy], Z_BUF(ix, iy), &x1, &y1); 
            plotLine(x0, y0, x1, y1);
            x0 = x1;
            y0 = y1;
        }
    }
}

static void redraw(void) {
    int iStartX = 8;
    int iStartY = 12;
    int iCenterY = iStartY + pRenderNode->sSize.iTop;

    SDL_FillRect(sfScreen, NULL, uBgColor);
    
    putText(8, 2, (const unsigned char *)"Plotter-Z");
    RenderNode_Draw(pRenderNode, &config, iStartX, iCenterY);

    Camera.sinA = (PZ_FLOAT)sin(Camera.alpha);
    Camera.cosA = (PZ_FLOAT)cos(Camera.alpha);
    Camera.sinB = (PZ_FLOAT)sin(Camera.beta);
    Camera.cosB = (PZ_FLOAT)cos(Camera.beta);

    drawSurfaceWireframe();

    if (bShowBox) {
        drawBoundingBox();
    }

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
    SDL_FillRect(sfScreen, NULL, uBgColor);
    putText(8, 2, (const unsigned char *)"Plotter-Z");
    putText(8, 20, (const unsigned char *)"ERROR:");
    putText(8, 30, (const unsigned char *)"  Expression: ");
    if (pAstExpr == NULL) {
        putText(8, 38, (const unsigned char *)"Syntax Error - could not parse expression");
    } else {
        putText(8, 38, (const unsigned char *)g_szErrorText);
    }
    putText(8, 54, (const unsigned char *)"Press [Tab] to exit");
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
            if (i + 1 >= argc || !parseResolution(argv[i + 1], &cfg.iWidth, &cfg.iHeight)) {
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
        /* Compile expression to VM */
        pRenderNode = Render_Transform(pAstExpr);
        RenderNode_EstimateSize(pRenderNode, &config);

        pVm = EzMachine_Create();
        EzMachine_DeclareVariable(pVm, "x");
        EzMachine_DeclareVariable(pVm, "y");
        EzMachine_AllocateVariables(pVm);
        iCompileError = EzMachine_Compile(pVm, pAstExpr, szErrorBuf);

        if (iCompileError != EZERR_NONE) {
            g_bError = 1;
            switch (iCompileError) {
                default:
                case EZERR_NONE:
                    break;
                case EZERR_VARIABLE_UNDEFINED:
                    sprintf(g_szErrorText, "VARIABLE_UNDEFINED: '%s'", szErrorBuf);
                    break;
                case EZERR_FUNCTION_UNDEFINED:
                    sprintf(g_szErrorText, "FUNCTION_UNDEFINED: '%s'", szErrorBuf);
                    break;
                case EZERR_FUNCTION_PARAM_MISMATCH:
                    sprintf(g_szErrorText, "FUNCTION_PARAM_MISMATCH: '%s'", szErrorBuf);
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
        sfScreen = SDL_SetVideoMode(iWidth, iHeight, 32, uFlags);
    }
    SDL_WM_SetCaption("Plotter-Z | SDL", NULL);

    if (sfScreen == NULL) {
        return 0;
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
                case SDL_KEYDOWN:
                    if (sdlEvent.key.keysym.sym == SDLK_TAB) {
                        bMainLoop = 0;
                        break;
                    }
                    if (!g_bError) {
                        if (sdlEvent.key.keysym.sym == SDLK_LEFT) {
                            Camera.beta -= 0.1f;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_RIGHT) {
                            Camera.beta += 0.1f;
                        }
                        if (sdlEvent.key.keysym.sym == SDLK_UP) {
                            Camera.alpha -= 0.1f;
                        }
                        else if (sdlEvent.key.keysym.sym == SDLK_DOWN) {
                            Camera.alpha += 0.1f;
                        }
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

    SDL_Quit();

    return 0;
}
