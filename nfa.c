/*
 * FileName: nfa.c
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

#define _TRUE   (1==1)
#define _FLASE  (1!=1)

#define debug 1

#define LIST_LOOP(list, fnc) \
do { \
    H_LIST_ITER pListIter = listIterNew(list); \
    while (1) { \
        void *ptIter = listIterFetch(pListIter); \
        if (NULL == ptIter) { \
            break; \
        } \
\
        fnc; \
    } \
    listIterFree(pListIter); \
} while(0);

/*---------------------------- Type define ------------------------------*/
typedef struct NfaNode {
    int iIndex;
    int iType;
    H_LIST ptToList;
    H_LIST ptFromList;
} T_NfaNode;

typedef struct NfaLink {
    char sKey[32];
    H_NFA_NODE ptSrcNode;
    H_NFA_NODE ptDstNode;
} T_NfaLink;

typedef struct Nfa {
    T_NfaNode *ptStart;
    H_LIST ptNodeList;
    H_LIST ptLinkList;
} T_Nfa;

typedef int (*FNC_LoopDo)(void *Item);
typedef int (*FNC_NfaLoopDo)(H_NFA hNfa, void *Item);

/*---------------------- Local function declaration ---------------------*/
static T_NfaNode* nfaNodeNew(int iIndex, int iType);
static int nfaNodeFree(T_NfaNode * ptNode);
static int nfaNodeDebug(T_NfaNode *ptNode);
static int nfaNodeDelete(H_NFA hNfa, T_NfaNode *ptNode);

static int nfaNodeSimple(H_NFA hNfa, T_NfaLink *ptLink);

static T_NfaLink * nfaLinkNew(T_NfaNode *ptSrcNode, T_NfaNode *ptDstNode, char *psKey);
static int nfaLinkDelete(H_NFA hNfa, T_NfaLink *ptLink);
static int nfaLinkFree(T_NfaLink* ptLink);
static int nfaLinkDebug(T_NfaLink *ptLink);
static int nfaLinkIsSame(T_NfaLink *ptLink,T_NfaNode *ptSrc, T_NfaNode *ptDst, char *psKey);
static int nfaNodeLinkTo(H_NFA hNfa, H_NFA_NODE ptSrc, H_NFA_NODE ptDst);
static int nfaNodeLinkFrom(H_NFA hNfa, H_NFA_NODE ptSrc, H_NFA_NODE ptDst);

static int nfaNodeIsSameToLink(T_NfaNode *ptFrist, T_NfaNode *ptSecond);
static int nfaNodeMerge(H_NFA hNfa, T_NfaNode *ptDst, T_NfaNode *ptSrc);
static int nfaNodeCheckSameToLink(H_NFA hNfa, T_NfaNode *ptNode);

/*-------------------------  Global variable ----------------------------*/

/*-------------------------  Global functions ---------------------------*/
H_NFA nfaNew()
{
    T_Nfa *ptNfa = (T_Nfa *)malloc(sizeof(T_Nfa));
    ptNfa->ptNodeList = listNew();
    ptNfa->ptLinkList = listNew();

    return ptNfa;
}

int nfaFree(H_NFA ptNfa)
{
    LIST_LOOP(ptNfa->ptNodeList, nfaNodeFree(ptIter));
    listFree(ptNfa->ptNodeList);
    LIST_LOOP(ptNfa->ptLinkList, nfaLinkFree(ptIter));
    listFree(ptNfa->ptLinkList);
    free(ptNfa);

    return 0;
}

int nfaDebug(H_NFA ptNfa)
{
    LIST_LOOP(ptNfa->ptNodeList, nfaNodeDebug(ptIter));
    printf("---------------------\n");
    return 0;
}

int nfaSimple(H_NFA hNfa)
{
    LIST_LOOP(hNfa->ptLinkList, nfaNodeSimple(hNfa, ptIter));
    LIST_LOOP(hNfa->ptNodeList, nfaNodeCheckSameToLink(hNfa, ptIter));

    return 0;
}

H_NFA_NODE nfaNewNode(H_NFA ptNfa, int iType)
{
    if (NFA_NODE_TYPE_START == iType && ptNfa->ptStart != NULL) {
        return NULL;
    }

    T_NfaNode *ptNode = nfaNodeNew(listNum(ptNfa->ptNodeList)+1, iType);
    listAdd(ptNfa->ptNodeList, ptNode);

    return ptNode;
}

int nfaAddLink(H_NFA ptNfa, H_NFA_NODE ptSrcNode, H_NFA_NODE ptDstNode, char *psKey)
{
    if (ptSrcNode == ptDstNode && (NULL == psKey || '\0' == psKey[0])) {
        return 0;
    }

    H_LIST_ITER pIter = listIterNew(ptSrcNode->ptToList);
    while (1) {
        T_NfaLink *ptTemp = listIterFetch(pIter);
        if (NULL == ptTemp) {
            break;
        }

        if (nfaLinkIsSame(ptTemp, ptSrcNode, ptDstNode, psKey)) {
            listIterFree(pIter);
            return 0;
        }
    }
    listIterFree(pIter);

    T_NfaLink *ptLink = nfaLinkNew(ptSrcNode, ptDstNode, psKey);
    listAdd(ptNfa->ptLinkList, ptLink);

    return 0;
}

