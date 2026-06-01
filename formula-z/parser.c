#include <stdlib.h>
#include "fz.h"

#define tokenIs(szString)           (Utils_StringViewEqual(&pToken->svContent, (szString)))
#define tokenTypeIs(iTokenType)     (pToken->iType == (iTokenType))

const struct {
    const char*     szOperator;
    FzOperatorId    iOperatorId;
} BinaryOperatorSzIdMap[] = {
    { "+",  OPR_ADD         },
    { "-",  OPR_SUB         },
    { "*",  OPR_MUL         },
    { "/",  OPR_DIV         },
    { "^",  OPR_POW         },
    { "\\", OPR_INTDIV      },
    { "%",  OPR_MOD         },
    { "!",  OPR_NOT         },
    { "&&", OPR_AND         },
    { "||", OPR_OR          },
    { "=",  OPR_EQUAL       },
    { "<>", OPR_NEQ         },
    { ">",  OPR_GT          },
    { "<",  OPR_LT          },
    { ">=", OPR_GTEQ        },
    { "<=", OPR_LTEQ        }
};

static FzOperatorId getOperatorId(const StringView* pStrViewOpr) {
    static const int iSize = sizeof(BinaryOperatorSzIdMap) / sizeof(BinaryOperatorSzIdMap[0]);
    int i;
    for (i = 0; i < iSize; ++i) {
        if (Utils_StringViewEqual(pStrViewOpr, BinaryOperatorSzIdMap[i].szOperator)) {
            return BinaryOperatorSzIdMap[i].iOperatorId;
        }
    }
    return OPR_NONE;
}

const struct {
    char* szOperatorSymbol;
    char* szOperatorName;
    int   iPrecedence;
} OperatorIdMetaMap[] = {
    { "",   "NONE",         0   },
    { "-",  "NEG",          500 },
    { "+",  "ADD",          100 },
    { "-",  "SUB",          100 },
    { "*",  "MUL",          200 },
    { "/",  "DIV",          200 },
    { "^",  "POW",          550 },
    { "\\", "INTDIV",       150 },
    { "%",  "MOD",          200 },
    { "!",  "NOT",          450  },
    { "&&", "AND",          40  },
    { "||", "OR",           30  },
    { "=",  "EQUAL",        50  },
    { "<>", "NEQ",          50  },
    { ">",  "GT",           60  },
    { "<",  "LT",           60  },
    { ">=", "GTEQ",         60  },
    { "<=", "LTEQ",         60  }
};

int FzOperator_GetPrecedenceById(FzOperatorId iOprId) {
    return OperatorIdMetaMap[iOprId].iPrecedence;
}

const char* FzOperator_GetNameById(FzOperatorId iOprId) {
    return OperatorIdMetaMap[iOprId].szOperatorName;
}

const char* FzOperator_GetSymbolById(FzOperatorId iOprId) {
    return OperatorIdMetaMap[iOprId].szOperatorSymbol;
}

static FzAstNode* createAst(AstNodeType iAstType, FzAstNode* pAstParent) {
    FzAstNode* pAstNode = (FzAstNode *)malloc(sizeof(FzAstNode));
    
    pAstNode->pAstParent    = pAstParent;
    pAstNode->iType         = iAstType;
    pAstNode->iLineNumber   = -1;
    pAstNode->iControlId    = 0;

    switch (iAstType) {
        default:
        case AST_EMPTY:
            break;
        case AST_UNARY_OPERATOR:
            pAstNode->uData.sUnaryOperator.iOperatorId = OPR_NONE;
            pAstNode->uData.sUnaryOperator.pAstOperand = NULL;
            break;
        case AST_BINARY_OPERATOR:
            pAstNode->uData.sBinaryOperator.iOperatorId = OPR_NONE;
            pAstNode->uData.sBinaryOperator.pAstLeftOperand = NULL;
            pAstNode->uData.sBinaryOperator.pAstRightOperand = NULL;
            break;
        case AST_PAREN:
            pAstNode->uData.sParen.pAstExpr = NULL;
            break;
        case AST_VARIABLE:
            /* pAstNode->uData.sVariable.szName = NULL; */
            Utils_StringViewCopy(&pAstNode->uData.sVariable.svName, NULL);
            break;
        case AST_LITERAL_NUMERIC:
            /* pAstNode->uData.sLiteralNumeric.szNumber = NULL; */
            Utils_StringViewCopy(&pAstNode->uData.sLiteralNumeric.svNumber, NULL);
            break;
        case AST_FUNCTION_CALL:
            /* pAstNode->uData.sFunctionCall.szFunction = NULL; */
            Utils_StringViewCopy(&pAstNode->uData.sFunctionCall.svFunction, NULL);
            pAstNode->uData.sFunctionCall.pListArguments = vlNewList();
            break;
    }

    return pAstNode;
}

