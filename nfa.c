/*
 * FileName: nfa.c
 *
 *  <Date>        <Author>       <Auditor>     <Desc>
 */
/*--------------------------- Include files -----------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "nfa.h"
#include "list.h"

/*--------------------------- Macro define ------------------------------*/
#define _DLEN_TINY_BUF  64
#define _DLEN_MINI_BUF  256
#define _DLEN_LARGE_BUF 1024
#define _DLEN_HUGE_BUF  10240

#define _DLEN_BITMAP     16

#define _TRUE   (1==1)
#define _FLASE  (1!=1)

#define _IS_START(type) _IS_STATUS((type), NFA_NODE_TYPE_START)
#define _IS_END(type) _IS_STATUS((type), NFA_NODE_TYPE_END)
#define _IS_STATUS(type, status) ((type) & (status))

#define BITMAP_MASK( bit )         (0x80>>(((bit)-1)&0x07))
#define BITMAP_INDEX( bit )        (((bit)-1)>>3)
#define BITMAP_TEST( bitmap, bit ) ((bitmap)[BITMAP_INDEX(bit)]&BITMAP_MASK(bit))
#define BITMAP_CLR( bitmap, bit )  (bitmap[BITMAP_INDEX(bit)]&=~BITMAP_MASK(bit))
#define BITMAP_SET( bitmap, bit )  (bitmap[BITMAP_INDEX(bit)]|=BITMAP_MASK(bit))
#define BITMAP_TEST_NULL( bitmap ) (!((bitmap)[0]|(bitmap)[1]|(bitmap)[2]|(bitmap)[3]|(bitmap)[4]|(bitmap)[5]|(bitmap)[6]|(bitmap)[7]|(bitmap)[8]|(bitmap)[9]|(bitmap)[10]|(bitmap)[11]|(bitmap)[12]|(bitmap)[13]|(bitmap)[14]|(bitmap)[15]))

#define debug 0

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
    unsigned char sBitMap[_DLEN_BITMAP];
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

static T_NfaLink * nfaLinkNew(T_NfaNode *ptSrcNode, T_NfaNode *ptDstNode, unsigned char *psBitMap);
static int nfaLinkDelete(H_NFA hNfa, T_NfaLink *ptLink);
static int nfaLinkFree(T_NfaLink* ptLink);
static int nfaLinkDebug(T_NfaLink *ptLink);
static int nfaLinkIsSame(T_NfaLink *ptLink,T_NfaNode *ptSrc, T_NfaNode *ptDst, unsigned char *psKey);
static int nfaNodeLinkTo(H_NFA hNfa, H_NFA_NODE ptSrc, H_NFA_NODE ptDst);
static int nfaNodeLinkFrom(H_NFA hNfa, H_NFA_NODE ptSrc, H_NFA_NODE ptDst);
static int nfaAddLinkBitMap(H_NFA ptNfa, H_NFA_NODE ptSrcNode, H_NFA_NODE ptDstNode, unsigned char *psBitMap);

static int nfaNodeIsSameToLink(T_NfaNode *ptFrist, T_NfaNode *ptSecond);
static int nfaNodeMerge(H_NFA hNfa, T_NfaNode *ptDst, T_NfaNode *ptSrc);
static int nfaNodeCheckSameToLink(H_NFA hNfa, T_NfaNode *ptNode);

static int _locListHasItem(H_LIST ptList, void *ptItem);

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
    if (_IS_START(iType) && ptNfa->ptStart != NULL) {
        return NULL;
    }

    T_NfaNode *ptNode = nfaNodeNew(listNum(ptNfa->ptNodeList)+1, iType);
    listAdd(ptNfa->ptNodeList, ptNode);

    return ptNode;
}

int nfaAddLink(H_NFA ptNfa, H_NFA_NODE ptSrcNode, H_NFA_NODE ptDstNode, char *psKey)
{
    unsigned char sBitMap[_DLEN_BITMAP];
    memset(sBitMap, '\0', sizeof(sBitMap));

    if (NULL != psKey) {
        int i = 0;
        for (i=0; psKey[i] != '\0'; i++) {
            BITMAP_SET(sBitMap, (unsigned char)psKey[i]);
        }
    }

    return nfaAddLinkBitMap(ptNfa, ptSrcNode, ptDstNode, sBitMap);
}

