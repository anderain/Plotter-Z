#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ez.h"
#include "../common/constants.h"

static const char* EzOpCodeName[] = {
    "NOP",
    "NEG",
    /* Basic: + , - , * , / , ^ */
    "ADD", "SUB", "MUL", "DIV", "POW",
    /* Integer: INTDIV , MOD */
    "INTDIV", "MOD",
    /* Logic: ! , && , ||*/
    "NOT", "AND", "OR",
    /* Comparison */
    "EQUAL", "NEQ", "GT", "LT",
    "GTEQ", "LTEQ",
    /* Push immediate data / variable */
    "PUSH_IMD", "PUSH_VAR",
    /* Function call */
    "FUNC"
};

const char* EzOpCode_GetName(EzOpCode iCode) {
    return EzOpCodeName[iCode];
}

EzMachine* EzMachine_Create () {
    EzMachine* pVm = NULL;
    pVm = (EzMachine *)malloc(sizeof(EzMachine));

    /* Variables */
    pVm->pListVariableName = vlNewList();
    pVm->pVariableValueArray = NULL;
    pVm->iVariableLength = 0;

    /* Operand Stack */
    pVm->pStack = NULL;
    pVm->iStackLength = 0;

    /* Instructions */
    pVm->iInstructionLength = 0;
    pVm->iInstructionCur = 0;
    pVm->pInstructions = NULL;
        
    return pVm;
}

void EzMachine_Destroy (EzMachine* pVm) {
    if (pVm == NULL) {
        return;
    }

    /* Variables */
    if (pVm->pVariableValueArray) free(pVm->pVariableValueArray);
    if (pVm->pListVariableName) vlDestroy(pVm->pListVariableName, free);

    /* Operand Stack */
    if (pVm->pStack) free(pVm->pStack);

    /* Instructions */
    if (pVm->pInstructions) free(pVm->pInstructions);

    free(pVm);
}

BOOL EzMachine_DeclareVariable (EzMachine* pVm, const char* szVarName) {
    VlistNode* pNode;
    Vlist* pListVar = pVm->pListVariableName;
    for (pNode = pListVar->pHead; pNode; pNode = pNode->pNext) {
        if (Utils_IsStringEqual((const char *)pNode->pData, szVarName)) {
            return FALSE;
        }
    }
    vlPushBack(pListVar, Utils_StringDump(szVarName));
    return TRUE;
}

void EzMachine_AllocateVariables(EzMachine* pVm) {
    int         iArraySizeInByte    = sizeof(PZ_FLOAT) * pVm->pListVariableName->iSize;
    PZ_FLOAT*   pNewArray           = NULL;
    PZ_FLOAT*   pOldArray           = pVm->pVariableValueArray;

    if (iArraySizeInByte <= 0) {
        if (pOldArray) {
            free(pOldArray);
        }
        pVm->pVariableValueArray = NULL;
        return;
    }

    pNewArray = (PZ_FLOAT *)malloc(iArraySizeInByte);
    memset(pNewArray, 0, iArraySizeInByte);

    if (pOldArray != NULL) {
        int i = 0;
        int iOldSize = pVm->iVariableLength;
        int iNewSize = pVm->pListVariableName->iSize;
        for (i = 0; i < iOldSize && i < iNewSize; ++i) {
            pNewArray[i] = pOldArray[i];
        }
    }

    pVm->iVariableLength = pVm->pListVariableName->iSize;
    pVm->pVariableValueArray = pNewArray;
}

int EzMachine_GetVariableIndexByName(EzMachine* pVm, const char* szVarName) {
    VlistNode*  pNode = NULL;
    int         i = 0;
    for (
        pNode = pVm->pListVariableName->pHead;
        pNode != NULL;
        pNode = pNode->pNext, i++
    ) {
        const char* szVarInVm = (const char *)pNode->pData;
        if (Utils_IsStringEqual(szVarName, szVarInVm)) {
            return i;
        }
    }
    return -1;
}

