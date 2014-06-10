/*
 * FileName: poc.c
 *
 *  <Date>        <Author>       <Auditor>     <Desc>
 */
/*--------------------------- Include files -----------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

#include "list.h"

/*--------------------------- Macro define ------------------------------*/
#define _DLEN_TINY_BUF  64
#define _DLEN_MINI_BUF  256
#define _DLEN_LARGE_BUF 1024
#define _DLEN_HUGE_BUF  10240

#define _DLEN_BITMAP     16
#define _NUM_BITMAP      _DLEN_BITMAP*8

#define BITMAP_MASK( bit )         (0x80>>(((bit)-1)&0x07))
#define BITMAP_INDEX( bit )        (((bit)-1)>>3)
#define BITMAP_TEST( bitmap, bit ) ((bitmap)[BITMAP_INDEX(bit)]&BITMAP_MASK(bit))
#define BITMAP_CLR( bitmap, bit )  (bitmap[BITMAP_INDEX(bit)]&=~BITMAP_MASK(bit))
#define BITMAP_SET( bitmap, bit )  (bitmap[BITMAP_INDEX(bit)]|=BITMAP_MASK(bit))

#define __ printf

/*---------------------------- Type define ------------------------------*/
typedef enum {
    TYPE_STRING,
    TYPE_BITMAP,
    TYPE_NODE_START,
    TYPE_NODE_END,
    TYPE_NUM
} E_TYPE;

typedef struct {
    char *psValue;
    int iLoop;
    E_TYPE eType;
} T_Value;

typedef void (*FNC_DEBUG)(T_Value *ptValue);
static int f_Level = 0;

/*---------------------- Local function declaration ---------------------*/
static int locParse(char *psBuf, H_LIST hList);
static T_Value * valueNew(E_TYPE eType, char *psValue, int iLen);

static void valueStringDebug(T_Value *ptValue);
static void valueBitMapDebug(T_Value *ptValue);
static int storeChar(unsigned char caChar, unsigned char *psFrist, unsigned char *psLast);
static char * getBitMap(char *psBuf, int *piLen);
static void valueNodeStartDebug(T_Value *ptValue);
static void valueNodeEndDebug(T_Value *ptValue);
static char * getName(char *psBuf, int *piLen);
static int getLoop(char *psBuf, int *piLen);
static int printfChar(unsigned char caFrist, unsigned char caLast, int *piFlag);
static int _s();
static int _(char *psBuf, ...);

/*-------------------------  Global variable ----------------------------*/
FNC_DEBUG f_fncDebug[TYPE_NUM] = {
    valueStringDebug,
    valueBitMapDebug,
    valueNodeStartDebug,
    valueNodeEndDebug
};


/*-------------------------  Global functions ---------------------------*/
int main(int argc, char *argv[])
{
    H_LIST hList = listNew();

    locParse("1(?:<test>23456)*[a-z]*890.{10}", hList);

    _("#include <string.h>");
    _("#include <stdio.h>");
    _("");
    
    _("int locParse(char *psBuf)");
    _("{");
    f_Level = 1;
    _("int iPos = 0;");
    LIST_LOOP(hList, f_fncDebug[((T_Value *)ptIter)->eType](ptIter));
    _("return 0;");
    f_Level = 0;
    _("}");

    _("");
    _("int main(int argc, char *argv[]) {");
    _("    printf(\"%%d\\n\", locParse(\"123456a8901a\"));");
    _("}");

    return 0;
}