/*-------------------------  Local functions ----------------------------*/
static T_NfaNode * nfaNodeNew(int iIndex, int iType)
{
    T_NfaNode *ptNode = (T_NfaNode *)malloc(sizeof(T_NfaNode));
    ptNode->iIndex = iIndex;
    ptNode->iType = 0;
    ptNode->ptToList = listNew();
    ptNode->ptFromList = listNew();

    ptNode->iType |= iType;

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
    printf("[");
    if (_IS_START(ptLink->ptSrcNode->iType)) {
        printf("s:");
    }
    if ( _IS_END(ptLink->ptSrcNode->iType) ) {
        printf("e:");
    }
    printf("%d]--(", ptLink->ptSrcNode->iIndex);

    unsigned int i;
    for (i=1; i<=128; i++) {
        if (BITMAP_TEST(ptLink->sBitMap, i)) {
            if (isprint(i)) {
                printf("%c", i);
            } else {
                printf("\\%03d", i);
            }
        }
    }

    printf(")-->[");
    if (_IS_START(ptLink->ptDstNode->iType)) {
        printf("s:");
    }
    if ( _IS_END(ptLink->ptDstNode->iType) ) {
        printf("e:");
    }
    printf("%d]\n", ptLink->ptDstNode->iIndex);

    return 0;
}

