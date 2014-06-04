/*
 * FileName: poc.c
 *
 *  <Date>        <Author>       <Auditor>     <Desc>
 */
/*--------------------------- Include files -----------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nfa.h"
#include "list.h"

/*--------------------------- Macro define ------------------------------*/
#define _DLEN_TINY_BUF  64
#define _DLEN_MINI_BUF  256
#define _DLEN_LARGE_BUF 1024
#define _DLEN_HUGE_BUF  10240

#define _DLEN_KEY_MAX   128

/*---------------------------- Type define ------------------------------*/
typedef struct NfaStruct {
    H_NFA_NODE hStart;
    H_NFA_NODE hEnd;
} T_NfaStruct;

typedef struct InPut {
    char *psBuf;
    int iCurrent;
} T_InPut;

/*---------------------- Local function declaration ---------------------*/
static int locParse(H_NFA hNfa, T_NfaStruct *ptStruct, T_InPut *ptInPut);
static int locStructInit(H_NFA hNfa, T_NfaStruct *ptStruct);
static int locStructAdd(H_NFA hNfa, T_NfaStruct *ptDst, T_NfaStruct *ptSrc);
static int locInputInit(T_InPut *ptInPut, char *psBuffer);
static char locInputNext(T_InPut *ptInput);
static int locStructMerge(H_NFA hNfa, T_NfaStruct *ptDst, T_NfaStruct *ptSrc);
static int locStructInitKey(H_NFA hNfa, T_NfaStruct *ptStrcut, char caInput);
static int locStructSetMuti(H_NFA hNfa, T_NfaStruct *ptStruct);
static int locStructInitAllKey(H_NFA hNfa, T_NfaStruct *ptStrcut);
static int locInputTest(T_InPut *ptInput , char caChar);
static int locStructInitMuti(H_NFA hNfa, T_NfaStruct *ptStrcut, T_InPut *ptInput);

/*-------------------------  Global variable ----------------------------*/

/*-------------------------  Global functions ---------------------------*/
int main(int argc, char *argv[])
{
    int iRet = 0;

    T_InPut tInPut;
    locInputInit(&tInPut, "asdfasdf");

    H_NFA hNfa;
    hNfa = nfaNew();

    T_NfaStruct tStruct;
    tStruct.hStart = nfaNewNode(hNfa, NFA_NODE_TYPE_START);
    tStruct.hEnd = nfaNewNode(hNfa, NFA_NODE_TYPE_END);

    iRet = locParse(hNfa, &tStruct, &tInPut);
    if (iRet != 0) {
        nfaFree(hNfa);
        printf("locParse err[%d]", iRet);
        return -1;
    }

    nfaDebug(hNfa);
    nfaSimple(hNfa);
    nfaDebug(hNfa);
    nfaReindex(hNfa);
    nfaDebug(hNfa);

    H_NFA_NODE ptFrist = nfaGetStartNode(hNfa);
    locDebug(ptFrist);

    nfaFree(hNfa);

    return 0;
}

/*-------------------------  Local functions ----------------------------*/
static int locParse(H_NFA hNfa, T_NfaStruct *ptStruct, T_InPut *ptInPut)
{
    T_NfaStruct tLocStruct;
    T_NfaStruct tLast;
    memset(&tLocStruct, '\0', sizeof(T_NfaStruct));
    memset(&tLast, '\0', sizeof(T_NfaStruct));

    while (1) {
        char caTmp = locInputNext(ptInPut);
        if ('\0' == caTmp || ')' == caTmp) {
            locStructMerge(hNfa, ptStruct, &tLocStruct);
            break;
        } else if ('(' == caTmp) {
            locStructInit(hNfa, &tLast);
            locParse(hNfa, &tLast, ptInPut);
            locStructAdd(hNfa, &tLocStruct, &tLast);
        } else if ('[' == caTmp) {
            locStructInitMuti(hNfa, &tLast, ptInPut);
            locStructAdd(hNfa, &tLocStruct, &tLast);
        } else if ('|' == caTmp) {
            locStructMerge(hNfa, ptStruct, &tLocStruct);
            memset(&tLocStruct, '\0', sizeof(T_NfaStruct));
        } else if ('*' == caTmp) {
            if (NULL == tLast.hEnd || NULL == tLast.hStart) {
                locStructInitKey(hNfa, &tLast, caTmp);
                locStructAdd(hNfa, &tLocStruct, &tLast);
            } else {
                locStructSetMuti(hNfa, &tLast);
            }
        } else if ( '.' == caTmp ) {
            locStructInitAllKey(hNfa, &tLast);
            locStructAdd(hNfa, &tLocStruct, &tLast);
        } else if ( '\\' == caTmp ) {
            caTmp = locInputNext(ptInPut);
            locStructInitKey(hNfa, &tLast, caTmp);
            locStructAdd(hNfa, &tLocStruct, &tLast);
        } else {
            locStructInitKey(hNfa, &tLast, caTmp);
            locStructAdd(hNfa, &tLocStruct, &tLast);
        }
    }

    return 0;
}

