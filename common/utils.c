#include <stdlib.h>
#include <string.h>
#include "utils.h"

int Utils_StringViewCompareString(const StringView* pStrView, const char* sz) {
    const char* p1 = (pStrView && pStrView->pBegin) ? pStrView->pBegin : "";
    int len = (pStrView && pStrView->iLen > 0) ? pStrView->iLen : 0;
    const char* p2 = sz ? sz : "";

    int i = 0;
    while (i < len && p2[i] != '\0') {
        if (p1[i] != p2[i]) {
            return (int)p1[i] - (int)p2[i];
        }
        ++i;
    }

    if (i == len && p2[i] == '\0') {
        return 0; 
    } else if (i == len) {
        return -(int)p2[i];
    } else {
        return (int)p1[i];
    }
}

void Utils_StringViewCopy(StringView* pDest, const StringView* pSource) {
    if (pSource == NULL) {
        pDest->pBegin = NULL;
        pDest->iLen = 0;
    } else {
        /* memcpy(pDest, sizeof(StringView), pSource); */
        pDest->pBegin = pSource->pBegin;
        pDest->iLen = pSource->iLen;
    }
}

void Utils_StringViewCopyToBuffer(char* pBuf, int iBufSize, const StringView* pSource) {
    int i;
    for (i = 0; i < iBufSize - 1 && i < pSource->iLen; ++i) {
        pBuf[i] = pSource->pBegin[i];
    }
    pBuf[i] = '\0';
}

char* Utils_StringViewDump(const StringView* pSource) {
    int iSize = pSource->iLen + 1;
    char* pBuf = (char *)malloc(iSize);
    Utils_StringViewCopyToBuffer(pBuf, iSize, pSource);
    return pBuf;
}

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

/*****************************************
 * Camera
 *****************************************/
#define ZOOM_LEVEL_DEFAULT  4
#define FOV_LEVEL_DEFAULT   6
#define DEFAULT_VIEW_ALPHA  330
#define DEFAULT_VIEW_BETA   30

PzCamera Camera;

const PzCamera DefaultCamera = {
    /* Viewport X, Y, S */  0, 0, 0,
    /* Alpha, Beta */       DEFAULT_VIEW_ALPHA, DEFAULT_VIEW_BETA,
    /* X Min, Max, Grid */  -6.0f, 6.0f, 20,
    /* Y Min, Max, Grid */  -6.0f, 6.0f, 20,
    /* Z Min, Max */        -3.0f, 3.0f,
    /* U Min, Max, Grid */  0, 2 * PZ_PI, 20,
    /* V Min, Max, Grid */  0, 2 * PZ_PI, 20,
    /* Zoom Level */        ZOOM_LEVEL_DEFAULT,
    /* Fov Level */         FOV_LEVEL_DEFAULT
};

const PZ_FLOAT fZoomLevels[] = {
    0.3f,
    0.4f,
    0.5f,
    0.6f,
    0.7f,
    0.8f,
    0.9f,
    1.0f,
    1.25f,
    1.5f,
    2.0f,
    4.0f,
    6.0f
};

const PZ_FIXED iZoomLevels[] = {
    PZ_FLOAT_TO_FIXED(0.30f),
    PZ_FLOAT_TO_FIXED(0.40f),
    PZ_FLOAT_TO_FIXED(0.50f),
    PZ_FLOAT_TO_FIXED(0.60f),
    PZ_FLOAT_TO_FIXED(0.70f),
    PZ_FLOAT_TO_FIXED(0.80f),
    PZ_FLOAT_TO_FIXED(0.90f),
    (int)PZ_FIXED_ONE,
    PZ_FLOAT_TO_FIXED(1.25f),
    PZ_FLOAT_TO_FIXED(1.50f),
    (int)PZ_FIXED_ONE * 2,
	(int)PZ_FIXED_ONE * 4,
	(int)PZ_FIXED_ONE * 6
};

const char* szZoomLevels[] = {
    "30",
    "40",
    "50",
    "60",
    "70",
    "80",
    "90",
    "100",
    "125",
    "150",
    "200",
    "400",
    "600"
};

const int iNumZoomLevel = sizeof(iZoomLevels) / sizeof(iZoomLevels[0]);

void PzCamera_Initialize() {
    memset(&Camera, 0, sizeof(Camera));
    memcpy(&Camera, &DefaultCamera, sizeof(Camera));
}

void PzCamera_Reset(int iViewportX, int iViewportY) {
    Camera.iViewportX = iViewportX;
    Camera.iViewportY = iViewportY;
    Camera.iZoomLevel = ZOOM_LEVEL_DEFAULT;
    Camera.iFovLevel = FOV_LEVEL_DEFAULT;
    Camera.iBetaDeg = DEFAULT_VIEW_BETA;
    Camera.iAlphaDeg = DEFAULT_VIEW_ALPHA;
}

