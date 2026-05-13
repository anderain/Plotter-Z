#include <stdio.h>
#include <string.h>
#include "rz.h"

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

void printRenderNode(int iLevel, const RenderNode* pNode) {
    VlistNode* pListNode = NULL;

    switch (pNode->iType) {
        case RN_TEXT:
            printIndent(iLevel);
            if (g_bPretty) {
                printf("<Text> %s </Text>", pNode->uData.sText.szText);
            }
            else {
                printf("<Text>%s</Text>", pNode->uData.sText.szText);
            }
            printNewline();
            break;
        case RN_COMMA:
            printIndent(iLevel);
            printf("<Comma />", pNode->uData.sText.szText);
            printNewline();
            break;
        case RN_HORIZONTAL:
            printIndent(iLevel);
            printf("<Horizontal>");
            printNewline();
            for (
                pListNode = pNode->uData.sHorizontal.pList->pHead;
                pListNode != NULL;
                pListNode = pListNode->pNext
            ) {
                printRenderNode(iLevel + 1, (RenderNode *)pListNode->pData);
            }
            printIndent(iLevel);
            printf("</Horizontal>");
            printNewline();
            break;
        case RN_ENCLOSURE:
            printIndent(iLevel);
            printf("<Enclosure>");
            printNewline();
            printRenderNode(iLevel + 1, pNode->uData.sEnclosure.pContent);
            printIndent(iLevel);
            printf("</Enclosure>");
            printNewline();
            break;
        case RN_STACK:
            printIndent(iLevel);
            printf("<Stack>");
            printNewline();
            printIndent(iLevel + 1);
            printf("<Top>");
            printNewline();
            printRenderNode(iLevel + 2, pNode->uData.sStack.pTop);
            printIndent(iLevel + 1);
            printf("</Top>");
            printNewline();
            printIndent(iLevel + 1);
            printf("<Bottom>");
            printNewline();
            printRenderNode(iLevel + 2, pNode->uData.sStack.pBottom);
            printIndent(iLevel + 1);
            printf("</Bottom>");
            printNewline();
            printIndent(iLevel);
            printf("</Stack>");
            printNewline();
            break;
        case RN_ROOT:
            printIndent(iLevel);
            printf("<Root>");
            printNewline();
            printRenderNode(iLevel + 1, pNode->uData.sRoot.pContent);
            printIndent(iLevel);
            printf("</Root>");
            printNewline();
            break;
        case RN_SUPERSCRIPT:
            printIndent(iLevel);
            printf("<Superscript>");
            printNewline();
            printIndent(iLevel + 1);
            printf("<Body>");
            printNewline();
            printRenderNode(iLevel + 2, pNode->uData.sSuperscript.pBody);
            printIndent(iLevel + 1);
            printf("</Body>");
            printNewline();
            printIndent(iLevel + 1);
            printf("<Script>");
            printNewline();
            printRenderNode(iLevel + 2, pNode->uData.sSuperscript.pScript);
            printIndent(iLevel + 1);
            printf("</Script>");
            printNewline();
            printIndent(iLevel);
            printf("</Superscript>");
            printNewline();
            break;
    }
}

int main(int argc, char* argv[]) {
    const char* szExpr = NULL;
    int         i;
    FzAstNode*  pAstExpr = NULL;
    RenderNode* pRenderNode;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            g_bPretty = 1;
        } else if (szExpr == NULL) {
            szExpr = argv[i];
        }
    }

    if (szExpr == NULL) {
        fprintf(stderr, "Usage: %s <expression>\n", argv[0]);
        return 2;
    }

    pAstExpr = FzParser_ParseExpression(szExpr);
    if (!pAstExpr) {
        fprintf(stderr, "Syntax Error\n");
        return 1;
    }

    pRenderNode = Render_Transform(pAstExpr);
    FzAstNode_Destroy(pAstExpr);

    printRenderNode(0, pRenderNode);
    RenderNode_Destroy(pRenderNode);

    return 0;
}
