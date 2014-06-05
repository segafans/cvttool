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

/*---------------------- Local function declaration ---------------------*/
static int locParse(char *psBuf, H_LIST hList);
static T_Value * valueNew(E_TYPE eType, char *psValue, int iLen);

static void valueStringDebug(T_Value *ptValue);
static void valueBitMapDebug(T_Value *ptValue);
static int printfChar(unsigned char caChar);
static char * getBitMap(char *psBuf, int *piLen);
static void valueNodeStartDebug(T_Value *ptValue);
static void valueNodeEndDebug(T_Value *ptValue);
static char * getName(char *psBuf, int *piLen);
static int getLoop(char *psBuf, int *piLen);

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

    locParse("1(?:<test>23456)*[a-z1-9A\\-Z]*890.{10}", hList);

    LIST_LOOP(hList, f_fncDebug[((T_Value *)ptIter)->eType](ptIter));

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
            listAdd(hList, valueNew(TYPE_NODE_END, NULL, 0));
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
    printf("[string][%d]%s\n", ptValue->iLoop, ptValue->psValue);
}

static void valueNodeStartDebug(T_Value *ptValue)
{
    printf("[node start][%d]", ptValue->iLoop);
    if (NULL != ptValue->psValue) {
        printf("%s", ptValue->psValue);
    }

    printf("\n");
}

static void valueNodeEndDebug(T_Value *ptValue)
{
    printf("[node end]\n");
}

static void valueBitMapDebug(T_Value *ptValue)
{
    printf("[bitmap][%d]", ptValue->iLoop);

    unsigned int i = 0;
    for (i=1; i<=128; i++) {
        if (BITMAP_TEST(ptValue->psValue, i)) {
            printfChar(i);
        } else {
            printfChar(0);
        }
    }
    printfChar(0);

    printf("\n");
}

static int printfChar(unsigned char caChar)
{
    static unsigned char caFrist = 0;
    static unsigned char caLast = 0;

    if (0 == caChar) {
        if (0 == caFrist) {
            return 0;
        }

        if (isprint(caFrist)) {
            printf("%c", caFrist);
        } else {
            printf("\\%03d", caFrist);
        }

        if (0 != caLast) {
            if (caFrist + 1 != caLast) {
                printf("-");
            }

            if (isprint(caLast)) {
                printf("%c", caLast);
            } else {
                printf("\\%03d", caLast);
            }
        }

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

/*-----------------------------  End ------------------------------------*/