void PzCamera_OrthoProjectFloat(PZ_FLOAT x, PZ_FLOAT y, PZ_FLOAT z, int *ox, int *oy) {
    PZ_FLOAT scale = Camera.iViewportS * fZoomLevels[Camera.iZoomLevel];
    PZ_FLOAT nx = y * Camera.uTrigBuf.sFloat.sinB - x * Camera.uTrigBuf.sFloat.cosB;
    PZ_FLOAT ny = (x * Camera.uTrigBuf.sFloat.sinB + y * Camera.uTrigBuf.sFloat.cosB) * Camera.uTrigBuf.sFloat.sinA - z * Camera.uTrigBuf.sFloat.cosA;
    *ox = (int)(Camera.iViewportX + scale * nx);
    *oy = (int)(Camera.iViewportY + scale * ny);
}

void PzCamera_OrthoProjectFixed(PZ_FIXED x, PZ_FIXED y, PZ_FIXED z, int *ox, int *oy) {
    PZ_FIXED_LONG iZoom = iZoomLevels[Camera.iZoomLevel];
    PZ_FIXED_LONG iScale = ((Camera.iViewportS * iZoom + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
    PZ_FIXED_LONG nx, ny;

    nx = PZ_FIXED_MUL(y, Camera.uTrigBuf.sFixed.sinB) - PZ_FIXED_MUL(x, Camera.uTrigBuf.sFixed.cosB);
    ny = PZ_FIXED_MUL(PZ_FIXED_MUL(x, Camera.uTrigBuf.sFixed.sinB) + PZ_FIXED_MUL(y, Camera.uTrigBuf.sFixed.cosB), Camera.uTrigBuf.sFixed.sinA)
       - PZ_FIXED_MUL(z, Camera.uTrigBuf.sFixed.cosA);
    
    *ox = Camera.iViewportX + (int)((iScale * nx + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
    *oy = Camera.iViewportY + (int)((iScale * ny + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
}

void PzCamera_PerspProjectFloat(PZ_FLOAT x, PZ_FLOAT y, PZ_FLOAT z, int *ox, int *oy) {
    PZ_FLOAT nx, y1, ny, nz;
    PZ_FLOAT scale, projX, projY;

    /* Rotate around Y axis (beta) */
    nx = y * Camera.uTrigBuf.sFloat.sinB - x * Camera.uTrigBuf.sFloat.cosB;
    y1 = x * Camera.uTrigBuf.sFloat.sinB + y * Camera.uTrigBuf.sFloat.cosB;

    /* Rotate around X axis (alpha) */
    ny = y1 * Camera.uTrigBuf.sFloat.sinA - z * Camera.uTrigBuf.sFloat.cosA;
    nz = y1 * Camera.uTrigBuf.sFloat.cosA + z * Camera.uTrigBuf.sFloat.sinA;

    /* Perspective division */
    nz += Camera.iFovLevel;
    if (nz < 0.1f) nz = 0.1f;

    scale = (Camera.iFovLevel / nz) * Camera.iViewportS * fZoomLevels[Camera.iZoomLevel];
    projX = nx * scale;
    projY = ny * scale;

    *ox = projX + Camera.iViewportX;
    *oy = projY + Camera.iViewportY;
}

void PzCamera_PerspProjectFixed(PZ_FIXED x, PZ_FIXED y, PZ_FIXED z, int *ox, int *oy) {
    PZ_FIXED nx, y1, ny, nz;
    PZ_FIXED_LONG iZoom, iFov;
    PZ_FIXED_LONG iFovFixed, iDiv, iScale, iCombined;

    /* Rotate around Y axis (beta) */
    nx = PZ_FIXED_MUL(y, Camera.uTrigBuf.sFixed.sinB) - PZ_FIXED_MUL(x, Camera.uTrigBuf.sFixed.cosB);
    y1 = PZ_FIXED_MUL(x, Camera.uTrigBuf.sFixed.sinB) + PZ_FIXED_MUL(y, Camera.uTrigBuf.sFixed.cosB);

    /* Rotate around X axis (alpha) */
    ny = PZ_FIXED_MUL(y1, Camera.uTrigBuf.sFixed.sinA) - PZ_FIXED_MUL(z, Camera.uTrigBuf.sFixed.cosA);
    nz = PZ_FIXED_MUL(y1, Camera.uTrigBuf.sFixed.cosA) + PZ_FIXED_MUL(z, Camera.uTrigBuf.sFixed.sinA);

    /* Perspective division */
    iFov = Camera.iFovLevel;
    nz += (PZ_FIXED)((PZ_FIXED_LONG)iFov << PZ_FIXED_SHIFT);
    if (nz < 1) nz = 1;

    iFovFixed = (PZ_FIXED_LONG)iFov << PZ_FIXED_SHIFT;
    iDiv = ((iFovFixed << PZ_FIXED_SHIFT) / (PZ_FIXED_LONG)nz);
    iZoom = iZoomLevels[Camera.iZoomLevel];
    iScale = ((Camera.iViewportS * iZoom + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
    iCombined = ((iScale * iDiv + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);

    *ox = Camera.iViewportX + (int)((iCombined * (PZ_FIXED_LONG)nx + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
    *oy = Camera.iViewportY + (int)((iCombined * (PZ_FIXED_LONG)ny + PZ_FIXED_HALF) >> PZ_FIXED_SHIFT);
}
