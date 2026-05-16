#ifndef _RENDERER_Z_
#define _RENDERER_Z_

#include "../formula-z/fz.h"

typedef enum tagRenderNodeType {
    RN_TEXT         = 1,
    RN_SPECIAL_CHAR = 2,
    RN_HORIZONTAL   = 4,
    RN_ENCLOSURE    = 8,
    RN_STACK        = 16,
    RN_ROOT         = 32,
    RN_SUPERSCRIPT  = 64
} RenderNodeType;

typedef struct tagRenderNode {
    RenderNodeType iType;
    struct {
        int iWidth;     /* Total width of the node */
        /* Total height of the node (iAscent + iDescent) */
        int iAscent;    /* Distance above the baseline (upward extent) */
        int iDescent;   /* Distance below the baseline (downward extent) */
    } sLayout;
    union {
        struct {
            char* szText;
        } sText;
        struct {
            unsigned char c;
        } sSpecialChar;
        struct {
            Vlist* pList; /* RenderNode* */
        } sHorizontal;
        struct {
            struct tagRenderNode* pContent;
        } sEnclosure;
        struct {
            struct tagRenderNode* pTop;
            struct tagRenderNode* pBottom;
        } sStack;
        struct {
            struct tagRenderNode* pContent;
        } sRoot;
        struct {
            struct tagRenderNode* pBody;
            struct tagRenderNode* pScript;
        } sSuperscript;
    } uData;
} RenderNode;

void        RenderNode_Destroy  (RenderNode* pNode);
RenderNode* Render_Transform    (FzAstNode* pAstNode);

typedef struct tagRenderConfig {
    struct {
        int iPaddingLeft;
    } sText;
    struct {
        int iWidth;
        int iHeight;
    } sFont;
    struct {
        int iSpacingX;
        int iSpacingY;
    } sStack;
    struct {
        int iSpacingLeft;
        int iSpacingTop;
    } sRoot;
    struct {
        int iPadding;
        int iMargin;
        int iRadius;
    } sEnclosure;
    struct {
        int bOutline;
    } sDebug;
    struct {
        void (*setPixel)(int x, int y);
        void (*plotLine)(int x1, int y1, int x2, int y2);
        void (*putChar)(int x, int y, unsigned char ch);
    } sInterfaces;
} RenderConfig;

void RenderConfig_GetDefaultStyle   (RenderConfig* pConfig);
void RenderNode_EstimateSize        (RenderNode* pNode, const RenderConfig* pConfig);
void RenderNode_Draw                (RenderNode* pNode, const RenderConfig* pConfig, int iStartX, int iCenterY);

#endif