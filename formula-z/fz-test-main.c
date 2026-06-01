#include <stdio.h>
#include <string.h>
#include "fz.h"

static int g_bPretty = 0;

static void printIndent(int iLevel) {
    if (g_bPretty) {
        int i;
        for (i = 0; i < iLevel; ++i) {
            printf("  ");
        }
    }
}

static void printNewline(void) {
    if (g_bPretty) {
        printf("\n");
    }
}

static void printAstNode(int iLevel, const FzAstNode* pAstNode) {
    char szBuf[50];

    if (pAstNode == NULL) {
        printIndent(iLevel);
        printf("<Null />");
        printNewline();
        return;
    }

    switch (pAstNode->iType) {
        default:
        case AST_EMPTY:
            printIndent(iLevel);
            printf("<Empty />");
            printNewline();
            break;
        case AST_UNARY_OPERATOR:
            printIndent(iLevel);
            printf("<Unary operator=\"%s\">", FzOperator_GetNameById(pAstNode->uData.sUnaryOperator.iOperatorId));
            printNewline();
            printAstNode(iLevel + 1, pAstNode->uData.sUnaryOperator.pAstOperand);
            printIndent(iLevel);
            printf("</Unary>");
            printNewline();
            break;
        case AST_BINARY_OPERATOR:
            printIndent(iLevel);
            printf("<Binary operator=\"%s\">", FzOperator_GetNameById(pAstNode->uData.sBinaryOperator.iOperatorId));
            printNewline();
            printAstNode(iLevel + 1, pAstNode->uData.sBinaryOperator.pAstLeftOperand);
            printAstNode(iLevel + 1, pAstNode->uData.sBinaryOperator.pAstRightOperand);
            printIndent(iLevel);
            printf("</Binary>");
            printNewline();
            break;
        case AST_PAREN:
            printIndent(iLevel);
            printf("<Paren>");
            printNewline();
            printAstNode(iLevel + 1, pAstNode->uData.sParen.pAstExpr);
            printIndent(iLevel);
            printf("</Paren>");
            printNewline();
            break;
        case AST_LITERAL_NUMERIC:
            printIndent(iLevel);
            Utils_StringViewCopyToBuffer(szBuf, sizeof(szBuf), &pAstNode->uData.sLiteralNumeric.svNumber);
            if (g_bPretty) {
                printf("<Literal> %s </Literal>", szBuf);
            } else {
                printf("<Literal>%s</Literal>", szBuf);
            }
            printNewline();
            break;
        case AST_VARIABLE:
            printIndent(iLevel);
            Utils_StringViewCopyToBuffer(szBuf, sizeof(szBuf), &pAstNode->uData.sVariable.svName);
            if (g_bPretty) {
                printf("<Variable> %s </Variable>", szBuf);
            } else {
                printf("<Variable>%s</Variable>", szBuf);
            }
            printNewline();
            break;
        case AST_FUNCTION_CALL: {
            VlistNode* pListNode;
            printIndent(iLevel);
            Utils_StringViewCopyToBuffer(szBuf, sizeof(szBuf), &pAstNode->uData.sFunctionCall.svFunction);
            printf("<Function name=\"%s\">", szBuf);
            printNewline();
            for (pListNode = pAstNode->uData.sFunctionCall.pListArguments->pHead; pListNode; pListNode = pListNode->pNext) {
                const FzAstNode* pAstParam = (FzAstNode *)pListNode->pData;
                printIndent(iLevel + 1);
                printf("<Parameter>");
                printNewline();
                printAstNode(iLevel + 2, pAstParam);
                printIndent(iLevel + 1);
                printf("</Parameter>");
                printNewline();
            }
            printIndent(iLevel);
            printf("</Function>");
            printNewline();
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    const char* szExpr = NULL;
    int         i;
    FzAstNode*  pAstExpr = NULL;

    g_bPretty = 0;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            g_bPretty = 1;
        } else if (szExpr == NULL) {
            szExpr = argv[i];
        }
    }

    if (szExpr == NULL) {
        fprintf(stderr, "Usage: %s [-h] <expression>\n", argv[0]);
        return 2;
    }

    pAstExpr = FzParser_ParseExpression(szExpr);
    if (!pAstExpr) {
        fprintf(stderr, "Syntax Error\n");
        return 1;
    }

    printAstNode(0, pAstExpr);
    FzAstNode_Destroy(pAstExpr);

    return 0;
}
