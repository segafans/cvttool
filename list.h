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
    T_ListIter tListIter; \
    listIterInit(&tListIter, (list)); \
    while (1) { \
        void *ptIter = listIterFetch(&tListIter); \
        if (NULL == ptIter) { \
            break; \
        } \
        \
        fnc; \
    } \
} while(0);

/*---------------------------- Type define ------------------------------*/
typedef struct List * H_LIST;
typedef struct ListIter {
    void *ptInfo;
} T_ListIter;

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
    int listDel(H_LIST ptList, void *ptItem);
    H_LIST listSort(H_LIST ptOldList, FNC_COMPARE fncCompare);
    void * listFrist(H_LIST ptList);

    int listPush(H_LIST ptList, void *ptItem);
    void * listPop(H_LIST ptList);

    int listIterInit(T_ListIter *ptIter, H_LIST hList);
    void * listIterFetch(T_ListIter *ptIter);

#ifdef __cplusplus
}
#endif

#endif /*_LIST_H_20100105092245_*/
/*-----------------------------  End ------------------------------------*/

