#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS 1
#include "BitmapBuffer.h"
#include "../../utils/hybird_6x8.h"

#define CURRENT_FONT_WIDTH  6

/*====================================================
 * Bitmap API
 -----------------------------------------------------
   Manually implement the bitmap API in releases prior
   to Palm OS 3.5
 *====================================================*/
#define BmpCreate	PzBmpCreate
#define BmpGetBits	PzBmpGetBits
#define BmpDelete	PzBmpDelete

typedef struct {
    Int16 width;
    Int16 height;
    UInt16 rowBytes;
    UInt16 flags;
    UInt8 pixelSize;
    UInt8 version;
    UInt16 nextDepthOffset;
    UInt8 transparentIndex;
    UInt8 compressionType;
    UInt16 reserved;
} PzBitmapType;

typedef PzBitmapType* PzBitmapPtr;

BitmapPtr PzBmpCreate(Int16 width, Int16 height, UInt8 depth, ColorTableType *colorTableP, UInt16 *error) {
    UInt16 rowBytes;
    UInt32 dataSize;
    UInt32 totalSize;
    MemHandle h;
    PzBitmapPtr bmp;

    rowBytes = (((((UInt32)width * depth) + 15) >> 4) << 1);

    dataSize = (UInt32)rowBytes * height;
    totalSize = sizeof(PzBitmapType) + dataSize;

    h = MemHandleNew(totalSize);
    if (!h) {
        if (error) *error = memErrNotEnoughSpace;
        return NULL;
    }

    bmp = (PzBitmapPtr)MemHandleLock(h);
    
    bmp->width = width;
    bmp->height = height;
    bmp->rowBytes = rowBytes;
    bmp->flags = 0;
    bmp->pixelSize = depth;
    bmp->version = 0;
    bmp->nextDepthOffset = 0;
	bmp->transparentIndex = 0;
	bmp->compressionType = 0;
	bmp->reserved = 0;

    MemSet((UInt8*)bmp + sizeof(PzBitmapType), dataSize, 0x00);

    if (error) *error = errNone;
    return (BitmapPtr)bmp;
}

void* PzBmpGetBits(BitmapPtr bitmapP) {
    if (!bitmapP) return NULL;
    return (void*)((UInt8*)bitmapP + sizeof(PzBitmapType));
}

Err PzBmpDelete(BitmapPtr bitmapP) {
    MemHandle h;

    if (!bitmapP) return /* bmpErrInvalidBitmap */0;

    h = MemPtrRecoverHandle((MemPtr)bitmapP);
    if (!h) return memErrInvalidParam;

    MemHandleUnlock(h);
    MemHandleFree(h);

    return errNone;
}

/*====================================================
 * 1bpp packed-pixel helpers (8 pixels per byte)
 *====================================================*/
#define BB_INDEX(pBuf, x, y)  ((y) * (pBuf)->iPitch + ((x) >> 3))
#define BB_SHIFT(x)           (7 - ((x) & 7))

#define ABS(v)  ((v) < 0 ? -(v) : (v))

/*====================================================
 * BmpBuffer_Create
 *   Allocates struct, creates a 1bpp off-screen bitmap,
 *   obtains raw pixel pointer and computes pitch.
 *====================================================*/
BmpBuffer* BmpBuffer_Create(Int16 iW, Int16 iH) {
    BmpBuffer* pBuf;
    Err        error;

    pBuf = (BmpBuffer*)MemPtrNew(sizeof(BmpBuffer));
    if (pBuf == NULL) return NULL;

    MemSet(pBuf, sizeof(BmpBuffer), 0);

    pBuf->iW = iW;
    pBuf->iH = iH;
    pBuf->iPitch = (iW + 7) >> 3;

    pBuf->pBmp = BmpCreate(iW, iH, 1, NULL, &error);
    if (pBuf->pBmp == NULL) {
        MemPtrFree(pBuf);
        return NULL;
    }

    pBuf->pRaw = (UInt8*)BmpGetBits(pBuf->pBmp);
    if (pBuf->pRaw == NULL) {
        BmpDelete(pBuf->pBmp);
        MemPtrFree(pBuf);
        return NULL;
    }

    return pBuf;
}

/*====================================================
 * BmpBuffer_Destroy
 *====================================================*/
void BmpBuffer_Destroy(BmpBuffer* pBuffer) {
    if (pBuffer == NULL) return;
    if (pBuffer->pBmp != NULL)
        BmpDelete(pBuffer->pBmp);
    MemPtrFree(pBuffer);
}

