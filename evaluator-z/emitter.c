#include <stdlib.h>
#include "ez.h"
#include "../common/constants.h"

#define PUSH    1
#define POP     -1

typedef struct {
    int iStackMax;
    int iStackSize;
    int iInstructionLength;
} EstimationContext;

static void estimateOperandStack(EstimationContext* pCtx, int iAction) {
    if (iAction == PUSH) {
        pCtx->iStackSize++;
        if (pCtx->iStackMax < pCtx->iStackSize) {
            pCtx->iStackMax = pCtx->iStackSize;
        }
    }
    else if (iAction == POP) {
        pCtx->iStackSize--;
    }
}

static void estimateAddInstruction(EstimationContext* pCtx) {
    pCtx->iInstructionLength++;
}

static EzError estimateAndCheck(EzMachine* pVm, const FzAstNode* pAstNode, EstimationContext* pCtx, char* szErrorContent) {
    EzError iErrorCode;

    switch (pAstNode->iType) {
        default:
        case AST_EMPTY:
            break;
        case AST_LITERAL_NUMERIC:
            estimateAddInstruction(pCtx);
            estimateOperandStack(pCtx, PUSH);
            break;
        case AST_VARIABLE:
            /* Check if variable exist */
            if (EzMachine_GetVariableIndexByName(pVm, pAstNode->uData.sVariable.szName) < 0) {
                Utils_StringCopy(szErrorContent, EZ_ERROR_CONTENT_LENGTH, pAstNode->uData.sVariable.szName);
                return EZERR_VARIABLE_UNDEFINED;
            }
            estimateAddInstruction(pCtx);
            estimateOperandStack(pCtx, PUSH);
            break;
        case AST_UNARY_OPERATOR:
            iErrorCode = estimateAndCheck(pVm, pAstNode->uData.sUnaryOperator.pAstOperand, pCtx, szErrorContent);
            if (iErrorCode) return iErrorCode;
            estimateAddInstruction(pCtx);
            estimateOperandStack(pCtx, POP);
            estimateOperandStack(pCtx, PUSH);
            break;
        case AST_BINARY_OPERATOR:
            iErrorCode = estimateAndCheck(pVm, pAstNode->uData.sBinaryOperator.pAstLeftOperand, pCtx, szErrorContent);
            if (iErrorCode) return iErrorCode;
            iErrorCode = estimateAndCheck(pVm, pAstNode->uData.sBinaryOperator.pAstRightOperand, pCtx, szErrorContent);
            if (iErrorCode) return iErrorCode;
            estimateOperandStack(pCtx, POP);
            estimateOperandStack(pCtx, POP);
            estimateAddInstruction(pCtx);
            estimateOperandStack(pCtx, PUSH);
            break;
        case AST_PAREN:
            iErrorCode = estimateAndCheck(pVm, pAstNode->uData.sParen.pAstExpr, pCtx, szErrorContent);
            if (iErrorCode) return iErrorCode;
            break;
        case AST_FUNCTION_CALL: {
            VlistNode*  pListNode;
            const PzFuncMeta* pFuncMeta;
            int i;
            /* Check if function exist */
            pFuncMeta = Constant_GetFunctionMetadata(pAstNode->uData.sFunctionCall.szFunction);
            if (pFuncMeta == NULL) {
                Utils_StringCopy(szErrorContent, EZ_ERROR_CONTENT_LENGTH, pAstNode->uData.sFunctionCall.szFunction);
                return EZERR_FUNCTION_UNDEFINED;
            }

            /* Check if the number of parameters is correct */
            if (pFuncMeta->iNumArguments != pAstNode->uData.sFunctionCall.pListArguments->iSize) {
                Utils_StringCopy(szErrorContent, EZ_ERROR_CONTENT_LENGTH, pAstNode->uData.sFunctionCall.szFunction);
                return EZERR_FUNCTION_PARAM_MISMATCH;
            }

            for (
                pListNode = pAstNode->uData.sFunctionCall.pListArguments->pHead;
                pListNode != NULL;
                pListNode = pListNode->pNext
            ) {
                iErrorCode = estimateAndCheck(pVm, (FzAstNode *)pListNode->pData, pCtx, szErrorContent);
                if (iErrorCode) return iErrorCode;
            }
            for (i = 0; i < pAstNode->uData.sFunctionCall.pListArguments->iSize; ++i) {
                estimateOperandStack(pCtx, POP);
            }
            estimateAddInstruction(pCtx);
            estimateOperandStack(pCtx, PUSH);
            break;
        }
    }

    return EZERR_NONE;
}

static void addInstructionPushImmediate(EzMachine* pVm, PZ_FLOAT fValue) {
    EzInstruction* pInst = pVm->pInstructions + pVm->iInstructionCur;

    pInst->iOpCode = EZOP_PUSH_IMD;
    pInst->uData.fImmediate = fValue;

    pVm->iInstructionCur++;
}

static void addInstructionPushVariable(EzMachine* pVm, int iVarIndex) {
    EzInstruction* pInst = pVm->pInstructions + pVm->iInstructionCur;

    pInst->iOpCode = EZOP_PUSH_VAR;
    pInst->uData.iVarIndex = iVarIndex;

    pVm->iInstructionCur++;
}