static void checkNullAndFreeString(char* pString) {
    if (!pString) return;
    free(pString);
}

static void destroyAstVoidPtr(void* pAstNodeVoidPtr);

void FzAstNode_Destroy(FzAstNode* pAstNode) {
    if (pAstNode == NULL) {
        return;
    }

    switch (pAstNode->iType) {
        default:
        case AST_EMPTY:
            break;
        case AST_UNARY_OPERATOR:
            FzAstNode_Destroy(pAstNode->uData.sUnaryOperator.pAstOperand);
            break;
        case AST_BINARY_OPERATOR:
            FzAstNode_Destroy(pAstNode->uData.sBinaryOperator.pAstLeftOperand);
            FzAstNode_Destroy(pAstNode->uData.sBinaryOperator.pAstRightOperand);
            break;
        case AST_PAREN:
            FzAstNode_Destroy(pAstNode->uData.sParen.pAstExpr);
            break;
        case AST_VARIABLE:
            break;
        case AST_LITERAL_NUMERIC:
            break;
        case AST_FUNCTION_CALL:
            vlDestroy(pAstNode->uData.sFunctionCall.pListArguments, destroyAstVoidPtr);
            break;
    }

    free(pAstNode);
}

static void destroyAstVoidPtr(void* pAstNodeVoidPtr) {
    FzAstNode_Destroy((FzAstNode *)pAstNodeVoidPtr);
}

static BOOL matchExpr(FzLineAnalyzer* pAnalyzer);

static BOOL matchTryBinaryOperator(FzLineAnalyzer *pAnalyzer) {
    FzToken* pToken = &pAnalyzer->token;
    FzAnalyzer_NextToken(pAnalyzer);
    if (tokenTypeIs(TOKEN_OPERATOR)) {
        /* Non-binary operator, failed */
        if (tokenIs("!")) {
            return FALSE;
        }
        return matchExpr(pAnalyzer);
    }
    FzAnalyzer_RewindToken(pAnalyzer);
    return TRUE;
}

static BOOL matchExpr(FzLineAnalyzer* pAnalyzer) {
    FzToken* pToken = &pAnalyzer->token;
    FzAnalyzer_NextToken(pAnalyzer);
    /* Literal value */
    if (tokenTypeIs(TOKEN_NUMERIC)) {
        return matchTryBinaryOperator(pAnalyzer);
    }
    /* Unary operators */
    else if (tokenTypeIs(TOKEN_OPERATOR) && (tokenIs("!") || tokenIs("-"))) {
        return matchExpr(pAnalyzer) && matchTryBinaryOperator(pAnalyzer);
    }
    /* Identifier */
    else if (tokenTypeIs(TOKEN_IDENTIFIER)) {
        FzAnalyzer_NextToken(pAnalyzer);
        /* Function call */
        if (tokenTypeIs(TOKEN_PAREN_L)) {
            FzAnalyzer_NextToken(pAnalyzer);
            if (tokenTypeIs(TOKEN_PAREN_R)) {
                /* No parameter */
            }
            else {
                FzAnalyzer_RewindToken(pAnalyzer);
                /* Have parameter */
                for(;;) {
                    BOOL bIsValid = matchExpr(pAnalyzer);
                    if (!bIsValid) {
                        return FALSE;
                    }
                    FzAnalyzer_NextToken(pAnalyzer);
                    if (tokenTypeIs(TOKEN_COMMA)) {
                        continue;
                    }
                    else if (tokenTypeIs(TOKEN_PAREN_R)) {
                        break;
                    }
                    else {
                        return FALSE;
                    }
                }
            }
        }
        /* It's not an array or function call, rewind by one token */
        else {
            FzAnalyzer_RewindToken(pAnalyzer);
        }
        return matchTryBinaryOperator(pAnalyzer);
    }
    /* Left parentheses */
    else if (tokenTypeIs(TOKEN_PAREN_L)) {
        BOOL bIsValid = matchExpr(pAnalyzer);
        if (!bIsValid) return FALSE;
        FzAnalyzer_NextToken(pAnalyzer);
        if (!tokenTypeIs(TOKEN_PAREN_R)) return FALSE;
        return matchTryBinaryOperator(pAnalyzer);
    }
    return FALSE;
}

static void buildExprAstTryOperand(FzLineAnalyzer *pAnalyzer, Vlist* pStackOperand, Vlist* pStackOperator);

