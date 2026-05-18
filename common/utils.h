#ifndef _PZ_UTILS_H_
#define _PZ_UTILS_H_

#ifndef BOOL
#   define BOOL int
#endif

#ifndef TRUE
#   define TRUE 1
#endif

#ifndef FALSE
#   define FALSE 0
#endif

/*
 * PZ_FLOAT = float
 */
#define PZ_FLOAT float

#define PZ_PI   ((PZ_FLOAT)3.14159265f)

/*====================================================
 * Fixed-point arithmetic (Q2.14)
 * PZ_FIXED = short, operations upcast to int
 *====================================================*/
#ifndef PZ_FIXED
#define PZ_FIXED short
#endif

#define PZ_FIXED_SHIFT     14
#define PZ_FIXED_ONE       ((int)(1 << PZ_FIXED_SHIFT))
#define PZ_FIXED_HALF      ((int)(1 << (PZ_FIXED_SHIFT - 1)))
#define PZ_FIXED_NEG_ONE   (-PZ_FIXED_ONE)

#define PZ_FLOAT_TO_FIXED(f)   ((PZ_FIXED)((float)(f) * PZ_FIXED_ONE))
#define PZ_FIXED_TO_FLOAT(x)   ((float)(int)(x) / PZ_FIXED_ONE)
#define PZ_INT_TO_FIXED(n)     ((PZ_FIXED)((int)(n) << PZ_FIXED_SHIFT))
#define PZ_FIXED_MUL(a, b)     ((int)(((int)(a) * (int)(b) + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT))
#define PZ_FIXED_DIV(a, b)     ((int)((((int)(a)) << PZ_FIXED_SHIFT) / (int)(b)))

#define isDigit(c)      ((c) >= '0' && (c) <= '9')
#define isUppercase(c)  ((c) >= 'A' && (c) <= 'Z')
#define isLowercase(c)  ((c) >= 'a' && (c) <= 'z')
#define isHexAlphaU(c)  ((c) >= 'A' && (c) <= 'F')
#define isHexAlphaL(c)  ((c) >= 'a' && (c) <= 'f')
#define isHexDigit(c)   (isDigit(c) || isHexAlphaU(c) || isHexAlphaL(c))
#define isAlpha(c)      (isUppercase(c) || isLowercase(c) || (c) == '_')
#define isAlphaNum(c)   ((isDigit(c)) || (isAlpha(c)))
#define isSpace(c)      ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

int     Utils_StringCopy        (char *dest, int max, const char *src);
char*   Utils_StringDump        (const char *str);
char*   Utils_StringConcat      (const char* szLeft, const char* szRight);
BOOL    Utils_IsStringEqual     (const char* szA, const char* szB); 
BOOL    Utils_IsStringEndWith   (const char *str, const char *token);
double  Utils_Atof              (const char *str);
char*   Utils_Ftoa              (double f, char * buf, int precision);
long    Utils_Atoi              (const char* str);
char*   Utils_Itoa              (int num, char* str, int base);
int     Utils_IsLittleEndian    ();

#define DEFAULT_FTOA_PRECISION  8

#endif