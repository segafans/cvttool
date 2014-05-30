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

typedef struct List {
    int iNum;
    T_ListNode *ptFrist;
    T_ListNode *ptLast;
    T_ListNode *ptCur;
} T_List;

/*---------------------- Local function declaration ---------------------*/

/*-------------------------  Global variable ----------------------------*/
H_LIST listNew()
{
    H_LIST p = (T_List *)malloc(sizeof(T_List));

    p->iNum = 0;
    p->ptFrist = NULL;
    p->ptLast = NULL;
    p->ptCur = NULL;

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

    free(ptList);

    return 0;
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

int listDel(H_LIST ptList, void *ptItem)
{
    T_ListNode *ptNode = ptList->ptFrist;
    T_ListNode *ptLast = NULL;
    for (ptNode=ptList->ptFrist; NULL != ptNode; ptLast=ptNode, ptNode=ptNode->ptNext) {
        if (ptItem != ptNode->ptData) {
            continue;
        }

        if (NULL == ptLast) {
            ptList->ptFrist = ptNode->ptNext;
        } else {
            ptLast->ptNext = ptNode->ptNext;
        }

        if (ptList->ptCur == ptNode) {
            ptList->ptCur = ptLast;
        }

        if (ptList->ptLast == ptNode) {
            ptList->ptLast = ptLast;
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

int listIterClean(H_LIST ptList)
{
    ptList->ptCur = NULL;

    return 0;
}

void * listIter(H_LIST ptList)
{
    if (NULL == ptList->ptCur) {
        ptList->ptCur = ptList->ptFrist;
    } else {
        ptList->ptCur = ptList->ptCur->ptNext;
    }

    if (NULL == ptList->ptCur) {
        return NULL;
    }

    return ptList->ptCur->ptData;
}

/*-------------------------  Global functions ---------------------------*/

/*-------------------------  Local functions ----------------------------*/

/*-----------------------------  End ------------------------------------*/

