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
    const char* szExpr;
} PzSdlConfig;

static const PzSdlConfig DEFAULT_CONFIG = {
    640, 480,                                   /* width, height */
    1,                                          /* scale */
    0,                                          /* lcd mode */
    0,                                          /* fullscreen */
    NULL
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
SDL_Surface*    sfScreen;
SDL_Surface*    sfCanvas;
Uint32          uBgColor = 0x99bb00;
Uint32          uSolidColor = 0x226600;
Uint32          uLcdDarken = 0x222222;
int             g_bError = 0;
int             g_bHelp = 0;
char            g_szErrorText[200] = "";
int             g_bMouseLeftDown = 0;
int             g_bMouseRightDown = 0;
int             g_iMousePrevX = 0;
int             g_iMousePrevY = 0;

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


static void applyConfig(const PzSdlConfig* pCfg) {
    iScreenWidth   = pCfg->iScreenWidth;
    iScreenHeight  = pCfg->iScreenHeight;
    iScale   = pCfg->iScale;
    bLcd     = pCfg->bLcd;

    iCanvasW = iScreenWidth / iScale;
    iCanvasH = iScreenHeight / iScale;
    iBlitOffX = (iScreenWidth  - iCanvasW * iScale) / 2;
    iBlitOffY = (iScreenHeight - iCanvasH * iScale) / 2;
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

static void drawErrorScreen(void) {
    SDL_FillRect(sfCanvas, NULL, uBgColor);
    putText(0, 0, (const unsigned char *)"Error", uSolidColor);
    SDL_FillRect(sfScreen, NULL, uBgColor);
    scaleBlit(sfCanvas, sfScreen, iScale, bLcd, uLcdDarken, iBlitOffX, iBlitOffY);
    SDL_Flip(sfScreen);
}

static void redraw(void) {

    SDL_FillRect(sfCanvas, NULL, uBgColor);
    
    /* Render expression */
    if (pAstExpr != NULL && pRenderNode != NULL){
        const int iMargin = 4;
        int iStartX = 2;
        int iStartY = 2;
        int iNodeWidth = pRenderNode->sLayout.iWidth;
        int iNodeHeight = pRenderNode->sLayout.iAscent + pRenderNode->sLayout.iDescent;
        int iBaseline;
        int iVmInfoStartY;
        char szBuf[100];
        int i;
        VlistNode* pListNode;

        putText(iStartX, iStartY, (const unsigned char *)"Expression:", uSolidColor);

        iStartY += CURRENT_FONT_HEIGHT + iMargin;
        iBaseline = iStartY + pRenderNode->sLayout.iAscent;

        RenderNode_Draw(pRenderNode, &config, iStartX, iBaseline);

        /* Render VM instructions */
        iVmInfoStartY = iStartY + iNodeHeight + iMargin * 2;
        iStartY = iVmInfoStartY;
        putText(iStartX, iStartY, (const unsigned char *)"EzMachine Instructions:", uSolidColor);
        iStartY += CURRENT_FONT_HEIGHT + iMargin;

        for (i = 0; i < pVm->iInstructionLength; ++i, iStartY += CURRENT_FONT_HEIGHT) {
            EzInstruction* pInst = pVm->pInstructions + i;
            switch(pInst->iOpCode) {
                case EZOP_PUSH_IMD: {
                    sprintf(szBuf, "%3d \x18 %-10s", i, EzOpCode_GetName(pInst->iOpCode));
                    Utils_Ftoa(pInst->uData.fImmediate, strchr(szBuf, '\0'), DEFAULT_FTOA_PRECISION);
                    break;
                }
                case EZOP_PUSH_VAR:
                    sprintf(szBuf, "%3d \x18 %-10s%d", i, EzOpCode_GetName(pInst->iOpCode), pInst->uData.iVarIndex);
                    break;
                case EZOP_FUNC: {
                    const PzFuncMeta* pFuncMeta = Constant_GetFunctionMetadataByIndex(pInst->uData.iFuncIndex);
                    sprintf(szBuf, "%3d \x18 %-10s%s", i, EzOpCode_GetName(pInst->iOpCode), pFuncMeta->szName);
                    break;
                }
                default:
                    sprintf(szBuf, "%3d \x18 %-10s", i, EzOpCode_GetName(pInst->iOpCode));
                    break;
            }
            putText(iStartX, iStartY, (const unsigned char *)szBuf, uSolidColor);
        }
    
        /* Render VM instructions */
        iStartY = iVmInfoStartY;
        iStartX = 160;
        putText(iStartX, iStartY, (const unsigned char *)"Predefined variables:", uSolidColor);
        iStartY += CURRENT_FONT_HEIGHT + iMargin;
        for (
            i = 0, pListNode = pVm->pListVariableName->pHead;
            pListNode != NULL;
            pListNode = pListNode->pNext, iStartY += CURRENT_FONT_HEIGHT, ++i
        ) {
            sprintf(szBuf, "%02d \x18 %s", i, (const char *)pListNode->pData);
            putText(iStartX, iStartY, (const unsigned char *)szBuf, uSolidColor);
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
        } else if (cfg.szExpr == NULL) {
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
    if (cfg.szExpr != NULL) {
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
            EzMachine_DeclareVariable(pVm, "const_i");
            EzMachine_DeclareVariable(pVm, "inf");
            EzMachine_DeclareVariable(pVm, "dots");
            EzMachine_DeclareVariable(pVm, "alpha");
            EzMachine_DeclareVariable(pVm, "beta");
            EzMachine_DeclareVariable(pVm, "theta");
            EzMachine_DeclareVariable(pVm, "lambda");
            EzMachine_DeclareVariable(pVm, "delta");
            EzMachine_DeclareVariable(pVm, "Delta");
            EzMachine_DeclareVariable(pVm, "omega");
            EzMachine_DeclareVariable(pVm, "Omega");
            EzMachine_AllocateVariables(pVm);
            EzMachine_SetVariableByIndex(pVm, 2, PZ_PI);

            iCompileError = EzMachine_Compile(pVm, pAstExpr, szErrorBuf);

            if (iCompileError != EZERR_NONE) {
                g_bError = 1;
            }
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
    SDL_WM_SetCaption("Evaluator-Z Inspector | SDL", NULL);

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
                case SDL_KEYDOWN:
                    if (sdlEvent.key.keysym.sym == SDLK_TAB) {
                        bMainLoop = 0;
                        break;
                    }
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
