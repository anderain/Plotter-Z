#ifndef _FORMULA_Z_
#define _FORMULA_Z_

#include "../common/vlist.h"
#include "../common/utils.h"

/*================================================
 * Common
 *------------------------------------------------
 * 
 *================================================*/

/* Maximum length of single token */
#define FZ_TOKEN_LENGTH_MAX             100
#define PZ_FLOAT                        float

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
    int         iSourceStart;
    int         iSourceLength;
    char        szContent[FZ_TOKEN_LENGTH_MAX];
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
            char* szNumber;
        } sLiteralNumeric;
        struct {
            char *szName;
        } sVariable;
        struct {
            char* szFunction;
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
const char* FzOperator_GetNameById(FzOperatorId iOprId);
void        FzAstNode_Destroy(FzAstNode* pAstNode);
FzAstNode*  FzParser_ParseExpression(const char* szSource);


void printAstNode(int iLevel, const FzAstNode* pAstNode);
#endif 