void EzMachine_SetVariableByIndex(EzMachine* pVm, int iIndex, PZ_FLOAT fValue) {
    if (iIndex < 0 || iIndex >= pVm->iVariableLength) {
        return;
    }
    pVm->pVariableValueArray[iIndex] = fValue;
}

#define evalPush(fValue)    (pVm->pStack[pVm->iStackSize++] = (fValue))
#define evalPop()           (pVm->pStack[--pVm->iStackSize])
#define handleSingleParamFunc(funcName) { fArg = evalPop(); evalPush((PZ_FLOAT)funcName(fArg)); } return

static void EzMachine_Function (EzMachine* pVm, PzFuncIndex iFuncIndex) {
    PZ_FLOAT fArg;

    switch (iFuncIndex) {
        default:
        case PZF_NONE:  return;
        case PZF_SIN:   handleSingleParamFunc(sin);
        case PZF_COS:   handleSingleParamFunc(cos);
        case PZF_TAN:   handleSingleParamFunc(tan);
        case PZF_ASIN:  handleSingleParamFunc(asin);
        case PZF_ACOS:  handleSingleParamFunc(acos);
        case PZF_ATAN:  handleSingleParamFunc(atan);
        case PZF_SQR:   handleSingleParamFunc(sqrt);
        case PZF_EXP:   handleSingleParamFunc(exp);
        case PZF_ABS:   handleSingleParamFunc(fabs);
        case PZF_LOG:   handleSingleParamFunc(log);
        case PZF_LN: {
            fArg = evalPop();
            evalPush((PZ_FLOAT)(log(fArg) / log(exp(1))));
            return;
        }
    }
}

PZ_FLOAT EzMachine_Eval (EzMachine* pVm) {
    int i;
    PZ_FLOAT fLeft, fRight;

    pVm->iStackSize = 0;
    
    for (i = 0; i < pVm->iInstructionLength; ++i) {
        EzInstruction* pInst = pVm->pInstructions + i;
        switch (pInst->iOpCode) {
            default:
            case EZOP_NOP: break;
            case EZOP_NEG:
                fLeft = evalPop();
                evalPush(-fLeft);
                break;
            case EZOP_ADD:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush(fLeft + fRight);
                break;
            case EZOP_SUB:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush(fLeft - fRight);
                break;
            case EZOP_MUL:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush(fLeft * fRight);
                break;
            case EZOP_DIV:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush(fLeft / fRight);
                break;
            case EZOP_POW:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)pow(fLeft, fRight));
                break;
            case EZOP_INTDIV:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)((int)fLeft / (int)fRight));
                break;
            case EZOP_MOD:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)((int)fLeft % (int)fRight));
                break;
            case EZOP_NOT:
                fLeft = evalPop();
                evalPush((PZ_FLOAT)(!(int)fLeft));
                break;
            case EZOP_AND:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)((int)fLeft && (int)fRight));
                break;
            case EZOP_OR:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)((int)fLeft || (int)fRight));
                break;
            case EZOP_EQUAL:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)(fLeft == fRight));
                break;
            case EZOP_NEQ:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)(fLeft != fRight));
                break;
            case EZOP_GT:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)(fLeft > fRight));
                break;
            case EZOP_LT:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)(fLeft < fRight));
                break;
            case EZOP_GTEQ:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)(fLeft >= fRight));
                break;
            case EZOP_LTEQ:
                fRight = evalPop();
                fLeft = evalPop();
                evalPush((PZ_FLOAT)(fLeft <= fRight));
                break;
            case EZOP_PUSH_IMD:
                evalPush(pInst->uData.fImmediate);
                break;
            case EZOP_PUSH_VAR:
                evalPush(pVm->pVariableValueArray[pInst->uData.iVarIndex]);
                break;
            case EZOP_FUNC:
                EzMachine_Function(pVm, pInst->uData.iFuncIndex);
                break;
        }
    }

    return pVm->pStack[0];
}