/*====================================================
 * BmpBuffer_AllClear
 *   Zeros all pixels in the buffer.
 *====================================================*/
void BmpBuffer_AllClear(BmpBuffer* pBuffer) {
    if (pBuffer == NULL || pBuffer->pRaw == NULL) return;
    MemSet(pBuffer->pRaw,
        (UInt32)(pBuffer->iPitch * pBuffer->iH), 0);
}

/*====================================================
 * BmpBuffer_SetPixelF
 *   Sets a single pixel using sPoint1 coordinates.
 *   iColor: 0 = clear bit, non-zero = set bit.
 *====================================================*/
void BmpBuffer_SetPixelF(BmpBuffer* pBuffer) {
    Int16  x, y;
    UInt16 iByte;
    UInt8  ucShift;

    x = pBuffer->sPoint1.iX;
    y = pBuffer->sPoint1.iY;

    if (x < 0 || x >= pBuffer->iW || y < 0 || y >= pBuffer->iH)
        return;

    iByte   = BB_INDEX(pBuffer, x, y);
    ucShift = BB_SHIFT(x);

    if (pBuffer->iColor)
        pBuffer->pRaw[iByte] |= (UInt8)(1 << ucShift);
    else
        pBuffer->pRaw[iByte] &= (UInt8)(~(1 << ucShift));
}

/*====================================================
 * BmpBuffer_PlotLineF
 *   Bresenham line from sPoint1 to sPoint2.
 *====================================================*/
void BmpBuffer_PlotLineF(BmpBuffer* pBuffer) {
    Int16 x0, y0, x1, y1;
    Int16 dx, dy, sx, sy;
    Int16 err, e2;

    x0 = pBuffer->sPoint1.iX;
    y0 = pBuffer->sPoint1.iY;
    x1 = pBuffer->sPoint2.iX;
    y1 = pBuffer->sPoint2.iY;

    dx = ABS((Int16)(x1 - x0));
    sx = (x0 < x1) ? 1 : -1;
    dy = -ABS((Int16)(y1 - y0));
    sy = (y0 < y1) ? 1 : -1;
    err = dx + dy;

    for (;;) {
        /* Inline setpixel at (x0, y0) */
        if (x0 >= 0 && x0 < pBuffer->iW && y0 >= 0 && y0 < pBuffer->iH) {
            UInt16 iByte   = BB_INDEX(pBuffer, x0, y0);
            UInt8  ucShift = BB_SHIFT(x0);
            if (pBuffer->iColor)
                pBuffer->pRaw[iByte] |= (UInt8)(1 << ucShift);
            else
                pBuffer->pRaw[iByte] &= (UInt8)(~(1 << ucShift));
        }

        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 = (Int16)(x0 + sx); }
        if (e2 <= dx) { err += dx; y0 = (Int16)(y0 + sy); }
    }
}

/*====================================================
 * BmpBuffer_Draw8x8F
 *   Draws a 1bpp 8x8 bitmap at sPoint1 using pSrc.
 *   Source: 8 bytes, MSB-first per row.
 *   Destination: 1bpp canvas (8 pixels per byte).
 *
 *   Handles three cases per row:
 *     - left-clipped (x < 0)
 *     - right-clipped (x + 8 >= iW)
 *     - full span writing up to 2 bytes
 *====================================================*/
