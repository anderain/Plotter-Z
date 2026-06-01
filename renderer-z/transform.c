#include <stdlib.h>
#include <string.h>
#include "rz.h"
#include "ascii_extended_mapping.h"

static const struct {
    char* szToken;
    unsigned char ucSpecial;
} SpecialCharMapping[] = {
    { "pi",         PZ_AE_GREEK_pi      },
    { "const_i",    PZ_AE_IMAG_I        },
    { "inf",        PZ_AE_INFINITY      },
    { "dots",       PZ_AE_ELLIPSIS      },
    { "alpha",      PZ_AE_GREEK_alpha   }, 
    { "beta",       PZ_AE_GREEK_beta    }, 
    { "theta",      PZ_AE_GREEK_theta   }, 
    { "lambda",     PZ_AE_GREEK_lambda  }, 
    { "delta",      PZ_AE_GREEK_delta   },
    { "Delta",      PZ_AE_GREEK_DELTA   }, 
    { "omega",      PZ_AE_GREEK_omega   },
    { "Omega",      PZ_AE_GREEK_OMEGA   },
    { NULL,         0x0                 }
};

static RenderNode* createRenderNode(RenderNodeType iType) {
    RenderNode* pNode = (RenderNode *)malloc(sizeof(RenderNode));

    pNode->iType = iType;
    pNode->sLayout.iWidth = 0;
    pNode->sLayout.iAscent = 0;
    pNode->sLayout.iDescent = 0;

    switch (iType) {
        case RN_TEXT:
            pNode->uData.sText.szText = NULL;
            break;
        case RN_SPECIAL_CHAR:
            break;
        case RN_HORIZONTAL:
            pNode->uData.sHorizontal.pList = vlNewList();  
            break;
        case RN_ENCLOSURE:
            pNode->uData.sEnclosure.bCurve = 1;
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
        case RN_OVERUNDER:
            pNode->uData.sOverunder.pOver = NULL;
            pNode->uData.sOverunder.pBase = NULL;
            pNode->uData.sOverunder.pUnder = NULL;
            break;
        case RN_BIG_SYMBOL:
            pNode->uData.sBigSymbol.iType = ST_NONE;
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
        case RN_SPECIAL_CHAR:
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
        case RN_OVERUNDER:
            RenderNode_Destroy(pNode->uData.sOverunder.pOver);
            RenderNode_Destroy(pNode->uData.sOverunder.pBase);
            RenderNode_Destroy(pNode->uData.sOverunder.pUnder);
            break;
        case RN_BIG_SYMBOL:
            pNode->uData.sBigSymbol.iType = ST_NONE;
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

static void transform(RenderNode* pParentHorz, const FzAstNode* pAstNode) {
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
            else if (iOprId == OPR_MUL) {
                RenderNode* pMul = createRenderNode(RN_SPECIAL_CHAR);
                pMul->uData.sSpecialChar.c = PZ_AE_MIDDLE_DOT;
                transform(pParentHorz, pAstNode->uData.sBinaryOperator.pAstLeftOperand);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pMul);
                transform(pParentHorz, pAstNode->uData.sBinaryOperator.pAstRightOperand);
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
            pText->uData.sText.szText = Utils_StringViewDump(&pAstNode->uData.sLiteralNumeric.svNumber);
            vlPushBack(pParentHorz->uData.sHorizontal.pList, pText);
            break;
        }
        case AST_VARIABLE: {
            unsigned char ucSpecialChar = 0;
            int i;
            for (i = 0; SpecialCharMapping[i].szToken != NULL; ++i) {
                if (Utils_StringViewEqual(&pAstNode->uData.sVariable.svName, SpecialCharMapping[i].szToken)) {
                    ucSpecialChar = SpecialCharMapping[i].ucSpecial;
                }
            }
            if (ucSpecialChar > 0) {
                RenderNode* pSpecial = createRenderNode(RN_SPECIAL_CHAR);
                pSpecial->uData.sSpecialChar.c = ucSpecialChar;
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pSpecial);
            }
            else {
                RenderNode* pText = createRenderNode(RN_TEXT);
                pText->uData.sText.szText = Utils_StringViewDump(&pAstNode->uData.sVariable.svName);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pText);
            }
            break;
        }
        case AST_FUNCTION_CALL: {
            const StringView* psvFuncName = &pAstNode->uData.sFunctionCall.svFunction;
            int iNumArg = pAstNode->uData.sFunctionCall.pListArguments->iSize;
            if (Utils_StringViewEqual(psvFuncName, "sqr") && iNumArg == 1) {
                RenderNode* pRoot = createRenderNode(RN_ROOT);
                RenderNode* pChildHorz = createRenderNode(RN_HORIZONTAL);
                const FzAstNode* pAstContent = (const FzAstNode *)pAstNode->uData.sFunctionCall.pListArguments->pHead->pData;
                
                pRoot->uData.sRoot.pContent = pChildHorz;
                transform(pChildHorz, pAstContent);

                vlPushBack(pParentHorz->uData.sHorizontal.pList, pRoot);
            }
            else if (Utils_StringViewEqual(psvFuncName, "exp") && iNumArg == 1) {
                RenderNode* pSuperscriptNode = createRenderNode(RN_SUPERSCRIPT);
                RenderNode* pBody = createRenderNode(RN_SPECIAL_CHAR);
                RenderNode* pScript = createRenderNode(RN_HORIZONTAL);
                const FzAstNode* pAstContent = (const FzAstNode *)pAstNode->uData.sFunctionCall.pListArguments->pHead->pData;

                pSuperscriptNode->uData.sSuperscript.pBody = pBody;
                pSuperscriptNode->uData.sSuperscript.pScript = pScript;

                pBody->uData.sSpecialChar.c = PZ_AE_EXP_E;
                transform(pScript, pAstContent);

                vlPushBack(pParentHorz->uData.sHorizontal.pList, pSuperscriptNode);
            }
            else if (Utils_StringViewEqual(psvFuncName, "abs") && iNumArg == 1) {
                RenderNode* pEnclosureNode = createRenderNode(RN_ENCLOSURE);
                RenderNode* pChildHorz = createRenderNode(RN_HORIZONTAL);
                const FzAstNode* pAstContent = (const FzAstNode *)pAstNode->uData.sFunctionCall.pListArguments->pHead->pData;

                pEnclosureNode->uData.sEnclosure.pContent = pChildHorz;
                transform(pChildHorz, pAstContent);

                pEnclosureNode->uData.sEnclosure.bCurve = 0;
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pEnclosureNode);
            }
            else if ((
                Utils_StringViewEqual(psvFuncName, "sum") ||
                Utils_StringViewEqual(psvFuncName, "prd")
            ) && iNumArg == 4) {
                RenderNode* pOverunder = createRenderNode(RN_OVERUNDER);
                RenderNode* pOver = createRenderNode(RN_HORIZONTAL);
                RenderNode* pSymbol = createRenderNode(RN_BIG_SYMBOL);
                RenderNode* pEqual = createRenderNode(RN_SPECIAL_CHAR);
                RenderNode* pUnder = createRenderNode(RN_HORIZONTAL);
                RenderNode* pRemaining = createRenderNode(RN_ENCLOSURE);
                VlistNode* pArg1st = pAstNode->uData.sFunctionCall.pListArguments->pHead;
                VlistNode* pArg2nd = pArg1st->pNext;
                VlistNode* pArg3rd = pArg2nd->pNext;
                VlistNode* pArg4th = pArg3rd->pNext;

                switch (psvFuncName->pBegin[0]) {
                    case 's': pSymbol->uData.sBigSymbol.iType = ST_SUM; break;
                    case 'p': pSymbol->uData.sBigSymbol.iType = ST_PRD; break;
                    default: pSymbol->uData.sBigSymbol.iType = ST_NONE;
                }

                /* Under part: [var] [=] [expr] */
                transform(pUnder, (const FzAstNode *)pArg1st->pData);
                pEqual->uData.sSpecialChar.c = '=';
                vlPushBack(pUnder->uData.sHorizontal.pList, pEqual);
                transform(pUnder, (const FzAstNode *)pArg2nd->pData);

                /* Over part: [expr] */
                transform(pOver, (const FzAstNode *)pArg3rd->pData);

                pOverunder->uData.sOverunder.pBase = pSymbol;
                pOverunder->uData.sOverunder.pOver = pOver;
                pOverunder->uData.sOverunder.pUnder = pUnder;

                vlPushBack(pParentHorz->uData.sHorizontal.pList, pOverunder);

                /* 4th argument as remaining */
                pRemaining->uData.sEnclosure.pContent = createRenderNode(RN_HORIZONTAL);
                transform(pRemaining->uData.sEnclosure.pContent, (const FzAstNode *)pArg4th->pData);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pRemaining);
            }
            else if (Utils_StringViewEqual(psvFuncName, "int") && iNumArg == 4) {
                RenderNode* pOverunder = createRenderNode(RN_OVERUNDER);
                RenderNode* pOver = createRenderNode(RN_HORIZONTAL);
                RenderNode* pSymbol = createRenderNode(RN_BIG_SYMBOL);
                RenderNode* pUnder = createRenderNode(RN_HORIZONTAL);
                RenderNode* pRemaining = createRenderNode(RN_HORIZONTAL);
                RenderNode* pAppended = createRenderNode(RN_HORIZONTAL);
                RenderNode* pCharD = createRenderNode(RN_SPECIAL_CHAR);

                VlistNode* pArg1st = pAstNode->uData.sFunctionCall.pListArguments->pHead;
                VlistNode* pArg2nd = pArg1st->pNext;
                VlistNode* pArg3rd = pArg2nd->pNext;
                VlistNode* pArg4th = pArg3rd->pNext;

                pSymbol->uData.sBigSymbol.iType = ST_INT;
   
                /* Under part */
                transform(pUnder, (const FzAstNode *)pArg2nd->pData);

                /* Over part: [expr] */
                transform(pOver, (const FzAstNode *)pArg3rd->pData);

                pOverunder->uData.sOverunder.pBase = pSymbol;
                pOverunder->uData.sOverunder.pOver = pOver;
                pOverunder->uData.sOverunder.pUnder = pUnder;

                vlPushBack(pParentHorz->uData.sHorizontal.pList, pOverunder);

                /* 4th argument as remaining */
                transform(pRemaining, (const FzAstNode *)pArg4th->pData);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pRemaining);

                /* [d][var] */
                pCharD->uData.sSpecialChar.c = 'd';
                vlPushBack(pAppended->uData.sHorizontal.pList, pCharD);
                transform(pAppended, (const FzAstNode *)pArg1st->pData);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pAppended);
            }
            else if (Utils_StringViewEqual(psvFuncName, "int") && iNumArg == 2) {
                RenderNode* pOverunder = createRenderNode(RN_OVERUNDER);
                RenderNode* pSymbol = createRenderNode(RN_BIG_SYMBOL);
                RenderNode* pRemaining = createRenderNode(RN_HORIZONTAL);
                RenderNode* pAppended = createRenderNode(RN_HORIZONTAL);
                RenderNode* pCharD = createRenderNode(RN_SPECIAL_CHAR);

                VlistNode* pArg1st = pAstNode->uData.sFunctionCall.pListArguments->pHead;
                VlistNode* pArg2nd = pArg1st->pNext;

                pSymbol->uData.sBigSymbol.iType = ST_INT;

                pOverunder->uData.sOverunder.pBase = pSymbol;
                pOverunder->uData.sOverunder.pOver = NULL;
                pOverunder->uData.sOverunder.pUnder = NULL;

                vlPushBack(pParentHorz->uData.sHorizontal.pList, pOverunder);

                /* 4th argument as remaining */
                transform(pRemaining, (const FzAstNode *)pArg2nd->pData);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pRemaining);

                /* [d][var] */
                pCharD->uData.sSpecialChar.c = 'd';
                vlPushBack(pAppended->uData.sHorizontal.pList, pCharD);
                transform(pAppended, (const FzAstNode *)pArg1st->pData);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pAppended);
            }
            else if (Utils_StringViewEqual(psvFuncName, "lim") && iNumArg == 3) {
                RenderNode* pOverunder = createRenderNode(RN_OVERUNDER);
                RenderNode* pLimitText = createRenderNode(RN_TEXT);
                RenderNode* pArrow = createRenderNode(RN_SPECIAL_CHAR);
                RenderNode* pUnder = createRenderNode(RN_HORIZONTAL);
                RenderNode* pRemaining = createRenderNode(RN_ENCLOSURE);

                VlistNode* pArg1st = pAstNode->uData.sFunctionCall.pListArguments->pHead;
                VlistNode* pArg2nd = pArg1st->pNext;
                VlistNode* pArg3rd = pArg2nd->pNext;
                
                pLimitText->uData.sText.szText = Utils_StringViewDump(psvFuncName);
   
                /* Under part: [var] [->] [expr] */
                transform(pUnder, (const FzAstNode *)pArg1st->pData);
                pArrow->uData.sSpecialChar.c = PZ_AE_ARROW_RIGHT;
                vlPushBack(pUnder->uData.sHorizontal.pList, pArrow);
                transform(pUnder, (const FzAstNode *)pArg2nd->pData);

                pOverunder->uData.sOverunder.pBase = pLimitText;
                pOverunder->uData.sOverunder.pOver = NULL;
                pOverunder->uData.sOverunder.pUnder = pUnder;

                vlPushBack(pParentHorz->uData.sHorizontal.pList, pOverunder);

                /* 4th argument as remaining */
                pRemaining->uData.sEnclosure.pContent = createRenderNode(RN_HORIZONTAL);
                transform(pRemaining->uData.sEnclosure.pContent, (const FzAstNode *)pArg3rd->pData);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pRemaining);
            }
            else {
                VlistNode* pParam = NULL;
                RenderNode* pFuncName = createRenderNode(RN_TEXT);
                RenderNode* pEnclosureNode = createRenderNode(RN_ENCLOSURE);
                RenderNode* pChildHorz = createRenderNode(RN_HORIZONTAL);

                pFuncName->uData.sText.szText = Utils_StringViewDump(&pAstNode->uData.sFunctionCall.svFunction);
                vlPushBack(pParentHorz->uData.sHorizontal.pList, pFuncName);

                pEnclosureNode->uData.sEnclosure.pContent = pChildHorz;
                for (
                    pParam = pAstNode->uData.sFunctionCall.pListArguments->pHead;
                    pParam != NULL;
                    pParam = pParam->pNext
                ) {
                    transform(pChildHorz, (const FzAstNode *)pParam->pData);
                    if (pParam->pNext != NULL) { /* Not last parameter */
                        RenderNode* pSpecialCharNode = createRenderNode(RN_SPECIAL_CHAR);
                        pSpecialCharNode->uData.sSpecialChar.c = ',';
                        vlPushBack(pChildHorz->uData.sHorizontal.pList, pSpecialCharNode);
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
        case RN_HORIZONTAL: {
            VlistNode* pListNode;
            for (
                pListNode = pNode->uData.sHorizontal.pList->pHead;
                pListNode != NULL;
                pListNode = pListNode->pNext
            ) {
                reduceHorizontal((RenderNode *)pListNode->pData);
            }
            /* Only one child, reduce */
            if (pNode->uData.sHorizontal.pList->iSize == 1) {
                RenderNode* pOnlyChild = (RenderNode *)vlPopBack(pNode->uData.sHorizontal.pList);
                cleanUpRenderNode(pNode);
                memcpy(pNode, pOnlyChild, sizeof(RenderNode));
                free(pOnlyChild);
            }
            break;
        }
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

RenderNode* Render_Transform(const FzAstNode* pAstNode) {
    RenderNode* pHorz = createRenderNode(RN_HORIZONTAL);
    transform(pHorz, pAstNode);
    reduceHorizontal(pHorz);
    return pHorz;
}