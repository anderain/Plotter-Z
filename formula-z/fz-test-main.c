#include <stdio.h>
#include "fz.h"

void printFloatPretty(float fValue) {
    char szBuf[100];
    int iLen = sprintf(szBuf, "%f", fValue);
    for (--iLen; szBuf[iLen] && szBuf[iLen] == '0'; --iLen) {
        szBuf[iLen] = '\0';
    }
    if (szBuf[iLen] == '.') szBuf[iLen] = '\0';
    printf("%s", szBuf);
}

void printTab(int iLevel) {
    int i = 0;
    for (i = 0; i < iLevel; ++i) {
        printf("  ");
    }
}

void printAstNode(int iLevel, const FzAstNode* pAstNode) {
    if (pAstNode == NULL) {
        printTab(iLevel);
        printf("<Null />\n");
        return;
    }

    switch (pAstNode->iType) {
        default:
        case AST_EMPTY:
            printTab(iLevel); printf("<Empty />\n");
            break;
        case AST_UNARY_OPERATOR:
            printTab(iLevel); printf("<Unary operator=\"%s\">\n", FzOperator_GetNameById(pAstNode->uData.sUnaryOperator.iOperatorId));
            printAstNode(iLevel + 1, pAstNode->uData.sUnaryOperator.pAstOperand);
            printTab(iLevel); printf("</Unary>\n");
            break;
        case AST_BINARY_OPERATOR:
            printTab(iLevel); printf("<Binary operator=\"%s\">\n", FzOperator_GetNameById(pAstNode->uData.sUnaryOperator.iOperatorId));
            printAstNode(iLevel + 1, pAstNode->uData.sBinaryOperator.pAstLeftOperand);
            printAstNode(iLevel + 1, pAstNode->uData.sBinaryOperator.pAstRightOperand);
            printTab(iLevel); printf("</Binary>\n");
            break;
        case AST_PAREN:
            printTab(iLevel); printf("<Paren>\n");
            printAstNode(iLevel + 1, pAstNode->uData.sParen.pAstExpr);
            printTab(iLevel); printf("</Paren>\n");
            break;
        case AST_LITERAL_NUMERIC:
            printTab(iLevel);
            printf("<Literal> ");
            printFloatPretty(pAstNode->uData.sLiteralNumeric.fValue);
            printf(" </Literal>\n");
            break;
        case AST_VARIABLE:
            printTab(iLevel); printf("<Variable> %s </Variable>\n", pAstNode->uData.sVariable.szName);
            break;
        case AST_FUNCTION_CALL: {
            VlistNode* pListNode;
            printTab(iLevel); printf("<Function name=\"%s\">\n", pAstNode->uData.sFunctionCall.szFunction);
            for (pListNode = pAstNode->uData.sFunctionCall.pListArguments->pHead; pListNode; pListNode = pListNode->pNext) {
                const FzAstNode* pAstParam = (FzAstNode *)pListNode->pData;
                printTab(iLevel + 1); printf("<Parameter>\n");
                printAstNode(iLevel + 2, pAstParam);
                printTab(iLevel + 1); printf("</Parameter>\n");
            }
            printTab(iLevel); printf("</Function>\n");
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    const char* szExpr = "1.5+2*3+sin(x*5/2)";

    FzAstNode* pAstExpr = FzParser_ParseExpression(szExpr);

    if (!pAstExpr) {
        printf("There are syntax in expr: %s\n", szExpr);
    }

    printAstNode(0, pAstExpr);
    FzAstNode_Destroy(pAstExpr);

    return 0;
}