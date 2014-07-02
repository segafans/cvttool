/*
 * FileName: poc.c
 *
 *  <Date>        <Author>       <Auditor>     <Desc>
 */
/*--------------------------- Include files -----------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include "list.h"

/*--------------------------- Macro define ------------------------------*/
#define _DLEN_TINY_BUF   64
#define _DLEN_MINI_BUF   256
#define _DLEN_LARGE_BUF  1024
#define _DLEN_HUGE_BUF   10240

#define _DLEN_BITMAP     16
#define _NUM_BITMAP      _DLEN_BITMAP*8

#define _LOOP_UNLIMITED  "-1"

#define _BOOL_YES         1

#define BITMAP_MASK( bit )         (0x80>>(((bit)-1)&0x07))
#define BITMAP_INDEX( bit )        (((bit)-1)>>3)
#define BITMAP_TEST( bitmap, bit ) ((bitmap)[BITMAP_INDEX(bit)]&BITMAP_MASK(bit))
#define BITMAP_CLR( bitmap, bit )  (bitmap[BITMAP_INDEX(bit)]&=~BITMAP_MASK(bit))
#define BITMAP_SET( bitmap, bit )  (bitmap[BITMAP_INDEX(bit)]|=BITMAP_MASK(bit))

/*---------------------------- Type define ------------------------------*/
typedef enum {
    TYPE_NULL,
    TYPE_STRING,
    TYPE_BITMAP,
    TYPE_NODE,
    TYPE_VAR,
    TYPE_LIMIT,
    TYPE_QUOTE,
    TYPE_ANY,
    TYPE_NUM
} E_TYPE;

typedef struct {
    E_TYPE eType;
    H_LIST hChild;
    union {
        struct {
            char *psBitMap;
            int iNotFlag;
            char *psOrg;
        } tBitMap;
        struct {
            char *psMax;
            char *psMin;
            int iNode;
        } tNode;
        struct {
            char *psLimit;
        } tLimit;
        struct {
            char *psVar;
        } tVar;
        struct {
            char *psString;
            int iLen;
        } tString;
        struct {
            char *psQutoe;
        } tQuote;
    };
} T_Value;

typedef struct {
    char *psName;
    char *psValue;
    H_LIST hList;
} T_Field;

typedef struct {
    char *psName;
    H_LIST hList;
    int iOrderFlag;
} T_Msg;

typedef struct {
    int iLoopStat;
    int iLoopIndex;
    int iMaxTempInit;
    int iPosTempInit;
    int iPreCheck;
    int iValueNum;
    int iHasValue;
    char *psName;
} T_Field_Option;

typedef void (*FNC_PROC)(T_Value *ptValue, T_Field_Option *ptOption);

#define ERR_BASE               -1000
#define ERR_BUFFER_TOO_SMALL   -1
#define ERR_STRING_NOT_SAMLE   -2
#define ERR_BITMAP_NOT_SAMLE   -3
#define ERR_GET_VALUE          -4
#define ERR_SET_VALUE          -5
#define ERR_LOOP_MIN           -6
#define ERR_QUORE_PARSE        -7
#define ERR_QUORE_GEN          -8

/*---------------------- Local function declaration ---------------------*/
static int genFile(H_LIST hMsgList);
static int locParse(char *psBuf, H_LIST hList, H_LIST hMsgList);
static int storeChar(unsigned char caChar, unsigned char *psFrist, unsigned char *psLast);
static char * setLoop(T_Value *ptValue, H_LIST ptList, char *pCur);
static char * setLoopNode(T_Value *ptNode, char *psBuf);
static char * setLoopNodeQuote(T_Value *ptValue, H_LIST hList, char *pCur);
static int printfChar(unsigned char caFrist, unsigned char caLast, int *piFlag);
static int _s();
static int _(char *psBuf, ...);
static int __(char *psBuf, ...);
static int FiledSort(void *ptFrist, void *ptSecond);
static T_Msg * msgFind(T_Msg *ptMsg, char *psName, T_Msg *ptRet);
static int msgFieldAdd(H_LIST ptList, char *psNodeName, char *psVarName, char *psValue);
static int msgGenCodeFuncDef(T_Msg *ptMsg);
static int msgGenCode(H_LIST hMsgList);
static T_Msg * msgGet(H_LIST hMsgList, char *psNodeName);

static T_Value * valueNew(E_TYPE eType);
static T_Value * valueNewVarIndex(int iIndex);
static T_Value * valueNewVar(char *psValue, int iLen);
static T_Value * valueNewString(char *psString, int iLen);
static T_Value * valueNewBitMap(char *psStart, char *psEnd);
static T_Value * valueNewQuote(H_LIST hMsgList, char *psStart, char *psEnd);

static void valueNullParase(T_Value *ptVcalue, T_Field_Option *ptOption);
static void valueStringParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueBitMapParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueNodeParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueVarParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueLimitParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueQuoteParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueAnyParse(T_Value *ptValue, T_Field_Option *ptOption);

static void valueNullGen(T_Value *ptVcalue, T_Field_Option *ptOption);
static void valueStringGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueBitMapGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueNodeGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueVarGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueLimitGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueQuoteGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueAnyGen(T_Value *ptValue, T_Field_Option *ptOption);

static int fieldGenCodeParse(T_Msg *ptMsg);
static int fieldGenCodeGen(T_Msg *ptMsg);
static int fieldGenCodeParseOne(T_Field *ptField);
static int filedGenCodeParseOnePart(H_LIST hList, T_Field_Option *ptOption);
static int fieldGenCodeGenOne(T_Field *ptField);
static int fieldGenCodeGenOnePart(H_LIST hList, T_Field_Option *ptOption);
static int fieldGenCodeParseUnSort(char *psNodeName, H_LIST hFieldList);
static int fieldCodeGenOneUnSort(T_Field *ptField, int iPreCheck);

