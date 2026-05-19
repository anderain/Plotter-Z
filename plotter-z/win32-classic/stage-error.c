/*====================================================
 * Stage: Error
 *====================================================*/

void drawErrorScreen(void) {
    fillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);

    fillRectCanvas(0, 0, g_iCanvasW, CURRENT_FONT_HEIGHT + 2, COLOR_BLACK);
    {
        const char* szTitle = "Plotter-Z";
        int iTW = (int)strlen(szTitle) * CURRENT_FONT_WIDTH;
        putTextCanvas((g_iCanvasW - iTW) / 2, 1, (const unsigned char*)szTitle, COLOR_WHITE);
    }

    putTextCanvas(8, CURRENT_FONT_HEIGHT + 8, (const unsigned char*)"ERROR:", COLOR_BLACK);
    putTextCanvas(8, CURRENT_FONT_HEIGHT * 2 + 6, (const unsigned char*)"  Expression: ", COLOR_BLACK);

    if (g_pAstExpr == NULL) {
        putTextCanvas(8, CURRENT_FONT_HEIGHT * 3 + 4,
            (const unsigned char*)"Syntax Error - could not parse expression", COLOR_BLACK);
    } else {
        putTextCanvas(8, CURRENT_FONT_HEIGHT * 3 + 4,
            (const unsigned char*)g_szErrorBuf, COLOR_BLACK);
    }

    putTextCanvas(8, CURRENT_FONT_HEIGHT * 5 + 2,
        (const unsigned char*)"Press [Tab] to exit", COLOR_DARK_GRAY);

    {
        const char* szHelp = "E - Edit Expression | W - Window Editor";
        int iHW = (int)strlen(szHelp) * CURRENT_FONT_WIDTH;
        int iHX = (g_iCanvasW - iHW) / 2;
        if (iHX < 2) iHX = 2;
        putTextCanvas(iHX, g_iCanvasH - CURRENT_FONT_HEIGHT - 2,
            (const unsigned char*)szHelp, COLOR_DARK_GRAY);
    }
}

void Error_OnKey(HWND hWnd, WPARAM wParam) {
    switch (wParam) {
        case 'W':
            g_iStage = STAGE_WINDOW_EDITOR;
            drawWindowEditor();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case 'E':
            g_iStage = STAGE_EXPRESSION_EDITOR;
            drawExpressionEditor();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
    }
}
