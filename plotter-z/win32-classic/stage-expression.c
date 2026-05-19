/*====================================================
 * Stage: Expression Editor
 *====================================================*/

#define EXPR_BUF EXPR_MAX

static char g_szExprBuf[EXPR_BUF];
static int  g_iExprCursor = 0;
static int  g_bExprActive = 0;
static int  g_iExprScroll = 0;

#define EXPR_VISIBLE_W  (g_iCanvasW - 64)

static void exprReset(void) {
    int iLen = (int)strlen(szExpr);
    if (iLen >= EXPR_BUF) iLen = EXPR_BUF - 1;
    memcpy(g_szExprBuf, szExpr, (size_t)iLen);
    g_szExprBuf[iLen] = '\0';
    g_iExprCursor = iLen;
    g_iExprScroll = 0;
    g_bExprActive = 1;
}

static void exprInsert(char c) {
    int iLen = (int)strlen(g_szExprBuf);
    int i;
    if (iLen >= EXPR_BUF - 1) return;
    for (i = iLen; i >= g_iExprCursor; --i)
        g_szExprBuf[i + 1] = g_szExprBuf[i];
    g_szExprBuf[g_iExprCursor] = c;
    g_iExprCursor++;
}

static void exprBackspace(void) {
    int iLen = (int)strlen(g_szExprBuf);
    int i;
    if (g_iExprCursor <= 0) return;
    for (i = g_iExprCursor - 1; i < iLen; ++i)
        g_szExprBuf[i] = g_szExprBuf[i + 1];
    g_iExprCursor--;
}

static void exprDelete(void) {
    int iLen = (int)strlen(g_szExprBuf);
    int i;
    if (g_iExprCursor >= iLen) return;
    for (i = g_iExprCursor; i < iLen; ++i)
        g_szExprBuf[i] = g_szExprBuf[i + 1];
}

static void exprConfirm(HWND hWnd) {
    int iLen;
    g_bExprActive = 0;
    iLen = (int)strlen(g_szExprBuf);
    if (iLen > 0) {
        memcpy(szExpr, g_szExprBuf, (size_t)iLen);
        szExpr[iLen] = '\0';
        if (g_pRenderNode != NULL) { RenderNode_Destroy(g_pRenderNode); g_pRenderNode = NULL; }
        if (g_pAstExpr != NULL) { FzAstNode_Destroy(g_pAstExpr); g_pAstExpr = NULL; }
        if (recalc()) {
            g_iStage = STAGE_WIREFRAME;
            redrawCanvas();
        } else {
            g_iStage = STAGE_ERROR;
            drawErrorScreen();
        }
    } else {
        g_iStage = STAGE_EXPRESSION_EDITOR;
        drawExpressionEditor();
    }
    InvalidateRect(hWnd, NULL, FALSE);
}

static void exprCancel(HWND hWnd) {
    g_bExprActive = 0;
    g_iStage = STAGE_WIREFRAME;
    redrawCanvas();
    InvalidateRect(hWnd, NULL, FALSE);
}

static void exprEnsureCursorVisible(void) {
    int iCursorX = (g_iExprCursor + 2) * CURRENT_FONT_WIDTH;
    int iMaxVis = EXPR_VISIBLE_W;
    if (iCursorX < g_iExprScroll)
        g_iExprScroll = iCursorX;
    else if (iCursorX > g_iExprScroll + iMaxVis - CURRENT_FONT_WIDTH * 3)
        g_iExprScroll = iCursorX - iMaxVis + CURRENT_FONT_WIDTH * 3;
    if (g_iExprScroll < 0) g_iExprScroll = 0;
}

void drawExpressionEditor(void) {
    char szLine[EXPR_BUF + 16];
    char szVisible[EXPR_BUF + 16];
    int iLen, iVisLen, iVisStart, iW, iX, iY;
    int iCursorX, iVisCursorX;

    fillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);

    fillRectCanvas(0, 0, g_iCanvasW, CURRENT_FONT_HEIGHT + 2, COLOR_BLACK);
    {
        const char* szTitle = "Expression Editor";
        int iTW = (int)strlen(szTitle) * CURRENT_FONT_WIDTH;
        putTextCanvas((g_iCanvasW - iTW) / 2, 1, (const unsigned char*)szTitle, COLOR_WHITE);
    }

    if (!g_bExprActive) exprReset();

    Salvia_Format(szLine, "> %s", g_szExprBuf);
    iLen = (int)strlen(szLine);

    exprEnsureCursorVisible();

    iVisStart = g_iExprScroll / CURRENT_FONT_WIDTH;
    if (iVisStart > iLen) iVisStart = iLen;
    iVisLen = iLen - iVisStart;
    if (iVisLen > EXPR_BUF + 15) iVisLen = EXPR_BUF + 15;
    memcpy(szVisible, szLine + iVisStart, (size_t)iVisLen);
    szVisible[iVisLen] = '\0';

    iW = iVisLen * CURRENT_FONT_WIDTH;
    iX = (g_iCanvasW - iW) / 2;
    if (iX < 12) iX = 12;

    iY = g_iCanvasH / 2 - CURRENT_FONT_HEIGHT;

    fillRectCanvas(iX - 4, iY - 1, iW + 8, CURRENT_FONT_HEIGHT + 2, COLOR_LIGHT_GRAY);
    putTextCanvas(iX, iY, (const unsigned char*)szVisible, COLOR_BLACK);

    /* Cursor indicator */
    iCursorX = (g_iExprCursor + 2) * CURRENT_FONT_WIDTH;
    iVisCursorX = iCursorX - g_iExprScroll;
    if (iVisCursorX >= 0 && iVisCursorX < iW) {
        fillRectCanvas(iX + iVisCursorX, iY + CURRENT_FONT_HEIGHT - 1,
            CURRENT_FONT_WIDTH, 2, COLOR_BLACK);
    }
}

/* WM_KEYDOWN for special keys */
void ExpressionEditor_OnKey(HWND hWnd, WPARAM wParam) {
    switch (wParam) {
        case VK_RETURN:
            exprConfirm(hWnd);
            break;
        case VK_ESCAPE:
            exprCancel(hWnd);
            break;
        case VK_BACK:
            exprBackspace();
            drawExpressionEditor();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case VK_DELETE:
            exprDelete();
            drawExpressionEditor();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case VK_LEFT:
            if (g_iExprCursor > 0) {
                g_iExprCursor--;
                drawExpressionEditor();
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        case VK_RIGHT:
            if (g_iExprCursor < (int)strlen(g_szExprBuf)) {
                g_iExprCursor++;
                drawExpressionEditor();
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
    }
}

/* WM_CHAR for ASCII character input */
void ExpressionEditor_OnChar(HWND hWnd, TCHAR ch) {
    if (ch >= 0x20 && ch <= 0x7E) {
        exprInsert((char)ch);
        drawExpressionEditor();
        InvalidateRect(hWnd, NULL, FALSE);
    }
}