static T_Field * filedParse(char *psName, char *psValue, H_LIST hMsgList);
static int getSameNum(T_Field *ptFrist, T_Field *ptSecond);
static int printfSameNum(int iLast, int iNow, int iMax, T_Field *ptField);
static int checkHasValue(T_Value *ptValue, T_Field_Option *ptOption);
static int splitN(char *sStr, const char *sSep, int *piCnt, char *psCol[]);
static int setOutPutFileByInPutFile(char *psFile);
static int setOutPutFile(char *psFile);
static int nodeInfoSet(char *sMax, char *sMin, char *sLoop, char *psRestore, T_Value *ptValue, int iIndex);
static int errorPrintf(int iType, T_Value *ptValue, T_Field_Option *ptOption);
static char * memdup(char *psBuf, int iLen);
static T_Value * fristStringValue(H_LIST hList);
static int isVarSetValue(T_Value *ptVar, T_Field_Option *ptOption);

/*-------------------------  Global variable ----------------------------*/
FNC_PROC f_fncParseDebug[TYPE_NUM] = {
    valueNullParase,
    valueStringParse,
    valueBitMapParse,
    valueNodeParse,
    valueVarParse,
    valueLimitParse,
    valueQuoteParse,
    valueAnyParse
};

FNC_PROC f_fncGenDebug[TYPE_NUM] = {
    valueNullGen,
    valueStringGen,
    valueBitMapGen,
    valueNodeGen,
    valueVarGen,
    valueLimitGen,
    valueQuoteGen,
    valueAnyGen
};

static int f_Level = 0;
static FILE *f_Fp = NULL;
static int f_NodeIndex = 0;

/*-------------------------  Global functions ---------------------------*/
int main(int argc, char *argv[])
{
    int iRet = 0;

    if (argc <= 1) {
        return -1;
    }

    f_Fp = stdout;

    char *psCsvFile = argv[1];
    FILE *ptFp = fopen(psCsvFile, "r");
    if (NULL == ptFp) {
        printf("file[%s] open err[%d:%s]", psCsvFile, errno, strerror(errno));
        return -2;
    }

    H_LIST hMsgList = listNew();

    setOutPutFileByInPutFile(psCsvFile);

    char sBuf[_DLEN_HUGE_BUF];
    while (1) {
        memset(sBuf, '\0', sizeof(sBuf));
        if (NULL == fgets(sBuf, sizeof(sBuf), ptFp) ) {
            break;
        }

        int iLen = strlen(sBuf);
        while (iLen >= 0) {
            if ('\r' == sBuf[iLen-1] || '\n' == sBuf[iLen-1]) {
                sBuf[iLen-1] = '\0';
                iLen -= 1;
                continue;
            }

            break;
        }

        enum {
            FIELD_NODE_NAME,
            FIELD_VAR_NAME,
            FIELD_VAR_VALUE,
            FIELD_BLANK,
            FIELD_NUM
        };

        int iNum = FIELD_NUM;
        char *ppsFiled[FIELD_NUM];
        iRet = splitN(sBuf, ",", &iNum, ppsFiled);
        if (iRet != 0) {
            return -3;
        }

        msgFieldAdd(hMsgList, ppsFiled[FIELD_NODE_NAME], ppsFiled[FIELD_VAR_NAME], ppsFiled[FIELD_VAR_VALUE]);
    }

    fclose(ptFp);

    genFile(hMsgList);

    return 0;
}


/*-------------------------  Local functions ----------------------------*/
static int genFile(H_LIST hMsgList)
{
    _("#include \"main.h\"");
    _("#include \"util_hash.h\"");

    msgGenCode(hMsgList);
    
    return 0;
}

static int msgGenCode(H_LIST hMsgList)
{
    LIST_LOOP(hMsgList, msgGenCodeFuncDef(ptIter));
    _("");

    LIST_LOOP(hMsgList, fieldGenCodeParse(ptIter);fieldGenCodeGen(ptIter));

    return 0;
}

static int msgGenCodeFuncDef(T_Msg *ptMsg)
{
    _("int %sParse(char *psBuf, int iPos, int iMax, T_HASH_TABLE *ptHash);", ptMsg->psName);
    _("int %sGen(char *psBuf, int iPos, int iMax, T_HASH_TABLE *ptHash);", ptMsg->psName);

    return 0;
}


static int msgFieldAdd(H_LIST hMsgList, char *psNodeName, char *psVarName, char *psValue)
{
    T_Msg *ptMsg = msgGet(hMsgList, psNodeName);
    listAdd(ptMsg->hList, filedParse(psVarName, psValue, hMsgList));

    return 0;
}

static T_Msg * msgGet(H_LIST hMsgList, char *psNodeName)
{
    T_Msg *ptMsg = NULL;
    LIST_LOOP(hMsgList, ptMsg = msgFind(ptIter, psNodeName, ptMsg));

    if (NULL == ptMsg) {
        ptMsg = malloc(sizeof(T_Msg));
        ptMsg->psName = strdup(psNodeName);
        ptMsg->hList = listNew();
        ptMsg->iOrderFlag = 0;
        listAdd(hMsgList, ptMsg);
    }

    return ptMsg;
}

static T_Msg * msgFind(T_Msg *ptMsg, char *psName, T_Msg *ptRet)
{
    if (0 == strcmp(ptMsg->psName, psName)) {
        return ptMsg;
    }

    return ptRet;
}

static T_Field * filedParse(char *psName, char *psValue, H_LIST hMsgList)
{
    T_Field * ptField = malloc(sizeof(T_Field));
    memset(ptField, '\0', sizeof(T_Field));

    ptField->psName = strdup(psName);
    ptField->psValue = strdup(psValue);
    ptField->hList = listNew();

    locParse(psValue, ptField->hList, hMsgList);

    return ptField;
}

static int fieldGenCodeParse(T_Msg *ptMsg)
{
    if (0 == listNum(ptMsg->hList)) {
        return 0;
    }

    if (ptMsg->iOrderFlag) {
        return fieldGenCodeParseUnSort(ptMsg->psName, ptMsg->hList);
    }

    /* Parse */
    _("int %sParse(char *psBuf, int iPos, int iMax, T_HASH_TABLE *ptHash)", ptMsg->psName);
    _("{");
    f_Level += 1;
    _("int iRet = 0;");
    _("");

    LIST_LOOP(ptMsg->hList, fieldGenCodeParseOne(ptIter));

    _("return iPos;");
    f_Level -= 1;
    _("}");
    _("");
    
    return 0;
}