static void addInstructionOperator(EzMachine* pVm, FzOperatorId iOprId) {
    EzInstruction* pInst = pVm->pInstructions + pVm->iInstructionCur;
    EzOpCode iCode = EZOP_NOP;

    switch (iOprId) {
        case OPR_NEG:   iCode = EZOP_NEG;       break;
        case OPR_ADD:   iCode = EZOP_ADD;       break;
        case OPR_SUB:   iCode = EZOP_SUB;       break;
        case OPR_MUL:   iCode = EZOP_MUL;       break;
        case OPR_DIV:   iCode = EZOP_DIV;       break;
        case OPR_POW:   iCode = EZOP_POW;       break;
        case OPR_INTDIV:iCode = EZOP_INTDIV;    break;
        case OPR_MOD:   iCode = EZOP_MOD;       break;
        case OPR_NOT:   iCode = EZOP_NOT;       break;
        case OPR_AND:   iCode = EZOP_AND;       break;
        case OPR_OR:    iCode = EZOP_OR;        break;
        case OPR_EQUAL: iCode = EZOP_EQUAL;     break;
        case OPR_NEQ:   iCode = EZOP_NEQ;       break;
        case OPR_GT:    iCode = EZOP_GT;        break;
        case OPR_LT:    iCode = EZOP_LT;        break;
        case OPR_GTEQ:  iCode = EZOP_GTEQ;      break;
        case OPR_LTEQ:  iCode = EZOP_LTEQ;      break;
        default:
        case OPR_NONE:  iCode = EZOP_NOP;       break;
    }

    pInst->iOpCode = iCode;
    pInst->uData.iNop = 0;

    pVm->iInstructionCur++;
}

static void addInstructionFunctionCall(EzMachine* pVm, int iFuncIndex) {
    EzInstruction* pInst = pVm->pInstructions + pVm->iInstructionCur;

    pInst->iOpCode = EZOP_FUNC;
    pInst->uData.iFuncIndex = iFuncIndex;

    pVm->iInstructionCur++;
}

static void compile(EzMachine* pVm, const FzAstNode* pAstNode) {
    switch (pAstNode->iType) {
        default:
        case AST_EMPTY:
            break;
        case AST_LITERAL_NUMERIC:
            addInstructionPushImmediate(pVm, (PZ_FLOAT)Utils_Atof(pAstNode->uData.sLiteralNumeric.szNumber));
            break;
        case AST_VARIABLE:
            addInstructionPushVariable(pVm, EzMachine_GetVariableIndexByName(pVm, pAstNode->uData.sVariable.szName));
            break;
        case AST_UNARY_OPERATOR:
            compile(pVm, pAstNode->uData.sUnaryOperator.pAstOperand);
            addInstructionOperator(pVm, pAstNode->uData.sUnaryOperator.iOperatorId);
            break;
        case AST_BINARY_OPERATOR:
            compile(pVm, pAstNode->uData.sBinaryOperator.pAstLeftOperand);
            compile(pVm, pAstNode->uData.sBinaryOperator.pAstRightOperand);
            addInstructionOperator(pVm, pAstNode->uData.sBinaryOperator.iOperatorId);
            break;
        case AST_PAREN:
            compile(pVm, pAstNode->uData.sParen.pAstExpr);
            break;
        case AST_FUNCTION_CALL: {
            VlistNode* pListNode;
            const PzFuncMeta* pFuncMeta = Constant_GetFunctionMetadata(pAstNode->uData.sFunctionCall.szFunction);
            for (
                pListNode = pAstNode->uData.sFunctionCall.pListArguments->pHead;
                pListNode != NULL;
                pListNode = pListNode->pNext
            ) {
                compile(pVm, (FzAstNode *)pListNode->pData);
            }
            addInstructionFunctionCall(pVm, pFuncMeta->iIndex);
            break;
        }
    }
}

EzError EzMachine_Compile (EzMachine* pVm, const FzAstNode* pAstExpr, char* szErrorContent) {
    EstimationContext   ctx;
    EzError             iErrorCode;

    /* Clean up */
    ctx.iInstructionLength = 0;
    ctx.iStackMax = 0;
    ctx.iStackSize = 0;
    if (pVm->pStack) {
        free(pVm->pStack);
        pVm->pStack = NULL;
    }
    if (pVm->pInstructions) {
        free(pVm->pInstructions);
        pVm->pInstructions = NULL;
    }

    /* Estimate in advance the space of stack and instructions */
    /* required to perform the operations. */
    iErrorCode = estimateAndCheck(pVm, pAstExpr, &ctx, szErrorContent);
    if (iErrorCode != EZERR_NONE) {
        return iErrorCode;
    }

    /* Allocate space */
    pVm->iStackLength = ctx.iStackMax;
    pVm->pStack = (PZ_FLOAT *)malloc(pVm->iStackLength * sizeof(PZ_FLOAT));

    pVm->iInstructionLength = ctx.iInstructionLength = ctx.iInstructionLength;
    pVm->pInstructions = (EzInstruction *)malloc(pVm->iInstructionLength * sizeof(EzInstruction));

    /* Start compile */
    pVm->iInstructionCur = 0;
    compile(pVm, pAstExpr);
    pVm->iInstructionCur = 0;

    return EZERR_NONE;
}
