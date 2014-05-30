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

/*---------------------- Local function declaration ---------------------*/
static T_NfaNode* nfaNodeNew(int iIndex, int iType);
static int nfaNodeFree(T_NfaNode * ptNode);
static int nfaNodeDebug(T_NfaNode *ptNode);
static int nfaNodeDelete(H_NFA hNfa, T_NfaNode *ptNode);

static T_NfaLink * nfaLinkNew(T_NfaNode *ptSrcNode, T_NfaNode *ptDstNode, char *psKey);
static int nfaLinkDelete(H_NFA hNfa, T_NfaLink *ptLink);
static int nfaLinkFree(T_NfaLink* ptLink);
static int nfaLinkDebug(T_NfaLink *ptLink);
static int nfaLinkIsSame(T_NfaLink *ptLink,T_NfaNode *ptSrc, T_NfaNode *ptDst, char *psKey);
static int nfaNodeLinkTo(H_NFA hNfa, H_NFA_NODE ptSrc, H_NFA_NODE ptDst);
static int nfaNodeLinkFrom(H_NFA hNfa, H_NFA_NODE ptSrc, H_NFA_NODE ptDst);

static int locListLoopDo(H_LIST hList, int (*f_LoopDo)(void *Item));
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
    locListLoopDo(ptNfa->ptNodeList, (FNC_LoopDo)nfaNodeFree);
    listFree(ptNfa->ptNodeList);
    locListLoopDo(ptNfa->ptLinkList, (FNC_LoopDo)nfaLinkFree);
    listFree(ptNfa->ptLinkList);
    free(ptNfa);

    return 0;
}

int nfaDebug(H_NFA ptNfa)
{
    printf("---------------------\n");
    locListLoopDo(ptNfa->ptNodeList, (FNC_LoopDo)nfaNodeDebug);
    return 0;
}

int nfaSimple(H_NFA hNfa)
{
    while (1) {
        T_NfaLink *ptLink = (T_NfaLink *)listIter(hNfa->ptLinkList);
        if (NULL == ptLink) {
            break;
        }

        if ('\0' != ptLink->sKey[0]) {
            continue;
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

        //printf("############\n");
        //nfaLinkDebug(ptLink);
        //nfaDebug(hNfa);
    }

    nfaDebug(hNfa);

    while (1) {
        T_NfaNode *ptNode = listIter(hNfa->ptNodeList);
        if (NULL == ptNode) {
            break;
        }

        nfaNodeCheckSameToLink(hNfa, ptNode);
    }

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

    while (1) {
        T_NfaLink *ptTemp = listIter(ptSrcNode->ptToList);
        if (NULL == ptTemp) {
            break;
        }

        if (nfaLinkIsSame(ptTemp, ptSrcNode, ptDstNode, psKey)) {
            listIterClean(ptSrcNode->ptToList);
            return 0;
        }
    }

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
    locListLoopDo(ptNode->ptToList, (FNC_LoopDo)nfaLinkDebug);

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

static int locListLoopDo(H_LIST hList, FNC_LoopDo f_LoopDo)
{
    while (1) {
        void *ptItem = listIter(hList);
        if (NULL == ptItem) {
            break;
        }

        f_LoopDo(ptItem);
    }

    return 0;
}

static int nfaLinkDelete(H_NFA hNfa, T_NfaLink *ptLink)
{
    listDel(ptLink->ptSrcNode->ptToList, ptLink);
    listDel(ptLink->ptDstNode->ptFromList, ptLink);
    listDel(hNfa->ptLinkList, ptLink);

    nfaLinkFree(ptLink);
    
    return 0;
}

static int nfaNodeLinkTo(H_NFA hNfa, H_NFA_NODE ptDst, H_NFA_NODE ptSrc)
{
    if (NFA_NODE_TYPE_END == ptSrc->iType) {
        ptDst->iType = NFA_NODE_TYPE_END;
    }

    while (1) {
        T_NfaLink *ptLink = (T_NfaLink *)listIter(ptSrc->ptToList);
        if (NULL == ptLink) {
            break;
        }

        nfaAddLink(hNfa, ptDst, ptLink->ptDstNode, ptLink->sKey);
    }

    return 0;
}

static int nfaNodeLinkFrom(H_NFA hNfa, H_NFA_NODE ptDst, H_NFA_NODE ptSrc)
{
    if (NFA_NODE_TYPE_END == ptSrc->iType) {
        ptDst->iType = NFA_NODE_TYPE_END;
    }

    while (1) {
        T_NfaLink *ptLink = (T_NfaLink *)listIter(ptSrc->ptFromList);
        if (NULL == ptLink) {
            break;
        }

        nfaAddLink(hNfa, ptLink->ptSrcNode, ptDst, ptLink->sKey);
    }

    return 0;
}

static int nfaNodeDelete(H_NFA hNfa, T_NfaNode *ptNode)
{
    while (1) {
        T_NfaLink *ptLink = (T_NfaLink *)listIter(ptNode->ptToList);
        if (NULL == ptLink) {
            break;
        }

        nfaLinkDelete(hNfa, ptLink);
    }

    while (1) {
        T_NfaLink *ptLink = (T_NfaLink *)listIter(ptNode->ptFromList);
        if (NULL == ptLink) {
            break;
        }

        nfaLinkDelete(hNfa, ptLink);
    }

    nfaNodeFree(ptNode);
    listDel(hNfa->ptNodeList, ptNode);

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

    while (1) {
        T_NfaLink *ptFristLink = listIter(ptFrist->ptToList);
        if (NULL == ptFristLink) {
            break;
        }

        while (1) {
            T_NfaLink *ptSecondLink = listIter(ptSecond->ptToList);
            if (NULL == ptSecondLink) {
                listIterClean(ptFrist->ptToList);
                return _FLASE;
            }

            if (nfaLinkIsSame(ptFristLink, ptFristLink->ptSrcNode, ptSecondLink->ptDstNode, ptSecondLink->sKey)) {
                listIterClean(ptSecond->ptToList);
                break;
            }
        }
    }

    return _TRUE;
}

static int nfaNodeCheckSameToLink(H_NFA hNfa, T_NfaNode *ptNode)
{
    T_NfaLink *ptLink = NULL;
    while (1) {
        while (1) {
            if (NULL == ptLink) {
                ptLink = listIter(ptNode->ptFromList);
                break;
            }

            T_NfaLink *ptTmp = listIter(ptNode->ptFromList);
            if (ptLink == ptTmp) {
                ptLink = listIter(ptNode->ptFromList);
                break;
            }
        }

        if (NULL == ptLink) {
            break;
        }

        while (1) {
            T_NfaLink *ptTmp = listIter(ptNode->ptFromList);
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
    }

    return 0;
}

static int nfaNodeMerge(H_NFA hNfa, T_NfaNode *ptDst, T_NfaNode *ptSrc)
{
    nfaNodeLinkTo(hNfa, ptDst, ptSrc);
    nfaNodeLinkFrom(hNfa, ptDst, ptSrc);
    nfaNodeDelete(hNfa, ptSrc);

    return 0;
}

/*-----------------------------  End ------------------------------------*/