void BmpBuffer_Draw8x8F(BmpBuffer* pBuffer) {
    Int16 dx, dy;
    Int16 row;
    Int16 rowStart, rowEnd;
    Int16 iByteOff, iBitOff;
    UInt8 ucColorBit;

    dx = pBuffer->sPoint1.iX;
    dy = pBuffer->sPoint1.iY;

    if (pBuffer->pSrc == NULL) return;

    /* Entirely off-screen */
    if (dx + 8 <= 0 || dx >= pBuffer->iW
        || dy + 8 <= 0 || dy >= pBuffer->iH) return;

    ucColorBit = pBuffer->iColor ? 1 : 0;
    iByteOff   = dx >> 3;
    iBitOff    = dx & 7;

    /* Visible row range */
    rowStart = (dy < 0) ? (-dy) : 0;
    rowEnd  = (dy + 8 >= pBuffer->iH) ? (pBuffer->iH - 1 - dy) : 7;
    if (rowStart > rowEnd) return;

    /* Left-clipped: only the right-shifted portion is visible */
    if (dx < 0) {
        for (row = rowStart; row <= rowEnd; ++row) {
            UInt16 iAddr;
            UInt8  ucSrc, ucPart2;
            ucSrc   = pBuffer->pSrc[row];
            iAddr   = (UInt16)(dy + row) * (UInt16)pBuffer->iPitch + (UInt16)iByteOff;
            ucPart2 = ucSrc << (8 - iBitOff);
            if (ucPart2 && iAddr + 1 < (UInt16)pBuffer->iPitch * (UInt16)pBuffer->iH) {
                if (ucColorBit)
                    pBuffer->pRaw[iAddr + 1] |= ucPart2;
                else
                    pBuffer->pRaw[iAddr + 1] &= (UInt8)(~ucPart2);
            }
        }
    }
    /* Right-clipped: only the left-shifted portion is visible */
    else if (dx + 8 >= pBuffer->iW) {
        for (row = rowStart; row <= rowEnd; ++row) {
            UInt16 iAddr;
            UInt8  ucSrc, ucPart1;
            ucSrc   = pBuffer->pSrc[row];
            iAddr   = (UInt16)(dy + row) * (UInt16)pBuffer->iPitch + (UInt16)iByteOff;
            ucPart1 = ucSrc >> iBitOff;
            if (ucPart1) {
                if (ucColorBit)
                    pBuffer->pRaw[iAddr] |= ucPart1;
                else
                    pBuffer->pRaw[iAddr] &= (UInt8)(~ucPart1);
            }
        }
    }
    /* Full span: writes to 2 bytes per row */
    else {
        for (row = rowStart; row <= rowEnd; ++row) {
            UInt16 iAddr;
            UInt8  ucSrc, ucPart1, ucPart2;
            ucSrc   = pBuffer->pSrc[row];
            iAddr   = (UInt16)(dy + row) * (UInt16)pBuffer->iPitch + (UInt16)iByteOff;
            ucPart1 = ucSrc >> iBitOff;
            ucPart2 = ucSrc << (8 - iBitOff);
            if (ucColorBit) {
                pBuffer->pRaw[iAddr]     |= ucPart1;
                if (iBitOff != 0)
                    pBuffer->pRaw[iAddr + 1] |= ucPart2;
            } else {
                pBuffer->pRaw[iAddr]     &= (UInt8)(~ucPart1);
                if (iBitOff != 0)
                    pBuffer->pRaw[iAddr + 1] &= (UInt8)(~ucPart2);
            }
        }
    }
}

/*====================================================
 * BmpBuffer_PutCharF
 *   Draws a single 6x8 character using the hybird
 *   font.  sPoint1 = (x,y), iChar = character code.
 *====================================================*/
void BmpBuffer_PutCharF(BmpBuffer* pBuffer) {
    pBuffer->pSrc = FONT_HYBIRD_6x8 + 8 * (UInt16)(pBuffer->iChar);
    BmpBuffer_Draw8x8F(pBuffer);
}

/*====================================================
 * BmpBuffer_InvertRectF
 *   Inverts all bits in rectangle at sPoint1=(x,y)
 *   with sPoint2=(w,h).
 *====================================================*/
void BmpBuffer_InvertRectF(BmpBuffer* pBuffer) {
    Int16 x, y, w, h;
    Int16 iRow;
    Int16 iStartByte, iEndByte;
    UInt8 ucFirstMask, ucLastMask;

    x = pBuffer->sPoint1.iX;
    y = pBuffer->sPoint1.iY;
    w = pBuffer->sPoint2.iX;
    h = pBuffer->sPoint2.iY;

    if (w <= 0 || h <= 0) return;

    /* Clamp to bitmap bounds */
    if (x < 0) { w = (Int16)(w + x); x = 0; }
    if (y < 0) { h = (Int16)(h + y); y = 0; }
    if (x + w > pBuffer->iW) w = (Int16)(pBuffer->iW - x);
    if (y + h > pBuffer->iH) h = (Int16)(pBuffer->iH - y);
    if (w <= 0 || h <= 0) return;

    iStartByte = x >> 3;
    iEndByte   = (x + w - 1) >> 3;
    ucFirstMask = (UInt8)(0xFF >> (x & 7));
    if ((x + w) & 7)
        ucLastMask = (UInt8)(0xFF << (8 - ((x + w) & 7)));
    else
        ucLastMask = 0xFF;

    for (iRow = 0; iRow < h; ++iRow) {
        UInt16 iBase;
        Int16  iDstY;
        iDstY = (Int16)(y + iRow);
        iBase = (UInt16)iDstY * (UInt16)pBuffer->iPitch;

        if (iStartByte == iEndByte) {
            UInt8 ucMask = (UInt8)(ucFirstMask & ucLastMask);
            pBuffer->pRaw[iBase + iStartByte] ^= ucMask;
        } else {
            UInt16 iByte;
            pBuffer->pRaw[iBase + iStartByte] ^= ucFirstMask;
            for (iByte = iStartByte + 1; iByte < iEndByte; ++iByte)
                pBuffer->pRaw[iBase + iByte] ^= 0xFF;
            pBuffer->pRaw[iBase + iEndByte] ^= ucLastMask;
        }
    }
}

