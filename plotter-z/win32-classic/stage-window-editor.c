/*====================================================
 * Stage: Window Editor
 *====================================================*/

#define WINEDIT_COLS   3
#define WINEDIT_ROWS   3
#define WINEDIT_COUNT  9
#define WINEDIT_BUF    32

static int  g_iWinEditCursor = 0;
static int  g_bWinEditActive = 0;
static char g_szWinEditBuf[WINEDIT_BUF];

static const char* g_szWinEditLabels[] = {
    "X Min", "X Max", "X Grid",
    "Y Min", "Y Max", "Y Grid",
    "Z Min", "Z Max", ""
};

static PZ_FLOAT* g_pWinEditFields[WINEDIT_COUNT];
static int       g_pWinEditIsGrid[WINEDIT_COUNT];
static int g_bWinEditFieldsInit = 0;

static void winEditInitFields(void) {
    if (g_bWinEditFieldsInit) return;
    g_pWinEditFields[0] = &Camera.xMin;  g_pWinEditIsGrid[0] = 0;
    g_pWinEditFields[1] = &Camera.xMax;  g_pWinEditIsGrid[1] = 0;
    g_pWinEditFields[2] = (PZ_FLOAT*)&Camera.xGrid; g_pWinEditIsGrid[2] = 1;
    g_pWinEditFields[3] = &Camera.yMin;  g_pWinEditIsGrid[3] = 0;
    g_pWinEditFields[4] = &Camera.yMax;  g_pWinEditIsGrid[4] = 0;
    g_pWinEditFields[5] = (PZ_FLOAT*)&Camera.yGrid; g_pWinEditIsGrid[5] = 1;
    g_pWinEditFields[6] = &Camera.zMin;  g_pWinEditIsGrid[6] = 0;
    g_pWinEditFields[7] = &Camera.zMax;  g_pWinEditIsGrid[7] = 0;
    g_pWinEditFields[8] = NULL;          g_pWinEditIsGrid[8] = 0;
    g_bWinEditFieldsInit = 1;
}

static void winEditStartEdit(void) {
    int iGrid;
    if (g_iWinEditCursor >= WINEDIT_COUNT || g_pWinEditFields[g_iWinEditCursor] == NULL) return;
    iGrid = g_pWinEditIsGrid[g_iWinEditCursor];
    if (iGrid)
        Salvia_Format(g_szWinEditBuf, "%d", *((int*)g_pWinEditFields[g_iWinEditCursor]));
    else
        Utils_Ftoa(*g_pWinEditFields[g_iWinEditCursor], g_szWinEditBuf, DEFAULT_FTOA_PRECISION);
    g_bWinEditActive = 1;
}

static void winEditConfirm(void) {
    PZ_FLOAT fVal;
    int iGrid, iLen;
    if (!g_bWinEditActive) return;
    iGrid = g_pWinEditIsGrid[g_iWinEditCursor];
    iLen = (int)strlen(g_szWinEditBuf);
    if (iLen == 0) return;
    if (iGrid) {
        int iVal = Utils_Atoi(g_szWinEditBuf);
        if (iVal < 5) iVal = 5;
        if (iVal > GRID_MAX) iVal = GRID_MAX;
        *((int*)g_pWinEditFields[g_iWinEditCursor]) = iVal;
    } else {
        fVal = (PZ_FLOAT)Utils_Atof(g_szWinEditBuf);
        *g_pWinEditFields[g_iWinEditCursor] = fVal;
    }
    g_bWinEditActive = 0;
}

static void winEditCancel(void) {
    g_bWinEditActive = 0;
}

static void winEditInput(char c) {
    int iLen;
    if (!g_bWinEditActive) return;
    iLen = (int)strlen(g_szWinEditBuf);
    if (c == '-') {
        if (iLen == 0) { g_szWinEditBuf[0] = c; g_szWinEditBuf[1] = '\0'; }
    } else if (c == '.' && !g_pWinEditIsGrid[g_iWinEditCursor]) {
        if (strchr(g_szWinEditBuf, '.') == NULL && iLen < WINEDIT_BUF - 1)
            { g_szWinEditBuf[iLen] = c; g_szWinEditBuf[iLen + 1] = '\0'; }
    } else if (c >= '0' && c <= '9') {
        if (iLen < WINEDIT_BUF - 1)
            { g_szWinEditBuf[iLen] = c; g_szWinEditBuf[iLen + 1] = '\0'; }
    }
}

static void winEditBackspace(void) {
    int iLen;
    if (!g_bWinEditActive) return;
    iLen = (int)strlen(g_szWinEditBuf);
    if (iLen > 0) g_szWinEditBuf[iLen - 1] = '\0';
}

