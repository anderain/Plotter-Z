#include <fxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitmaps.h"
#include "../utils/hybird_6x8.h"
#include "../../formula-z\fz.h"

typedef unsigned int uint;
typedef unsigned char uchar;

/*====================================================
 * SysCall
 *====================================================*/

#define SCA 0xD201D002
#define SCB 0x422B0009
#define SCE 0x80010070

typedef void* (*pGetVRAMAddress)(void);
const static unsigned int sc0x135[] = { SCA, SCB, SCE, 0x135 };
#define GetVRAMAddress (*(pGetVRAMAddress)sc0x135)

typedef int (*pGetTicks)(void);
const static unsigned int sc003B[] = { SCA, SCB, SCE, 0x03B };
#define RTC_GetTicks (*(pGetTicks)sc003B)

unsigned char* pVRAM = 0;

/*====================================================
 * Graphics
 *====================================================*/

#define CURRENT_FONT_WIDTH  6
#define CURRENT_FONT_HEIGHT 8

void DrawSprite8x8(int iX, int iY, const uchar* pSprite) {
    int iRowOffset;
    int iStartByte = iX >> 3;
    int iBitOffset = iX & 0x07;
    unsigned char ucSpriteByte;
    int iAddr;
    int iRow, iRowStart, iRowEnd;
    unsigned char ucPart1, ucPart2;

    /* Entirely off-screen */
    if (iX + 8 <= 0 || iX >= 128 || iY + 8 <= 0 || iY >= 64) {
        return;
    }
    
    /* Visible iRow range */
    iRowStart = iY < 0 ? -iY : 0;
    iRowEnd = iY + 8 >= 64 ? 63 - iY : 7;

    /* Left-clipped: only the right-shifted portion is visible */
    if (iX < 0) {
        for (iRow = iRowStart; iRow <= iRowEnd; ++iRow) {
            iRowOffset = (iY + iRow) << 4;
            ucSpriteByte = pSprite[iRow];
            iAddr = iRowOffset + iStartByte;
            ucPart2 = ucSpriteByte << (8 - iBitOffset);
            pVRAM[iAddr + 1] |= ucPart2;
        }
    }
    /* Right-clipped: only the left-shifted portion is visible */
    else if (iX + 8 >= 128) {
        for (iRow = iRowStart; iRow <= iRowEnd; ++iRow) {
            iRowOffset = (iY + iRow) << 4;
            ucSpriteByte = pSprite[iRow];
            iAddr = iRowOffset + iStartByte;
            ucPart1 = ucSpriteByte >> iBitOffset;
            pVRAM[iAddr] |= ucPart1;
        }
    }
    /* Full span: writes to 2 bytes per iRow */
    else {
        for (iRow = iRowStart; iRow <= iRowEnd; ++iRow) {
            iRowOffset = (iY + iRow) << 4;
            ucSpriteByte = pSprite[iRow];
            iAddr = iRowOffset + iStartByte;
            ucPart1 = ucSpriteByte >> iBitOffset;
            ucPart2 = ucSpriteByte << (8 - iBitOffset);
            pVRAM[iAddr] |= ucPart1;
            pVRAM[iAddr + 1] |= ucPart2;
        }
    }
}

void DrawBitmap(int iDestX, int iDestY, const uchar* pBitmap) {
    int iW = pBitmap[0];
    int iH = pBitmap[1];
    int iCol = (iW / 8) + (iW % 8 != 0);
    int iRow = (iH / 8) + (iH % 8 != 0);
    int iX, iY;

    pBitmap += 2;

    for (iY = 0; iY < iRow; iY++) {
        for (iX = 0; iX < iCol; iX++) {
            DrawSprite8x8(iDestX + (iX << 3), iDestY + (iY << 3), pBitmap);
            pBitmap += 8;
        }
    }
}

void PutText(int iDestX, int iDestY, const uchar* szText) {
    uchar* pText;
    for (pText = szText; *pText; ++pText) {
        DrawSprite8x8(iDestX, iDestY, FONT_HYBIRD_6x8 + (((int)*pText) << 3));
        iDestX += CURRENT_FONT_WIDTH;
    }
}

/*====================================================
 * Main Stage
 *====================================================*/

void DrawMainStage(void) {
    static const int iMenuLeft = 2;
    static const int iMenuWidth = 21;
    static const int iMenuTop = 56;
    static const uchar* pMenuBitmap[] = { MENU_EXPR, MENU_WIN, MENU_SAMPLE, 0, 0, MENU_PLOT };
    static const int iMenuSize = sizeof(pMenuBitmap) / sizeof(pMenuBitmap[0]);
    int bMenuItemVisible[6];
    DISPBOX boxMenuArea = { 0, 56, 127, 63 };
    int i;
    /* Clear Screen */
    Bdisp_AllClr_VRAM();
    /* Clear Bottom Area */
    Bdisp_AreaClr_VRAM(&boxMenuArea);
    /* Check Visibility */
    memset(bMenuItemVisible, 0, sizeof(bMenuItemVisible));
    /* Bottom Menu */
    for (i = 0; i < iMenuSize; ++i) {
        if (pMenuBitmap[i]) {
            DrawBitmap(iMenuLeft + iMenuWidth * i, iMenuTop, pMenuBitmap[i]);
        }
    }
    PutText(2, 2, (const uchar*) "Hello \x18");
}

int MainStage() {
    uint key;
    pVRAM = GetVRAMAddress();
    while (1) {
        DrawMainStage();
        GetKey(&key);
    }
}

/*====================================================
 * fx-9860G SDK boilerplate
 *====================================================*/
int AddIn_main(int isAppli, unsigned short OptionNum) {
    MainStage();
    return 1;
}

#pragma section _BR_Size
unsigned long BR_Size;
#pragma section

#pragma section _TOP

int InitializeSystem(int isAppli, unsigned short OptionNum) {
    return INIT_ADDIN_APPLICATION(isAppli, OptionNum);
}

#pragma section

