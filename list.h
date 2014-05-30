/*
 *
 *
 * swt base code.
 *
 *
 * FileName: list.h
 *
 *  <Date>        <Author>       <Auditor>     <Desc>
 */
#ifndef _LIST_H_20100105092245_
#define _LIST_H_20100105092245_
/*--------------------------- Include files -----------------------------*/

/*--------------------------- Macro define ------------------------------*/

/*---------------------------- Type define ------------------------------*/
typedef struct List * H_LIST;
typedef struct ListIter * H_LIST_ITER;

/*---------------------- Global variable declaration---------------------*/

/*---------------------- Global function declaration --------------------*/
#ifdef __cplusplus
extern "C" {
#endif

    H_LIST listNew();
    int listFree(H_LIST ptList);
    int listAdd(H_LIST ptList, void *ptItem);
    int listNum(H_LIST ptList);
    int listDel(H_LIST ptList, void *ptItem);

    H_LIST_ITER listIterNew(H_LIST ptList);
    H_LIST_ITER listIterCopy(H_LIST ptList, H_LIST_ITER ptIter);
    void * listIterFetch(H_LIST_ITER ptIter);
    int listIterFree(H_LIST_ITER ptIter);

#ifdef __cplusplus
}
#endif

#endif /*_LIST_H_20100105092245_*/
/*-----------------------------  End ------------------------------------*/

