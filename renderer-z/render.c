#include <stdlib.h>
#include <string.h>
#include "rz.h"

void RenderConfig_GetDefaultStyle(RenderConfig* pConfig) {
    pConfig->sText.iPaddingLeft     = 1;
    pConfig->sFont.iWidth           = 6;
    pConfig->sFont.iHeight          = 8;
    pConfig->sStack.iSpacingX       = 2;
    pConfig->sStack.iSpacingY       = 2;
    pConfig->sRoot.iSpacingLeft     = 4;
    pConfig->sRoot.iSpacingTop      = 2;
    pConfig->sEnclosure.iPadding    = 2;
    pConfig->sEnclosure.iMargin     = 1;
    pConfig->sEnclosure.iRadius     = 1;
    pConfig->sDebug.bOutline        = 0;
}

void RenderNode_EstimateSize(RenderNode* pNode, const RenderConfig* pConfig) {
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
                RenderNode_EstimateSize(pChild, pConfig);
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
            RenderNode_EstimateSize(pContent, pConfig);
            pNode->sLayout.iWidth = pContent->sLayout.iWidth + 2 * pConfig->sEnclosure.iPadding + 2 * pConfig->sEnclosure.iMargin;
            pNode->sLayout.iAscent = pContent->sLayout.iAscent;
            pNode->sLayout.iDescent = pContent->sLayout.iDescent;
            break;
        }
        case RN_STACK: {
            RenderNode* pTop = pNode->uData.sStack.pTop;
            RenderNode* pBottom = pNode->uData.sStack.pBottom;
            RenderNode_EstimateSize(pTop, pConfig);
            RenderNode_EstimateSize(pBottom, pConfig);
            pNode->sLayout.iWidth = pConfig->sStack.iSpacingY * 2 + (pTop->sLayout.iWidth > pBottom->sLayout.iWidth ? pTop->sLayout.iWidth : pBottom->sLayout.iWidth);
            pNode->sLayout.iAscent = pTop->sLayout.iAscent + pTop->sLayout.iDescent + pConfig->sStack.iSpacingY;
            pNode->sLayout.iDescent = pBottom->sLayout.iAscent + pBottom->sLayout.iDescent + pConfig->sStack.iSpacingY;
            break;
        }
        case RN_ROOT: {
            RenderNode* pContent = pNode->uData.sRoot.pContent;
            RenderNode_EstimateSize(pContent, pConfig);
            pNode->sLayout.iWidth = pContent->sLayout.iWidth + pConfig->sRoot.iSpacingLeft;
            pNode->sLayout.iAscent = pContent->sLayout.iAscent + pConfig->sRoot.iSpacingTop;
            pNode->sLayout.iDescent = pContent->sLayout.iDescent;
            break;
        }
        case RN_SUPERSCRIPT: {
            RenderNode* pBody = pNode->uData.sSuperscript.pBody;
            RenderNode* pScript = pNode->uData.sSuperscript.pScript;
            RenderNode_EstimateSize(pBody, pConfig);
            RenderNode_EstimateSize(pScript, pConfig);
            pNode->sLayout.iWidth = pBody->sLayout.iWidth + pScript->sLayout.iWidth;
            pNode->sLayout.iAscent = pBody->sLayout.iAscent + (pScript->sLayout.iAscent + pScript->sLayout.iDescent - pConfig->sFont.iHeight / 2);
            pNode->sLayout.iDescent = pBody->sLayout.iDescent;
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
            int t = pNode->sLayout.iAscent;
            int b = pNode->sLayout.iDescent;
            int r = pConfig->sEnclosure.iRadius;

            RenderNode* pContent = pNode->uData.sEnclosure.pContent;

            x = iStartX + (pNode->sLayout.iWidth - pContent->sLayout.iWidth) / 2;
            y = iBaseline;
            
            RenderNode_Draw(pContent, pConfig, x, y);

            x = iStartX + pConfig->sEnclosure.iMargin;
            w = pNode->sLayout.iWidth - pConfig->sEnclosure.iMargin * 2 - 1;

            pConfig->sInterfaces.plotLine(
                x, y - t + r,
                x, y + b - r
            );
            pConfig->sInterfaces.plotLine(
                x, y - t + r,
                x + r, y - t
            );
            pConfig->sInterfaces.plotLine(
                x, y + b - r,
                x + r, y + b
            );
            pConfig->sInterfaces.plotLine(
                x + w, y - t + r,
                x + w, y + b - r
            );
            pConfig->sInterfaces.plotLine(
                x + w, y - t + r,
                x + w - r, y - t
            );
            pConfig->sInterfaces.plotLine(
                x + w, y + b - r,
                x + w - r, y + b
            );

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
    }
}