static int fieldGenCodeParseOne(T_Field *ptField)
{
    T_Field_Option tOption;
    memset(&tOption, '\0', sizeof(tOption));

    tOption.psName = ptField->psName;

    _("/*");
    _(" * FIELD:%s VALUE:%s", ptField->psName, ptField->psValue);
    _(" */");
    _("{");
    f_Level += 1;

    LIST_LOOP(ptField->hList, checkHasValue(ptIter, &tOption));
    filedGenCodeParseOnePart(ptField->hList, &tOption);
    f_Level -= 1;
    _("}");
    _("");

    return 0;
}

static int filedGenCodeParseOnePart(H_LIST hList, T_Field_Option *ptOption)
{
    LIST_LOOP(hList, f_fncParseDebug[((T_Value *)ptIter)->eType](ptIter, ptOption));
    return 0;
}

static int fieldGenCodeGen(T_Msg *ptMsg)
{
    if (0 == listNum(ptMsg->hList)) {
        return 0;
    }

    /* Gen */
    _("int %sGen(char *psBuf, int iPos, int iMax, T_HASH_TABLE *ptHash)", ptMsg->psName);
    _("{");
    f_Level = 1;
    _("int iRet = 0;");
    _("");

    LIST_LOOP(ptMsg->hList, fieldGenCodeGenOne(ptIter));

    _("return iPos;");
    f_Level = 0;
    _("}");
    _("");

    return 0;
}

static int fieldGenCodeGenOne(T_Field *ptField)
{
    T_Field_Option tOption;
    memset(&tOption, '\0', sizeof(tOption));

    tOption.psName = ptField->psName;

    _("/*");
    _(" * FIELD:%s VALUE:%s", ptField->psName, ptField->psValue);
    _(" */");
    _("{");
    f_Level += 1;
    fieldGenCodeGenOnePart(ptField->hList, &tOption);
    f_Level -= 1;
    _("}");
    _("");

    return 0;
}

static int fieldGenCodeGenOnePart(H_LIST hList, T_Field_Option *ptOption)
{
    LIST_LOOP(hList, f_fncGenDebug[((T_Value *)ptIter)->eType](ptIter, ptOption));
    return 0;
}

static int fieldGenCodeParseUnSort(char *psNodeName, H_LIST hFieldList)
{
    /* Parse */
    _("int %sParse(char *psBuf, int iPos, int iMax, T_HASH_TABLE *ptHash)", psNodeName);
    _("{");
    f_Level = 1;
    _("int iRet = 0;");
    _("");

    H_LIST hSortFieldList = listSort(hFieldList, FiledSort);

    _("while (1) {");
    f_Level += 1;

    int iLastSameNum = 0;
    int iMaxSameNum = -1;
    T_Field *ptNow = NULL;
    T_ListIter tIter;
    listIterInit(&tIter, hSortFieldList);
    while (1) {
        void *ptIter = listIterFetch(&tIter);
        if (NULL == ptIter) {
            break;
        }

        T_Field *ptNext = ptIter;
        if (NULL == ptNow) {
            ptNow = ptNext;
            continue;
        }

        int iSameNum = getSameNum(ptNow, ptNext);
        iLastSameNum = printfSameNum(iLastSameNum, iSameNum, iMaxSameNum, ptNow);
        if (iSameNum > iMaxSameNum) {
            iMaxSameNum = iSameNum;
        }
        ptNow = ptNext;
    }

    printfSameNum(iLastSameNum, 0, iMaxSameNum, ptNow);

    f_Level -= 1;
    _("}");
    _("");
    _("return iPos;");
    f_Level = 0;
    _("}");
    _("");

    return 0;
}

static int getSameNum(T_Field *ptFrist, T_Field *ptSecond)
{
    T_Value *ptFristValue = fristStringValue(ptFrist->hList);
    T_Value *ptSecodeValue = fristStringValue(ptSecond->hList);

    int i = 0;
    while (1) {
        if (ptFristValue->tVar.psVar[i] != ptSecodeValue->tVar.psVar[i]) {
            break;
        }
        i++;
    }

    return i+1;
}

static int printfSameNum(int iLast, int iNow, int iMax, T_Field *ptField)
{
    T_Value *pValue = fristStringValue(ptField->hList);

    if (iLast != 0) {
        f_Level -= 1;
        _("} else if ('%c' == psBuf[iPos+%d]) {", pValue->tVar.psVar[iLast-1], iLast-1);
        f_Level += 1;
        if (iLast >= iNow) {
            fieldCodeGenOneUnSort(ptField, iNow);
        }
    }

    int i = 0;
    while (1) {
        if (iLast + i < iNow) {
            if (iLast + i >= iMax) {
                _("if ( iPos + %d >= iMax ) {", iLast+i);
                _("    break;");
                _("}");
            }

            _("if ('%c' == psBuf[iPos+%d]) {", pValue->tVar.psVar[iLast+i], iLast+i);
            i += 1;
            f_Level += 1;
        } else if ( iLast + i > iNow ) {
            f_Level -= 1;
            _("} else {");
            _("    break;");
            _("}");
            i -= 1;
        } else {
            break;
        }
    }

    if (iLast < iNow) {
        fieldCodeGenOneUnSort(ptField, iNow);
    }

    return iNow;
}

static int fieldCodeGenOneUnSort(T_Field *ptField, int iPreCheck)
{
    T_Field_Option tOption;
    memset(&tOption, '\0', sizeof(tOption));

    tOption.psName = ptField->psName;
    tOption.iPreCheck = iPreCheck;
    tOption.iLoopStat = 1;

    _("/* FIELD:%s VALUE:%s */", ptField->psName, ptField->psValue);
    LIST_LOOP(ptField->hList, checkHasValue(ptIter, &tOption));
    filedGenCodeParseOnePart(ptField->hList, &tOption);
    _("");

    return 0;
}

static int FiledSort(void *ptFrist, void *ptSecond)
{
    T_Value *ptFristValue = fristStringValue(((T_Field *)ptFrist)->hList);
    T_Value *ptSecodeValue = fristStringValue(((T_Field *)ptSecond)->hList);

    return strcmp(ptFristValue->tVar.psVar, ptSecodeValue->tVar.psVar);
}