static void buildExprAstHandleOperator(Vlist* pStackOperand, Vlist* pStackOperator) {
    FzAstNode* pAstTop;
    if (pStackOperator->iSize <= 0) {
        return;
    }
    pAstTop = (FzAstNode *)vlPopBack(pStackOperator);
    
    if (pAstTop->iType == AST_BINARY_OPERATOR) {
        /* Pop two operands from stack as children of the operator node */
        pAstTop->uData.sBinaryOperator.pAstRightOperand = (FzAstNode *)vlPopBack(pStackOperand);
        pAstTop->uData.sBinaryOperator.pAstLeftOperand = (FzAstNode *)vlPopBack(pStackOperand);
    }
    else if (pAstTop->iType == AST_UNARY_OPERATOR) {
        pAstTop->uData.sUnaryOperator.pAstOperand = (FzAstNode *)vlPopBack(pStackOperand);
    }
    /* Push the operator node (computed result) onto the operand stack */
     vlPushBack(pStackOperand, pAstTop);
}

static void buildExprAstTryOperator(FzLineAnalyzer* pAnalyzer, Vlist* pStackOperand, Vlist* pStackOperator) {
    FzToken* pToken = &pAnalyzer->token;
    FzAnalyzer_NextToken(pAnalyzer);
    if (tokenTypeIs(TOKEN_OPERATOR)) {
        /* Create operator node */
        FzAstNode*      pAstOpr = createAst(AST_BINARY_OPERATOR, NULL);
        FzOperatorId    iOprId  = getOperatorId(&pToken->svContent);
        int             iCurrentPrecedence = FzOperator_GetPrecedenceById(iOprId);

        pAstOpr->uData.sBinaryOperator.iOperatorId = iOprId;

        /* Check existing operator stack for precedence */
        while (pStackOperator->iSize > 0) {
            FzAstNode*      pAstTop = (FzAstNode *)vlPeek(pStackOperator);
            FzOperatorId    iTopOprId;
            int             iTopPrecedence;

            /* Not operator, exit */
            if (pAstTop->iType != AST_BINARY_OPERATOR && pAstTop->iType != AST_UNARY_OPERATOR) {
                break;
            }

            iTopOprId = pAstTop->uData.sBinaryOperator.iOperatorId;
            iTopPrecedence = FzOperator_GetPrecedenceById(iTopOprId);

            /* If the stack top has higher or equal precedence */
            if (iTopPrecedence >= iCurrentPrecedence) {
                buildExprAstHandleOperator(pStackOperand, pStackOperator);
            }
            else {
                break;
            }
        }
        /* Push this operator node onto the operator stack */
        vlPushBack(pStackOperator, pAstOpr);
        /* Push right operand */
        buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
    }
    else {
        FzAnalyzer_RewindToken(pAnalyzer);
    }
}

