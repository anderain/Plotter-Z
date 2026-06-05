#include <stdlib.h>
#include <string.h>
#include "rz.h"

static int maxOfThree(int a, int b, int c);

static const RenderConfig DefaultConfig = {
    /* Text */      { 1 },
    /* Font */      { 6, 8 },
    /* Stack */     { 2, 2 },
    /* Root */      { 4, 2 },
    /* Enclosure */ { 2, 1, 1 },
    /* Overunder */ { 2, 2 },
    /* BigSymbol */ {
        /* Sum */   { 12, 12 },
        /* Prd */   { 12, 12 },
        /* Int */   { 8, 18 }
    },
    /* Debug */     { 0 },
    /* Interface */ { NULL, NULL, NULL }
};

void RenderConfig_GetDefaultStyle(RenderConfig* pConfig) {
    if (pConfig == NULL) return;
    memcpy(pConfig, &DefaultConfig, sizeof(DefaultConfig));
}

void RenderConfig_CalculateBigSymbolPoints(RenderConfig* pConfig) {
    GlyphPoint* pts;
    int w, hh;

    /* sum */
    w = pConfig->sBigSymbol.sSum.iWidth;
    hh = pConfig->sBigSymbol.sSum.iHeight / 2;
    pts = pConfig->sBigSymbol.sSum.sPoints;
    pts[0].x = w;       pts[0].y = -hh + hh/3;
    pts[1].x = w;       pts[1].y = -hh;
    pts[2].x = 0;       pts[2].y = -hh;
    pts[3].x = w/2;     pts[3].y = 0;
    pts[4].x = 0;       pts[4].y = hh;
    pts[5].x = w;       pts[5].y = hh;
    pts[6].x = w;       pts[6].y = hh - hh/3;

    /* prd */
    w = pConfig->sBigSymbol.sPrd.iWidth;
    hh = pConfig->sBigSymbol.sPrd.iHeight / 2;
    pts = pConfig->sBigSymbol.sPrd.sPoints;
    pts[0].x = 0;           pts[0].y = -hh;
    pts[1].x = w;           pts[1].y = -hh;
    pts[2].x = w/6;         pts[2].y = -hh;
    pts[3].x = w/6;         pts[3].y = hh;
    pts[4].x = w - w/6;     pts[4].y = -hh;
    pts[5].x = w - w/6;     pts[5].y =  hh;

    /* int */
    w = pConfig->sBigSymbol.sInt.iWidth;
    hh = pConfig->sBigSymbol.sInt.iHeight / 2;
    pts = pConfig->sBigSymbol.sInt.sPoints;
    pts[0].x = w;           pts[0].y = -hh + hh/4;
    pts[1].x = w/2 + w/4;   pts[1].y = -hh;
    pts[2].x = w/2;         pts[2].y = -hh + hh/4;
    pts[3].x = w/2;         pts[3].y = hh - hh/4;
    pts[4].x = w/2 - w/4;   pts[4].y = hh;
    pts[5].x = 0;           pts[5].y = hh -hh/4;
}