/*-------------------------  Local functions ----------------------------*/
static T_NfaNode * nfaNodeNew(int iIndex, int iType)
{
    T_NfaNode *ptNode = (T_NfaNode *)malloc(sizeof(T_NfaNode));
    ptNode->iIndex = iIndex;
    ptNode->iType = iType;
    ptNode->ptToList = listNew();
    ptNode->ptFromList = listNew();

    return ptNode;
}

static int nfaNodeFree(T_NfaNode * ptNode)
{
    listFree(ptNode->ptFromList);
    listFree(ptNode->ptToList);
    free(ptNode);

    return 0;
}

static int nfaNodeDebug(T_NfaNode *ptNode)
{
    LIST_LOOP(ptNode->ptToList, nfaLinkDebug(ptIter));

    return 0;
}

static int nfaLinkDebug(T_NfaLink *ptLink)
{
    char *psSrcType = "";
    if (NFA_NODE_TYPE_START == ptLink->ptSrcNode->iType) {
        psSrcType = "s:";
    } else if ( NFA_NODE_TYPE_END == ptLink->ptSrcNode->iType ) {
        psSrcType = "e:";
    }

    char *psDstType = "";
    if (NFA_NODE_TYPE_START == ptLink->ptDstNode->iType) {
        psDstType = "s:";
    } else if ( NFA_NODE_TYPE_END == ptLink->ptDstNode->iType ) {
        psDstType = "e:";
    }

    printf("[%s%d]--(%s)-->[%s%d]\n", psSrcType, ptLink->ptSrcNode->iIndex, ptLink->sKey, psDstType, ptLink->ptDstNode->iIndex);
    return 0;
}

static T_NfaLink * nfaLinkNew(T_NfaNode *ptSrcNode, T_NfaNode *ptDstNode, char *psKey)
{
    T_NfaLink *ptLink = (T_NfaLink *)malloc(sizeof(T_NfaLink));
    ptLink->ptSrcNode = ptSrcNode;
    ptLink->ptDstNode = ptDstNode;

    if (NULL == psKey) {
        memset(ptLink->sKey, '\0', sizeof(ptLink->sKey));
    } else {
        strcpy(ptLink->sKey, psKey);
    }

    listAdd(ptSrcNode->ptToList, ptLink);
    listAdd(ptDstNode->ptFromList, ptLink);

    return ptLink;
}

static int nfaLinkFree(T_NfaLink * ptLink)
{
    free(ptLink);
    
    return 0;
}

