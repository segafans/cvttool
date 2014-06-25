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
} T_List;

/*---------------------- Local function declaration ---------------------*/
static T_ListNode * _newListNode(void *ptItem);

/*-------------------------  Global variable ----------------------------*/

/*-------------------------  Global functions ---------------------------*/
H_LIST listNew()
{
    H_LIST p = (T_List *)malloc(sizeof(T_List));

    p->iNum = 0;
    p->ptFrist = NULL;
    p->ptLast = NULL;

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

int listPush(H_LIST ptList, void *ptItem)
{
    if (NULL == ptList || NULL == ptItem) {
        return -1;
    }

    T_ListNode *ptNode = (T_ListNode *)malloc(sizeof(T_ListNode));
    ptNode->ptData = ptItem;
    ptNode->ptNext = ptList->ptFrist;
    ptList->ptFrist = ptNode;
    ptList->ptLast = ptNode;

    ptList->iNum += 1;
    
    return 0;
}

void * listPop(H_LIST ptList)
{
    if (NULL == ptList) {
        return NULL;
    }

    if (NULL == ptList->ptFrist) {
        return NULL;
    }

    T_ListNode *ptNode = ptList->ptFrist;
    ptList->ptFrist = ptNode->ptNext;
    ptList->ptLast = ptNode->ptNext;

    ptList->iNum -= 1;

    void *ptItem = ptNode->ptData;
    free(ptNode);

    return ptItem;
}

H_LIST listSort(H_LIST ptOldList, FNC_COMPARE fncCompare)
{
    H_LIST ptList = listNew();

    T_ListNode *ptNode = ptOldList->ptFrist->ptNext;

    ptList->ptFrist = _newListNode(ptOldList->ptFrist->ptData);
    ptList->ptLast = ptList->ptFrist;
    ptList->iNum = 1;

    while (NULL != ptNode) {
        T_ListNode *ptNew = _newListNode(ptNode->ptData);
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
            ptList->ptLast = ptNew;
        }

        ptList->iNum += 1;
    }

    return ptList;
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

int listIterInit(T_ListIter *ptIter, H_LIST hList)
{
    if (NULL  == ptIter || NULL == hList) {
        return -1;
    }

    ptIter->ptInfo = hList->ptFrist;

    return 0;
}

void * listIterFetch(T_ListIter *ptIter)
{
    if (NULL == ptIter || NULL == ptIter->ptInfo) {
        return NULL;
    }

    T_ListNode *ptNode = (T_ListNode *)ptIter->ptInfo;
    ptIter->ptInfo = ptNode->ptNext;

    return ptNode->ptData;
}

/*-------------------------  Local functions ----------------------------*/
static T_ListNode * _newListNode(void *ptItem)
{
    T_ListNode *ptNode = (T_ListNode *)malloc(sizeof(T_ListNode));
    ptNode->ptData = ptItem;
    ptNode->ptNext = NULL;

    return ptNode;
}

/*-----------------------------  End ------------------------------------*/

