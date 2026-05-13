#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rz.h"

static RenderNode* createRenderNode(RenderNodeType iType) {
    RenderNode* pNode = (RenderNode *)malloc(sizeof(RenderNode));

    pNode->iType = iType;
    pNode->sPixel.iWidth = 0;
    pNode->sPixel.iHeight = 0;

    switch (iType) {
        case RN_TEXT:
            pNode->uData.sText.szText = NULL;
            break;
        case RN_COMMA:
            break;
        case RN_HORIZONTAL:
            pNode->uData.sHorizontal.pList = vlNewList();  
            break;
        case RN_ENCLOSURE:
            pNode->uData.sEnclosure.pContent = NULL;
            break;
        case RN_STACK:
            pNode->uData.sStack.pTop = NULL;
            pNode->uData.sStack.pBottom = NULL;
            break;
        case RN_ROOT:
            pNode->uData.sRoot.pContent = NULL;
            break;
        case RN_SUPERSCRIPT:
            pNode->uData.sSuperscript.pBody = NULL;
            pNode->uData.sSuperscript.pScript = NULL;
            break;
    }

    return pNode;
}

static void destroyRenderNodeVoidPtr(void* ptr) {
    RenderNode_Destroy((RenderNode *)ptr);
}

static void cleanUpRenderNode(RenderNode* pNode) {
    if (pNode == NULL) {
        return;
    }
    switch (pNode->iType) {
        case RN_TEXT:
            if (pNode->uData.sText.szText) {
                free(pNode->uData.sText.szText);
            }
            break;
        case RN_COMMA:
            break;
        case RN_HORIZONTAL:
            if (pNode->uData.sHorizontal.pList) {
                vlDestroy(pNode->uData.sHorizontal.pList, destroyRenderNodeVoidPtr);
            }
            break;
        case RN_ENCLOSURE:
            RenderNode_Destroy(pNode->uData.sEnclosure.pContent);
            break;
        case RN_STACK:
            RenderNode_Destroy(pNode->uData.sStack.pTop);
            RenderNode_Destroy(pNode->uData.sStack.pBottom);
            break;
        case RN_ROOT:
            RenderNode_Destroy(pNode->uData.sRoot.pContent);
            break;
        case RN_SUPERSCRIPT:
            RenderNode_Destroy(pNode->uData.sSuperscript.pBody);
            RenderNode_Destroy(pNode->uData.sSuperscript.pScript);
            break;
    }
}

void RenderNode_Destroy(RenderNode* pNode) {
    if (pNode == NULL) {
        return;
    }
    cleanUpRenderNode(pNode);
    free(pNode);
}