/*-------------------------  Local functions ----------------------------*/
static int locParse(char *psBuf, H_LIST hList)
{
    T_Value *ptLast = NULL;
    T_Value *ptLastNode = NULL;
    char *pStart= psBuf;
    char *pCur = psBuf;

    for (;'\0'!=*pCur;pCur++) {
        char caTemp = *pCur;
        if (caTemp != '.' && caTemp != '[' && caTemp != '*' && caTemp != '(' && caTemp != ')' && caTemp != '{') {
            continue;
        }

        if ('*' == caTemp || '{' == caTemp) {
            int iLoop = -1;
            int iLen = 0;
            if ('{' == caTemp) {
                iLoop = getLoop(pCur+1, &iLen);
            }

            if (pCur == pStart && ptLast != NULL) {
                ptLast->iLoop = iLoop;
            }

            if (pCur != pStart) {
                listAdd(hList, valueNew(TYPE_STRING, pStart, (int)(pCur-pStart)-1));
                T_Value *ptValue = valueNew(TYPE_STRING, pCur-1, 1);
                ptValue->iLoop = iLoop;
                listAdd(hList, ptValue);
            }

            pStart = pCur + iLen + 1;
            continue;
        }

        if (pCur != pStart) {
            listAdd(hList, valueNew(TYPE_STRING, pStart, (int)(pCur-pStart)));
        }

        if ('.' == caTemp) {
            ptLast = valueNew(TYPE_BITMAP, getBitMap(NULL, NULL), _DLEN_BITMAP);
            listAdd(hList, ptLast);
        } else if ('[' == caTemp) {
            int iLen = 0;
            ptLast = valueNew(TYPE_BITMAP, getBitMap(pCur+1, &iLen), _DLEN_BITMAP);
            listAdd(hList, ptLast);
            pCur += iLen + 1;
        } else if ('(' == caTemp) {
            char *psName = NULL;
            int iLen = 0;
            if ('?' == *(pCur+1)) {
                psName = getName(pCur+2, &iLen);
            }
            ptLastNode = valueNew(TYPE_NODE_START, psName, iLen);
            listAdd(hList, ptLastNode);
            if (psName != NULL) {
                pCur += iLen + 4;
            }
        } else if (')' == caTemp) {
            listAdd(hList, valueNew(TYPE_NODE_END, ptLastNode->psValue, -1));
            ptLast = ptLastNode;
            ptLastNode = NULL;
        }

        pStart = pCur+1;
    }

    if (pCur != pStart) {
        listAdd(hList, valueNew(TYPE_STRING, pStart, (int)(pCur-pStart)));
    }

    return 0;
}

static int getLoop(char *psBuf, int *piLen)
{
    char sLen[_DLEN_TINY_BUF];
    memset(sLen, '\0', sizeof(sLen));

    int iLen = 0;
    while (psBuf[iLen] != '}') {
        sLen[iLen] = psBuf[iLen];
        iLen += 1;
    }
    *piLen = iLen+1;

    return atoi(sLen);
}

static char * getName(char *psBuf, int *piLen)
{
    char *psName = NULL;

    if (':' != psBuf[0] || '<' != psBuf[1]) {
        return NULL;
    }

    psName = psBuf + 2;
    int iLen = 0;
    while (psName[iLen]!='>' && psName[iLen]!='\0') {
        iLen += 1;
    }
    *piLen = iLen;

    return psName;
}

static char * getBitMap(char *psBuf, int *piLen)
{
    static char sBitMap[_DLEN_BITMAP];

    memset(sBitMap, '\0', sizeof(sBitMap));
    if (NULL == psBuf) {
        unsigned int i = 1;
        for (i=1; i<=_NUM_BITMAP; i++) {
            BITMAP_SET(sBitMap, i);
        }
    } else {
        int iLen = 0;
        for (; psBuf[iLen] != ']' && psBuf[iLen] != '\0'; ++iLen) {
            if (iLen != 0 && '-' == psBuf[iLen]) {
                if (psBuf[iLen-1] == psBuf[iLen+1]) {
                    iLen += 1;
                    continue;
                }

                unsigned char caStart = psBuf[iLen-1];
                unsigned char caEnd   = psBuf[iLen+1];

                if (caStart > caEnd) {
                    caStart = psBuf[iLen+1];
                    caEnd   = psBuf[iLen-1];
                }

                for (; caStart<=caEnd; caStart++) {
                    BITMAP_SET(sBitMap, caStart);
                }
                continue;
            } else if ( '\\' == psBuf[iLen] ) {
                iLen += 1;
            }

            BITMAP_SET(sBitMap, (unsigned char )psBuf[iLen]);
        }
        *piLen = iLen;
    }

    return sBitMap;
}

static T_Value * valueNew(E_TYPE eType, char *psValue, int iLen)
{
    T_Value *ptValue = malloc(sizeof(T_Value));
    ptValue->eType = eType;
    ptValue->iLoop = 1;

    if (iLen < 0) {
        iLen = strlen(psValue);
    }

    ptValue->psValue = NULL;
    if (NULL != psValue && 0 != iLen) {
        ptValue->psValue = malloc(iLen+1);
        memcpy(ptValue->psValue, psValue, iLen);
        ptValue->psValue[iLen] = '\0';
    }

    return ptValue;
}

