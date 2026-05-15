#include <stdlib.h>
#include <string.h>
#include "utils.h"

int Utils_StringCopy(char *dest, int max, const char *src) {
    int i;
    for (i = 0; i < max - 1 && src[i]; ++i) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return i;
}

char* Utils_StringDump(const char *str) {
    int length = strlen(str) + 1;
    char *buffer = (char *)malloc(length);
    Utils_StringCopy(buffer, length + 1, str);
    return buffer;
}

int Utils_IsStringEqual(const char* szA, const char* szB) {
    return strcmp(szA, szB) == 0;
}

int Utils_IsStringEndWith(const char *str, const char *token) {
    if (str == NULL || token == NULL) {
        return 0;
    }
    else {
        int strLen = strlen(str);
        int tokenLen = strlen(token);
        
        if (tokenLen > strLen) {
            return 0;
        }

        return strcmp(str + strLen - tokenLen, token) == 0;
    }
}

char* Utils_StringConcat(const char* szLeft, const char* szRight) {
    char* szBuf;
    size_t lenLeft, lenRight, lenTotal;

    /* Parameter validation */
    if (szLeft == NULL || szRight == NULL) {
        return NULL;
    }

    lenLeft = strlen(szLeft);
    lenRight = strlen(szRight);
    lenTotal = lenLeft + lenRight;

    szBuf = (char*)malloc(lenTotal + 1);
    if (szBuf == NULL) {
        return NULL; /* Memory allocation failed */
    }

    /* Copy strings */
    memcpy(szBuf, szLeft, lenLeft);
    memcpy(szBuf + lenLeft, szRight, lenRight);

    /* Add null terminator */
    szBuf[lenTotal] = '\0';

    return szBuf;
}

double Utils_Atof(const char *str) {
    int sign;
    double number = 0.0, power = 1.0;
    /* Skip whitespace */
    while(isSpace(*str)) ++str;
    /* Handle sign */
    sign = (*str == '-') ? -1 : 1; 
    if (*str == '-' || *str == '+') {
        str++;
    }
    /* Process digits before decimal point */
    while(*str >= '0' && *str <= '9') {
        number = 10.0 * number + (*str - '0');
        str++;
    }
    /* Skip decimal point */
    if(*str == '.') {
        str++;
    }
    /* Process digits after decimal point */
    while(*str >= '0' && *str <= '9') {
        number = 10.0 * number + (*str - '0');
        power *= 10.0;
        str++;
    }
    return sign * number / power;
}

#define MAX_PRECISION   (10)

static const double rounders[MAX_PRECISION + 1] = {
    0.5,                /* 0 */
    0.05,               /* 1 */
    0.005,              /* 2 */
    0.0005,             /* 3 */
    0.00005,            /* 4 */
    0.000005,           /* 5 */
    0.0000005,          /* 6 */
    0.00000005,         /* 7 */
    0.000000005,        /* 8 */
    0.0000000005,       /* 9 */
    0.00000000005       /* 10 */
};

char* Utils_Ftoa(double f, char * buf, int precision) {
    char* ptr = buf;
    char* p = ptr;
    char* p1;
    char c;
    long intPart;

    /* Check precision boundaries */
    if (precision > MAX_PRECISION)
        precision = MAX_PRECISION;

    /* Handle sign */
    if (f < 0) {
        f = -f;
        *ptr++ = '-';
    }

    /* Negative precision indicates automatic precision detection */
    if (precision < 0) {
        if (f < 1.0) precision = 6;
        else if (f < 10.0) precision = 5;
        else if (f < 100.0) precision = 4;
        else if (f < 1000.0) precision = 3;
        else if (f < 10000.0) precision = 2;
        else if (f < 100000.0) precision = 1;
        else precision = 0;
    }

    /* Round value according to precision */
    if (precision)
        f += rounders[precision];

    /* Integer part... */
    intPart = (long)f;
    f -= intPart;

    if (!intPart)
        *ptr++ = '0';
    else {
        /* Save start pointer */
        p = ptr;

        /* Convert in reverse order */
        while (intPart) {
            *p++ = '0' + intPart % 10;
            intPart /= 10;
        }

        /* Save end position */
        p1 = p;

        /* Reverse the result */
        while (p > ptr) {
            c = *--p;
            *p = *ptr;
            *ptr++ = c;
        }

        /* Restore end position */
        ptr = p1;
    }

    /* Decimal part */
    if (precision) {
        /* Place decimal point */
        *ptr++ = '.';

        /* Perform conversion */
        while (precision--) {
            f *= 10.0;
            c = (int)f % 10;
            *ptr++ = '0' + c;
            f -= c;
        }
    }
    *ptr = 0;

    /* Remove trailing zeros from decimal part */
    if (precision) {
        int j = 0;
        char *cBuf = buf;
        for (j = strlen(cBuf) - 1; j > 0 && cBuf[j] == '0'; --j) {
            cBuf[j] = 0;
        }
        if (j >= 1 && cBuf[j] == '.') cBuf[j] = 0;
    }

    return buf;
}

/* Reimplement atoi(), some platforms lack this function */
long Utils_Atoi(const char* str) {
    long num = 0;
 
    int i = 0;
 
    /* Run until reaching end of string, */
    /* or current character is not a digit */
    while (str[i] && (str[i] >= '0' && str[i] <= '9')) {
        num = num * 10 + (str[i] - '0');
        i++;
    }
 
    return num;
}

/* String reversal function needed for itoa */
static void Utils_ItoaReverse(char str[], int length) {
    int start = 0;
    int end = length -1;
    while (start < end) {
        char t = *(str+start);
        *(str+start) = *(str+end);
        *(str+end) = t;
        start++;
        end--;
    }
}
 
/* Reimplement itoa(), some platforms lack this function */
char* Utils_Itoa(int num, char* str, int base) {
    int i = 0;
    int isNegative = 0;
 
    /* Specifically handle input being 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    /* In standard itoa(), negative numbers are only handled for decimal base */
    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }
 
    /* Process individual digits */
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    /* If number is negative, append "-" */
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0';
 
    /* Reverse the content */
    Utils_ItoaReverse(str, i);
 
    return str;
}

#define UTILS_DWORD     unsigned int
#define UTILS_BYTE      unsigned char

int Utils_IsLittleEndian() {
    UTILS_DWORD num = 1;
    UTILS_BYTE *bytePtr = (UTILS_BYTE *)&num;
    return *bytePtr == 1;
}