static T_Value * fristStringValue(H_LIST hList)
{
    T_Value *ptVar = NULL;

    T_ListIter tListIter;
    listIterInit(&tListIter, hList);
    while (1) {
        T_Value *ptIter = (T_Value *)listIterFetch(&tListIter);
        if (NULL == ptIter) {
            break;
        }

        if (TYPE_STRING == ptIter->eType) {
            ptVar = ptIter;
            break;
        }

        if (NULL != ptIter->hChild) {
            ptVar = fristStringValue(ptIter->hChild);
            if (ptVar != NULL) {
                break;
            }
        }
    }

    return ptVar;
}

static int locParse(char *psBuf, H_LIST hList, H_LIST hMsgList)
{
    H_LIST hHeap = listNew();
    char *pCur = psBuf;
    int iIndex = 1;
    f_NodeIndex = 1;

    while ('\0'!=*pCur) {

        /* normal string */
        char *pStart = pCur;
        while (1) {
            char caTemp = *pCur;
            if ('\\' == caTemp) {
                pCur += 2;
                continue;
            }

            if ('.' == caTemp || '[' == caTemp || '(' == caTemp || ')' == caTemp || '\0' == caTemp || '$' == caTemp) {
                break;
            }

            if (pStart != pCur && ('*' == caTemp || '{' == caTemp || '<' == caTemp)) {
                break;
            }

            pCur += 1;
        }

        if (pStart != pCur) {
            if ('*' == *pCur || '{' == *pCur || '<' == *pCur) {
                if (pCur -1 > pStart) {
                    listAdd(hList, valueNewString(pStart, (int)(pCur-pStart-1)));
                }
                pCur = setLoop(valueNewString(pCur-1, 1), hList, pCur);
                continue;
            } else {
                listAdd(hList, valueNewString(pStart, (int)(pCur-pStart)));
            }
        }

        /* . */
        if ('.' == *pCur) {
            T_Value * ptLast = valueNew(TYPE_ANY);
            pCur = setLoop(ptLast, hList, pCur+1);
            continue;
        }

        /* [....] */
        if ('[' == *pCur) {
            char *psStart = pCur + 1;
            char *psEnd = strchr(pCur, ']');
            T_Value *ptBitMap = valueNewBitMap(psStart, psEnd);
            pCur = setLoop(ptBitMap, hList, psEnd+1);
            continue;
        }

        /* ( | (?: */
        if ('(' == *pCur) {
            listPush(hHeap, hList);
            hList = listNew();

            if ('?' != pCur[1] || ':' != pCur[2]) {
                /* ( */
                T_Value *ptVar = valueNewVarIndex(iIndex);
                listPush(hHeap, ptVar);

                iIndex += 1;
                pCur += 1;
            } else if ( '<' == pCur[3] ) {
                /* (?:<...> */
                char *psStart = pCur + 4;
                char *psEnd = strchr(psStart, '>');
                T_Value *ptVar = valueNewVar(psStart, psEnd-psStart);
                listPush(hHeap, ptVar);

                pCur +=  3 + (psEnd - psStart) + 1;
            } else {
                /* (?: */
                T_Value *ptNode = valueNew(TYPE_NULL);
                listPush(hHeap, ptNode);

                pCur += 3;
            }

            continue;
        }

        /*  ) */
        if (')' == *pCur) {
            T_Value *ptValue = listPop(hHeap);
            ptValue->hChild = hList;
            hList = listPop(hHeap);

            if (TYPE_VAR == ptValue->eType) {
                pCur = setLoop(ptValue, hList, pCur+1);
            } else {
                pCur = setLoopNode(ptValue, pCur+1);
                listAdd(hList, ptValue);
            }

            continue;
        }

        /* $ */
        if ('$' == *pCur) {
            char *psStart = pCur + 1;
            if ( '{' != psStart[0] ) {
                listFree(hHeap);
                return -4;
            }

            char *psEnd = strchr(psStart, '}');
            T_Value *ptQuote = valueNewQuote(hMsgList, psStart+1, psEnd);
            pCur = setLoopNodeQuote(ptQuote, hList, psEnd+1);
        }

        if ('\0' == *pCur) {
            break;
        }
    }

    listFree(hHeap);

    return 0;
}

static T_Value * valueNewVarIndex(int iIndex)
{
    char sName[_DLEN_TINY_BUF];
    sprintf(sName, "Var%d", iIndex);

    return valueNewVar(sName, -1);
}

static T_Value * valueNewVar(char *psValue, int iLen)
{
    if (NULL == psValue || 0 == iLen) {
        return NULL;
    }

    T_Value *ptValue = valueNew(TYPE_VAR);

    if (iLen < 0) {
        iLen = strlen(psValue);
    }

    ptValue->tVar.psVar = malloc(iLen+1);
    memcpy(ptValue->tVar.psVar, psValue, iLen);
    ptValue->tVar.psVar[iLen] = '\0';

    ptValue->hChild = listNew();

    return ptValue;
}

static T_Value * valueNewString(char *psString, int iLen)
{
    if (NULL == psString || 0 == iLen) {
        return NULL;
    }

    T_Value *ptValue = valueNew(TYPE_STRING);

    if (iLen < 0) {
        iLen = strlen(psString);
    }

    ptValue->tString.psString = malloc(iLen+1);
    memcpy(ptValue->tString.psString, psString, iLen);
    ptValue->tString.psString[iLen] = '\0';

    int i=0,j=0;
    for (; '\0' != ptValue->tString.psString[i]; i++,j++) {
        if ('\\' == ptValue->tString.psString[i] && '\0' != ptValue->tString.psString[i+1]) {
            i++;
        }

        if (i != j) {
            ptValue->tString.psString[j] = ptValue->tString.psString[i];
        }
    }
    ptValue->tString.psString[j] = '\0';

    ptValue->tString.iLen = j;


    return ptValue;
}

