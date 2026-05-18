#include <stdlib.h>
#include "fz.h"

static void assignToken(FzLineAnalyzer* pAnalyzer, FzTokenType iType, const char *szContent, const char *pSourceStart) {
    FzToken* pToken = &pAnalyzer->token;
    pToken->iType = iType;

    /* This is the processed token content. Since escape characters, quotation marks, */
    /* left parentheses, etc. may have been consumed, the original length is recorded. */
    Utils_StringCopy(pToken->szContent, sizeof(pToken->szContent), szContent);

    /* The starting index of the token in this line */
    pToken->iSourceStart = pSourceStart - pAnalyzer->szLine;

    /* The original length of the token in this line. */
    pToken->iSourceLength = pAnalyzer->pCurrent - pSourceStart;
}

void FzAnalyzer_NextToken(FzLineAnalyzer *pAnalyzer) {
    char        firstChar, secondChar;
    char        szBuffer[FZ_TOKEN_LENGTH_MAX * 2];
    char*       pBuffer = szBuffer;
    const char* pSourceStart;

    while (isSpace(*pAnalyzer->pCurrent)) {
        pAnalyzer->pCurrent++;
    }

    pSourceStart = pAnalyzer->pCurrent;

    firstChar = *pAnalyzer->pCurrent;

    /* Operators */
    switch (firstChar) {
    case '+':   case '-':   case '*':   case '/':   case '^':
    case '%':   case '=':   case '!':   case '\\':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
        return;
    case '(':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        assignToken(pAnalyzer, TOKEN_PAREN_L, szBuffer, pSourceStart);
        return;
    case ')':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        assignToken(pAnalyzer, TOKEN_PAREN_R, szBuffer, pSourceStart);
        return;
    case ',':
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        pAnalyzer->pCurrent++;
        assignToken(pAnalyzer, TOKEN_COMMA, szBuffer, pSourceStart);
        return;
    case '>':
        secondChar = *(pAnalyzer->pCurrent + 1);
        if (secondChar == '=') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
            return;
        } else {
            szBuffer[0] = firstChar;
            szBuffer[1] = '\0';
            pAnalyzer->pCurrent++;
            assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
            return;
        }
    case '<':
        secondChar = *(pAnalyzer->pCurrent + 1);
        if (secondChar == '=') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
            return;
        } else if (secondChar == '>') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
            return;
        } else {
            szBuffer[0] = firstChar;
            szBuffer[1] = '\0';
            pAnalyzer->pCurrent++;
            assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
            return;
        }
    case '&':
        secondChar = *(pAnalyzer->pCurrent + 1);
        if (secondChar == '&') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
            return;
        } else {
            szBuffer[0] = firstChar;
            szBuffer[1] = '\0';
            pAnalyzer->pCurrent++;
            assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
            return;
        }
    case '|':
        secondChar = *(pAnalyzer->pCurrent + 1);
        if (secondChar == '|') {
            szBuffer[0] = firstChar;
            szBuffer[1] = secondChar;
            szBuffer[2] = '\0';
            pAnalyzer->pCurrent += 2;
            assignToken(pAnalyzer, TOKEN_OPERATOR, szBuffer, pSourceStart);
            return;
        } else {
            szBuffer[0] = firstChar;
            szBuffer[1] = '\0';
            pAnalyzer->pCurrent++;
            assignToken(pAnalyzer, TOKEN_UNDEFINED, szBuffer, pSourceStart);
            return;
        }
    }

    /* Line end */
    if (firstChar == '\0') {
        assignToken(pAnalyzer, TOKEN_LINE_END, "", pSourceStart);
        return;
    }
    
    /* Numeric */
    if (isDigit(firstChar)) {
        while (isDigit(*pAnalyzer->pCurrent)) {
            *pBuffer++ = *pAnalyzer->pCurrent++;
        }
        /* Check if it contains decimals. */
        if (*pAnalyzer->pCurrent == '.') {
            /* Preserve decimal point */
            *pBuffer++ = *pAnalyzer->pCurrent++;
            /* Continue writing numbers */
            while (isDigit(*pAnalyzer->pCurrent)) {
                *pBuffer++ = *pAnalyzer->pCurrent++;
            }
        }

        *pBuffer = '\0';

        assignToken(pAnalyzer, TOKEN_NUMERIC, szBuffer, pSourceStart);
        return;
    }
    /* Identifier */
    else if (isAlpha(firstChar)) {
        while (isAlphaNum(*pAnalyzer->pCurrent)) {
            *pBuffer++ = *pAnalyzer->pCurrent++;
        }
        *pBuffer = '\0';

        assignToken(pAnalyzer, TOKEN_IDENTIFIER, szBuffer, pSourceStart);
        return;
    }
    /* Undefined character */
    else {
        pAnalyzer->pCurrent++;
        szBuffer[0] = firstChar;
        szBuffer[1] = '\0';
        assignToken(pAnalyzer, TOKEN_UNDEFINED, szBuffer, pSourceStart);
        return;
    }
}

void FzAnalyzer_RewindToken(FzLineAnalyzer* pAnalyzer) {
    pAnalyzer->pCurrent -= pAnalyzer->token.iSourceLength;
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