static int locStructInitMuti(H_NFA hNfa, T_NfaStruct *ptStrcut, T_InPut *ptInput)
{
    char sKey[_DLEN_KEY_MAX+1];
    int i = 0;
    int iFlag = 0;

    if (locInputTest(ptInput, '^')) {
        iFlag = 1;
        locInputNext(ptInput);
    }

    while (1) {
        char caTmp = locInputNext(ptInput);
        if (']' == caTmp) {
            break;
        }

        if ('\\' == caTmp) {
            caTmp = locInputNext(ptInput);
            sKey[i++] = caTmp;
            continue;
        }

        if (i != 0 && '-' == caTmp) {
            char caFrist = sKey[i-1]+1;
            char caLast = locInputNext(ptInput);

            if (caLast < caFrist) {
                caFrist = caLast;
                caLast = sKey[i-1]-1;
            }

            for (; caFrist <= caLast; caFrist++) {
                sKey[i++] = caFrist;
            }

            continue;
        }

        sKey[i++] = caTmp;
    }

    sKey[i] = '\0';

    locStructInit(hNfa, ptStrcut);
    nfaAddLink(hNfa, ptStrcut->hStart, ptStrcut->hEnd, sKey, iFlag);

    return 0;
}

static int locStructInit(H_NFA hNfa, T_NfaStruct *ptStruct)
{
    ptStruct->hStart = nfaNewNode(hNfa, NFA_NODE_TYPE_NORMAL);
    ptStruct->hEnd = nfaNewNode(hNfa, NFA_NODE_TYPE_NORMAL);

    return 0;
}

static int locStructInitAllKey(H_NFA hNfa, T_NfaStruct *ptStrcut)
{
    char sKey[_DLEN_KEY_MAX+1];

    int i = 1;
    for (i=1; i<=128; i++) {
        sKey[i-1] = (unsigned char)i;
    }

    locStructInit(hNfa, ptStrcut);
    nfaAddLink(hNfa, ptStrcut->hStart, ptStrcut->hEnd, sKey, 0);

    return 0;
}

static int locStructInitKey(H_NFA hNfa, T_NfaStruct *ptStrcut, char caInput)
{
    char sKey[2];

    sKey[0] = caInput;
    sKey[1] = '\0';

    locStructInit(hNfa, ptStrcut);
    nfaAddLink(hNfa, ptStrcut->hStart, ptStrcut->hEnd, sKey, 0);

    return 0;
}

static int locStructMerge(H_NFA hNfa, T_NfaStruct *ptDst, T_NfaStruct *ptSrc)
{
    nfaAddLink(hNfa, ptDst->hStart, ptSrc->hStart, NULL, 0);
    nfaAddLink(hNfa, ptSrc->hEnd, ptDst->hEnd, NULL, 0);

    return 0;
}

static int locStructAdd(H_NFA hNfa, T_NfaStruct *ptDst, T_NfaStruct *ptSrc)
{
    if (NULL == ptDst->hStart) {
        memcpy(ptDst, ptSrc, sizeof(T_NfaStruct));
        return 0;
    }

    nfaAddLink(hNfa, ptDst->hEnd, ptSrc->hStart, NULL, 0);
    ptDst->hEnd = ptSrc->hEnd;

    return 0;
}

static int locInputInit(T_InPut *ptInPut, char *psBuffer)
{
    ptInPut->psBuf = psBuffer;
    ptInPut->iCurrent = 0;

    return 0;
}

static char locInputNext(T_InPut *ptInput)
{
    return ptInput->psBuf[ptInput->iCurrent++];
}

static int locInputTest(T_InPut *ptInput , char caChar)
{
    return (ptInput->psBuf[ptInput->iCurrent] == caChar);
}

static int locStructSetMuti(H_NFA hNfa, T_NfaStruct *ptStruct)
{
    nfaAddLink(hNfa, ptStruct->hEnd, ptStruct->hStart, NULL, 0);
    nfaAddLink(hNfa, ptStruct->hStart, ptStruct->hEnd, NULL, 0);
    return 0;
}

static int locDebug(H_NFA_NODE hNode)
{
    H_NFA_LINK hLink = NULL;

    while (1) {
        hLink = nfaNodeFetchLink(hNode);
    }

    return 0;
}

/*-----------------------------  End ------------------------------------*/

