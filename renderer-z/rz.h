#ifndef _RENDERER_Z_
#define _RENDERER_Z_

/* Workaround for fx-9860 SHC compiler's inability to parse paths correctly */
#if defined(_SH3) || defined(_SH4)
#   ifndef PLATFORM_FX9860
#       define PLATFORM_FX9860
#   endif
#endif

#ifdef PLATFORM_FX9860
#   include "../../formula-z/fz.h"
#else
#   include "../formula-z/fz.h"
#endif

typedef enum tagRenderNodeType {
    RN_TEXT         = 1,
    RN_SPECIAL_CHAR = 2,
    RN_HORIZONTAL   = 4,
    RN_ENCLOSURE    = 8,
    RN_STACK        = 16,
    RN_ROOT         = 32,
    RN_SUPERSCRIPT  = 64,
    RN_OVERUNDER    = 128,
    RN_BIG_SYMBOL   = 256
} RenderNodeType;

typedef enum tagBigSymbolType {
    ST_NONE = 0,
    ST_SUM, /* Summation */
    ST_PRD, /* Product */
    ST_INT  /* Integration */
} BigSymbolType;

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
            int bCurve;
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
        struct {
            struct tagRenderNode* pOver;
            struct tagRenderNode* pBase;
            struct tagRenderNode* pUnder;
        } sOverunder;
        struct {
            BigSymbolType iType;
        } sBigSymbol;
    } uData;
} RenderNode;

void        RenderNode_Destroy  (RenderNode* pNode);
RenderNode* Render_Transform    (const FzAstNode* pAstNode);

typedef struct tagGlyphPoint {
    int x, y;
} GlyphPoint;

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
        int iPaddingY;
        int iPaddingRight;
    } sOverunder;
    struct {
        struct {
            int iWidth;
            int iHeight;
            GlyphPoint sPoints[7];
        } sSum;
        struct {
            int iWidth;
            int iHeight;
            GlyphPoint sPoints[6];
        } sPrd;
        struct {
            int iWidth;
            int iHeight;
            GlyphPoint sPoints[6];
        } sInt;
    } sBigSymbol;
    struct {
        int bOutline;
    } sDebug;
    struct {
        void (*setPixel)(int x, int y);
        void (*plotLine)(int x1, int y1, int x2, int y2);
        void (*putChar)(int x, int y, unsigned char ch);
    } sInterfaces;
} RenderConfig;

void RenderConfig_GetDefaultStyle           (RenderConfig* pConfig);
void RenderConfig_CalculateBigSymbolPoints  (RenderConfig* pConfig);
void RenderNode_CalculateSize               (RenderNode* pNode, const RenderConfig* pConfig);
void RenderNode_Draw                        (RenderNode* pNode, const RenderConfig* pConfig, int iStartX, int iBaseline);

#endif