#ifndef _UTILS_H_
#define _UTILS_H_

#ifndef BOOL
#   define BOOL int
#endif

#ifndef TRUE
#   define TRUE 1
#endif

#ifndef FALSE
#   define FALSE 0
#endif

#define PZ_FLOAT float

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