void drawWindowEditor(void) {
    int iCol, iRow, iIdx;
    int iStartY;
    int iColW, iColCenter[3];
    int iColMaxW[3];
    char szLabelFmt[16];
    int i;

    fillRectCanvas(0, 0, g_iCanvasW, g_iCanvasH, COLOR_WHITE);

    /* Title bar */
    fillRectCanvas(0, 0, g_iCanvasW, CURRENT_FONT_HEIGHT + 2, COLOR_BLACK);
    {
        const char* szTitle = "Window Editor";
        int iTW = (int)strlen(szTitle) * CURRENT_FONT_WIDTH;
        putTextCanvas((g_iCanvasW - iTW) / 2, 1, (const unsigned char*)szTitle, COLOR_WHITE);
    }

    winEditInitFields();

    /* Compute column widths (max entry width per column) */
    iColW = g_iCanvasW / WINEDIT_COLS;
    for (i = 0; i < WINEDIT_COLS; ++i)
        iColMaxW[i] = 0;

    for (iRow = 0; iRow < WINEDIT_ROWS; ++iRow) {
        for (iCol = 0; iCol < WINEDIT_COLS; ++iCol) {
            int iLen;
            char szBuf[48];
            iIdx = iRow * WINEDIT_COLS + iCol;
            if (g_pWinEditFields[iIdx] == NULL) continue;
            if (g_pWinEditIsGrid[iIdx])
                Salvia_Format(szLabelFmt, "%s %%d", g_szWinEditLabels[iIdx]);
            else
                Salvia_Format(szLabelFmt, "%s %%.2f", g_szWinEditLabels[iIdx]);
            if (g_pWinEditIsGrid[iIdx])
                Salvia_Format(szBuf, szLabelFmt, *((int*)g_pWinEditFields[iIdx]));
            else
                Salvia_Format(szBuf, szLabelFmt, (double)(*g_pWinEditFields[iIdx]));
            iLen = (int)strlen(szBuf);
            if (iLen > iColMaxW[iCol]) iColMaxW[iCol] = iLen;
        }
    }

    /* Column centers: pack columns tight, center the whole block */
    {
        int iBlockW = 0;
        for (i = 0; i < WINEDIT_COLS; ++i)
            iBlockW += iColMaxW[i] * CURRENT_FONT_WIDTH + CURRENT_FONT_WIDTH * 2;
        iColCenter[0] = (g_iCanvasW - iBlockW) / 2 + iColMaxW[0] * CURRENT_FONT_WIDTH / 2;
        iColCenter[1] = iColCenter[0] + iColMaxW[0] * CURRENT_FONT_WIDTH / 2
                      + CURRENT_FONT_WIDTH * 2 + iColMaxW[1] * CURRENT_FONT_WIDTH / 2;
        iColCenter[2] = iColCenter[1] + iColMaxW[1] * CURRENT_FONT_WIDTH / 2
                      + CURRENT_FONT_WIDTH * 2 + iColMaxW[2] * CURRENT_FONT_WIDTH / 2;
    }

    /* Vertical centering */
    iStartY = 24;
    if (iStartY < CURRENT_FONT_HEIGHT + 4) iStartY = CURRENT_FONT_HEIGHT + 4;

    /* Draw entries */
    for (iRow = 0; iRow < WINEDIT_ROWS; ++iRow) {
        for (iCol = 0; iCol < WINEDIT_COLS; ++iCol) {
            int iTextY, iX, iLen;
            char szBuf[48];
            iIdx = iRow * WINEDIT_COLS + iCol;
            if (g_pWinEditFields[iIdx] == NULL) continue;

            if (g_pWinEditIsGrid[iIdx])
                Salvia_Format(szLabelFmt, "%s %%d", g_szWinEditLabels[iIdx]);
            else
                Salvia_Format(szLabelFmt, "%s %%.2f", g_szWinEditLabels[iIdx]);

            if (g_pWinEditIsGrid[iIdx])
                Salvia_Format(szBuf, szLabelFmt, *((int*)g_pWinEditFields[iIdx]));
            else
                Salvia_Format(szBuf, szLabelFmt, (double)(*g_pWinEditFields[iIdx]));

            iLen = (int)strlen(szBuf);
            iTextY = iStartY + (iRow * 3) * CURRENT_FONT_HEIGHT;
            iX = iColCenter[iCol] - iLen * CURRENT_FONT_WIDTH / 2;
            if (iX < 4) iX = 4;

            if (!g_bWinEditActive && iIdx == g_iWinEditCursor) {
                fillRectCanvas(iX - 1, iTextY - 1,
                    iLen * CURRENT_FONT_WIDTH + 2, CURRENT_FONT_HEIGHT + 1, COLOR_BLACK);
                putTextCanvas(iX, iTextY, (const unsigned char*)szBuf, COLOR_WHITE);
            } else {
                putTextCanvas(iX, iTextY, (const unsigned char*)szBuf, COLOR_BLACK);
            }
        }
    }

    /* Input box */
    if (g_bWinEditActive) {
        int iInputY, iInputW, iInputX;
        const char* szPrompt;
        char szInputLine[WINEDIT_BUF + 32];
        int iPromptLen;
        if (g_pWinEditIsGrid[g_iWinEditCursor])
            szPrompt = "Enter integer: ";
        else
            szPrompt = "Enter number: ";
        Salvia_Format(szInputLine, "%s%s", szPrompt, g_szWinEditBuf);
        iPromptLen = (int)strlen(szInputLine);
        iInputW = iPromptLen * CURRENT_FONT_WIDTH;
        iInputX = (g_iCanvasW - iInputW) / 2;
        if (iInputX < 4) iInputX = 4;
        iInputY = iStartY + (WINEDIT_ROWS * 3) * CURRENT_FONT_HEIGHT + 4;
        fillRectCanvas(iInputX - 2, iInputY - 1, iInputW + 4, CURRENT_FONT_HEIGHT + 2, COLOR_LIGHT_GRAY);
        putTextCanvas(iInputX, iInputY, (const unsigned char*)szInputLine, COLOR_BLACK);
    }

    /* Footer hint */
    if (!g_bWinEditActive) {
        const char* szHelp = "ESC: Back | Arrows: Select | Enter: Edit";
        int iHW = (int)strlen(szHelp) * CURRENT_FONT_WIDTH;
        int iHX = (g_iCanvasW - iHW) / 2;
        if (iHX < 2) iHX = 2;
        putTextCanvas(iHX, g_iCanvasH - CURRENT_FONT_HEIGHT - 2,
            (const unsigned char*)szHelp, COLOR_DARK_GRAY);
    } else {
        const char* szHelp = "Enter: Confirm | ESC: Cancel | BS: Delete";
        int iHW = (int)strlen(szHelp) * CURRENT_FONT_WIDTH;
        int iHX = (g_iCanvasW - iHW) / 2;
        if (iHX < 2) iHX = 2;
        putTextCanvas(iHX, g_iCanvasH - CURRENT_FONT_HEIGHT - 2,
            (const unsigned char*)szHelp, COLOR_DARK_GRAY);
    }
}

