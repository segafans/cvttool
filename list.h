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

/*---------------------- Global variable declaration---------------------*/

/*---------------------- Global function declaration --------------------*/
#ifdef __cplusplus
extern "C" {
#endif

    H_LIST listNew();
    int listFree(H_LIST ptList);
    int listAdd(H_LIST ptList, void *ptItem);
    void * listIter(H_LIST ptList);
    int listNum(H_LIST ptList);
    int listIterClean(H_LIST ptList);
    int listDel(H_LIST ptList, void *ptItem);

#ifdef __cplusplus
}
#endif

#endif /*_LIST_H_20100105092245_*/
/*-----------------------------  End ------------------------------------*/

