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
typedef struct List * H_LIST;
typedef struct ListIter * H_LIST_ITER;

typedef int (*FNC_COMPARE)(void *ptFrist, void *ptSecond);

/*---------------------- Global variable declaration---------------------*/

/*---------------------- Global function declaration --------------------*/
#ifdef __cplusplus
extern "C" {
#endif

    H_LIST listNew();
    int listFree(H_LIST ptList);
    int listAdd(H_LIST ptList, void *ptItem);
    int listNum(H_LIST ptList);
    int listInsert(H_LIST ptList, void *ptItem, void *ptNew);
    int listDel(H_LIST ptList, void *ptItem);
    int listSort(H_LIST ptList, FNC_COMPARE fncCompare);
    void * listFrist(H_LIST ptList);

    H_LIST_ITER listIterNew(H_LIST ptList);
    H_LIST_ITER listIterCopy(H_LIST ptList, H_LIST_ITER ptIter);
    void * listIterFetch(H_LIST_ITER ptIter);
    int listIterFree(H_LIST_ITER ptIter);

#ifdef __cplusplus
}
#endif

#endif /*_LIST_H_20100105092245_*/
/*-----------------------------  End ------------------------------------*/

