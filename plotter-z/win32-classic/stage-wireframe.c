/*====================================================
 * Stage: Wireframe
 *====================================================*/

void Wireframe_OnKey(HWND hWnd, WPARAM wParam) {
    switch (wParam) {
        case VK_LEFT:  Camera.iBetaDeg -= 5; break;
        case VK_RIGHT: Camera.iBetaDeg += 5; break;
        case VK_UP:    Camera.iAlphaDeg -= 5; break;
        case VK_DOWN:  Camera.iAlphaDeg += 5; break;
        case 'R':
            Camera.iViewportX = g_iCanvasW / 2;
            Camera.iViewportY = g_iCanvasH / 2;
            Camera.iZoomLevel = ZOOM_LEVEL_DEFAULT;
            Camera.iBetaDeg = DEFAULT_VIEW_BETA;
            Camera.iAlphaDeg = DEFAULT_VIEW_ALPHA;
            g_iExprPosX = 0;
            g_iExprPosY = 0;
            break;
        case 'B': g_bShowBox = !g_bShowBox; break;
        case 'Z':
            Camera.iZoomLevel--;
            if (Camera.iZoomLevel < 0) Camera.iZoomLevel = 0;
            break;
        case 'X':
            Camera.iZoomLevel++;
            if (Camera.iZoomLevel >= iNumZoomLevel) Camera.iZoomLevel = iNumZoomLevel - 1;
            break;
        case 'W':
            g_iStage = STAGE_WINDOW_EDITOR;
            drawWindowEditor();
            InvalidateRect(hWnd, NULL, FALSE);
            return;
        case 'E':
            g_iStage = STAGE_EXPRESSION_EDITOR;
            InvalidateRect(hWnd, NULL, FALSE);
            return;
        case 'M':
            g_bPanMode = 1;
            return;
        case 'F':
            g_bExprDrag = 1;
            return;
        default: return;
    }
    Camera.iBetaDeg = Camera.iBetaDeg % 360;
    if (Camera.iBetaDeg < 0) Camera.iBetaDeg += 360;
    Camera.iAlphaDeg = Camera.iAlphaDeg % 360;
    if (Camera.iAlphaDeg < 0) Camera.iAlphaDeg += 360;
    redrawCanvas();
    InvalidateRect(hWnd, NULL, FALSE);
}