static T_Value * valueNewBitMap(char *psStart, char *psEnd)
{
    T_Value *ptBitMap = valueNew(TYPE_BITMAP);

    char *p = psStart;
    if ('^' == p[0]) {
        ptBitMap->tBitMap.iNotFlag = 1;
        p += 1;
    }

    char *psBitMap = malloc(_DLEN_BITMAP);
    memset(psBitMap, '\0', _DLEN_BITMAP);
    for (; p != psEnd; ++p) {
        if (p != psStart && '-' == p[0]) {
            if (p[-1] == p[1]) {
                p += 1;
                continue;
            }

            unsigned char caStart = p[-1];
            unsigned char caEnd   = p[1];

            if (caStart > caEnd) {
                caStart = p[1];
                caEnd   = p[-1];
            }

            for (; caStart<=caEnd; caStart++) {
                BITMAP_SET(psBitMap, caStart);
            }
            continue;
        } else if ( '\\' == p[0] ) {
            p += 1;
        }

        BITMAP_SET(psBitMap, (unsigned char )(p[0]));
    }

    ptBitMap->tBitMap.psBitMap = psBitMap;
    ptBitMap->tBitMap.psOrg = memdup(psStart, psEnd-psStart);

    return ptBitMap;
}

static T_Value * valueNewQuote(H_LIST hMsgList, char *psStart, char *psEnd)
{
    T_Value *ptValue = valueNew(TYPE_QUOTE);

    int iOrederFlag = 0;
    if ('?' == psStart[0]) {
        iOrederFlag = 1;
        psStart += 1;
    }

    ptValue->tQuote.psQutoe = memdup(psStart, psEnd-psStart);
    T_Msg *ptMsg = msgGet(hMsgList, ptValue->tQuote.psQutoe);
    ptMsg->iOrderFlag = iOrederFlag;

    return ptValue;
}

static char * setLoop(T_Value *ptValue, H_LIST hList, char *pCur)
{
    T_Value tNode;
    memset(&tNode, '\0', sizeof(tNode));

    char *pRet = setLoopNode(&tNode, pCur);
    if (pRet == pCur) {
        listAdd(hList, ptValue);
        return pCur;
    }

    T_Value *ptNew = valueNew(tNode.eType);
    memcpy(ptNew, &tNode, sizeof(tNode));

    ptNew->hChild = listNew();
    listAdd(ptNew->hChild, ptValue);
    listAdd(hList, ptNew);

    return pRet;
}

static char * setLoopNodeQuote(T_Value *ptQuote, H_LIST hList, char *pCur)
{
    T_Value *ptNew = valueNew(TYPE_NULL);

    pCur = setLoopNode(ptNew, pCur);

    ptNew->hChild = listNew();
    listAdd(ptNew->hChild, ptQuote);
    listAdd(hList, ptNew);

    return pCur;
}

static char * setLoopNode(T_Value *ptNode, char *psBuf)
{
    if ('*' == psBuf[0] ) {
        ptNode->eType = TYPE_NODE;
        ptNode->tNode.psMax = _LOOP_UNLIMITED;
        ptNode->tNode.psMin = "0";

        return psBuf + 1;
    }

    if ('+' == psBuf[0]) {
        ptNode->eType = TYPE_NODE;
        ptNode->tNode.psMax = _LOOP_UNLIMITED;
        ptNode->tNode.psMin = "1";

        return psBuf + 1;
    }

    if ('?' == psBuf[0]) {
        ptNode->eType = TYPE_NODE;
        ptNode->tNode.psMax = "1";
        ptNode->tNode.psMin = "0";

        return psBuf + 1;
    }

    if ('{' == psBuf[0]) {
        ptNode->eType = TYPE_NODE;
        char *psMin = NULL;
        char *psEnd = strpbrk(psBuf+1, ",}");
        if (NULL == psEnd) {
            return NULL;
        }

        if (',' == *psEnd) {
            psMin = psEnd;
            psEnd = strchr(psBuf, '}');
        }

        if (NULL == psMin) {
            ptNode->tNode.psMin = NULL;
            ptNode->tNode.psMax = memdup(psBuf+1, psEnd-psBuf-1);
        } else {
            ptNode->tNode.psMin = memdup(psBuf+1, psMin-psBuf-1);
            ptNode->tNode.psMax = memdup(psMin+1, psEnd-psMin-1);
        }
        
        return psEnd + 1;
    }

    if ('<' == psBuf[0]) {
        ptNode->eType = TYPE_LIMIT;
        char *psEnd = strchr(psBuf+1, '>');
        ptNode->tLimit.psLimit = memdup(psBuf+1, psEnd-psBuf-1);

        return psEnd + 1;
    }

    ptNode->eType = TYPE_NODE;
    ptNode->tNode.psMax = "1";
    ptNode->tNode.psMin = NULL;

    return psBuf;
}

static T_Value * valueNew(E_TYPE eType)
{
    T_Value *ptValue = malloc(sizeof(T_Value));
    memset(ptValue, '\0', sizeof(T_Value));

    ptValue->eType = eType;
    ptValue->hChild = NULL;

    return ptValue;
}

static void valueNullParase(T_Value *ptVcalue, T_Field_Option *ptOption)
{
    _("should not have null error;");
}

static void valueStringParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("/* [string] \"%s\" */", ptValue->tString.psString);
    if (ptOption->iPreCheck >= ptValue->tString.iLen) {
        return;
    }

    char *psStr = ptValue->tString.psString;
    int iLen = ptValue->tString.iLen;
    if (ptOption->iPreCheck > 0) {
        _("iPos += %d;", ptOption->iPreCheck);
        iLen -= ptOption->iPreCheck;
        psStr += ptOption->iPreCheck;
    }

    _("if ( iPos + %d > iMax ) {", iLen);
    if (ptOption->iLoopStat) {
        _("    break;");
    } else {
        errorPrintf(ERR_BUFFER_TOO_SMALL, ptValue, ptOption);
    }
    _("}");

    if (1 == iLen) {
        _("if ('%c' != psBuf[iPos]) {", psStr[0]);
    } else {
        _("if (0 != memcmp(psBuf+iPos, \"%s\", %ld)) {", psStr, iLen);
    }

    if (ptOption->iLoopStat) {
        _("    break;");
    } else {
        errorPrintf(ERR_STRING_NOT_SAMLE, ptValue, ptOption);
    }
    _("}");
    _("iPos += %ld;", iLen);

    _("");

    ptOption->iPreCheck = 0;
}

