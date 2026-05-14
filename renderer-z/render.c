#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rz.h"

void RenderConfig_GetDefaultStyle(RenderConfig* pConfig) {
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
            pNode->sSize.iWidth = pConfig->sFont.iWidth * strlen(pNode->uData.sText.szText);
            pNode->sSize.iTop = pConfig->sFont.iHeight / 2;
            pNode->sSize.iBottom = pConfig->sFont.iHeight / 2;
            break;
        case RN_SPECIAL_CHAR:
            pNode->sSize.iWidth = pConfig->sFont.iWidth;
            pNode->sSize.iTop = pConfig->sFont.iHeight / 2;
            pNode->sSize.iBottom = pConfig->sFont.iHeight / 2;
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
                if (pChild->sSize.iTop > iTopMax) iTopMax = pChild->sSize.iTop;
                if (pChild->sSize.iBottom > iBottomMax) iBottomMax = pChild->sSize.iBottom;
                iSumWidth += pChild->sSize.iWidth;
            }
            pNode->sSize.iWidth = iSumWidth;
            pNode->sSize.iTop = iTopMax;
            pNode->sSize.iBottom = iBottomMax;
            break;
        }
        case RN_ENCLOSURE: {
            RenderNode* pContent = pNode->uData.sEnclosure.pContent;
            RenderNode_EstimateSize(pContent, pConfig);
            pNode->sSize.iWidth = pContent->sSize.iWidth + 2 * pConfig->sEnclosure.iPadding + 2 * pConfig->sEnclosure.iMargin;
            pNode->sSize.iTop = pContent->sSize.iTop;
            pNode->sSize.iBottom = pContent->sSize.iBottom;
            break;
        }
        case RN_STACK: {
            RenderNode* pTop = pNode->uData.sStack.pTop;
            RenderNode* pBottom = pNode->uData.sStack.pBottom;
            RenderNode_EstimateSize(pTop, pConfig);
            RenderNode_EstimateSize(pBottom, pConfig);
            pNode->sSize.iWidth = pConfig->sStack.iSpacingY * 2 + (pTop->sSize.iWidth > pBottom->sSize.iWidth ? pTop->sSize.iWidth : pBottom->sSize.iWidth);
            pNode->sSize.iTop = pTop->sSize.iTop + pTop->sSize.iBottom + pConfig->sStack.iSpacingY;
            pNode->sSize.iBottom = pBottom->sSize.iTop + pBottom->sSize.iBottom + pConfig->sStack.iSpacingY;
            break;
        }
        case RN_ROOT: {
            RenderNode* pContent = pNode->uData.sRoot.pContent;
            RenderNode_EstimateSize(pContent, pConfig);
            pNode->sSize.iWidth = pContent->sSize.iWidth + pConfig->sRoot.iSpacingLeft;
            pNode->sSize.iTop = pContent->sSize.iTop + pConfig->sRoot.iSpacingTop;
            pNode->sSize.iBottom = pContent->sSize.iBottom;
            break;
        }
        case RN_SUPERSCRIPT: {
            RenderNode* pBody = pNode->uData.sSuperscript.pBody;
            RenderNode* pScript = pNode->uData.sSuperscript.pScript;
            RenderNode_EstimateSize(pBody, pConfig);
            RenderNode_EstimateSize(pScript, pConfig);
            pNode->sSize.iWidth = pBody->sSize.iWidth + pScript->sSize.iWidth;
            pNode->sSize.iTop = pBody->sSize.iTop + (pScript->sSize.iTop + pScript->sSize.iBottom - pConfig->sFont.iHeight / 2);
            pNode->sSize.iBottom = pBody->sSize.iBottom;
            break;
        }
    }
}

