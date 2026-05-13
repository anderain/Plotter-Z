#include <stdio.h>
#include "ez.h"
#include "../common/constants.h"

void printFloatPretty(float fValue) {
    char szBuf[100];
    int iLen = sprintf(szBuf, "%f", fValue);
    for (--iLen; szBuf[iLen] && szBuf[iLen] == '0'; --iLen) {
        szBuf[iLen] = '\0';
    }
    if (szBuf[iLen] == '.') szBuf[iLen] = '\0';
    printf("%s", szBuf);
}

void printInstructions(EzMachine* pVm) {
    int i;
    for (i = 0; i < pVm->iInstructionLength; ++i) {
        EzInstruction* pInst = pVm->pInstructions + i;
        switch(pInst->iOpCode) {
            case EZOP_PUSH_IMD:
                printf("%-12s", EzOpCode_GetName(pInst->iOpCode));
                printFloatPretty(pInst->uData.fImmediate);
                printf("\n");
                break;
            case EZOP_PUSH_VAR:
                printf("%-12s%d\n", EzOpCode_GetName(pInst->iOpCode), pInst->uData.iVarIndex);
                break;
            case EZOP_FUNC: {
                const PzFuncMeta* pFuncMeta = Constant_GetFunctionMetadataByIndex(pInst->uData.iFuncIndex);
                printf("%-12s%s\n", EzOpCode_GetName(pInst->iOpCode), pFuncMeta->szName);
                break;
            }
            default:
                printf("%-12s\n", EzOpCode_GetName(pInst->iOpCode));
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    const char* szExpr      = NULL;
    int         i           = 0;
    FzAstNode*  pAstExpr    = NULL;
    EzMachine*  pVm         = NULL;
    EzError     iError      = EZERR_NONE;
    char        szError[EZ_ERROR_CONTENT_LENGTH];
    PZ_FLOAT    fResult;

    for (i = 1; i < argc; ++i) {
        if (szExpr == NULL) {
            szExpr = argv[i];
        }
    }

    if (szExpr == NULL) {
        fprintf(stderr, "Usage: %s <expression>\n", argv[0]);
        return 2;
    }

    pAstExpr = FzParser_ParseExpression(szExpr);

    if (!pAstExpr) {
        fprintf(stderr, "Syntax Error\n");
        return 1;
    }

    pVm = EzMachine_Create();

    EzMachine_DeclareVariable(pVm, "x");
    EzMachine_DeclareVariable(pVm, "y");
    EzMachine_DeclareVariable(pVm, "z");
    EzMachine_AllocateVariables(pVm);
    
    EzMachine_SetVariableByIndex(pVm, 0, 1);
    EzMachine_SetVariableByIndex(pVm, 1, 2);
    EzMachine_SetVariableByIndex(pVm, 2, 3);

    iError = EzMachine_Compile(pVm, pAstExpr, szError);
    FzAstNode_Destroy(pAstExpr);
    
    switch (iError) {
        default:
        case EZERR_NONE:
            break;
        case EZERR_VARIABLE_UNDEFINED:
            printf("VARIABLE_UNDEFINED: '%s'\n", szError);
            goto cleanUp;
        case EZERR_FUNCTION_UNDEFINED:
            printf("FUNCTION_UNDEFINED: '%s'\n", szError);
            goto cleanUp;
        case EZERR_FUNCTION_PARAM_MISMATCH:
            printf("FUNCTION_PARAM_MISMATCH: '%s'\n", szError);
            goto cleanUp;
    }

    printInstructions(pVm);

    fResult = EzMachine_Eval(pVm);
    printf("Result = ");
    printFloatPretty(fResult);
    printf("\n");

cleanUp:
    EzMachine_Destroy(pVm);

    return iError == EZERR_NONE ? 0 : 1;
}