static void valueNodeParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    char sMax[_DLEN_TINY_BUF];
    char sMin[_DLEN_TINY_BUF];
    char sLoop[_DLEN_TINY_BUF];
    char sRestore[_DLEN_TINY_BUF];
    int iIndex = ptOption->iLoopIndex;
    ptOption->iLoopIndex += 1;

    nodeInfoSet(sMax, sMin, sLoop, sRestore, ptValue, iIndex);

    if (0 == strcmp(sMax, "1") && NULL == ptValue->tNode.psMin) {
        filedGenCodeParseOnePart(ptValue->hChild, ptOption);
        return;
    }

    if ('$' == ptValue->tNode.psMax[0]) {
        _("int iLoopMax%d = strStr2Int(Var%s.psValue, Var%s.iLen);", iIndex, ptValue->tNode.psMax+1, ptValue->tNode.psMax+1);
    }

    if (NULL != ptValue->tNode.psMin) {
        _("int %s = iPos;", sRestore);
    }

    _("int %s = 0;", sLoop);
    if ('-' == sMax[0]) {
        _("for (;; %s++) {", sLoop);
    } else {
        _("for (;%s < %s; %s++) {", sLoop, sMax, sLoop);
    }
    if (NULL != ptValue->tNode.psMin) {
        ptOption->iLoopStat += 1;
    }
    f_Level += 1;

    if (ptValue->tNode.iNode && NULL != ptOption->psName && '\0' != ptOption->psName[0]) {
        _("iRet = setNode(\"%s\");", ptOption->psName);
        _("if ( iRet != 0 ) {");
        _("    return -4;");
        _("}");
        _("");
    }

    filedGenCodeParseOnePart(ptValue->hChild, ptOption);

    if (NULL != ptValue->tNode.psMin) {
        _("%s = iPos;", sRestore);
    }

    f_Level -= 1;
    _("}");
    _("");

    if (NULL != ptValue->tNode.psMin) {
        ptOption->iLoopStat -= 1;
        if (0 != strcmp("0", sMin)) {
            _("if ( %s < %s ) { ", sLoop, sMin);
            errorPrintf(ERR_LOOP_MIN, ptValue, ptOption);
            _("}");
        }
        _("iPos = %s;", sRestore);
    }
}

static int nodeInfoSet(char *sMax, char *sMin, char *sLoop, char *psRestore, T_Value *ptValue, int iIndex)
{
    if ('$' == ptValue->tNode.psMax[0]) {
        sprintf(sMax, "iLoopMax%d", iIndex);
    } else {
        sprintf(sMax, "%d", atoi(ptValue->tNode.psMax));
    }

    if (NULL != ptValue->tNode.psMin) {
        if ('$' == ptValue->tNode.psMin[0]) {
            sprintf(sMin, "iLoopMin%d", iIndex);
        } else {
            sprintf(sMin, "%d", atoi(ptValue->tNode.psMin));
        }
    }

    sprintf(sLoop, "iLoop%d", iIndex);
    sprintf(psRestore, "iRestore%d", iIndex);

    return 0;
}

static void valueLimitParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    if ('$' != ptValue->tLimit.psLimit[0] && atoi(ptValue->tLimit.psLimit) <= 0) {
        return;
    }

    if (!ptOption->iMaxTempInit) {
        ptOption->iMaxTempInit = 1;
        _("int iMaxTemp = iMax;");
    } else {
        _("iMaxTemp = iMax;");
    }

    if ('$' == ptValue->tLimit.psLimit[0]) {
        _("iMax = iPos + strStr2Int(Var%s.psValue, Var%s.iLen);", ptValue->tLimit.psLimit+1, ptValue->tLimit.psLimit+1);
    } else {
        _("iMax = iPos + %s;", ptValue->tLimit.psLimit);
    }

    _("");

    filedGenCodeParseOnePart(ptValue->hChild, ptOption);

    _("if ( iPos != iMax ) {");
    _("    return -2;");
    _("}");
    _("iMax = iMaxTemp;");
    _("");
}

static void valueVarParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("/* [value start] %s */", ptValue->tVar.psVar);
    _("T_VALUE %s;", ptValue->tVar.psVar);
    _("%s.psValue = psBuf+iPos;", ptValue->tVar.psVar);
    _("%s.iLen = 0;", ptValue->tVar.psVar);
    _("");

    filedGenCodeParseOnePart(ptValue->hChild, ptOption);

    _("/* [value end] %s */", ptValue->tVar.psVar);
    _("%s.iLen = psBuf + iPos - %s.psValue;", ptValue->tVar.psVar, ptValue->tVar.psVar);
    _("");

    if (isVarSetValue(ptValue, ptOption)) {
        _("iRet = setValue(\"%s\", &%s);", ptOption->psName, ptValue->tVar.psVar);
        _("if ( iRet != 0 ) {");
        errorPrintf(ERR_SET_VALUE, NULL, ptOption);
        _("}");
    }
}

static int isVarSetValue(T_Value *ptVar, T_Field_Option *ptOption)
{
    if (1 == ptOption->iValueNum) {
        return 1;
    }

    if (0 == ptOption->iHasValue && 0 != memcmp(ptVar->tVar.psVar, "Var", 3)) {
        return 1;
    }

    if (1 == ptOption->iHasValue && 0 == strcmp(ptVar->tVar.psVar, "Value")) {
        return 1;
    }

    return 0;
}

static void valueQuoteParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("iRet = %sParse(psBuf, iPos, iMax, ptHash);", ptValue->tQuote.psQutoe);
    _("if ( iRet < 0 ) {");
    errorPrintf(ERR_QUORE_PARSE, ptValue, ptOption);
    _("}");
    _("iPos = iRet;");
    _("");
}

static void valueAnyParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("if ( iPos + 1 > iMax ) {");
    if (ptOption->iLoopStat) {
        _("    break;");
    } else {
        errorPrintf(ERR_BUFFER_TOO_SMALL, ptValue, ptOption);
    }
    _("}");
    _("iPos += 1;");
    _("");
}

