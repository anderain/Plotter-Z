#ifndef _EVALUATOR_Z_
#define _EVALUATOR_Z_

#include "../formula-z/fz.h"

/*================================================
 * Common
 *------------------------------------------------
 * 
 *================================================*/

typedef enum {
    EZOP_NOP = 0,
    EZOP_NEG,
    /* Basic: + , - , * , / , ^ */
    EZOP_ADD, EZOP_SUB, EZOP_MUL, EZOP_DIV, EZOP_POW,
    /* Integer: INTDIV , MOD */
    EZOP_INTDIV, EZOP_MOD,
    /* Logic: ! , && , ||*/
    EZOP_NOT, EZOP_AND, EZOP_OR,
    /* Comparison */
    EZOP_EQUAL, EZOP_NEQ, EZOP_GT, EZOP_LT,
    EZOP_GTEQ, EZOP_LTEQ,
    /* Push immediate data / variable */
    EZOP_PUSH_IMD, EZOP_PUSH_VAR,
    /* Function call */
    EZOP_FUNC
} EzOpCode;

typedef struct {
    EzOpCode iOpCode;
    union {
        PZ_FLOAT    fImmediate;
        int         iVarIndex;
        int         iFuncIndex;
        int         iNop;
    } uData;
} EzInstruction;

const char* EzOpCode_GetName(EzOpCode iCode);

typedef struct {
    /* Variables */
    Vlist*          pListVariableName; /* char* */
    PZ_FLOAT*       pVariableValueArray;
    int             iVariableLength;

    /* Operand Stack */
    int             iStackLength;
    int             iStackSize;
    PZ_FLOAT*       pStack;

    /* Instructions */
    int             iInstructionLength;
    int             iInstructionCur;
    EzInstruction*  pInstructions;
} EzMachine;


/*================================================
 * Emitter
 *------------------------------------------------
 * 
 *================================================*/

#define EZ_ERROR_CONTENT_LENGTH 100

typedef enum {
    EZERR_NONE = 0,
    EZERR_VARIABLE_UNDEFINED,
    EZERR_FUNCTION_UNDEFINED,
    EZERR_FUNCTION_PARAM_MISMATCH
} EzError;

EzError             EzMachine_Compile               (EzMachine* pVm, const FzAstNode* pAstExpr, char* szErrorContent);

/*================================================
 * Evaluator
 *------------------------------------------------
 * 
 *================================================*/

EzMachine*          EzMachine_Create                ();
void                EzMachine_Destroy               (EzMachine* pVm);
BOOL                EzMachine_DeclareVariable       (EzMachine* pVm, const char* szVarName);
void                EzMachine_AllocateVariables     (EzMachine* pVm);
int                 EzMachine_GetVariableIndexByName(EzMachine* pVm, const char* szVarName);
void                EzMachine_SetVariableByIndex    (EzMachine* pVm, int iIndex, PZ_FLOAT fValue);
PZ_FLOAT            EzMachine_Eval                  (EzMachine* pVm);

#endif