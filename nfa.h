/*
 *
 * FileName: nfa.h
 *
 *  <Date>        <Author>       <Auditor>     <Desc>
 */
#ifndef _NFA_H_20100105092245_
#define _NFA_H_20100105092245_
/*--------------------------- Include files -----------------------------*/

/*--------------------------- Macro define ------------------------------*/
#define NFA_NODE_TYPE_START  0x1
#define NFA_NODE_TYPE_END    0x2
#define NFA_NODE_TYPE_NORMAL 0

/*---------------------------- Type define ------------------------------*/
typedef struct Nfa *     H_NFA;
typedef struct NfaNode * H_NFA_NODE;
typedef struct NfaLink * H_NFA_LINK;

/*---------------------- Global variable declaration---------------------*/

/*---------------------- Global function declaration --------------------*/
#ifdef __cplusplus
extern "C" {
#endif

    H_NFA nfaNew();
    int nfaFree(H_NFA ptNfa);
    int nfaDebug(H_NFA ptNfa);
    int nfaSimple(H_NFA hNfa);
    int nfaReindex(H_NFA hNfa);
    H_NFA_NODE nfaNewNode(H_NFA ptNfa, int iType);
    int nfaAddLink(H_NFA ptNfa, H_NFA_NODE ptSrcNode, H_NFA_NODE ptDstNode, char *psKey, int iFlag);

    H_NFA_NODE nfaGetStartNode(H_NFA hNfa);

#ifdef __cplusplus
}
#endif

#endif /*_NFA_H_20100105092245_*/
/*-----------------------------  End ------------------------------------*/

