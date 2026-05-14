#include <stdio.h>
#include <math.h>
#include <SDL/SDL.h>
#include "../utils/vga_index.h"
#include "../utils/vga_palette.h"
#include "../utils/hybird_6x8.h"
#include "../../renderer-z/rz.h"
#include "../../evaluator-z/ez.h"

#define ABS(v) ((v) < 0 ? -(v) : (v))

int             bMainLoop = 1;
int             iWidth = 320;
int             iHeight = 240;
SDL_Surface*    sfScreen;
Uint32          uBgColor = VgaMode13hColorPalette[VGA_COLOR_WHITE];
Uint32          uSolidColor = VgaMode13hColorPalette[VGA_COLOR_BLACK];

typedef struct { PZ_FLOAT x, y, z; } Vertex;
typedef struct { int i0, i1; } Edge;

struct {
    PZ_FLOAT alpha; PZ_FLOAT beta;
    PZ_FLOAT cosA;  PZ_FLOAT sinA;  PZ_FLOAT cosB;  PZ_FLOAT sinB; 
    PZ_FLOAT xMin;  PZ_FLOAT xMax;  int xGrid;
    PZ_FLOAT yMin;  PZ_FLOAT yMax;  int yGrid;
    PZ_FLOAT zMin;  PZ_FLOAT zMax;
} Camera = {
    0.5, 0.5,
    0, 0, 0, 0,
    -10, 6, 20,
    -10, 6, 20,
    -3, 3
};

PZ_FLOAT zBuf[2000];
PZ_FLOAT xBuf[50];
PZ_FLOAT yBuf[50];

#define Z_BUF(x,y) (zBuf[(x) + (y) * Camera.xGrid])

const char*     szExpr = "sin(sqr(x^2+y^2))*cos(sqr(x^2+y^2))" /* "(1-(x^2+y^2)/2)*exp(-(x^2+y^2)/(2*(1-(x^2+y^2)/2)))" */;
char            szErrorBuf[100] = "";
FzAstNode*      pAstExpr = NULL;
EzMachine*      pVm;
RenderNode*     pRenderNode;
RenderConfig    config;
RenderInterface interface;

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

static void draw1bpp(const unsigned char *raw, int dx, int dy, int w, int h, int rev, unsigned char colorIndex) {
    int pitch = (w >> 3) + (w % 8 ? 1 : 0);
    int x, y, dot;
    unsigned char eightPixels;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            eightPixels = *(raw + y * pitch + (x >> 3));
            dot = (eightPixels >> (7 - x % 8)) & 1;
            if (dot ^ rev) {
                setPixelToSurface(sfScreen, dx + x, dy + y, colorIndex);
            }
        }
    }
}

static void putChar(int x, int y, unsigned char ch) {
    draw1bpp(FONT_HYBIRD_6x8 + 8 * ch, x, y, 8, 8, 0, uSolidColor);
}

static void xyz2xy(PZ_FLOAT x, PZ_FLOAT y, PZ_FLOAT z, int *ox, int *oy) {
    PZ_FLOAT zoom = iHeight / 2;
    PZ_FLOAT nx = (x * Camera.cosB - y * Camera.sinB);
    PZ_FLOAT ny = (-x * Camera.sinB * Camera.sinA - y * Camera.cosB * Camera.sinA + z * Camera.cosA);
    *ox = (int)(iWidth / 2 + zoom * nx);
    *oy = (int)(iHeight / 2 + zoom * ny);
}

static void redraw() {
    int iStartX = 10;
    int iStartY = 10;
    int iCenterY = iStartY + pRenderNode->sSize.iTop;

    SDL_FillRect(sfScreen, NULL, uBgColor);
    
    RenderNode_Draw(pRenderNode, &config, &interface, iStartX, iCenterY);

    {
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
        int ix, iy, x0, y0, x1, y1;
        int i;

        Camera.sinA = sin(Camera.alpha);
        Camera.cosA = cos(Camera.alpha);
        Camera.sinB = sin(Camera.beta);
        Camera.cosB = cos(Camera.beta);

        for (i = 0; i < numEdges; ++i) {
            const Edge* e = BoxEdges + i;
            const Vertex* v0 = BoxVertices + e->i0;
            const Vertex* v1 = BoxVertices + e->i1;
            xyz2xy(v0->x, v0->y, v0->z, &x0, &y0); 
            xyz2xy(v1->x, v1->y, v1->z, &x1, &y1); 
            plotLine(x0, y0, x1, y1);
        }

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

    SDL_Flip(sfScreen);
}

static void recalc() {
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
            Z_BUF(ix, iy) = 2 * (fz - (Camera.zMax + Camera.zMin) / 2) / (Camera.zMax - Camera.zMin);
        }
    }

    /* Normalize x and y grid coordinates into [-1, 1] */
    for (ix = 0; ix < Camera.xGrid; ++ix) {
        xBuf[ix] = 2 * (xBuf[ix] - (Camera.xMax + Camera.xMin) / 2) / (Camera.xMax - Camera.xMin);
    }
    for (iy = 0; iy < Camera.yGrid; ++iy) {
        yBuf[iy] = 2 * (yBuf[iy] - (Camera.yMax + Camera.yMin) / 2) / (Camera.yMax - Camera.yMin);
    }   
}

int main(int argc, char* argv[]) {
    interface.setPixel = setPixel;
    interface.plotLine = plotLine;
    interface.putChar = putChar;
    
    RenderConfig_GetDefault(&config);
    /* config.sDebug.bOutline |= RN_HORIZONTAL; */

    pAstExpr = FzParser_ParseExpression(szExpr);
    pRenderNode = Render_Transform(pAstExpr);
    RenderNode_EstimateSize(pRenderNode, &config);

    pVm = EzMachine_Create();
    EzMachine_DeclareVariable(pVm, "x");
    EzMachine_DeclareVariable(pVm, "y");
    EzMachine_AllocateVariables(pVm);
    EzMachine_Compile(pVm, pAstExpr, szErrorBuf);

    recalc();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }
    sfScreen = SDL_SetVideoMode(iWidth, iHeight, 32, SDL_HWSURFACE);
    SDL_WM_SetCaption("Plotter-Z | SDL", NULL);

    if (sfScreen == NULL) {
        return 0;
    }

    redraw();

    while (bMainLoop) {
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            switch (sdlEvent.type) {
                case SDL_QUIT:
                    bMainLoop = 0;
                    break;
                case SDL_KEYDOWN:
                    if (sdlEvent.key.keysym.sym == SDLK_LEFT) {
                        Camera.beta -= 0.1;
                    }
                    else if (sdlEvent.key.keysym.sym == SDLK_RIGHT) {
                        Camera.beta += 0.1;
                    }
                    if (sdlEvent.key.keysym.sym == SDLK_UP) {
                        Camera.alpha -= 0.1;
                    }
                    else if (sdlEvent.key.keysym.sym == SDLK_DOWN) {
                        Camera.alpha += 0.1;
                    }
                    redraw();
                    break;
            }
        }
    }

    RenderNode_Destroy(pRenderNode);
    EzMachine_Destroy(pVm);

    SDL_Quit();

    return 0;
}