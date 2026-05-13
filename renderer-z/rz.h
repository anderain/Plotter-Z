#ifndef _RENDERER_Z_
#define _RENDERER_Z_

#include "../formula-z/fz.h"

typedef enum tagRenderNodeType {
    RN_TEXT = 0,
    RN_COMMA,
    RN_HORIZONTAL,
    RN_ENCLOSURE,
    RN_STACK,
    RN_ROOT,
    RN_SUPERSCRIPT,
} RenderNodeType;

typedef struct tagRenderNode {
    RenderNodeType iType;
    struct {
        int iWidth;
        int iHeight;
    } sPixel;
    union {
        struct {
            char* szText;
        } sText;
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

#endif