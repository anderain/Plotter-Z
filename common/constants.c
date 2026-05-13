#include "utils.h"
#include "constants.h"

const static PzFuncMeta FunctionMetadata[] = {
    { "N/A",    0,  PZF_NONE    },
    { "sin",    1,  PZF_SIN     },
    { "cos",    1,  PZF_COS     },
    { "tan",    1,  PZF_TAN     },
    { "asin",   1,  PZF_ASIN    },
    { "acos",   1,  PZF_ACOS    },
    { "atan",   1,  PZF_ATAN    },
    { "sqr",    1,  PZF_SQR     },
    { "exp",    1,  PZF_EXP     },
    { "abs",    1,  PZF_ABS     },
    { "log",    1,  PZF_LOG     },
    { "ln",     1,  PZF_LN      }
};

const PzFuncMeta* Constant_GetFunctionMetadata(const char* szFuncName) {
    static const int iSize = sizeof(FunctionMetadata) / sizeof(FunctionMetadata[0]);
    int i;
    for (i = 0; i < iSize; ++i) {
        if (Utils_IsStringEqual(szFuncName, FunctionMetadata[i].szName)) {
            return FunctionMetadata + i;
        }
    }
    return (void *)0;
}

const PzFuncMeta* Constant_GetFunctionMetadataByIndex(PzFuncIndex iIndex) {
    static const int iSize = sizeof(FunctionMetadata) / sizeof(FunctionMetadata[0]);
    if (iIndex <= 0 || iIndex >= iSize) return FunctionMetadata + 0;
    return FunctionMetadata + iIndex;
}