static void valueStringDebug(T_Value *ptValue)
{
    _("/* [string][%d]%s */", ptValue->iLoop, ptValue->psValue);
    _("if (0 != memcmp(psBuf+iPos, \"%s\", %ld)) {", ptValue->psValue, strlen(ptValue->psValue));
    _("    return -1;");
    _("}");
    _("iPos += %ld;", strlen(ptValue->psValue));
    _("");
}

static void valueNodeStartDebug(T_Value *ptValue)
{
    _("/* [node start][%d]%s */", ptValue->iLoop, ptValue->psValue);

    _("char *s%s = psBuf+iPos;", ptValue->psValue);
    _("");
}

static void valueNodeEndDebug(T_Value *ptValue)
{
    _("/* [node end] %s*/", ptValue->psValue);

    _("int i%sLen = psBuf + iPos - s%s;", ptValue->psValue, ptValue->psValue);
    _("");
}

static void valueBitMapDebug(T_Value *ptValue)
{
    unsigned char caFrist;
    unsigned char caLast;
    _s();__("/* [bitmap][%d]", ptValue->iLoop);

    unsigned int i = 0;
    for (i=1; i<=128; i++) {
        if (BITMAP_TEST(ptValue->psValue, i)) {
            storeChar(i, NULL, NULL);
        } else {
            storeChar(0, &caFrist, &caLast);
            if ('\0' != caFrist) {
                if ('\0' == caLast) {
                    __("0x%03d", caFrist);
                } else {
                    __("0x%03d-0x%03d", caFrist, caLast);
                }
            }
        }
    }
    storeChar(0, &caFrist, &caLast);
    if ('\0' != caFrist) {
        if ('\0' == caLast) {
            __("0x%03d", caFrist);
        } else {
            __("0x%03d-0x%03d", caFrist, caLast);
        }
    }

    __(" */\n");

    _s();__("if (");
    int iFlag = 0;
    for (i=1; i<=128; i++) {
        if (BITMAP_TEST(ptValue->psValue, i)) {
            storeChar(i, NULL, NULL);
        } else {
            storeChar(0, &caFrist, &caLast);
            printfChar(caFrist, caLast, &iFlag);
        }
    }
    storeChar(0, &caFrist, &caLast);
    printfChar(caFrist, caLast, &iFlag);

    __(") {\n");

    _("    return -1;");
    _("}");
    _("iPos += 1;");
    _("");
}

static int printfChar(unsigned char caFrist, unsigned char caLast, int *piFlag)
{
    char sFrist[6];
    char sLast[6];

    if ('\0' == caFrist) {
        return 0;
    }

    if (*piFlag) {
        __(" && ");
    }

    if (isprint(caFrist)) {
        sprintf(sFrist, "'%c'", caFrist);
    } else {
        sprintf(sFrist, "0x%03d", caFrist);
    }

    *piFlag = 1;
    if ('\0' == caLast) {
        _("(psBuf[iPos] != %s)", sFrist);
    } else {
        if (isprint(caLast)) {
            sprintf(sLast, "'%c'", caLast);
        } else {
            sprintf(sLast, "0x%03d", caLast);
        }
        __("(psBuf[iPos] < %s || psBuf[iPos] > %s)", sFrist, sLast);
    }

    return 0;
}

static int storeChar(unsigned char caChar, unsigned char *psFrist, unsigned char *psLast)
{
    static unsigned char caFrist = 0;
    static unsigned char caLast = 0;

    if (0 == caChar) {
        *psFrist = caFrist;
        *psLast = caLast;

        caFrist = 0;
        caLast = 0;

        return 0;
    }

    if (0 == caFrist) {
        caFrist = caChar;
        return 0;
    }
    
    caLast = caChar;
    
    return 0;
}

static int _s()
{
    int i = 0;
    for (i=0; i<f_Level; i++) {
        printf("\t");
    }

    return 0;
}

static int _(char *psBuf, ...)
{
    va_list	vaArgs;
	int		iRet;

    _s();

	va_start(vaArgs, psBuf);
	iRet = vprintf(psBuf, vaArgs);
	va_end(vaArgs);
    printf("\n");

	return iRet;
}

/*-----------------------------  End ------------------------------------*/

