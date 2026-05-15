#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

typedef enum {
    PZF_NONE = 0,
    PZF_SIN,
    PZF_COS,
    PZF_TAN,
    PZF_ASIN,
    PZF_ACOS,
    PZF_ATAN,
    PZF_SQR,
    PZF_EXP,
    PZF_ABS,
    PZF_LOG,
    PZF_LN
} PzFuncIndex;

typedef struct {
    char* szName;
    int iNumArguments;
    PzFuncIndex iIndex;
} PzFuncMeta;

const PzFuncMeta* Constant_GetFunctionMetadata(const char* szFuncName);
const PzFuncMeta* Constant_GetFunctionMetadataByIndex(PzFuncIndex iIndex);

#endif