void RenderNode_CalculateSize(RenderNode* pNode, const RenderConfig* pConfig) {
    switch (pNode->iType) {
        case RN_TEXT:
            pNode->sLayout.iWidth = pConfig->sFont.iWidth * strlen(pNode->uData.sText.szText) + pConfig->sText.iPaddingLeft;
            pNode->sLayout.iAscent = pConfig->sFont.iHeight / 2;
            pNode->sLayout.iDescent = pConfig->sFont.iHeight / 2;
            break;
        case RN_SPECIAL_CHAR:
            pNode->sLayout.iWidth = pConfig->sFont.iWidth + pConfig->sText.iPaddingLeft;
            pNode->sLayout.iAscent = pConfig->sFont.iHeight / 2;
            pNode->sLayout.iDescent = pConfig->sFont.iHeight / 2;
            break;
        case RN_HORIZONTAL: {
            int iSumWidth = 0;
            int iTopMax = 0;
            int iBottomMax = 0;
            VlistNode* pListNode;
            for (
                pListNode = pNode->uData.sHorizontal.pList->pHead;
                pListNode != NULL;
                pListNode = pListNode->pNext
            ) {
                RenderNode* pChild = (RenderNode *)pListNode->pData;
                RenderNode_CalculateSize(pChild, pConfig);
                if (pChild->sLayout.iAscent > iTopMax) iTopMax = pChild->sLayout.iAscent;
                if (pChild->sLayout.iDescent > iBottomMax) iBottomMax = pChild->sLayout.iDescent;
                iSumWidth += pChild->sLayout.iWidth;
            }
            pNode->sLayout.iWidth = iSumWidth;
            pNode->sLayout.iAscent = iTopMax;
            pNode->sLayout.iDescent = iBottomMax;
            break;
        }
        case RN_ENCLOSURE: {
            RenderNode* pContent = pNode->uData.sEnclosure.pContent;
            RenderNode_CalculateSize(pContent, pConfig);
            pNode->sLayout.iWidth = pContent->sLayout.iWidth + 2 * pConfig->sEnclosure.iPadding + 2 * pConfig->sEnclosure.iMargin;
            pNode->sLayout.iAscent = pContent->sLayout.iAscent;
            pNode->sLayout.iDescent = pContent->sLayout.iDescent;
            break;
        }
        case RN_STACK: {
            RenderNode* pTop = pNode->uData.sStack.pTop;
            RenderNode* pBottom = pNode->uData.sStack.pBottom;
            RenderNode_CalculateSize(pTop, pConfig);
            RenderNode_CalculateSize(pBottom, pConfig);
            pNode->sLayout.iWidth = pConfig->sStack.iSpacingY * 2 + (pTop->sLayout.iWidth > pBottom->sLayout.iWidth ? pTop->sLayout.iWidth : pBottom->sLayout.iWidth);
            pNode->sLayout.iAscent = pTop->sLayout.iAscent + pTop->sLayout.iDescent + pConfig->sStack.iSpacingY;
            pNode->sLayout.iDescent = pBottom->sLayout.iAscent + pBottom->sLayout.iDescent + pConfig->sStack.iSpacingY;
            break;
        }
        case RN_ROOT: {
            RenderNode* pContent = pNode->uData.sRoot.pContent;
            RenderNode_CalculateSize(pContent, pConfig);
            pNode->sLayout.iWidth = pContent->sLayout.iWidth + pConfig->sRoot.iSpacingLeft;
            pNode->sLayout.iAscent = pContent->sLayout.iAscent + pConfig->sRoot.iSpacingTop;
            pNode->sLayout.iDescent = pContent->sLayout.iDescent;
            break;
        }
        case RN_SUPERSCRIPT: {
            RenderNode* pBody = pNode->uData.sSuperscript.pBody;
            RenderNode* pScript = pNode->uData.sSuperscript.pScript;
            RenderNode_CalculateSize(pBody, pConfig);
            RenderNode_CalculateSize(pScript, pConfig);
            pNode->sLayout.iWidth = pBody->sLayout.iWidth + pScript->sLayout.iWidth;
            pNode->sLayout.iAscent = pBody->sLayout.iAscent + (pScript->sLayout.iAscent + pScript->sLayout.iDescent - pConfig->sFont.iHeight / 2);
            pNode->sLayout.iDescent = pBody->sLayout.iDescent;
            break;
        }
        case RN_OVERUNDER: {
            RenderNode* pOver = pNode->uData.sOverunder.pOver;
            RenderNode* pBase = pNode->uData.sOverunder.pBase;
            RenderNode* pUnder = pNode->uData.sOverunder.pUnder;
            int iOverWidth = 0, iOverHeight = 0;
            int iBaseWidth = 0;
            int iUnderWidth = 0, iUnderHeight = 0;
            if (pOver) {
                RenderNode_CalculateSize(pOver, pConfig);
                iOverWidth = pOver->sLayout.iWidth;
                iOverHeight = pOver->sLayout.iAscent + pOver->sLayout.iDescent;
            }
            if (pBase) {
                RenderNode_CalculateSize(pBase, pConfig);
                iBaseWidth = pBase->sLayout.iWidth;
            }
            if (pUnder) {
                RenderNode_CalculateSize(pUnder, pConfig);
                iUnderWidth = pUnder->sLayout.iWidth;
                iUnderHeight = pUnder->sLayout.iAscent + pUnder->sLayout.iDescent;
            }
            pNode->sLayout.iWidth = maxOfThree(iOverWidth, iUnderWidth, iBaseWidth) + pConfig->sOverunder.iPaddingRight;
            pNode->sLayout.iAscent = pBase->sLayout.iAscent + pConfig->sOverunder.iPaddingY + iOverHeight;
            pNode->sLayout.iDescent = pBase->sLayout.iDescent + pConfig->sOverunder.iPaddingY + iUnderHeight;
            break;
        }
        case RN_BIG_SYMBOL: {
            switch (pNode->uData.sBigSymbol.iType) {
                case ST_SUM:
                    pNode->sLayout.iWidth = pConfig->sBigSymbol.sSum.iWidth;
                    pNode->sLayout.iAscent = pConfig->sBigSymbol.sSum.iHeight / 2;
                    pNode->sLayout.iDescent = pConfig->sBigSymbol.sSum.iHeight / 2;
                    break;
                case ST_PRD:
                    pNode->sLayout.iWidth = pConfig->sBigSymbol.sPrd.iWidth;
                    pNode->sLayout.iAscent = pConfig->sBigSymbol.sPrd.iHeight / 2;
                    pNode->sLayout.iDescent = pConfig->sBigSymbol.sPrd.iHeight / 2;
                    break;
                case ST_INT:
                    pNode->sLayout.iWidth = pConfig->sBigSymbol.sInt.iWidth;
                    pNode->sLayout.iAscent = pConfig->sBigSymbol.sInt.iHeight / 2;
                    pNode->sLayout.iDescent = pConfig->sBigSymbol.sInt.iHeight / 2;
                    break;
            }
            break;
        }
    }
}