void WindowEditor_OnKey(HWND hWnd, WPARAM wParam) {
    winEditInitFields();
    if (g_bWinEditActive) {
        switch (wParam) {
            case VK_RETURN:  winEditConfirm(); break;
            case VK_ESCAPE:  winEditCancel(); break;
            case VK_BACK:    winEditBackspace(); break;
            default: return;
        }
        drawWindowEditor();
        InvalidateRect(hWnd, NULL, FALSE);
    } else {
        switch (wParam) {
            case VK_ESCAPE:
                g_iStage = STAGE_WIREFRAME;
                recalc();
                redrawCanvas();
                InvalidateRect(hWnd, NULL, FALSE);
                return;
            case VK_RETURN:
                winEditStartEdit();
                break;
            case VK_LEFT:
                if (g_iWinEditCursor % WINEDIT_COLS > 0) g_iWinEditCursor--;
                break;
            case VK_RIGHT:
                if (g_iWinEditCursor % WINEDIT_COLS < WINEDIT_COLS - 1) g_iWinEditCursor++;
                break;
            case VK_UP:
                if (g_iWinEditCursor >= WINEDIT_COLS) g_iWinEditCursor -= WINEDIT_COLS;
                break;
            case VK_DOWN:
                if (g_iWinEditCursor + WINEDIT_COLS < WINEDIT_COUNT)
                    g_iWinEditCursor += WINEDIT_COLS;
                break;
            default: return;
        }
        if (g_iWinEditCursor >= WINEDIT_COUNT) g_iWinEditCursor = WINEDIT_COUNT - 1;
        if (g_pWinEditFields[g_iWinEditCursor] == NULL) g_iWinEditCursor = WINEDIT_COUNT - 1;
        drawWindowEditor();
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

/* WM_CHAR for character input in edit mode */
void WindowEditor_OnChar(HWND hWnd, TCHAR ch) {
    if (!g_bWinEditActive) return;
    if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-') {
        winEditInput((char)ch);
        drawWindowEditor();
        InvalidateRect(hWnd, NULL, FALSE);
    }
}