/*====================================================
 * BmpBuffer_FillRectF
 *   Fills rectangle with iColor.
 *   sPoint1=(x,y), sPoint2=(w,h).
 *====================================================*/
void BmpBuffer_FillRectF(BmpBuffer* pBuffer) {
    Int16 x, y, w, h;
    Int16 iRow;
    Int16 iStartByte, iEndByte;
    UInt8 ucFirstMask, ucLastMask;

    x = pBuffer->sPoint1.iX;
    y = pBuffer->sPoint1.iY;
    w = pBuffer->sPoint2.iX;
    h = pBuffer->sPoint2.iY;

    if (w <= 0 || h <= 0) return;

    if (x < 0) { w = (Int16)(w + x); x = 0; }
    if (y < 0) { h = (Int16)(h + y); y = 0; }
    if (x + w > pBuffer->iW) w = (Int16)(pBuffer->iW - x);
    if (y + h > pBuffer->iH) h = (Int16)(pBuffer->iH - y);
    if (w <= 0 || h <= 0) return;

    iStartByte  = x >> 3;
    iEndByte    = (x + w - 1) >> 3;
    ucFirstMask = (UInt8)(0xFF >> (x & 7));
    if ((x + w) & 7)
        ucLastMask = (UInt8)(0xFF << (8 - ((x + w) & 7)));
    else
        ucLastMask = 0xFF;

    for (iRow = 0; iRow < h; ++iRow) {
        UInt16 iBase;
        iBase = (UInt16)(y + iRow) * (UInt16)pBuffer->iPitch;

        if (iStartByte == iEndByte) {
            UInt8 ucMask = (UInt8)(ucFirstMask & ucLastMask);
            if (pBuffer->iColor)
                pBuffer->pRaw[iBase + iStartByte] |= ucMask;
            else
                pBuffer->pRaw[iBase + iStartByte] &= (UInt8)(~ucMask);
        } else {
            UInt16 iByte;
            if (pBuffer->iColor) {
                pBuffer->pRaw[iBase + iStartByte] |= ucFirstMask;
                for (iByte = iStartByte + 1; iByte < iEndByte; ++iByte)
                    pBuffer->pRaw[iBase + iByte] = 0xFF;
                pBuffer->pRaw[iBase + iEndByte] |= ucLastMask;
            } else {
                pBuffer->pRaw[iBase + iStartByte] &= (UInt8)(~ucFirstMask);
                for (iByte = iStartByte + 1; iByte < iEndByte; ++iByte)
                    pBuffer->pRaw[iBase + iByte] = 0;
                pBuffer->pRaw[iBase + iEndByte] &= (UInt8)(~ucLastMask);
            }
        }
    }
}
/*====================================================
 *   Draws a null-terminated string using the hybird
 *   font.  sPoint1 = (x,y), pSrc = string pointer.
 *====================================================*/
void BmpBuffer_PutTextF(BmpBuffer* pBuffer) {
    const UInt8* sz;
    Int16 x;

    sz = pBuffer->pSrc;
    if (sz == NULL) return;

    x = pBuffer->sPoint1.iX;

    while (*sz != '\0') {
        pBuffer->iChar  = *sz;
        pBuffer->pSrc   = FONT_HYBIRD_6x8 + 8 * (UInt16)(*sz);
        pBuffer->sPoint1.iX = x;
        BmpBuffer_Draw8x8F(pBuffer);

        ++sz;
        x = (Int16)(x + CURRENT_FONT_WIDTH);
    }
}