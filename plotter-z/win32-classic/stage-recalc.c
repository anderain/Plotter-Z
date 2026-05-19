/*====================================================
 * Stage: Recalc progress
 *====================================================*/

void drawRecalcScreen(HWND hWnd) {
    int iPercent;
    char szBuf[32];
    const char* szTitle = "Recalc ...";
    int iTW, iTY, iPW, iPX, iPY;

    if (g_iRecalcTotal > 0)
        iPercent = g_iRecalcProgress * 100 / g_iRecalcTotal;
    else
        iPercent = 0;

    fillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);

    iTW = (int)strlen(szTitle) * CURRENT_FONT_WIDTH;
    iTY = g_iCanvasH / 2 - CURRENT_FONT_HEIGHT;
    putTextCanvas((g_iCanvasW - iTW) / 2, iTY, (const unsigned char*)szTitle, COLOR_BLACK);

    Salvia_Format(szBuf, "%d%%", iPercent);
    iPW = (int)strlen(szBuf) * CURRENT_FONT_WIDTH;
    iPY = g_iCanvasH / 2 + CURRENT_FONT_HEIGHT;
    putTextCanvas((g_iCanvasW - iPW) / 2, iPY, (const unsigned char*)szBuf, COLOR_BLACK);

    (void)hWnd;
}

void Recalc_OnKey(HWND hWnd, WPARAM wParam) {
    (void)wParam;
    (void)hWnd;
}
