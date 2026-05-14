#include <stdio.h>
#include <SDL/SDL.h>
#include "../utils/vga_index.h"
#include "../utils/vga_palette.h"
#include "../utils/hybird_6x8.h"
#include "../../renderer-z/rz.h"

#define ABS(v) ((v) < 0 ? -(v) : (v))

int             bMainLoop = 1;
int             iWidth = 640;
int             iHeight = 480;
SDL_Surface*    sfScreen;
Uint32          uBgColor = VgaMode13hColorPalette[VGA_COLOR_WHITE];
Uint32          uSolidColor = VgaMode13hColorPalette[VGA_COLOR_BLACK];

const char*     szExpr = "(1-(x^2+y^2)/2)*exp(-(x^2+y^2)/(2*(1-(x^2+y^2)/2)))";
FzAstNode*      pAstExpr = NULL;
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

static void redraw() {
    int iStartX = 10;
    int iStartY = 10;
    int iCenterY = iStartY + pRenderNode->sSize.iTop;

    SDL_FillRect(sfScreen, NULL, uBgColor);
    
    RenderNode_Draw(pRenderNode, &config, &interface, iStartX, iCenterY);
    
    SDL_Flip(sfScreen);
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
            }
        }
    }

    RenderNode_Destroy(pRenderNode);

    SDL_Quit();

    return 0;
}