static void valueBitMapParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    unsigned char caFrist;
    unsigned char caLast;
    _("/* [bitmap] %s */", ptValue->tBitMap.psOrg);

    _("if ( iPos + 1 > iMax ) {");
    if (ptOption->iLoopStat) {
        _("    break;");
    } else {
        errorPrintf(ERR_BUFFER_TOO_SMALL, ptValue, ptOption);
    }
    _("}");
    _("");

    _s();__("if (");
    if (ptValue->tBitMap.iNotFlag) {
        __("!(");
    }

    int iFlag = 0;
    unsigned int i = 0;
    for (i=1; i<=128; i++) {
        if (BITMAP_TEST(ptValue->tBitMap.psBitMap, i)) {
            storeChar(i, NULL, NULL);
        } else {
            storeChar(0, &caFrist, &caLast);
            printfChar(caFrist, caLast, &iFlag);
        }
    }
    storeChar(0, &caFrist, &caLast);
    printfChar(caFrist, caLast, &iFlag);

    if (ptValue->tBitMap.iNotFlag) {
        __(")");
    }

    __(") {\n");

    if (ptOption->iLoopStat) {
        _("    break;");
    } else {
        errorPrintf(ERR_BITMAP_NOT_SAMLE, ptValue, ptOption);
    }
    _("}");
    _("iPos += 1;");
    _("");
}

static int printfChar(unsigned char caFrist, unsigned char caLast, int *piFlag)
{
    char sFrist[6];
    char sLast[6];

    if ('\0' == caFrist) {
        return 0;
    }

    if (*piFlag) {
        __(" && ");
    }

    if (isprint(caFrist)) {
        sprintf(sFrist, "'%c'", caFrist);
    } else {
        sprintf(sFrist, "0x%03d", caFrist);
    }

    *piFlag = 1;
    if ('\0' == caLast) {
        __("(psBuf[iPos] != %s)", sFrist);
    } else {
        if (isprint(caLast)) {
            sprintf(sLast, "'%c'", caLast);
        } else {
            sprintf(sLast, "0x%03d", caLast);
        }
        __("(psBuf[iPos] < %s || psBuf[iPos] > %s)", sFrist, sLast);
    }

    return 0;
}

static int storeChar(unsigned char caChar, unsigned char *psFrist, unsigned char *psLast)
{
    static unsigned char caFrist = 0;
    static unsigned char caLast = 0;

    if (0 == caChar) {
        *psFrist = caFrist;
        *psLast = caLast;

        caFrist = 0;
        caLast = 0;

        return 0;
    }

    if (0 == caFrist) {
        caFrist = caChar;
        return 0;
    }
    
    caLast = caChar;
    
    return 0;
}

static int _s()
{
    int i = 0;
    for (i=0; i<f_Level; i++) {
        fprintf(f_Fp, "    ");
    }

    return 0;
}

static int _(char *psBuf, ...)
{
    va_list	vaArgs;
	int		iRet;

    _s();

	va_start(vaArgs, psBuf);
	iRet = vfprintf(f_Fp, psBuf, vaArgs);
	va_end(vaArgs);
    fprintf(f_Fp, "\n");

	return iRet;
}

static int __(char *psBuf, ...)
{
    va_list	vaArgs;
	int		iRet;

	va_start(vaArgs, psBuf);
	iRet = vfprintf(f_Fp, psBuf, vaArgs);
	va_end(vaArgs);

	return iRet;
}

static void valueNullGen(T_Value *ptVcalue, T_Field_Option *ptOption)
{
    _("should not have null gen error;");
}

static void valueStringGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    char *psString = ptValue->tString.psString;
    int iLen = ptValue->tString.iLen;

    _("/* [string] \"%s\" */", psString);

    _("if (iPos + %d > iMax) {", iLen);
    if (ptOption->iLoopStat) {
        _("    break;");
    } else {
        errorPrintf(ERR_BUFFER_TOO_SMALL, ptValue, ptOption);
    }
    _("}");

    if (1 == iLen) {
        _("psBuf[iPos] = '%s';", psString);
    } else {
        _("memcpy(psBuf+iPos, \"%s\", %d);", psString, iLen);
    }
    _("iPos += %d;", iLen);
    _("");
}

static void valueBitMapGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("/* [bitmap] */");

    _("if (iPos + 1 > iMax) {");
    if (ptOption->iLoopStat) {
        _("    break;");
    } else {
        errorPrintf(ERR_BUFFER_TOO_SMALL, ptValue, ptOption);
    }
    _("}");

    unsigned int i = 0;
    for (i=1; i<=128; i++) {
        if (BITMAP_TEST(ptValue->tBitMap.psBitMap, i)) {
            break;
        }
    }
    if (isprint(i)) {
        _("psBuf[iPos] = '%c';", i);
    } else {
        _("psBuf[iPos] = %d;", i);
    }
    _("iPos += 1;");
}

static void valueNodeGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (ptOption->iLoopStat) {
        return;
    }

    char sMax[_DLEN_TINY_BUF];
    char sMin[_DLEN_TINY_BUF];
    char sLoop[_DLEN_TINY_BUF];
    char sRestore[_DLEN_TINY_BUF];
    int iIndex = ptOption->iLoopIndex;
    ptOption->iLoopIndex += 1;

    nodeInfoSet(sMax, sMin, sLoop, sRestore, ptValue, iIndex);

    if (0 == strcmp(sMax, "1") && NULL == ptValue->tNode.psMin) {
        fieldGenCodeGenOnePart(ptValue->hChild, ptOption);
        return;
    }

    _("int %s = 0;", sLoop);
    if ('$' == ptValue->tNode.psMax[0]) {
        _("for (;;%s++) {", sLoop);
        ptOption->iLoopStat += 1;
    } else {
        _("int %s = iPos;", sRestore);
        _("for (;%s<%s;%s++) {", sLoop, sMax, sLoop);
        ptOption->iLoopStat += 1;
    }
    f_Level += 1;

    if (ptValue->tNode.iNode && NULL != ptOption->psName && '\0' != ptOption->psName[0]) {
        _("iRet = getNode(\"%s\");", ptOption->psName);
        _("if ( iRet != 0 ) {");
        if (ptOption->iLoopStat) {
            _("    break;");
        } else {
            _("    return -3;");
        }
        _("}");
        _("");
    }

    fieldGenCodeGenOnePart(ptValue->hChild, ptOption);

    if ('$' != ptValue->tNode.psMax[0]) {
        _("%s = iPos;", sRestore);
    }

    f_Level -= 1;
    _("}");
    _("");

    if ('$' != ptValue->tNode.psMax[0]) {
        ptOption->iLoopStat -= 1;
        if (0 != strcmp("0", sMin)) {
            _("if ( %s < %s ) { ", sLoop, sMin);
            errorPrintf(ERR_LOOP_MIN, ptValue, ptOption);
            _("}");
        }
        _("iPos = %s;", sRestore);
    }

    if ('$' == ptValue->tNode.psMax[0]) {
        ptOption->iLoopStat -= 1;
        _("strInt2Str(%s, Var%s.psValue, Var%s.iLen);", sLoop, ptValue->tNode.psMax+1, ptValue->tNode.psMax+1);
    }
}