void RenderNode_Draw(RenderNode* pNode, const RenderConfig* pConfig, int iStartX, int iCenterY) {
    if (pNode->iType & pConfig->sDebug.bOutline) {
        pConfig->sInterfaces.plotLine(
            iStartX, iCenterY - pNode->sSize.iTop,
            iStartX + pNode->sSize.iWidth, iCenterY - pNode->sSize.iTop
        );
        pConfig->sInterfaces.plotLine(
            iStartX + pNode->sSize.iWidth, iCenterY - pNode->sSize.iTop,
            iStartX + pNode->sSize.iWidth, iCenterY + pNode->sSize.iBottom
        );
        pConfig->sInterfaces.plotLine(
            iStartX + pNode->sSize.iWidth, iCenterY + pNode->sSize.iBottom,
            iStartX, iCenterY + pNode->sSize.iBottom
        );
        pConfig->sInterfaces.plotLine(
            iStartX, iCenterY + pNode->sSize.iBottom,
            iStartX, iCenterY - pNode->sSize.iTop
        );
    }
    switch (pNode->iType) {
        case RN_TEXT: {
            int i;
            int x = iStartX;
            int y = iCenterY - pNode->sSize.iTop + pConfig->sFont.iHeight / 8;
            const char* szText = pNode->uData.sText.szText;
            for (i = 0; szText[i]; ++i) {
                pConfig->sInterfaces.putChar(x, y, (unsigned char)szText[i]);
                x += pConfig->sFont.iWidth;
            }
            break;
        }
        case RN_SPECIAL_CHAR: {
            unsigned char c = pNode->uData.sSpecialChar.c;
            if (c == '*') {
                /*
                static const int iCrossSize = 2;
                int x = iStartX + pConfig->sFont.iWidth / 2 - iCrossSize / 2;
                int y = iCenterY - pNode->sSize.iTop + pConfig->sFont.iHeight / 2;
                pConfig->sInterfaces.plotLine(x - iCrossSize, y - iCrossSize, x + iCrossSize, y + iCrossSize);
                pConfig->sInterfaces.plotLine(x + iCrossSize, y - iCrossSize, x - iCrossSize, y + iCrossSize);
                */
                int x = iStartX + pConfig->sFont.iWidth / 2 - 1;
                int y = iCenterY - pNode->sSize.iTop + pConfig->sFont.iHeight / 2;
                pConfig->sInterfaces.setPixel(x, y);
            }
            else if (c == 'e') {
                int x = iStartX;
                int y = iCenterY - pNode->sSize.iTop + pConfig->sFont.iHeight / 8;
                pConfig->sInterfaces.putChar(x, y, c);
                pConfig->sInterfaces.putChar(x + 1, y, c);
            }
            else {
                int x = iStartX;
                int y = iCenterY - pNode->sSize.iTop + pConfig->sFont.iHeight / 8;
                pConfig->sInterfaces.putChar(x, y, c);
            }
            break;
        }
        case RN_HORIZONTAL: {
            int x = iStartX;
            int y = iCenterY;
            VlistNode* pListNode;
            for (
                pListNode = pNode->uData.sHorizontal.pList->pHead;
                pListNode != NULL;
                pListNode = pListNode->pNext
            ) {
                RenderNode* pChild = (RenderNode *)pListNode->pData;
                RenderNode_Draw(pChild, pConfig, x, y);
                x += pChild->sSize.iWidth;
            }
            break;
        }
        case RN_ENCLOSURE: {
            int x, y;
            int w;
            int t = pNode->sSize.iTop;
            int b = pNode->sSize.iBottom;
            int r = pConfig->sEnclosure.iRadius;

            RenderNode* pContent = pNode->uData.sEnclosure.pContent;

            x = iStartX + (pNode->sSize.iWidth - pContent->sSize.iWidth) / 2;
            y = iCenterY;
            
            RenderNode_Draw(pContent, pConfig, x, y);

            x = iStartX + pConfig->sEnclosure.iMargin;
            w = pNode->sSize.iWidth - pConfig->sEnclosure.iMargin * 2 - 1;

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

            x = iStartX + (pNode->sSize.iWidth - pTop->sSize.iWidth) / 2 ;
            y = iCenterY - pNode->sSize.iTop + pTop->sSize.iTop;
            RenderNode_Draw(pTop, pConfig, x, y);
            
            x = iStartX + (pNode->sSize.iWidth - pBottom->sSize.iWidth) / 2;
            y = iCenterY + pNode->sSize.iBottom - pBottom->sSize.iBottom;
            RenderNode_Draw(pBottom, pConfig, x, y);

            pConfig->sInterfaces.plotLine(iStartX, iCenterY, iStartX + pNode->sSize.iWidth - 2, iCenterY);
            break;
        }
        case RN_ROOT: {
            int x, y, w, h;
            int checkSize = 1;
            RenderNode* pContent = pNode->uData.sRoot.pContent;
            x = iStartX + pConfig->sRoot.iSpacingLeft;
            y = iCenterY;
            RenderNode_Draw(pContent, pConfig, x, y);

            w = pNode->sSize.iWidth;
            h = pNode->sSize.iBottom + pNode->sSize.iTop;

            x = iStartX;
            y = iCenterY - pNode->sSize.iTop;
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
            y = iCenterY;
            RenderNode_Draw(pBody, pConfig, x, y);
            
            x = iStartX + pBody->sSize.iWidth;
            y = iCenterY - pNode->sSize.iTop + pScript->sSize.iTop;
            RenderNode_Draw(pScript, pConfig, x, y);
            
            break;
        }
    }
}