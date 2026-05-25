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
#define PZ_E    ((PZ_FLOAT) 2.7182818f)

/*====================================================
 * Fixed-point arithmetic (Q2.14)
 * PZ_FIXED = short, operations upcast to int
 *====================================================*/
#ifndef PZ_FIXED
#define PZ_FIXED short
#endif

#ifndef PZ_FIXED_LONG
#   if defined(__MWERKS__) && (defined(__MC68K__) || defined(__PALMOS_TRAPS__))
#       define PZ_FIXED_LONG long
#   else
#       define PZ_FIXED_LONG int
#   endif
#endif

#define PZ_FIXED_SHIFT     14
#define PZ_FIXED_ONE       ((PZ_FIXED_LONG)(1 << PZ_FIXED_SHIFT))
#define PZ_FIXED_HALF      ((PZ_FIXED_LONG)(1 << (PZ_FIXED_SHIFT - 1)))
#define PZ_FIXED_NEG_ONE   (-PZ_FIXED_ONE)

#define PZ_FLOAT_TO_FIXED(f)            ((PZ_FIXED)((0.5) + (float)(f) * PZ_FIXED_ONE))
#define PZ_FIXED_TO_FLOAT(x)            ((float)(PZ_FIXED_LONG)(x) / PZ_FIXED_ONE)
#define PZ_PZ_FIXED_LONG_TO_FIXED(n)    ((PZ_FIXED)((PZ_FIXED_LONG)(n) << PZ_FIXED_SHIFT))
#define PZ_FIXED_MUL(a, b)              ((PZ_FIXED_LONG)(((PZ_FIXED_LONG)(a) * (PZ_FIXED_LONG)(b) + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT))
#define PZ_FIXED_DIV(a, b)              ((PZ_FIXED_LONG)((((PZ_FIXED_LONG)(a)) << PZ_FIXED_SHIFT) / (PZ_FIXED_LONG)(b)))

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