static void valueLimitGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    if ('$' == ptValue->tLimit.psLimit[0]) {
        if (!ptOption->iPosTempInit) {
            ptOption->iPosTempInit = 1;
            _("int iPosTemp = iPos;");
        }
        _("iPosTemp = iPos;");
    }

    fieldGenCodeGenOnePart(ptValue->hChild, ptOption);

    if ('$' == ptValue->tLimit.psLimit[0]) {
        _("strInt2Str(iPos-iPosTemp, Var%s.psValue, Var%s.iLen);", ptValue->tLimit.psLimit+1, ptValue->tLimit.psLimit+1);
    }
}

static int checkHasValue(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (TYPE_VAR != ptValue->eType) {
        if (ptValue->hChild != NULL) {
            LIST_LOOP(ptValue->hChild, checkHasValue(ptIter, ptOption));
        }

        return 0;
    }

    ptOption->iValueNum += 1;
    if (0 == strcmp(ptValue->tVar.psVar, "Value")) {
        ptOption->iHasValue = 1;
        return 0;
    }

    return 0;
}

static void valueVarGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("/* [var start] %s */", ptValue->tVar.psVar);
    _("T_VALUE %s;", ptValue->tVar.psVar);
    _("%s.psValue = psBuf+iPos;", ptValue->tVar.psVar);
    _("iRet = getValue(\"%s\", psBuf + iPos, iMax);", ptOption->psName);
    _("if ( iRet < 0 ) {");
    if (ptOption->iLoopStat) {
        _("    break;");
    } else {
        errorPrintf(ERR_GET_VALUE, ptValue, ptOption);
    }
    _("}");
    _("%s.iLen = iRet;", ptValue->tVar.psVar);
    _("iPos += iRet;");
    _("");
}

static void valueQuoteGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("iRet = %sGen(psBuf, iPos, iMax, ptHash);", ptValue->tQuote.psQutoe);
    _("if ( iRet < 0 ) {");
    errorPrintf(ERR_QUORE_GEN, ptValue, ptOption);
    _("}");
    _("iPos = iRet;");
    _("");
}

static void valueAnyGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("if ( iPos + 1 > iMax ) {");
    if (ptOption->iLoopStat) {
        _("    break;");
    } else {
        errorPrintf(ERR_BUFFER_TOO_SMALL, ptValue, ptOption);
    }
    _("}");
    _("psBuf[iPos] = '\\0';");
    _("iPos += 1;");
    _("");
}

static int splitN(char *sStr, const char *sSep, int *piCnt, char *psCol[])
{
	char	*pCur;
	int		iFlag, iMax = *piCnt, iLen = strlen(sSep);

	*piCnt = 0;
	if (iMax <= 0) return 0;

	pCur = sStr - 1;
	iFlag = 1;

	while (pCur < sStr || *pCur != '\0') {
		if (strncmp(pCur, sSep, iLen) == 0) {
			if (pCur >= sStr) {
				*pCur = '\0';
				pCur += iLen - 1;
				iFlag = 1;
			}
		}

		if (iFlag) {
			psCol[(*piCnt)++] = pCur + 1;
			if (*piCnt >= iMax) return 1;
			iFlag = 0;
		}

		pCur++;
	}
    
	return 0;
}

static int setOutPutFileByInPutFile(char *psFile)
{
    char *psOutFile = strdup(psFile);
    char *p = strrchr(psOutFile, '.');
    if (NULL == p) {
        return -1;
    }

    p[1] = 'c';
    p[2] = '\0';

    return setOutPutFile(psOutFile);
}

static int setOutPutFile(char *psFile)
{
    f_Fp = fopen(psFile, "w");
    if (NULL == f_Fp) {
        printf("file[%s] open err[%d:%s]\n", psFile, errno, strerror(errno));
        return -1;
    }

    return 0;
}

static int errorPrintf(int iType, T_Value *ptValue, T_Field_Option *ptOption)
{
    _s();

    if (NULL != ptOption->psName && '\0' != ptOption->psName[0]) {
        __("    printf(\"Filed[%s] ", ptOption->psName);
    } else {
        __("    printf(\"");
    }

    switch (iType) {
        case ERR_BUFFER_TOO_SMALL:
            __("ERROR: buffer[%%d] too small\\n\", iMax);");
            break;

        case ERR_STRING_NOT_SAMLE:
            __("ERROR at[%%d]: [%%.*s] need [%s]\\n\", iPos, %d, psBuf+iPos);", ptValue->tString.psString+ptOption->iPreCheck, ptValue->tString.iLen-ptOption->iPreCheck);
            break;

        case ERR_BITMAP_NOT_SAMLE:
            __("ERROR at[%%d]: [%%c] need [%s]\\n\", iPos, psBuf[iPos]);", ptOption->psName, ptValue->tBitMap.psOrg);
            break;

        case ERR_GET_VALUE:
            __("ERROR: get value err[%%d]\\n\", iRet);");
            break;

        case ERR_SET_VALUE:
            __("ERROR: set value err[%%d]\\n\", iRet);");
            break;

        case ERR_LOOP_MIN:
            __("ERROR: loop miss\\n\");");
            break;

        case ERR_QUORE_PARSE:
            __("ERROR: %sParse err[%%d]\\n\", iRet);", ptValue->tQuote.psQutoe);
            break;

        case ERR_QUORE_GEN:
            __("ERROR: %sGen err[%%d]\\n\", iRet);", ptValue->tQuote.psQutoe);
            break;
    }

    __("\n");
    _("    return %d;", ERR_BASE + iType);

    return 0;
}

static char * memdup(char *psBuf, int iLen)
{
    char *psReturn = malloc(iLen + 1);
    memcpy(psReturn, psBuf, iLen);
    psReturn[iLen] = '\0';

    return psReturn;
}

/*-----------------------------  End ------------------------------------*/