void RenderNode_Draw(RenderNode* pNode, const RenderConfig* pConfig, int iStartX, int iBaseline) {
    if (pNode->iType & pConfig->sDebug.bOutline) {
        pConfig->sInterfaces.plotLine(
            iStartX, iBaseline - pNode->sLayout.iAscent,
            iStartX + pNode->sLayout.iWidth, iBaseline - pNode->sLayout.iAscent
        );
        pConfig->sInterfaces.plotLine(
            iStartX + pNode->sLayout.iWidth, iBaseline - pNode->sLayout.iAscent,
            iStartX + pNode->sLayout.iWidth, iBaseline + pNode->sLayout.iDescent
        );
        pConfig->sInterfaces.plotLine(
            iStartX + pNode->sLayout.iWidth, iBaseline + pNode->sLayout.iDescent,
            iStartX, iBaseline + pNode->sLayout.iDescent
        );
        pConfig->sInterfaces.plotLine(
            iStartX, iBaseline + pNode->sLayout.iDescent,
            iStartX, iBaseline - pNode->sLayout.iAscent
        );
    }
    switch (pNode->iType) {
        case RN_TEXT: {
            int i;
            int x = iStartX + pConfig->sText.iPaddingLeft;
            int y = iBaseline - pNode->sLayout.iAscent + pConfig->sFont.iHeight / 8;
            const char* szText = pNode->uData.sText.szText;
            for (i = 0; szText[i]; ++i) {
                pConfig->sInterfaces.putChar(x, y, (unsigned char)szText[i]);
                x += pConfig->sFont.iWidth;
            }
            break;
        }
        case RN_SPECIAL_CHAR: {
            unsigned char c = pNode->uData.sSpecialChar.c;
            int x = iStartX + pConfig->sText.iPaddingLeft;
            int y = iBaseline - pNode->sLayout.iAscent + pConfig->sFont.iHeight / 8;
            pConfig->sInterfaces.putChar(x, y, c);
            break;
        }
        case RN_HORIZONTAL: {
            int x = iStartX;
            int y = iBaseline;
            VlistNode* pListNode;
            for (
                pListNode = pNode->uData.sHorizontal.pList->pHead;
                pListNode != NULL;
                pListNode = pListNode->pNext
            ) {
                RenderNode* pChild = (RenderNode *)pListNode->pData;
                RenderNode_Draw(pChild, pConfig, x, y);
                x += pChild->sLayout.iWidth;
            }
            break;
        }
        case RN_ENCLOSURE: {
            int x, y;
            int w;
            int asc = pNode->sLayout.iAscent;
            int dsc = pNode->sLayout.iDescent;
            int rad = pConfig->sEnclosure.iRadius;

            RenderNode* pContent = pNode->uData.sEnclosure.pContent;

            x = iStartX + (pNode->sLayout.iWidth - pContent->sLayout.iWidth) / 2;
            y = iBaseline;
            
            RenderNode_Draw(pContent, pConfig, x, y);

            x = iStartX + pConfig->sEnclosure.iMargin;
            w = pNode->sLayout.iWidth - pConfig->sEnclosure.iMargin * 2 - 1;

            /* Curved parentheses */
            if (pNode->uData.sEnclosure.bCurve) {
                pConfig->sInterfaces.plotLine(
                    x, y - asc + rad,
                    x, y + dsc - rad
                );
                pConfig->sInterfaces.plotLine(
                    x, y - asc + rad,
                    x + rad, y - asc
                );
                pConfig->sInterfaces.plotLine(
                    x, y + dsc - rad,
                    x + rad, y + dsc
                );
                pConfig->sInterfaces.plotLine(
                    x + w, y - asc + rad,
                    x + w, y + dsc - rad
                );
                pConfig->sInterfaces.plotLine(
                    x + w, y - asc + rad,
                    x + w - rad, y - asc
                );
                pConfig->sInterfaces.plotLine(
                    x + w, y + dsc - rad,
                    x + w - rad, y + dsc
                );
            }
            /* Straight absolute value bars */
            else {
                pConfig->sInterfaces.plotLine(
                    x, y - asc,
                    x, y + dsc
                );
                pConfig->sInterfaces.plotLine(
                    x + w, y - asc,
                    x + w, y + dsc
                );
            }
            break;
        }
        case RN_STACK: {
            int x, y;
            RenderNode* pTop = pNode->uData.sStack.pTop;
            RenderNode* pBottom = pNode->uData.sStack.pBottom;

            x = iStartX + (pNode->sLayout.iWidth - pTop->sLayout.iWidth) / 2 ;
            y = iBaseline - pNode->sLayout.iAscent + pTop->sLayout.iAscent;
            RenderNode_Draw(pTop, pConfig, x, y);
            
            x = iStartX + (pNode->sLayout.iWidth - pBottom->sLayout.iWidth) / 2;
            y = iBaseline + pNode->sLayout.iDescent - pBottom->sLayout.iDescent;
            RenderNode_Draw(pBottom, pConfig, x, y);

            pConfig->sInterfaces.plotLine(iStartX, iBaseline, iStartX + pNode->sLayout.iWidth - 2, iBaseline);
            break;
        }
        case RN_ROOT: {
            int x, y, w, h;
            int checkSize = 1;
            RenderNode* pContent = pNode->uData.sRoot.pContent;
            x = iStartX + pConfig->sRoot.iSpacingLeft;
            y = iBaseline;
            RenderNode_Draw(pContent, pConfig, x, y);

            w = pNode->sLayout.iWidth;
            h = pNode->sLayout.iDescent + pNode->sLayout.iAscent;

            x = iStartX;
            y = iBaseline - pNode->sLayout.iAscent;
            pConfig->sInterfaces.plotLine(
                x + pConfig->sRoot.iSpacingLeft / 2, y,
                x + w, y
            );

            pConfig->sInterfaces.plotLine(
                x + pConfig->sRoot.iSpacingLeft / 2,
                y,
                x + checkSize,
                y + h
            );

            pConfig->sInterfaces.plotLine(
                x + checkSize,
                y + h,
                x,
                y + h - checkSize
            );

            break;
        }
        case RN_SUPERSCRIPT: {
            int x, y;
            RenderNode* pBody = pNode->uData.sSuperscript.pBody;
            RenderNode* pScript = pNode->uData.sSuperscript.pScript;
            
            x = iStartX;
            y = iBaseline;
            RenderNode_Draw(pBody, pConfig, x, y);
            
            x = iStartX + pBody->sLayout.iWidth;
            y = iBaseline - pNode->sLayout.iAscent + pScript->sLayout.iAscent;
            RenderNode_Draw(pScript, pConfig, x, y);
            
            break;
        }
        case RN_OVERUNDER: {
            RenderNode* pOver = pNode->uData.sOverunder.pOver;
            RenderNode* pBase = pNode->uData.sOverunder.pBase;
            RenderNode* pUnder = pNode->uData.sOverunder.pUnder;
            int iWidth = pNode->sLayout.iWidth - pConfig->sOverunder.iPaddingRight;
            int x, y;
            /* Over */
            if (pOver) {
                x = iStartX + (iWidth - pOver->sLayout.iWidth) / 2;
                y = iBaseline - pConfig->sOverunder.iPaddingY - pBase->sLayout.iAscent - pOver->sLayout.iDescent;
                RenderNode_Draw(pOver, pConfig, x, y);
            }
            /* Base */
            if (pBase) {
                x = iStartX + (iWidth - pBase->sLayout.iWidth) / 2;
                y = iBaseline;
                RenderNode_Draw(pBase, pConfig, x, y);
            }
            /* Under */
            if (pUnder) {
                x = iStartX + (iWidth - pUnder->sLayout.iWidth) / 2;
                y = iBaseline + pConfig->sOverunder.iPaddingY + pBase->sLayout.iDescent + pUnder->sLayout.iAscent;
                RenderNode_Draw(pUnder, pConfig, x, y);
            }
        }
        case RN_BIG_SYMBOL: {
            const GlyphPoint* pts;
            int i;
            switch (pNode->uData.sBigSymbol.iType) {
                case ST_SUM:
                    pts = pConfig->sBigSymbol.sSum.sPoints;
                    for (i = 0; i < 6; ++i) {
                        pConfig->sInterfaces.plotLine(
                            iStartX + pts[i].x,
                            iBaseline + pts[i].y,
                            iStartX + pts[i + 1].x,
                            iBaseline + pts[i + 1].y
                        );
                    }
                    break;
                case ST_PRD:
                    pts = pConfig->sBigSymbol.sPrd.sPoints;
                    for (i = 0; i < 6; i += 2) {
                        pConfig->sInterfaces.plotLine(
                            iStartX + pts[i].x,
                            iBaseline + pts[i].y,
                            iStartX + pts[i + 1].x,
                            iBaseline + pts[i + 1].y
                        );
                    }
                    break;
                case ST_INT: {
                    pts = pConfig->sBigSymbol.sInt.sPoints;
                    for (i = 0; i < 5; ++i) {
                        pConfig->sInterfaces.plotLine(
                            iStartX + pts[i].x,
                            iBaseline + pts[i].y,
                            iStartX + pts[i + 1].x,
                            iBaseline + pts[i + 1].y
                        );
                    }
                }
            }
            break;
        }
    }
}

static int maxOfThree(int a, int b, int c) {
    int max = a;
    if (b > max) max = b;
    if (c > max) max = c;
    return max;
}
