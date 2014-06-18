/*
 * FileName: list.c
 *
 *  <Date>        <Author>       <Auditor>     <Desc>
 */
/*--------------------------- Include files -----------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "list.h"

/*--------------------------- Macro define ------------------------------*/
#define _DLEN_TINY_BUF  64
#define _DLEN_MINI_BUF  256
#define _DLEN_LARGE_BUF 1024
#define _DLEN_HUGE_BUF  10240

#define T() \
do { \
    printf("[%p]:[%p][%p][%p]\n", ptList, ptList->ptFrist, ptList->ptCur, ptList->ptLast); \
    T_ListNode *p = ptList->ptFrist; \
    while (p) { \
        printf("--[%p]\n", p); \
        p = p->ptNext; \
    } \
} while(0);

/*---------------------------- Type define ------------------------------*/
typedef struct ListNode {
    void *ptData;
    struct ListNode *ptNext;
} T_ListNode;

typedef struct ListIter {
    int iStatus;
    T_ListNode *ptCur;
    struct ListIter *ptNext;
} T_ListIter;

typedef struct List {
    int iNum;
    T_ListNode *ptFrist;
    T_ListNode *ptLast;
    T_ListIter *ptIter;
} T_List;

/*---------------------- Local function declaration ---------------------*/
static int listIterDebug(H_LIST ptList);

/*-------------------------  Global variable ----------------------------*/

/*-------------------------  Global functions ---------------------------*/
H_LIST listNew()
{
    H_LIST p = (T_List *)malloc(sizeof(T_List));

    p->iNum = 0;
    p->ptFrist = NULL;
    p->ptLast = NULL;
    p->ptIter = NULL;

    return p;
}

int listFree(H_LIST ptList)
{
    if (NULL == ptList) {
        return 0;
    }

    T_ListNode *p = ptList->ptFrist;
    while (p) {
        T_ListNode *ptTemp = p->ptNext;
        free(p);
        p = ptTemp;
    }

    T_ListIter *q = ptList->ptIter;
    while (q) {
        T_ListIter *ptTemp = q->ptNext;
        free(q);
        q = ptTemp;
    }

    free(ptList);

    return 0;
}

int listSort(H_LIST ptList, FNC_COMPARE fncCompare)
{
    T_ListNode *ptNode = ptList->ptFrist->ptNext;
    ptList->ptFrist->ptNext = NULL;

    while (NULL != ptNode) {
        T_ListNode *ptNew = ptNode;
        ptNode = ptNode->ptNext;

        T_ListNode *ptLast = NULL;
        T_ListNode *ptNow = ptList->ptFrist;
        while (ptNow != NULL) {
            int iRet = fncCompare(ptNow->ptData, ptNew->ptData);
            if (iRet >= 0) {
                if (NULL == ptLast) {
                    ptNew->ptNext = ptList->ptFrist;
                    ptList->ptFrist = ptNew;
                } else {
                    ptNew->ptNext = ptLast->ptNext;
                    ptLast->ptNext = ptNew;
                }
                break;
            }

            ptLast = ptNow;
            ptNow = ptNow->ptNext;
        }

        if (NULL == ptNow) {
            ptNew->ptNext = ptLast->ptNext;
            ptLast->ptNext = ptNew;
        }
    }

    return 0;
}

void * listFrist(H_LIST ptList)
{
    return ptList->ptFrist->ptData;
}

int listAdd(H_LIST ptList, void *ptItem)
{
    if (NULL == ptList || NULL == ptItem) {
        return -1;
    }

    T_ListNode *ptNode = (T_ListNode *)malloc(sizeof(T_ListNode));
    ptNode->ptData = ptItem;
    ptNode->ptNext = NULL;

    if (NULL != ptList->ptLast) {
        ptList->ptLast->ptNext = ptNode;
    }

    if (NULL == ptList->ptFrist) {
        ptList->ptFrist = ptNode;
    }

    ptList->ptLast = ptNode;
    ptList->iNum += 1;

    return 0;
}

int listInsert(H_LIST ptList, void *ptItem, void *ptNew)
{
    T_ListNode *ptNode = ptList->ptFrist;
    T_ListNode *ptLast = NULL;
    for (; NULL != ptNode; ptLast=ptNode, ptNode=ptNode->ptNext) {
        if (ptItem == ptNode->ptData) {
            break;
        }
    }

    if (NULL == ptNode) {
        return -1;
    }

    T_ListNode *ptNewNode = (T_ListNode *)malloc(sizeof(T_ListNode));
    ptNewNode->ptData = ptNew;
    ptNewNode->ptNext = NULL;

    if (NULL == ptLast) {
        ptList->ptFrist = ptNewNode;
        ptNewNode->ptNext = ptNode;
    } else {
        ptLast->ptNext = ptNewNode;
        ptNewNode->ptNext = ptNode->ptNext;
    }

    ptList->iNum += 1;
    
    return 0;
}

int listDel(H_LIST ptList, void *ptItem)
{
    T_ListNode *ptNode = ptList->ptFrist;
    T_ListNode *ptLast = NULL;
    for (; NULL != ptNode; ptLast=ptNode, ptNode=ptNode->ptNext) {
        if (ptItem != ptNode->ptData) {
            continue;
        }

        if (NULL == ptLast) {
            ptList->ptFrist = ptNode->ptNext;
        } else {
            ptLast->ptNext = ptNode->ptNext;
        }

        if (ptList->ptLast == ptNode) {
            ptList->ptLast = ptLast;
        }

        T_ListIter *p = ptList->ptIter;
        while (p) {
            if (p->ptCur == ptNode) {
                p->ptCur = ptLast;
            }
            p = p->ptNext;
        }

        free(ptNode);
        ptList->iNum -= 1;
        break;
    }

    return 0;
}

int listNum(H_LIST ptList)
{
    return ptList->iNum;
}

H_LIST_ITER listIterCopy(H_LIST ptList, H_LIST_ITER ptIter)
{
    H_LIST_ITER ptIterRet = listIterNew(ptList);
    ptIterRet->ptCur = ptIter->ptCur;

    return ptIterRet;
}

H_LIST_ITER listIterNew(H_LIST ptList)
{
    listIterDebug(ptList);
    T_ListIter *ptIter = ptList->ptIter;
    while (ptIter) {
        if (0 == ptIter->iStatus) {
            break;
        }
        ptIter = ptIter->ptNext;
    }

    if (NULL == ptIter) {
        ptIter = (T_ListIter *)malloc(sizeof(T_ListIter));
        ptIter->ptNext = ptList->ptIter;
        ptList->ptIter = ptIter;
    }

    ptIter->ptCur = ptList->ptFrist;
    ptIter->iStatus = 1;

    return ptIter;
}

void * listIterFetch(H_LIST_ITER ptIter)
{
    if (NULL == ptIter->ptCur || 0 == ptIter->iStatus) {
        return NULL;
    }

    void *p = ptIter->ptCur->ptData;
    ptIter->ptCur = ptIter->ptCur->ptNext;

    return p;
}

int listIterFree(H_LIST_ITER ptIter)
{
    ptIter->iStatus = 0;

    return 0;
}

/*-------------------------  Local functions ----------------------------*/

static int listIterDebug(H_LIST ptList)
{
    T_ListIter *ptIter = ptList->ptIter;
    while (ptIter) {
        ptIter = ptIter->ptNext;
    }

    return 0;
}

/*-----------------------------  End ------------------------------------*/