static void buildExprAstTryOperand(FzLineAnalyzer *pAnalyzer, Vlist* pStackOperand, Vlist* pStackOperator) {
    FzToken* pToken = &pAnalyzer->token;

    FzAnalyzer_NextToken(pAnalyzer);

    /* Literal: number */
    if (tokenTypeIs(TOKEN_NUMERIC)) {
        FzAstNode *pAstLiteral = createAst(AST_LITERAL_NUMERIC, NULL);
        Utils_StringViewCopy(&pAstLiteral->uData.sLiteralNumeric.svNumber, &pToken->svContent);
        /* Push directly onto operand stack */
        vlPushBack(pStackOperand, pAstLiteral);
        /* Check if the next token is an operator */
        buildExprAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* Unary operator*/
    else if (tokenTypeIs(TOKEN_OPERATOR)) {
        FzAstNode* pAstOprUnary = createAst(AST_UNARY_OPERATOR, NULL);
        if (tokenIs("-")) {
            pAstOprUnary->uData.sUnaryOperator.iOperatorId = OPR_NEG;
        }
        else if (tokenIs("!")) {
            pAstOprUnary->uData.sUnaryOperator.iOperatorId = OPR_NOT;
        }
        vlPushBack(pStackOperator, pAstOprUnary);
        buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* Identifier */
    else if (tokenTypeIs(TOKEN_IDENTIFIER)) {
        StringView svIdentifier;
        Utils_StringViewCopy(&svIdentifier, &pToken->svContent);
        FzAnalyzer_NextToken(pAnalyzer);
        /* Function call */
        if (tokenTypeIs(TOKEN_PAREN_L)) {
            /* Create function call node and push onto operator stack */
            FzAstNode* pAstFuncCall = createAst(AST_FUNCTION_CALL, NULL);
            Utils_StringViewCopy(&pAstFuncCall->uData.sFunctionCall.svFunction, &svIdentifier);
            vlPushBack(pStackOperator, pAstFuncCall);
            
            FzAnalyzer_NextToken(pAnalyzer);
            if (tokenTypeIs(TOKEN_PAREN_R)) {
                /* No parameters */
            }
            else {
                /* At least one parameter */
                FzAnalyzer_RewindToken(pAnalyzer);
                for(;;) {
                    /* Push operand */
                    buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
                    /* Process function parameter expressions */
                    while (pStackOperator->iSize > 0) {
                        FzAstNode* pAstTop = (FzAstNode *)vlPeek(pStackOperator);
                        /* Stop when reaching the function call node on the stack */
                        if (pAstTop == pAstFuncCall) {
                            break;
                        }
                        /* Process operator */
                        buildExprAstHandleOperator(pStackOperand, pStackOperator);
                    }
                    /* Pop operand and add it to the function call node */
                    vlPushBack(pAstFuncCall->uData.sFunctionCall.pListArguments, vlPopBack(pStackOperand));
                    /* Check if function parameters have ended */
                    FzAnalyzer_NextToken(pAnalyzer);
                    if (tokenTypeIs(TOKEN_COMMA)) {
                        continue;
                    }
                    else if (tokenTypeIs(TOKEN_PAREN_R)) {
                        break;
                    }
                }
            }
            /* Pop function node from operator stack, push onto operand stack */
            vlPushBack(pStackOperand, vlPopBack(pStackOperator));
        }
        /* Variable */
        else {
            FzAstNode *pAstVar = createAst(AST_VARIABLE, NULL);
            Utils_StringViewCopy(&pAstVar->uData.sVariable.svName, &svIdentifier);
            /* Push directly onto operand stack */
            vlPushBack(pStackOperand, pAstVar);
            /* Rewind to previous token */
            FzAnalyzer_RewindToken(pAnalyzer);
        }
        /* Check if the next token is an operator */
        buildExprAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
    /* Parentheses */
    else if (tokenTypeIs(TOKEN_PAREN_L)) {
        FzAstNode* pAstParen = createAst(AST_PAREN, NULL);
        /* Push parentheses node onto operator stack */
        vlPushBack(pStackOperator, pAstParen);
        /* Build expression inside parentheses */
        buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);
        /* Consume the closing parenthesis */
        FzAnalyzer_NextToken(pAnalyzer);
        /* Merge expressions within parentheses */
        while (pStackOperator->iSize > 0) {
            FzAstNode *pAstTop = (FzAstNode *)vlPeek(pStackOperator);
            /* Reached the current parentheses node, all contents have been popped */
            if (pAstTop == pAstParen) {
                vlPopBack(pStackOperator);
                break;
            }
            /* Process operator */
            buildExprAstHandleOperator(pStackOperand, pStackOperator);
        }
        /* Pop expression inside parentheses as child of parentheses node */
        pAstParen->uData.sParen.pAstExpr = (FzAstNode *)vlPopBack(pStackOperand);
        /* Push parentheses node onto operand stack */
        vlPushBack(pStackOperand, pAstParen);
        /* Check if the next token is an operator */
        buildExprAstTryOperator(pAnalyzer, pStackOperand, pStackOperator);
    }
}

static FzAstNode* buildExprAst(FzLineAnalyzer *pAnalyzer) {
    Vlist*      pStackOperand;  /* FzAstNode */
    Vlist*      pStackOperator; /* FzAstNode */
    FzAstNode*  pAstExprRoot;
    
    pStackOperand = vlNewList();
    pStackOperator = vlNewList();
    buildExprAstTryOperand(pAnalyzer, pStackOperand, pStackOperator);

    /* Pop all remaining operators from the operator stack */
    while (pStackOperator->iSize > 0) {
        buildExprAstHandleOperator(pStackOperand, pStackOperator);
    }

    pAstExprRoot = (FzAstNode *)vlPopBack(pStackOperand);

    vlDestroy(pStackOperand, destroyAstVoidPtr);
    vlDestroy(pStackOperator, destroyAstVoidPtr);

    return pAstExprRoot;
}

FzAstNode* FzParser_ParseExpression(const char* szSource) {
    FzAstNode*          pAstRootNode = NULL;
    FzLineAnalyzer      analyzer;
    BOOL                bIsValidExpr = FALSE;

    FzAnalyzer_Initialize(&analyzer, szSource);

    bIsValidExpr = matchExpr(&analyzer);
    if (!bIsValidExpr) {
        return NULL;
    }

    FzAnalyzer_NextToken(&analyzer);
    if (analyzer.token.iType != TOKEN_LINE_END) {
        return NULL;
    }
    
    FzAnalyzer_ResetToken(&analyzer);

    pAstRootNode = buildExprAst(&analyzer);

    return pAstRootNode;
}