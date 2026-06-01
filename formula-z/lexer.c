#include <stdlib.h>
#include "fz.h"

static void assignToken(FzLineAnalyzer* pAnalyzer, FzTokenType iType, const char* pSourceBegin, int iLength) {
    FzToken* pToken = &pAnalyzer->token;
    pToken->iType = iType;
    pToken->svContent.pBegin = pSourceBegin;
    pToken->svContent.iLen = iLength;
}

void FzAnalyzer_NextToken(FzLineAnalyzer *pAnalyzer) {
    char        chFirst, chSecond;
    const char* pSourceStart;

    while (isSpace(*pAnalyzer->pCurrent)) {
        pAnalyzer->pCurrent++;
    }

    pSourceStart = pAnalyzer->pCurrent;

    chFirst = *pAnalyzer->pCurrent;

    /* Operators */
    switch (chFirst) {
    case '+':   case '-':   case '*':   case '/':   case '^':
    case '%':   case '=':   case '!':   case '\\':
        pAnalyzer->pCurrent++;
        assignToken(pAnalyzer, TOKEN_OPERATOR, pSourceStart, 1);
        return;
    case '(':
        pAnalyzer->pCurrent++;
        assignToken(pAnalyzer, TOKEN_PAREN_L, pSourceStart, 1);
        return;
    case ')':
        pAnalyzer->pCurrent++;
        assignToken(pAnalyzer, TOKEN_PAREN_R, pSourceStart, 1);
        return;
    case ',':
        pAnalyzer->pCurrent++;
        assignToken(pAnalyzer, TOKEN_COMMA, pSourceStart, 1);
        return;
    case '>':
        chSecond = *(pAnalyzer->pCurrent + 1);
        if (chSecond == '=') {
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, pSourceStart, 2);
            return;
        } else {
            pAnalyzer->pCurrent++;
            assignToken(pAnalyzer, TOKEN_OPERATOR, pSourceStart, 1);
            return;
        }
    case '<':
        chSecond = *(pAnalyzer->pCurrent + 1);
        if (chSecond == '=') {
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, pSourceStart, 2);
            return;
        } else if (chSecond == '>') {
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, pSourceStart, 2);
            return;
        } else {
            pAnalyzer->pCurrent++;
            assignToken(pAnalyzer, TOKEN_OPERATOR, pSourceStart, 1);
            return;
        }
    case '&':
        chSecond = *(pAnalyzer->pCurrent + 1);
        if (chSecond == '&') {
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, pSourceStart, 2);
            return;
        } else {
            pAnalyzer->pCurrent++;
            assignToken(pAnalyzer, TOKEN_OPERATOR, pSourceStart, 1);
            return;
        }
    case '|':
        chSecond = *(pAnalyzer->pCurrent + 1);
        if (chSecond == '|') {
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, pSourceStart, 2);
            return;
        } else {
            pAnalyzer->pCurrent++;
            assignToken(pAnalyzer, TOKEN_UNDEFINED, pSourceStart, 1);
            return;
        }
    }

    /* Line end */
    if (chFirst == '\0') {
        assignToken(pAnalyzer, TOKEN_LINE_END, pSourceStart, 0);
        return;
    }
    
    /* Numeric */
    if (isDigit(chFirst)) {
        while (isDigit(*pAnalyzer->pCurrent)) {
            pAnalyzer->pCurrent++;
        }
        /* Check if it contains decimals. */
        if (*pAnalyzer->pCurrent == '.') {
            /* Decimal point */
            pAnalyzer->pCurrent++;
            /* Continue writing numbers */
            while (isDigit(*pAnalyzer->pCurrent)) {
                pAnalyzer->pCurrent++;
            }
        }

        assignToken(pAnalyzer, TOKEN_NUMERIC, pSourceStart, pAnalyzer->pCurrent - pSourceStart);
        return;
    }
    /* Identifier */
    else if (isAlpha(chFirst)) {
        while (isAlphaNum(*pAnalyzer->pCurrent)) {
            pAnalyzer->pCurrent++;
        }
        assignToken(pAnalyzer, TOKEN_IDENTIFIER, pSourceStart, pAnalyzer->pCurrent - pSourceStart);
        return;
    }
    /* Undefined character */
    else {
        pAnalyzer->pCurrent++;
        assignToken(pAnalyzer, TOKEN_UNDEFINED, pSourceStart, 1);
        return;
    }
}

void FzAnalyzer_RewindToken(FzLineAnalyzer* pAnalyzer) {
    pAnalyzer->pCurrent -= pAnalyzer->token.svContent.iLen;
}

void FzAnalyzer_ResetToken(FzLineAnalyzer* pAnalyzer) {
    pAnalyzer->pCurrent = pAnalyzer->szLine;
}

const char* FzAnalyzer_GetCurrentPtr(FzLineAnalyzer* pAnalyzer) {
    return pAnalyzer->pCurrent;
}

void FzAnalyzer_SetCurrentPtr(FzLineAnalyzer* pAnalyzer, const char* pCurrent) {
    pAnalyzer->pCurrent = pCurrent;
}

void FzAnalyzer_Initialize(FzLineAnalyzer* pAnalyzer, const char* szLineSource) {
    pAnalyzer->szLine = szLineSource;
    FzAnalyzer_ResetToken(pAnalyzer);
}

const char* FzToken_GetTypeName(FzTokenType iTokenType) {
    static const char* TokenName[] = {
        "Error",        "LineEnd",      "Numeric",
        "Identifier",   "Operator",     "ParenLeft",
        "ParenRight",   "Comma",        "Undefined"
    };

    if (iTokenType < TOKEN_ERR || iTokenType > TOKEN_UNDEFINED) {
        return "N/A";
    }

    return TokenName[iTokenType - TOKEN_ERR];
}