static T_NfaLink * nfaLinkNew(T_NfaNode *ptSrcNode, T_NfaNode *ptDstNode, unsigned char *psBitMap)
{
    T_NfaLink *ptLink = (T_NfaLink *)malloc(sizeof(T_NfaLink));
    ptLink->ptSrcNode = ptSrcNode;
    ptLink->ptDstNode = ptDstNode;
    memcpy(ptLink->sBitMap, psBitMap, sizeof(ptLink->sBitMap));

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
#if debug
    printf("Delete Link:");
    nfaLinkDebug(ptLink);
#endif

    listDel(ptLink->ptSrcNode->ptToList, ptLink);
    listDel(ptLink->ptDstNode->ptFromList, ptLink);
    listDel(hNfa->ptLinkList, ptLink);

    nfaLinkFree(ptLink);

#if debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaNodeLinkTo(H_NFA hNfa, H_NFA_NODE ptDst, H_NFA_NODE ptSrc)
{
#if debug
    printf("Copy To Link:%d==>%d\n", ptSrc->iIndex, ptDst->iIndex);
#endif

    if (_IS_END(ptSrc->iType)) {
        ptDst->iType |= NFA_NODE_TYPE_END;
    }

    LIST_LOOP(ptSrc->ptToList, nfaAddLinkBitMap(hNfa, ptDst, ((T_NfaLink*) ptIter)->ptDstNode, ((T_NfaLink*) ptIter)->sBitMap));

#if debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaNodeLinkFrom(H_NFA hNfa, H_NFA_NODE ptDst, H_NFA_NODE ptSrc)
{
#if debug
    printf("Copy From Link:%d==>%d\n", ptSrc->iIndex, ptDst->iIndex);
#endif

    if (_IS_END(ptSrc->iType)) {
        ptDst->iType |= NFA_NODE_TYPE_END;
    }

    LIST_LOOP(ptSrc->ptFromList, nfaAddLinkBitMap(hNfa, ((T_NfaLink*) ptIter)->ptSrcNode, ptDst, ((T_NfaLink*) ptIter)->sBitMap));

#if debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaNodeDelete(H_NFA hNfa, T_NfaNode *ptNode)
{
#if debug
    printf("Delete Node:%d\n", ptNode->iIndex);
#endif

    LIST_LOOP(ptNode->ptToList, nfaLinkDelete(hNfa, ptIter));
    LIST_LOOP(ptNode->ptFromList, nfaLinkDelete(hNfa, ptIter));

    listDel(hNfa->ptNodeList, ptNode);
    nfaNodeFree(ptNode);

#if debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaLinkIsSame(T_NfaLink *ptLink, T_NfaNode *ptSrc, T_NfaNode *ptDst, unsigned char *psBitMap)
{
    if (ptLink->ptSrcNode != ptSrc) {
        return _FLASE;
    }

    if (ptLink->ptDstNode != ptDst) {
        return _FLASE;
    }

    if (0 != memcmp(ptLink->sBitMap, psBitMap, sizeof(ptLink->sBitMap))) {
        return _FLASE;
    }

    return _TRUE;
}

static int nfaNodeIsSameToLink(T_NfaNode *ptFrist, T_NfaNode *ptSecond)
{
    if (_IS_END(ptFrist->iType) != _IS_END(ptSecond->iType)) {
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
                listIterFree(ptIterFrist);
                listIterFree(ptIterSeconde);
                return _FLASE;
            }

            if (nfaLinkIsSame(ptLinkFrist, ptLinkFrist->ptSrcNode, ptLinkSeconde->ptDstNode, ptLinkSeconde->sBitMap)) {
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
#if debug
    int iIndex = ptNode->iIndex;
    printf("##check node %d:\n", iIndex);
#endif

    int iFlag = _FLASE;

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

            if (0 != memcmp(ptLink->sBitMap, ptTmp->sBitMap, sizeof(ptLink->sBitMap))) {
                continue;
            }

            if (!nfaNodeIsSameToLink(ptLink->ptSrcNode, ptTmp->ptSrcNode)) {
                continue;
            }

            T_NfaNode *ptKeepNode = ptLink->ptSrcNode;
            T_NfaNode *ptDelNode = ptTmp->ptSrcNode;

            if (_IS_START(ptDelNode->iType)) {
                ptDelNode = ptLink->ptSrcNode;
                ptKeepNode = ptTmp->ptSrcNode;
            }

            nfaNodeMerge(hNfa, ptKeepNode, ptDelNode);

            if (ptKeepNode != ptNode) {
                nfaNodeCheckSameToLink(hNfa, ptKeepNode);
            }

            iFlag = _TRUE;
            break;
        }
        listIterFree(hIterTmp);

        if (iFlag) {
            break;
        }
    }
    listIterFree(hIter);

    if (iFlag && _locListHasItem(hNfa->ptNodeList, ptNode)) {
        return nfaNodeCheckSameToLink(hNfa, ptNode);
    }

#if debug
    printf("##check node %d end\n", iIndex);
#endif

    return 0;
}

static int nfaNodeMerge(H_NFA hNfa, T_NfaNode *ptKeep, T_NfaNode *ptDelete)
{
#if debug
    printf("Merge Node:delete %d => keep %d\n", ptDelete->iIndex, ptKeep->iIndex);
#endif

    nfaNodeLinkTo(hNfa, ptKeep, ptDelete);
    nfaNodeLinkFrom(hNfa, ptKeep, ptDelete);
    nfaNodeDelete(hNfa, ptDelete);

#if debug
    nfaDebug(hNfa);
#endif

    return 0;
}

static int nfaNodeSimple(H_NFA hNfa, T_NfaLink *ptLink)
{
    if (!BITMAP_TEST_NULL(ptLink->sBitMap)) {
        return 0;
    }

    if (_IS_END(ptLink->ptDstNode->iType) && 1 == listNum(ptLink->ptSrcNode->ptToList)) {
        nfaNodeMerge(hNfa, ptLink->ptDstNode, ptLink->ptSrcNode);
    } else {
        nfaNodeLinkTo(hNfa, ptLink->ptSrcNode, ptLink->ptDstNode);

        T_NfaNode *ptDstNode = ptLink->ptDstNode;
        nfaLinkDelete(hNfa, ptLink);
        if (0 == listNum(ptDstNode->ptFromList)) {
            nfaNodeDelete(hNfa, ptDstNode);
        }
    }

    return 0;
}

static int _locListHasItem(H_LIST ptList, void *ptItem)
{
    int iRet = _FLASE;

    H_LIST_ITER pListIter = listIterNew(ptList);
    while (1) {
        void *ptIter = listIterFetch(pListIter);
        if (NULL == ptIter) {
            break;
        }

        if (ptItem == ptIter) {
            iRet = _TRUE;
            break;
        }
    }
    listIterFree(pListIter);

    return iRet;
}

int nfaAddLinkBitMap(H_NFA ptNfa, H_NFA_NODE ptSrcNode, H_NFA_NODE ptDstNode, unsigned char *psBitMap)
{
    if (ptSrcNode == ptDstNode && BITMAP_TEST_NULL(psBitMap)) {
        return 0;
    }

    H_LIST_ITER pIter = listIterNew(ptSrcNode->ptToList);
    while (1) {
        T_NfaLink *ptTemp = listIterFetch(pIter);
        if (NULL == ptTemp) {
            break;
        }

        if (ptTemp->ptDstNode != ptDstNode) {
            continue;
        }

        if (!nfaLinkIsSame(ptTemp, ptSrcNode, ptDstNode, psBitMap)) {
            ptTemp->sBitMap[0] |= psBitMap[0];
            ptTemp->sBitMap[1] |= psBitMap[1];
            ptTemp->sBitMap[2] |= psBitMap[2];
            ptTemp->sBitMap[3] |= psBitMap[3];
            ptTemp->sBitMap[4] |= psBitMap[4];
            ptTemp->sBitMap[5] |= psBitMap[5];
            ptTemp->sBitMap[6] |= psBitMap[6];
            ptTemp->sBitMap[7] |= psBitMap[7];
            ptTemp->sBitMap[8] |= psBitMap[8];
            ptTemp->sBitMap[9] |= psBitMap[9];
            ptTemp->sBitMap[10] |= psBitMap[10];
            ptTemp->sBitMap[11] |= psBitMap[11];
            ptTemp->sBitMap[12] |= psBitMap[12];
            ptTemp->sBitMap[13] |= psBitMap[13];
            ptTemp->sBitMap[14] |= psBitMap[14];
            ptTemp->sBitMap[15] |= psBitMap[15];
        }

        listIterFree(pIter);
        return 0;
    }
    listIterFree(pIter);

    T_NfaLink *ptLink = nfaLinkNew(ptSrcNode, ptDstNode, psBitMap);
    listAdd(ptNfa->ptLinkList, ptLink);
    
    return 0;
}

/*-----------------------------  End ------------------------------------*/

