#ifndef _FORMULA_Z_
#define _FORMULA_Z_

/* Workaround for fx-9860 SHC compiler's inability to parse paths correctly */
#if defined(_SH3) || defined(_SH4)
#   ifndef PLATFORM_FX9860
#       define PLATFORM_FX9860
#   endif
#endif

#ifdef PLATFORM_FX9860
#   include "../../common/vlist.h"
#   include "../../common/utils.h"
#else
#   include "../common/vlist.h"
#   include "../common/utils.h"
#endif

/*================================================
 * Common
 *------------------------------------------------
 * 
 *================================================*/

/* Maximum length of single token */
#define FZ_TOKEN_LENGTH_MAX 100

typedef enum {
    OPR_NONE = 0,
    OPR_NEG,
    /* Basic: + , - , * , / , ^ */
    OPR_ADD, OPR_SUB, OPR_MUL, OPR_DIV, OPR_POW,
    /* Integer: INTDIV , MOD */
    OPR_INTDIV, OPR_MOD,
    /* Logic: ! , && , ||*/
    OPR_NOT, OPR_AND, OPR_OR,
    /* Comparison */
    OPR_EQUAL, OPR_NEQ, OPR_GT, OPR_LT,
    OPR_GTEQ, OPR_LTEQ 
} FzOperatorId;

/*================================================
 * Lexer
 *------------------------------------------------
 * 
 *================================================*/

typedef enum tagFzTokenType {
    TOKEN_ERR,          TOKEN_LINE_END,     TOKEN_NUMERIC,
    TOKEN_IDENTIFIER,   TOKEN_OPERATOR,     TOKEN_PAREN_L,
    TOKEN_PAREN_R,      TOKEN_COMMA,        TOKEN_UNDEFINED
} FzTokenType;

typedef struct tagFzToken {
    FzTokenType iType;
    StringView svContent;
} FzToken;

typedef struct tagFzLineAnalyzer {
    const char* szLine;
    const char* pCurrent;
    FzToken     token;
} FzLineAnalyzer;

void        FzAnalyzer_NextToken         (FzLineAnalyzer *pAnalyzer);
void        FzAnalyzer_RewindToken       (FzLineAnalyzer* pAnalyzer);
void        FzAnalyzer_ResetToken        (FzLineAnalyzer* pAnalyzer);
void        FzAnalyzer_Initialize        (FzLineAnalyzer* pAnalyzer, const char* szLineSource);
const char* FzAnalyzer_GetCurrentPtr     (FzLineAnalyzer* pAnalyzer);
void        FzAnalyzer_SetCurrentPtr     (FzLineAnalyzer* pAnalyzer, const char* pCurrent);
const char* FzToken_GetTypeName          (FzTokenType iTokenType);

/*================================================
 * Parser
 *------------------------------------------------
 * 
 *================================================*/

typedef struct tagFzAstNode {
    int                 iType;
    int                 iControlId;
    int                 iLineNumber;
    struct tagFzAstNode*pAstParent;
    union {
        struct {
            int iOperatorId;
            struct tagFzAstNode* pAstOperand;
        } sUnaryOperator;
        struct {
            struct tagFzAstNode* pAstExpr;
        } sParen;
        struct {
            int iOperatorId;
            struct tagFzAstNode* pAstLeftOperand;
            struct tagFzAstNode* pAstRightOperand;
        } sBinaryOperator;
        struct {
            StringView svNumber;
        } sLiteralNumeric;
        struct {
            StringView svName;
        } sVariable;
        struct {
            StringView svFunction;
            Vlist* pListArguments; /* <AstNode> */
        } sFunctionCall;
    } uData;
} FzAstNode;

typedef enum tagFzAstNodeType {
    AST_EMPTY = 0,
    AST_UNARY_OPERATOR,
    AST_BINARY_OPERATOR,
    AST_PAREN,
    AST_LITERAL_NUMERIC,
    AST_VARIABLE,
    AST_FUNCTION_CALL
} AstNodeType;

int         FzOperator_GetPrecedenceById(FzOperatorId iOprId);
const char* FzOperator_GetNameById      (FzOperatorId iOprId);
const char* FzOperator_GetSymbolById    (FzOperatorId iOprId);
void        FzAstNode_Destroy           (FzAstNode* pAstNode);
FzAstNode*  FzParser_ParseExpression    (const char* szSource);

#endif 