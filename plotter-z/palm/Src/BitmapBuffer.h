#ifndef _BITMAP_BUFFER_H_
#define _BITMAP_BUFFER_H_

#include <PalmOS.h>

#define CURRENT_FONT_WIDTH  6
#define CURRENT_FONT_HEIGHT 8

/*====================================================
 * BmpBuffer - 1bpp monochrome off-screen bitmap
 *             buffer for Palm OS.
 *
 * Use the macros to set coordinates and color in
 * the struct, then call the F-suffixed functions
 * to draw.  This avoids pushing parameters onto
 * the stack on low-end devices.
 *====================================================*/
typedef struct tagBmpBuffer {
    BitmapType* pBmp;   /* Palm OS bitmap handle           */
    UInt8*      pRaw;   /* Raw pixel data (1bpp, 8px/byte) */
    Int16       iW;     /* Width in pixels                 */
    Int16       iH;     /* Height in pixels                */
    Int16       iPitch; /* Bytes per row ((iW+7)/8)        */
    UInt8       iColor; /* Drawing color (0 or non-zero)   */
    UInt8       iChar;  /* Character code for PutChar      */
    struct {
        Int16 iX, iY;
    } sPoint1;          /* Point 1 for setpixel / line     */
    struct {
        Int16 iX, iY;
    } sPoint2;          /* Point 2 for line                */
    const UInt8* pSrc;  /* Source data (8x8 glyph / text)  */
} BmpBuffer;

/* Lifecycle */
BmpBuffer*  BmpBuffer_Create    (Int16 iW, Int16 iH);
void        BmpBuffer_Destroy   (BmpBuffer* pBuffer);
void        BmpBuffer_AllClear  (BmpBuffer* pBuffer);

/* Fast drawing functions (use coordinates from struct) */
void        BmpBuffer_SetPixelF (BmpBuffer* pBuffer);
void        BmpBuffer_PlotLineF (BmpBuffer* pBuffer);
void        BmpBuffer_Draw8x8F  (BmpBuffer* pBuffer);
void        BmpBuffer_PutCharF  (BmpBuffer* pBuffer);
void        BmpBuffer_PutTextF  (BmpBuffer* pBuffer);
void        BmpBuffer_InvertRectF(BmpBuffer* pBuffer);
void        BmpBuffer_FillRectF  (BmpBuffer* pBuffer);

/* Convenience macros: assign coords to struct, then draw */
#define BmpBuffer_SetPixel(pBuf, x, y, color) \
    do { \
        (pBuf)->iColor     = (UInt8)(color); \
        (pBuf)->sPoint1.iX = (Int16)(x); \
        (pBuf)->sPoint1.iY = (Int16)(y); \
        BmpBuffer_SetPixelF(pBuf); \
    } while (0)

#define BmpBuffer_PlotLine(pBuf, x0, y0, x1, y1, color) \
    do { \
        (pBuf)->iColor     = (UInt8)(color); \
        (pBuf)->sPoint1.iX = (Int16)(x0); \
        (pBuf)->sPoint1.iY = (Int16)(y0); \
        (pBuf)->sPoint2.iX = (Int16)(x1); \
        (pBuf)->sPoint2.iY = (Int16)(y1); \
        BmpBuffer_PlotLineF(pBuf); \
    } while (0)

#define BmpBuffer_Draw8x8(pBuf, x, y, pSrcData, color) \
    do { \
        (pBuf)->iColor     = (UInt8)(color); \
        (pBuf)->sPoint1.iX = (Int16)(x); \
        (pBuf)->sPoint1.iY = (Int16)(y); \
        (pBuf)->pSrc       = (pSrcData); \
        BmpBuffer_Draw8x8F(pBuf); \
    } while (0)

#define BmpBuffer_PutChar(pBuf, x, y, ch, color) \
    do { \
        (pBuf)->iColor     = (UInt8)(color); \
        (pBuf)->sPoint1.iX = (Int16)(x); \
        (pBuf)->sPoint1.iY = (Int16)(y); \
        (pBuf)->iChar      = (UInt8)(ch); \
        BmpBuffer_PutCharF(pBuf); \
    } while (0)

#define BmpBuffer_PutText(pBuf, x, y, szText, color) \
    do { \
        (pBuf)->iColor     = (UInt8)(color); \
        (pBuf)->sPoint1.iX = (Int16)(x); \
        (pBuf)->sPoint1.iY = (Int16)(y); \
        (pBuf)->pSrc       = (const UInt8*)(szText); \
        BmpBuffer_PutTextF(pBuf); \
    } while (0)

#define BmpBuffer_InvertRect(pBuf, x, y, w, h) \
    do { \
        (pBuf)->sPoint1.iX = (Int16)(x); \
        (pBuf)->sPoint1.iY = (Int16)(y); \
        (pBuf)->sPoint2.iX = (Int16)(w); \
        (pBuf)->sPoint2.iY = (Int16)(h); \
        BmpBuffer_InvertRectF(pBuf); \
    } while (0)

#define BmpBuffer_FillRect(pBuf, x, y, w, h, color) \
    do { \
        (pBuf)->iColor     = (UInt8)(color); \
        (pBuf)->sPoint1.iX = (Int16)(x); \
        (pBuf)->sPoint1.iY = (Int16)(y); \
        (pBuf)->sPoint2.iX = (Int16)(w); \
        (pBuf)->sPoint2.iY = (Int16)(h); \
        BmpBuffer_FillRectF(pBuf); \
    } while (0)

#endif /* _BITMAP_BUFFER_H_ */