static int nfaLinkDelete(H_NFA hNfa, T_NfaLink *ptLink)
{
#ifdef debug
    printf("Delete Link:");
    nfaLinkDebug(ptLink);
#endif

    listDel(ptLink->ptSrcNode->ptToList, ptLink);
    listDel(ptLink->ptDstNode->ptFromList, ptLink);
    listDel(hNfa->ptLinkList, ptLink);

    nfaLinkFree(ptLink);

#ifdef debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaNodeLinkTo(H_NFA hNfa, H_NFA_NODE ptDst, H_NFA_NODE ptSrc)
{
#ifdef debug
    printf("Copy To Link:%d==>%d\n", ptSrc->iIndex, ptDst->iIndex);
#endif

    if (NFA_NODE_TYPE_END == ptSrc->iType) {
        ptDst->iType = NFA_NODE_TYPE_END;
    }

    LIST_LOOP(ptSrc->ptToList, nfaAddLink(hNfa, ptDst, ((T_NfaLink*) ptIter)->ptDstNode, ((T_NfaLink*) ptIter)->sKey));

#ifdef debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaNodeLinkFrom(H_NFA hNfa, H_NFA_NODE ptDst, H_NFA_NODE ptSrc)
{
#ifdef debug
    printf("Copy From Link:%d==>%d\n", ptSrc->iIndex, ptDst->iIndex);
#endif

    if (NFA_NODE_TYPE_END == ptSrc->iType) {
        ptDst->iType = NFA_NODE_TYPE_END;
    }

    LIST_LOOP(ptSrc->ptFromList, nfaAddLink(hNfa, ((T_NfaLink*) ptIter)->ptSrcNode, ptDst, ((T_NfaLink*) ptIter)->sKey));

#ifdef debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaNodeDelete(H_NFA hNfa, T_NfaNode *ptNode)
{
#ifdef debug
    printf("Delete Node:%d\n", ptNode->iIndex);
#endif

    LIST_LOOP(ptNode->ptToList, nfaLinkDelete(hNfa, ptIter));
    LIST_LOOP(ptNode->ptFromList, nfaLinkDelete(hNfa, ptIter));

    nfaNodeFree(ptNode);
    listDel(hNfa->ptNodeList, ptNode);

#ifdef debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaLinkIsSame(T_NfaLink *ptLink, T_NfaNode *ptSrc, T_NfaNode *ptDst, char *psKey)
{
    if (ptLink->ptSrcNode != ptSrc) {
        return _FLASE;
    }

    if (ptLink->ptDstNode != ptDst) {
        return _FLASE;
    }

    if (NULL == psKey || '\0' == psKey[0]) {
        if ('\0' == ptLink->sKey[0]) {
            return _TRUE;
        } else {
            return _FLASE;
        }
    }

    if (0 != strcmp(ptLink->sKey, psKey)) {
        return _FLASE;
    }

    return _TRUE;
}

static int nfaNodeIsSameToLink(T_NfaNode *ptFrist, T_NfaNode *ptSecond)
{
    if (ptFrist->iType != ptSecond->iType) {
        return _FLASE;
    }

    if (listNum(ptFrist->ptToList) != listNum(ptSecond->ptToList)) {
        return _FLASE;
    }

    H_LIST_ITER ptIterFrist = listIterNew(ptFrist->ptToList);
    while (1) {
        T_NfaLink *ptLinkFrist = (T_NfaLink *)listIterFetch(ptIterFrist);
        if (NULL == ptLinkFrist) {
            break;
        }

        H_LIST_ITER ptIterSeconde = listIterNew(ptSecond->ptToList);
        while (1) {
            T_NfaLink *ptLinkSeconde = (T_NfaLink *)listIterFetch(ptIterSeconde);
            if (NULL == ptLinkSeconde) {
                listIterFree(ptIterSeconde);
                return _FLASE;
            }

            if (nfaLinkIsSame(ptLinkFrist, ptLinkFrist->ptSrcNode, ptLinkSeconde->ptDstNode, ptLinkSeconde->sKey)) {
                break;
            }
        }
        listIterFree(ptIterSeconde);
    }
    listIterFree(ptIterFrist);

    return _TRUE;
}

static int nfaNodeCheckSameToLink(H_NFA hNfa, T_NfaNode *ptNode)
{
    H_LIST_ITER hIter = listIterNew(ptNode->ptFromList);
    while (1) {
        T_NfaLink *ptLink = (T_NfaLink *)listIterFetch(hIter);
        if (NULL == ptLink) {
            break;
        }

        H_LIST_ITER hIterTmp = listIterCopy(ptNode->ptFromList, hIter);
        while (1) {
            T_NfaLink *ptTmp = (T_NfaLink *)listIterFetch(hIterTmp);
            if (NULL == ptTmp) {
                break;
            }

            if (0 != strcmp(ptLink->sKey, ptTmp->sKey)) {
                continue;
            }

            if (nfaNodeIsSameToLink(ptLink->ptSrcNode, ptTmp->ptSrcNode)) {
                if (ptTmp->ptSrcNode == ptNode) {
                    printf("[%d]==>[%d]\n", ptLink->ptSrcNode->iIndex, ptTmp->ptSrcNode->iIndex);
                    nfaNodeMerge(hNfa, ptTmp->ptSrcNode, ptLink->ptSrcNode);
                    nfaDebug(hNfa);
                    ptLink = NULL;
                    break;
                } else {
                    printf("[%d]==>[%d]\n", ptTmp->ptSrcNode->iIndex, ptLink->ptSrcNode->iIndex);
                    nfaNodeMerge(hNfa, ptLink->ptSrcNode, ptTmp->ptSrcNode);
                    nfaDebug(hNfa);
                    nfaNodeCheckSameToLink(hNfa, ptLink->ptSrcNode);
                }
            }
        }
        listIterFree(hIterTmp);
    }
    listIterFree(hIter);

    return 0;
}

static int nfaNodeMerge(H_NFA hNfa, T_NfaNode *ptDst, T_NfaNode *ptSrc)
{
#ifdef debug
    printf("Merge Node:%d==>%d\n", ptSrc->iIndex, ptDst->iIndex);
#endif

    nfaNodeLinkTo(hNfa, ptDst, ptSrc);
    nfaNodeLinkFrom(hNfa, ptDst, ptSrc);
    nfaNodeDelete(hNfa, ptSrc);

#ifdef debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaNodeSimple(H_NFA hNfa, T_NfaLink *ptLink)
{
    if ('\0' != ptLink->sKey[0]) {
        return 0;
    }

    if (NFA_NODE_TYPE_END == ptLink->ptDstNode->iType && 1 == listNum(ptLink->ptSrcNode->ptToList)) {
        nfaNodeMerge(hNfa, ptLink->ptDstNode, ptLink->ptSrcNode);
    } else {
        nfaNodeLinkTo(hNfa, ptLink->ptSrcNode, ptLink->ptDstNode);
        nfaLinkDelete(hNfa, ptLink);
        if (0 == listNum(ptLink->ptDstNode->ptFromList)) {
            nfaNodeDelete(hNfa, ptLink->ptDstNode);
        }
    }

    return 0;
}

/*-----------------------------  End ------------------------------------*/

