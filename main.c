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
static int f_Index = 1;
static int f_Loop = 0;

/*---------------------- Local function declaration ---------------------*/
static int locParse(char *psBuf, H_LIST hList);
static T_Value * valueNew(E_TYPE eType, char *psValue, int iLen);

static void valueStringDebug(T_Value *ptValue);
static void valueBitMapDebug(T_Value *ptValue);
static int storeChar(unsigned char caChar, unsigned char *psFrist, unsigned char *psLast);
static char * getBitMap(char *psBuf, int *piLen);
static void valueNodeStartDebug(T_Value *ptValue);
static void valueNodeEndDebug(T_Value *ptValue);
static char * setLoop(T_Value *ptValue, H_LIST ptList, char *pCur);
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

    locParse("1(?:23456)*([a-z]*890).{10}", hList);

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
    T_Value *ptLastNode = NULL;
    char *pCur = psBuf;

    while ('\0'!=*pCur) {

        /* normal string */
        char *pStart = pCur;
        while (1) {
            char caTemp = *pCur;
            if (caTemp != '.' && caTemp != '[' && caTemp != '(' && caTemp != ')' && caTemp != '{') {
                pCur += 1;
                continue;
            }

            break;
        }

        if (pStart != pCur) {
            pCur = setLoop(valueNew(TYPE_STRING, pStart, (int)(pCur-pStart)), hList, pCur);
        }

        /* . */
        if ('.' == *pCur) {
            pCur = setLoop(valueNew(TYPE_BITMAP, getBitMap(NULL, NULL), _DLEN_BITMAP), hList, pCur+1);
            continue;
        }

        /* [....] */
        if ('[' == *pCur) {
            int iLen = 0;
            T_Value * ptLast = valueNew(TYPE_BITMAP, getBitMap(pCur+1, &iLen), _DLEN_BITMAP);
            pCur = setLoop(ptLast, hList, pCur + iLen + 2);
            continue;
        }

        /* ( | (?: */
        if ('(' == *pCur) {
            if (ptLastNode != NULL) {
                return -1;
            }
            
            char *psName = NULL;
            int iLen = 0;
            if ('?' != *(pCur+1) || ':' != *(pCur+2)) {
                psName = malloc(20);
                iLen = sprintf(psName, "Var%d", f_Index);
                f_Index += 1;
                pCur += 1;
            } else {
                pCur += 3;
            }
            ptLastNode = valueNew(TYPE_NODE_START, psName, iLen);
            listAdd(hList, ptLastNode);
            continue;
        }

        /*  ) */
        if (')' == *pCur) {
            if (NULL == ptLastNode) {
                return -1;
            }

            T_Value *ptLast = valueNew(TYPE_NODE_END, ptLastNode->psValue, -1);
            pCur = setLoop(ptLast, NULL, pCur+1);
            ptLastNode->iLoop = ptLast->iLoop;
            listAdd(hList, ptLast);
            ptLastNode = NULL;
            continue;
        }
    }

    return 0;
}

static char * setLoop(T_Value *ptValue, H_LIST hList, char *pCur)
{
    if ('*' != *pCur && '{' != *pCur) {
        listAdd(hList, ptValue);
        return pCur;
    }

    int iLoop = -1;
    int iLen = 0;
    if ('{' == *pCur) {
        iLoop = getLoop(pCur+1, &iLen);
    }

    if (NULL == hList) {
        ptValue->iLoop = iLoop;
    } else {
        T_Value *ptStart = valueNew(TYPE_NODE_START, NULL, 0);
        T_Value *ptEnd = valueNew(TYPE_NODE_END, NULL, 0);

        ptStart->iLoop = iLoop;
        ptEnd->iLoop = iLoop;

        listAdd(hList, ptStart);
        listAdd(hList, ptValue);
        listAdd(hList, ptEnd);
    }

    return pCur + iLen + 1;
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

    if (iLen < 0 && psValue != NULL) {
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
    _("/* [string] \"%s\" */", ptValue->psValue);

    if (1 == strlen(ptValue->psValue)) {
        _("if ('%c' != psBuf[iPos]) {", ptValue->psValue[0]);
    } else {
        _("if (0 != memcmp(psBuf+iPos, \"%s\", %ld)) {", ptValue->psValue, strlen(ptValue->psValue));
    }

    if (f_Loop) {
        _("    break;");
    } else {
        _("    return -1;");
    }
    _("}");
    _("iPos += %ld;", strlen(ptValue->psValue));

    _("");
}

static void valueNodeStartDebug(T_Value *ptValue)
{
    if (ptValue->iLoop != 1) {
        if (ptValue->iLoop < 0) {
            _("while (1) {", ptValue->iLoop);
            f_Loop = 1;
        } else {
            _("for (i=0; i<%d; i++) {", ptValue->iLoop);
        }
        f_Level += 1;
    }

    if (NULL != ptValue->psValue) {
        _("/* [get start][%d]%s */", ptValue->iLoop, ptValue->psValue);
        _("char *s%s = psBuf+iPos;", ptValue->psValue);
        _("");
    }
}

static void valueNodeEndDebug(T_Value *ptValue)
{
    if (NULL != ptValue->psValue) {
        _("/* [get end] %s */", ptValue->psValue);
        _("int iLen%s = psBuf + iPos - s%s;", ptValue->psValue, ptValue->psValue);
        _("");
    }

    if (ptValue->iLoop != 1) {
        f_Loop = 0;
        f_Level -= 1;
        _("}");
        _("");
    }
}

static void valueBitMapDebug(T_Value *ptValue)
{
    unsigned char caFrist;
    unsigned char caLast;
    _s();__("/* [bitmap]");

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