static void transform(RenderNode* pParentHorz, FzAstNode* pAstNode) {
    switch (pAstNode->iType){
        case AST_UNARY_OPERATOR: {
            RenderNode* pTextOperator = createRenderNode(RN_TEXT);
            pTextOperator->uData.sText.szText = Utils_StringDump(FzOperator_GetSymbolById(pAstNode->uData.sUnaryOperator.iOperatorId));
            vlPushBack(pParentHorz->uData.sHorizontal.pList, pTextOperator);
            transform(pParentHorz, pAstNode->uData.sUnaryOperator.pAstOperand);
            break;
        }
        case AST_BINARY_OPERATOR: {
            FzOperatorId iOprId = pAstNode->uData.sBinaryOperator.iOperatorId;
            if (iOprId == OPR_DIV) {
                RenderNode* pStackNode = createRenderNode(RN_STACK);
                RenderNode* pTop = createRenderNode(RN_HORIZONTAL);
                RenderNode* pBottom = createRenderNode(RN_HORIZONTAL);
                FzAstNode* pAstLeft = pAstNode->uData.sBinaryOperator.pAstLeftOperand;
                FzAstNode* pAstRight = pAstNode->uData.sBinaryOperator.pAstRightOperand;

                pStackNode->uData.sStack.pTop = pTop;
                pStackNode->uData.sStack.pBottom = pBottom;

                if (pAstLeft->iType == AST_PAREN) pAstLeft = pAstLeft->uData.sParen.pAstExpr;
                if (pAstRight->iType == AST_PAREN) pAstRight = pAstRight->uData.sParen.pAstExpr;

                transform(pTop, pAstLeft);
                transform(pBottom, pAstRight);

                vlPushBack(pParentHorz->uData.sHorizontal.pList, pStackNode);
            }
            else if (iOprId == OPR_POW) {
                RenderNode* pSuperscriptNode = createRenderNode(RN_SUPERSCRIPT);
                RenderNode* pBody = createRenderNode(RN_HORIZONTAL);
                RenderNode* pScript = createRenderNode(RN_HORIZONTAL);
                FzAstNode* pAstLeft = pAstNode->uData.sBinaryOperator.pAstLeftOperand;
                FzAstNode* pAstRight = pAstNode->uData.sBinaryOperator.pAstRightOperand;

                pSuperscriptNode->uData.sSuperscript.pBody = pBody;
                pSuperscriptNode->uData.sSuperscript.pScript = pScript;

                transform(pBody, pAstLeft);
                transform(pScript, pAstRight);

                vlPushBack(pParentHorz->uData.sHorizontal.pList, pSuperscriptNode);
            }
            else {
                RenderNode* pTextOperator = createRenderNode(RN_TEXT);
                pTextOperator->uData.sText.szText = Utils_StringDump(FzOperator_GetSymbolById(iOprId));
                transform(pParentHorz, pAstNode->uData.sBinaryOperator.pAstLeftOperand);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pTextOperator);
                transform(pParentHorz, pAstNode->uData.sBinaryOperator.pAstRightOperand);
            }
            break;
        }
        case AST_PAREN: {
            RenderNode* pEnclosureNode = createRenderNode(RN_ENCLOSURE);
            RenderNode* pChildHorz = createRenderNode(RN_HORIZONTAL);
            pEnclosureNode->uData.sEnclosure.pContent = pChildHorz;
            transform(pChildHorz, pAstNode->uData.sParen.pAstExpr);
            vlPushBack(pParentHorz->uData.sHorizontal.pList, pEnclosureNode);
            break;
        }
        case AST_LITERAL_NUMERIC: {
            RenderNode* pText = createRenderNode(RN_TEXT);
            pText->uData.sText.szText = Utils_StringDump(pAstNode->uData.sLiteralNumeric.szNumber);
            vlPushBack(pParentHorz->uData.sHorizontal.pList, pText);
            break;
        }
        case AST_VARIABLE: {
            RenderNode* pText = createRenderNode(RN_TEXT);
            pText->uData.sText.szText = Utils_StringDump(pAstNode->uData.sVariable.szName);
            vlPushBack(pParentHorz->uData.sHorizontal.pList, pText);
            break;
        }
        case AST_FUNCTION_CALL: {
            const char* szFuncName = pAstNode->uData.sFunctionCall.szFunction;
            int iNumArg = pAstNode->uData.sFunctionCall.pListArguments->iSize;
            if (Utils_IsStringEqual(szFuncName, "sqr") && iNumArg == 1) {
                RenderNode* pRoot = createRenderNode(RN_ROOT);
                RenderNode* pChildHorz = createRenderNode(RN_HORIZONTAL);
                FzAstNode* pAstContent = (FzAstNode *)pAstNode->uData.sFunctionCall.pListArguments->pHead->pData;
                
                pRoot->uData.sRoot.pContent = pChildHorz;
                transform(pChildHorz, pAstContent);

                vlPushBack(pParentHorz->uData.sHorizontal.pList, pRoot);
            }
            else {
                VlistNode* pParam = NULL;
                RenderNode* pFuncName = createRenderNode(RN_TEXT);
                RenderNode* pEnclosureNode = createRenderNode(RN_ENCLOSURE);
                RenderNode* pChildHorz = createRenderNode(RN_HORIZONTAL);

                pFuncName->uData.sText.szText = Utils_StringDump(pAstNode->uData.sFunctionCall.szFunction);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pFuncName);

                pEnclosureNode->uData.sEnclosure.pContent = pChildHorz;
                for (
                    pParam = pAstNode->uData.sFunctionCall.pListArguments->pHead;
                    pParam != NULL;
                    pParam = pParam->pNext
                ) {
                    transform(pChildHorz, (FzAstNode *)pParam->pData);
                    if (pParam->pNext != NULL) { /* Not last parameter */
                        vlPushBack(pChildHorz->uData.sHorizontal.pList, createRenderNode(RN_COMMA));
                    }
                }
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pEnclosureNode);
            }
            break;
        }
        default:
            break;
    }
}

static void reduceHorizontal(RenderNode* pNode) {
    switch (pNode->iType) {
        case RN_HORIZONTAL:
            /* Only one child, reduce */
            if (pNode->uData.sHorizontal.pList->iSize == 1) {
                RenderNode* pChild = (RenderNode *)pNode->uData.sHorizontal.pList->pHead->pData;
                reduceHorizontal(pChild);
                pNode->uData.sHorizontal.pList->pHead->pData = NULL;
                cleanUpRenderNode(pNode);
                memcpy(pNode, pChild, sizeof(RenderNode));
                free(pChild);
            }
            else {
                VlistNode* pListNode;
                for (
                    pListNode = pNode->uData.sHorizontal.pList->pHead;
                    pListNode != NULL;
                    pListNode = pListNode->pNext
                ) {
                    reduceHorizontal((RenderNode *)pListNode->pData);
                }
            }
            break;
        case RN_ENCLOSURE:
            reduceHorizontal(pNode->uData.sEnclosure.pContent);
            break;
        case RN_STACK:
            reduceHorizontal(pNode->uData.sStack.pTop);
            reduceHorizontal(pNode->uData.sStack.pBottom);
            break;
        case RN_ROOT:
            reduceHorizontal(pNode->uData.sRoot.pContent);
            break;
        case RN_SUPERSCRIPT:
            reduceHorizontal(pNode->uData.sSuperscript.pBody);
            reduceHorizontal(pNode->uData.sSuperscript.pScript);
            break;
        default:
            break;
    }
}

RenderNode* Render_Transform(FzAstNode* pAstNode) {
    RenderNode* pHorz = createRenderNode(RN_HORIZONTAL);
    transform(pHorz, pAstNode);
    reduceHorizontal(pHorz);
    return pHorz;
}