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

#include "list.h"

/*--------------------------- Macro define ------------------------------*/
#define _DLEN_TINY_BUF   64
#define _DLEN_MINI_BUF   256
#define _DLEN_LARGE_BUF  1024
#define _DLEN_HUGE_BUF   10240

#define _DLEN_BITMAP     16
#define _NUM_BITMAP      _DLEN_BITMAP*8

#define _LOOP_UNLIMITED  (-1)
#define _LOOP_VAR        (-2)

#define BITMAP_MASK( bit )         (0x80>>(((bit)-1)&0x07))
#define BITMAP_INDEX( bit )        (((bit)-1)>>3)
#define BITMAP_TEST( bitmap, bit ) ((bitmap)[BITMAP_INDEX(bit)]&BITMAP_MASK(bit))
#define BITMAP_CLR( bitmap, bit )  (bitmap[BITMAP_INDEX(bit)]&=~BITMAP_MASK(bit))
#define BITMAP_SET( bitmap, bit )  (bitmap[BITMAP_INDEX(bit)]|=BITMAP_MASK(bit))

#define __ printf

/*---------------------------- Type define ------------------------------*/
typedef enum {
    TYPE_STRING,
    TYPE_BITMAP,
    TYPE_NODE_START,
    TYPE_NODE_END,
    TYPE_VALUE_START,
    TYPE_VALUE_END,
    TYPE_LIMIT_START,
    TYPE_LIMIT_END,
    TYPE_QUOTE_START,
    TYPE_QUOTE_END,
    TYPE_NODE_QUOTE,
    TYPE_NUM
} E_TYPE;

typedef struct {
    char *psValue;
    char *psVar;
    union {
        int iLoop;
        int iFlag;
        int iLen;
    };
    E_TYPE eType;
} T_Value;

typedef struct {
    char *psName;
    char *psValue;
    H_LIST hList;
} T_Field;

typedef struct {
    char *psName;
    H_LIST hList;
    int iType;
} T_Msg;

typedef struct {
    int iLoop;
    int iLoopInit;
    int iINum;
    int iMaxTempInit;
    int iLenInit;
    int iPosTempInit;
    int iPreCheck;
    char *psName;
} T_Field_Option;
static int f_Level = 0;

typedef void (*FNC_PROC)(T_Value *ptValue, T_Field_Option *ptOption);

/*---------------------- Local function declaration ---------------------*/
static int locParse(char *psBuf, H_LIST hList, H_LIST hMsgList);
static T_Value * valueNew(E_TYPE eType, char *psValue, int iLen);

static int storeChar(unsigned char caChar, unsigned char *psFrist, unsigned char *psLast);
static char * getBitMap(char *psBuf, int *piFlag, int *piLen);
static char * setLoop(T_Value *ptValue, H_LIST ptList, char *pCur);
static char * getVarName(char *psBuf, int iLen, H_LIST hList);
static int getLoop(char *psBuf, int *piLen, E_TYPE *eStart, E_TYPE *eEnd);
static int printfChar(unsigned char caFrist, unsigned char caLast, int *piFlag);
static int _s();
static int _(char *psBuf, ...);
static char * setLoopNode(T_Value *ptStartNode, T_Value *ptEndNode, H_LIST hList, char *pCur);
static int setQuote(T_Value *ptValue, char *psVarName);
static int FiledSort(void *ptFrist, void *ptSecond);
static T_Msg * msgFind(T_Msg *ptMsg, char *psName, T_Msg *ptRet);
static int msgFieldAdd(H_LIST ptList, char *psNodeName, char *psVarName, char *psValue);
static int msgGenCodeFuncDef(T_Msg *ptMsg);
static int msgGenCode(H_LIST hMsgList);
static T_Msg * msgGet(H_LIST hMsgList, char *psNodeName);

static void valueStringParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueBitMapParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueNodeStartParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueNodeEndParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueValueStartParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueValueEndParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueLimitStartParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueLimitEndParse(T_Value *ptValue, T_Field_Option *ptOption);
static void valueNodeQuoteParse(T_Value *ptValue, T_Field_Option *ptOption);

static void valueStringGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueBitMapGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueNodeStartGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueNodeEndGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueValueStartGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueValueEndGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueLimitStartGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueLimitEndGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueQuoteStartGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueQuoteEndGen(T_Value *ptValue, T_Field_Option *ptOption);
static void valueNodeQuoteGen(T_Value *ptValue, T_Field_Option *ptOption);

static int fieldGenCodeParse(T_Msg *ptMsg);
static int fieldGenCodeGen(T_Msg *ptMsg);
static char * locPrintVar(T_Value *ptValue, char *psVar);
static T_Field * filedParse(char *psName, char *psValue, H_LIST hMsgList);
static int fieldGenCodeParseOne(T_Field *ptField);
static int fieldGenCodeGenOne(T_Field *ptField);
static int fieldGenCodeParseUnSort(char *psNodeName, H_LIST hFieldList);
static int getSameNum(T_Field *ptFrist, T_Field *ptSecond);
static int printfSameNum(int iLast, int iNow, int iMax, T_Field *ptField);
static int fieldCodeGenOneUnSort(T_Field *ptField, int iPreCheck);

/*-------------------------  Global variable ----------------------------*/
FNC_PROC f_fncParseDebug[TYPE_NUM] = {
    valueStringParse,
    valueBitMapParse,
    valueNodeStartParse,
    valueNodeEndParse,
    valueValueStartParse,
    valueValueEndParse,
    valueLimitStartParse,
    valueLimitEndParse,
    valueValueStartParse,
    valueValueEndParse,
    valueNodeQuoteParse
};

FNC_PROC f_fncGenDebug[TYPE_NUM] = {
    valueStringGen,
    valueBitMapGen,
    valueNodeStartGen,
    valueNodeEndGen,
    valueValueStartGen,
    valueValueEndGen,
    valueLimitStartGen,
    valueLimitEndGen,
    valueQuoteStartGen,
    valueQuoteEndGen,
    valueNodeQuoteGen
};


/*-------------------------  Global functions ---------------------------*/
int main(int argc, char *argv[])
{
    _("#include <string.h>");
    _("#include <stdio.h>");
    _("");
    _("#define	min(x,y)		((x) < (y) ? (x) : (y))");
    _("");
    _("typedef struct {");
    _("    char *psValue;");
    _("    int iLen;");
    _("} T_VALUE;");
    _("");

    _("char *strInt2Str(int nNum, char *sBuf, size_t nBufLen)");
    _("{");
	_("    char sTmp[32];");
	_("    int  nLen = nBufLen;");
    _("");
	_("    sprintf(sTmp, \"%%0*d\", nLen, nNum);");
	_("    memcpy(sBuf, sTmp, nBufLen);");
    _("");
	_("    return (sBuf);");
    _("}");

    _("");
    _("int strStr2Int(char *sBuf, size_t nBufLen)");
    _("{");
	_("    char sTmp[32];");
    _("");
	_("    memset(sTmp, '\\0', sizeof(sTmp));");
	_("    memcpy(sTmp, sBuf, min(nBufLen, sizeof(sTmp) - 1));");
    _("");
	_("    return (atoi(sTmp));");
    _("}");
    _("");

    _("int setNode(char *psName)");
    _("{");
    _("    printf(\"enter node[%%s]\\n\", psName);");
    _("    return 0;");
    _("}");
    _("");

    _("int getNode(char *psName)");
    _("{");
    _("    printf(\"enter node[%%s]\\n\", psName);");
    _("    return 0;");
    _("}");
    _("");

    _("int setValue(char *psName, T_VALUE *ptValue)");
    _("{");
    _("    printf(\"%%s:%%.*s\\n\", psName, ptValue->iLen, ptValue->psValue);");
    _("    return 0;");
    _("}");
    _("");

    _("int getValue(char *psName, char *psValue, int iMax)");
    _("{");
    _("    memcpy(psValue, psName, strlen(psName));");
    _("    return strlen(psName);");
    _("}");
    _("");

    H_LIST hMsgList = listNew();
    msgFieldAdd(hMsgList, "msg", "test1", "(.{10})");
    msgFieldAdd(hMsgList, "msg", "test6", "(?: *(.*))<10>");
    msgFieldAdd(hMsgList, "msg", "test6", "([a-zA-Z]{3})");
    msgFieldAdd(hMsgList, "msg", "test3", "([0-9]{3})(?:<Value>.*)<$1>");
    msgFieldAdd(hMsgList, "msg", "test5", "test2=(.{5})");
    msgFieldAdd(hMsgList, "msg", "child", "${child}");
    msgFieldAdd(hMsgList, "msg", "unsort", "([0-9])(?:${?unsort}){$1}");

    msgFieldAdd(hMsgList, "child", "child1", "(.{10})");
    msgFieldAdd(hMsgList, "child", "child2", "(.{5})");

    msgFieldAdd(hMsgList, "unsort", "unsort1", ":111:([^:]*)");
    msgFieldAdd(hMsgList, "unsort", "unsort1", ":122:([^:]*)");
    msgFieldAdd(hMsgList, "unsort", "unsort1", ":123:([^:]*)");
    msgFieldAdd(hMsgList, "unsort", "unsort2", ":134:([^:]*)");
    msgFieldAdd(hMsgList, "unsort", "unsort2", ":133:([^:]*)");
    msgFieldAdd(hMsgList, "unsort", "unsort2", ":233:([^:]*)");

    msgGenCode(hMsgList);

    _("int main(int argc, char *argv[]) {");
    _("    char *psInBuf = \"12345678900987654321002zytest2=12345ABC\";");
    _("    printf(\"parse:%%d\\n\", msgParse(psInBuf, strlen(psInBuf), 0));");
    _("    char sBuf[100];");
    _("    memset(sBuf, '\\0', sizeof(sBuf));");
    _("    int iMax = sizeof(sBuf);");
    _("    printf(\"gen:%%d\\n\", msgGen(sBuf, iMax, 0));");
    _("    printf(\"%%s\\n\", sBuf);");
    _("}");

    return 0;
}

/*-------------------------  Local functions ----------------------------*/
static int msgGenCode(H_LIST hMsgList)
{
    LIST_LOOP(hMsgList, msgGenCodeFuncDef(ptIter));
    _("");

    LIST_LOOP(hMsgList, fieldGenCodeParse(ptIter);fieldGenCodeGen(ptIter));

    return 0;
}

static int msgGenCodeFuncDef(T_Msg *ptMsg)
{
    _("int %sParse(char *psBuf, int iMax, int iPos);", ptMsg->psName);
    _("int %sGen(char *psBuf, int iMax, int iPos);", ptMsg->psName);

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
        ptMsg->iType = 0;
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
    if (ptMsg->iType) {
        return fieldGenCodeParseUnSort(ptMsg->psName, ptMsg->hList);
    }

    /* Parse */
    _("int %sParse(char *psBuf, int iPos, int iMax)", ptMsg->psName);
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
    LIST_LOOP(ptField->hList, f_fncParseDebug[((T_Value *)ptIter)->eType](ptIter, &tOption));

    char *psVar = NULL;
    LIST_LOOP(ptField->hList, psVar = locPrintVar(ptIter, psVar));
    if (NULL != psVar) {
        _("iRet = setValue(\"%s\", &%s);", ptField->psName, psVar);
        _("if ( iRet != 0 ) {");
        _("    return -1;");
        _("}");
    }
    f_Level -= 1;
    _("}");
    _("");

    return 0;
}

static int fieldGenCodeGen(T_Msg *ptMsg)
{
    /* Gen */
    _("int %sGen(char *psBuf, int iPos, int iMax)", ptMsg->psName);
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
    LIST_LOOP(ptField->hList, f_fncGenDebug[((T_Value *)ptIter)->eType](ptIter, &tOption));
    f_Level -= 1;
    _("}");
    _("");

    return 0;
}

static int fieldGenCodeParseUnSort(char *psNodeName, H_LIST hFieldList)
{
    /* Parse */
    _("int %sParse(char *psBuf, int iPos, int iMax)", psNodeName);
    _("{");
    f_Level = 1;
    _("int iRet = 0;");
    _("");

    listSort(hFieldList, FiledSort);

    _("while (1) {");
    f_Level += 1;

    int iLastSameNum = 0;
    int iMaxSameNum = -1;
    T_Field *ptNow = NULL;
    H_LIST_ITER pListIter = listIterNew(hFieldList);
    while (1) {
        void *ptIter = listIterFetch(pListIter);
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
    listIterFree(pListIter);

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
    T_Value *ptFristValue = listFrist(ptFrist->hList);
    T_Value *ptSecodeValue = listFrist(ptSecond->hList);

    int i = 0;
    while (1) {
        if (ptFristValue->psValue[i] != ptSecodeValue->psValue[i]) {
            break;
        }
        i++;
    }

    return i+1;
}

static int printfSameNum(int iLast, int iNow, int iMax, T_Field *ptField)
{
    T_Value *pValue = listFrist(ptField->hList);

    if (iLast != 0) {
        f_Level -= 1;
        _("} else if ('%c' == psBuf[iPos+%d]) {", pValue->psValue[iLast-1], iLast-1);
        f_Level += 1;
        if (iLast > iNow) {
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

            _("if ('%c' == psBuf[iPos+%d]) {", pValue->psValue[iLast+i], iLast+i);
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
    tOption.iLoop = 1;

    _("/* FIELD:%s VALUE:%s */", ptField->psName, ptField->psValue);
    LIST_LOOP(ptField->hList, f_fncParseDebug[((T_Value *)ptIter)->eType](ptIter, &tOption));
    char *psVar = NULL;
    LIST_LOOP(ptField->hList, psVar = locPrintVar(ptIter, psVar));
    if (NULL != psVar) {
        _("iRet = setValue(\"%s\", &%s);", ptField->psName, psVar);
        _("if ( iRet != 0 ) {");
        _("    return -1;");
        _("}");
    }
    _("");

    return 0;
}

static int FiledSort(void *ptFrist, void *ptSecond)
{
    T_Value *ptFristValue = listFrist(((T_Field *)ptFrist)->hList);
    T_Value *ptSecodeValue = listFrist(((T_Field *)ptSecond)->hList);

    return strcmp(ptFristValue->psValue, ptSecodeValue->psValue);
}

static int locParse(char *psBuf, H_LIST hList, H_LIST hMsgList)
{
    H_LIST hHeap = listNew();
    T_Value *ptLastValue = NULL;
    char *pCur = psBuf;
    int iIndex = 1;

    while ('\0'!=*pCur) {

        /* normal string */
        char *pStart = pCur;
        while (1) {
            char caTemp = *pCur;
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
                    listAdd(hList, valueNew(TYPE_STRING, pStart, (int)(pCur-pStart-1)));
                }
                pCur = setLoop(valueNew(TYPE_STRING, pCur-1, 1), hList, pCur);
                continue;
            } else {
                listAdd(hList, valueNew(TYPE_STRING, pStart, (int)(pCur-pStart)));
            }
        }

        /* . */
        if ('.' == *pCur) {
            T_Value * ptLast = valueNew(TYPE_BITMAP, NULL, _DLEN_BITMAP);
            pCur = setLoop(ptLast, hList, pCur+1);
            continue;
        }

        /* [....] */
        if ('[' == *pCur) {
            int iLen = 0;
            int iFlag = 0;
            T_Value * ptLast = valueNew(TYPE_BITMAP, getBitMap(pCur+1, &iFlag, &iLen), _DLEN_BITMAP);
            ptLast->iFlag = iFlag;
            pCur = setLoop(ptLast, hList, pCur + iLen + 2);
            continue;
        }

        /* ( | (?: */
        if ('(' == *pCur) {
            T_Value * ptNode = valueNew(TYPE_NODE_START, NULL, 0);
            listAdd(hList, ptNode);
            listPush(hHeap, ptNode);

            if ('?' != pCur[1] || ':' != pCur[2]) {
                if (ptLastValue != NULL) {
                    listFree(hHeap);
                    return -1;
                }

                char *psName = malloc(20);
                int iLen = 0;
                iLen = sprintf(psName, "Var%d", iIndex);
                ptLastValue = valueNew(TYPE_VALUE_START, psName, iLen);
                listAdd(hList, ptLastValue);

                iIndex += 1;
                pCur += 1;
            } else if ( '<' == pCur[3] ) {
                int iLen = 0;
                while (1) {
                    if ('>' == pCur[iLen+4] || '\0' == pCur[iLen+4]) {
                        break;
                    }
                    iLen += 1;
                }

                if ('\0' == pCur || 0 == iLen) {
                    listFree(hHeap);
                    return -3;
                }

                ptLastValue = valueNew(TYPE_VALUE_START, pCur+4, iLen);
                listAdd(hList, ptLastValue);
                pCur += 3 + 2 + iLen;
            } else {
                pCur += 3;
            }
            continue;
        }

        /*  ) */
        if (')' == *pCur) {
            if (NULL != ptLastValue) {
                T_Value *ptLast = valueNew(TYPE_VALUE_END, ptLastValue->psValue, -1);
                listAdd(hList, ptLast);
                ptLastValue = NULL;
            }

            T_Value *ptNodeEnd = valueNew(TYPE_NODE_END, NULL, 0);
            T_Value *ptNodeStart = listPop(hHeap);
            if (NULL == ptNodeStart) {
                listFree(hHeap);
                return -4;
            }

            pCur = setLoopNode(ptNodeStart, ptNodeEnd, hList, pCur+1);
            continue;
        }

        /* $ */
        if ('$' == *pCur) {
            int iFlag = 0;
            int iLen = 1;
            if ( '{' != pCur[iLen] ) {
                listFree(hHeap);
                return -4;
            }
            iLen += 1;

            if ('?' == pCur[iLen]) {
                iFlag = 1;
                iLen += 1;
            }

            while (1) {
                if ('}' == pCur[iLen] || '\0' == pCur[iLen]) {
                    iLen += 1;
                    break;
                }
                iLen += 1;
            }

            if ('\0' == pCur || iLen <= 3) {
                listFree(hHeap);
                return -3;
            }

            T_Value *ptNodeQuote = valueNew(TYPE_NODE_QUOTE, pCur+2+iFlag, iLen-3-iFlag);
            T_Msg *ptMsg = msgGet(hMsgList, ptNodeQuote->psValue);
            ptMsg->iType = iFlag;
            pCur = setLoop(ptNodeQuote, hList, pCur+iLen);
        }

        if ('\0' == *pCur) {
            break;
        }
    }

    listFree(hHeap);

    return 0;
}

static char * setLoopNode(T_Value *ptStartNode, T_Value *ptEndNode, H_LIST hList, char *pCur)
{
    int iLen = 0;
    E_TYPE eStart;
    E_TYPE eEnd;

    int iLoop = getLoop(pCur, &iLen, &eStart, &eEnd);

    char *psVarName = NULL;
    if (_LOOP_VAR == iLoop) {
        psVarName = getVarName(pCur, iLen, hList);
    }

    ptStartNode->eType = eStart;
    ptStartNode->iLoop = iLoop;
    ptStartNode->psVar = psVarName;

    ptEndNode->eType = eEnd;
    ptEndNode->iLoop = iLoop;
    ptEndNode->psVar = psVarName;
    listAdd(hList, ptEndNode);

    return pCur + iLen;
}

static char * setLoop(T_Value *ptValue, H_LIST hList, char *pCur)
{
    int iLen = 0;
    E_TYPE eStart;
    E_TYPE eEnd;

    int iLoop = getLoop(pCur, &iLen, &eStart, &eEnd);
    if (1 == iLoop) {
        listAdd(hList, ptValue);
        return pCur + iLen;
    }

    char *psVarName = NULL;
    if (_LOOP_VAR == iLoop) {
        psVarName = getVarName(pCur, iLen, hList);
    }

    if ( iLoop != _LOOP_UNLIMITED && TYPE_BITMAP == ptValue->eType && NULL == ptValue->psValue) {
        ptValue->iLoop = iLoop;
        ptValue->psVar = psVarName;
        listAdd(hList, ptValue);
    } else {
        T_Value *ptStart = valueNew(eStart, NULL, 0);
        ptStart->iLoop = iLoop;
        ptStart->psVar = psVarName;
        listAdd(hList, ptStart);

        listAdd(hList, ptValue);

        T_Value *ptEnd = valueNew(eEnd, NULL, 0);
        ptEnd->psVar = psVarName;
        ptEnd->iLoop = iLoop;
        listAdd(hList, ptEnd);
    }

    return pCur + iLen;
}

static char * getVarName(char *psBuf, int iLen, H_LIST hList)
{
    char *psVarName = NULL;

    if ('$' != psBuf[1]) {
        return psVarName;
    }

    int iMax = iLen - 1;
    int i = 2;
    for (i=2; i<iMax; i++) {
        if (psBuf[i] < '0' || psBuf[i] > '9') {
            break;
        }
    }

    if (i>=iMax) {
        psVarName = malloc(iLen-3+3+1);
        sprintf(psVarName, "Var%.*s", iLen-3, psBuf+2);
        LIST_LOOP(hList, setQuote(ptIter, psVarName));
    } else {
        psVarName = malloc(iLen-3+1);
        memcpy(psVarName, psBuf+2, iLen-3);
    }

    return psVarName;
}

static int setQuote(T_Value *ptValue, char *psVarName)
{
    if (TYPE_VALUE_START == ptValue->eType && 0 == strcmp(psVarName, ptValue->psValue)) {
        ptValue->eType = TYPE_QUOTE_START;
    }

    if (TYPE_VALUE_END == ptValue->eType && 0 == strcmp(psVarName, ptValue->psValue)) {
        ptValue->eType = TYPE_QUOTE_END;
    }

    return 0;
}

static int getLoop(char *psBuf, int *piLen, E_TYPE *eStart, E_TYPE *eEnd)
{
    char sLen[_DLEN_TINY_BUF];
    memset(sLen, '\0', sizeof(sLen));

    if ('*' == psBuf[0] ) {
        *eStart = TYPE_NODE_START;
        *eEnd   = TYPE_NODE_END;
        *piLen = 1;
        return _LOOP_UNLIMITED;
    }

    if ('{' == psBuf[0]) {
        int iLen = 1;
        while (psBuf[iLen] != '}') {
            sLen[iLen-1] = psBuf[iLen];
            iLen += 1;
        }
        *piLen = iLen + 1;
        *eStart = TYPE_NODE_START;
        *eEnd   = TYPE_NODE_END;

        if ('$' == psBuf[1]) {
            return _LOOP_VAR;
        }
        
        return atoi(sLen);
    }

    if ('<' == psBuf[0]) {
        int iLen = 1;
        while (psBuf[iLen] != '>') {
            sLen[iLen-1] = psBuf[iLen];
            iLen += 1;
        }
        *piLen = iLen + 1;
        *eStart = TYPE_LIMIT_START;
        *eEnd = TYPE_LIMIT_END;

        if ('$' == psBuf[1]) {
            return _LOOP_VAR;
        }

        return atoi(sLen);
    }

    /* No Loop */
    *piLen = 0;
    *eStart = TYPE_NODE_START;
    *eEnd   = TYPE_NODE_END;

    return 1;
}

static char * getBitMap(char *psBuf, int *piFlag, int *piLen)
{
    static char sBitMap[_DLEN_BITMAP];

    memset(sBitMap, '\0', sizeof(sBitMap));
    int iLen = 0;
    if (NULL != piFlag) {
        *piFlag = 0;
    }

    if ('^' == psBuf[0]) {
        if (NULL != piFlag) {
            *piFlag = 1;
        }
        iLen += 1;
    }

    for (; psBuf[iLen] != ']' && psBuf[iLen] != '\0'; ++iLen) {
        if (iLen != 0 && '-' == psBuf[iLen]) {
            if (psBuf[iLen-1] == psBuf[iLen+1]) {
                iLen += 1;
                continue;
            }

            unsigned char caStart = psBuf[iLen-1];
            unsigned char caEnd   = psBuf[iLen+1];

            if (caStart > caEnd) {
                caStart = psBuf[iLen+1];
                caEnd   = psBuf[iLen-1];
            }

            for (; caStart<=caEnd; caStart++) {
                BITMAP_SET(sBitMap, caStart);
            }
            continue;
        } else if ( '\\' == psBuf[iLen] ) {
            iLen += 1;
        }

        BITMAP_SET(sBitMap, (unsigned char )psBuf[iLen]);
    }
    *piLen = iLen;

    return sBitMap;
}

static T_Value * valueNew(E_TYPE eType, char *psValue, int iLen)
{
    T_Value *ptValue = malloc(sizeof(T_Value));
    memset(ptValue, '\0', sizeof(T_Value));

    ptValue->eType = eType;
    ptValue->iLoop = 1;
    ptValue->psVar = NULL;

    if (iLen < 0 && psValue != NULL) {
        iLen = strlen(psValue);
    }

    ptValue->psValue = NULL;
    if (NULL != psValue && 0 != iLen) {
        ptValue->psValue = malloc(iLen+1);
        memcpy(ptValue->psValue, psValue, iLen);
        ptValue->psValue[iLen] = '\0';
    }

    return ptValue;
}

static void valueStringParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("/* [string] \"%s\" */", ptValue->psValue);
    if (ptOption->iPreCheck == strlen(ptValue->psValue)) {
        return;
    }

    char *psStr = ptValue->psValue;
    int iLen = strlen(ptValue->psValue);
    if (ptOption->iPreCheck > 0) {
        _("iPos += %d;", ptOption->iPreCheck);
        iLen -= ptOption->iPreCheck;
        psStr += ptOption->iPreCheck;
        ptOption->iPreCheck = 0;
    }

    _("if ( iPos + %d > iMax ) {", iLen);
    if (ptOption->iLoop) {
        _("    break;");
    } else {
        _("    return -1;");
    }
    _("}");

    if (1 == iLen) {
        _("if ('%c' != psBuf[iPos]) {", psStr[0]);
    } else {
        _("if (0 != memcmp(psBuf+iPos, \"%s\", %ld)) {", psStr, iLen);
    }

    if (ptOption->iLoop) {
        _("    break;");
    } else {
        _("    return -1;");
    }
    _("}");
    _("iPos += %ld;", iLen);

    _("");
}

static void valueNodeStartParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (NULL != ptValue->psValue) {
        _("/* %s */", ptValue->psValue);
    }

    if (_LOOP_VAR == ptValue->iLen) {
        if (!ptOption->iLoopInit) {
            ptOption->iLoopInit = 1;
            _("int iLoop = 0;");
        }
        _("iLoop = strStr2Int(%s.psValue, %s.iLen);", ptValue->psVar, ptValue->psVar);

        if (0 == ptOption->iINum) {
            ptOption->iINum += 1;
            _("int i = 0;");
            _("for (i=0; i<iLoop; i++) {");
        } else {
            ptOption->iINum += 1;
            _("int i%d = 0;", ptOption->iINum);
            _("for (i%d=0; i%d<iLoop; i%d++) {", ptOption->iINum);
        }
        f_Level += 1;
        return;
    }

    if (ptValue->iLoop != 1) {
        if (ptValue->iLoop < 0) {
            _("while (1) {", ptValue->iLoop);
            ptOption->iLoop += 1;
        } else {
            if (0 == ptOption->iINum) {
                ptOption->iINum += 1;
                _("int i = 0;");
                _("for (i=0; i<%d; i++) {", ptValue->iLoop);
            } else {
                ptOption->iINum += 1;
                _("int i%d = 0;", ptOption->iINum);
                _("for (i%d=0; i%d<%d; i%d++) {", ptOption->iINum, ptOption->iINum, ptValue->iLoop, ptOption->iINum);
            }
        }
        f_Level += 1;
    }
}

static void valueNodeEndParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (NULL != ptValue->psValue) {
        _("/* %s */", ptValue->psValue);
    }

    if (_LOOP_VAR == ptValue->iLen) {
        f_Level -= 1;
        _("}");
        _("");

        return;
    }

    if (ptValue->iLoop != 1) {
        if (ptValue->iLoop < 0) {
            ptOption->iLoop -= 1;
        }
        f_Level -= 1;
        _("}");
        _("");
    }
}

static void valueLimitStartParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (ptValue->iLen <= 0 && _LOOP_VAR != ptValue->iLen) {
        return;
    }

    if (!ptOption->iMaxTempInit) {
        ptOption->iMaxTempInit = 1;
        _("int iMaxTemp = 0;");
    }

    if (_LOOP_VAR == ptValue->iLen) {
        _("iMaxTemp = iMax;");
        if (0 == memcmp(ptValue->psVar, "Var", 3)) {
            _("iMax = iPos + strStr2Int(%s.psValue, %s.iLen);", ptValue->psVar, ptValue->psVar);
        } else {
            _("iMax = iPos + POC_GetValueInt32(\"%s\");", ptValue->psVar);
        }
        _("");

        return;
    }

    _("iMaxTemp = iMax;");
    _("iMax = iPos + %d;", ptValue->iLen);
    _("");
}

static void valueLimitEndParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (ptValue->iLen <= 0 && _LOOP_VAR != ptValue->iLen) {
        return;
    }

    _("if ( iPos != iMax ) {");
    _("    return -2;");
    _("}");
    _("iMax = iMaxTemp;");
    _("");
}

static void valueValueStartParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("/* [value start] %s */", ptValue->psValue);
    _("T_VALUE %s;", ptValue->psValue);
    _("%s.psValue = psBuf+iPos;", ptValue->psValue);
    _("%s.iLen = 0;", ptValue->psValue);
    _("");
}

static void valueValueEndParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("/* [value end] %s */", ptValue->psValue);
    _("%s.iLen = psBuf + iPos - %s.psValue;", ptValue->psValue, ptValue->psValue);
    _("");
}

static void valueNodeQuoteParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (NULL != ptOption->psName) {
        _("iRet = setNode(\"%s\");", ptOption->psName);
        _("if ( iRet != 0 ) {");
        _("    return -4;");
        _("}");
        _("");
    }

    _("iRet = %sParse(psBuf, iMax, iPos);", ptValue->psValue);
    _("if ( iRet < 0 ) {");
    _("    return -3;");
    _("}");
    _("iPos = iRet;");
}

static void valueBitMapParse(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (NULL == ptValue->psValue) {
        _("/* [bitmap][%d] . */", ptValue->iLoop);
        if (_LOOP_VAR == ptValue->iLoop) {
            if (!ptOption->iLenInit) {
                ptOption->iLenInit = 1;
                _("int iLen = 0;");
            }

            if (0 == memcmp(ptValue->psVar, "Var", 3)) {
                _("iLen = strStr2Int(%s.psValue, %s.iLen);", ptValue->psVar, ptValue->psVar);
            } else {
                _("iLen = POC_GetValueInt32(\"%s\");", ptValue->psVar);
            }
            _("if ( iPos + iLen > iMax ) {");
        } else {
            _("if ( iPos + %d > iMax ) {", ptValue->iLoop);
        }
        if (ptOption->iLoop) {
            _("    break;");
        } else {
            _("    return -1;");
        }
        _("}");
        if (_LOOP_VAR == ptValue->iLen) {
            _("iPos += iLen;");
        } else {
            _("iPos += %d;", ptValue->iLoop);
        }
        _("");

        return;
    }

    unsigned char caFrist;
    unsigned char caLast;
    _s();__("/* [bitmap]");

    unsigned int i = 0;
    for (i=1; i<=128; i++) {
        if (BITMAP_TEST(ptValue->psValue, i)) {
            storeChar(i, NULL, NULL);
        } else {
            storeChar(0, &caFrist, &caLast);
            if ('\0' != caFrist) {
                if ('\0' == caLast) {
                    __("0x%03d", caFrist);
                } else {
                    __("0x%03d-0x%03d", caFrist, caLast);
                }
            }
        }
    }
    storeChar(0, &caFrist, &caLast);
    if ('\0' != caFrist) {
        if ('\0' == caLast) {
            __("0x%03d", caFrist);
        } else {
            __("0x%03d-0x%03d", caFrist, caLast);
        }
    }

    __(" */\n");

    _("if ( iPos + 1 > iMax ) {");
    if (ptOption->iLoop) {
        _("    break;");
    } else {
        _("    return -1;");
    }
    _("}");
    _("");

    _s();__("if (");
    if (ptValue->iFlag) {
        __("!(");
    }

    int iFlag = 0;
    for (i=1; i<=128; i++) {
        if (BITMAP_TEST(ptValue->psValue, i)) {
            storeChar(i, NULL, NULL);
        } else {
            storeChar(0, &caFrist, &caLast);
            printfChar(caFrist, caLast, &iFlag);
        }
    }
    storeChar(0, &caFrist, &caLast);
    printfChar(caFrist, caLast, &iFlag);

    if (ptValue->iFlag) {
        __(")");
    }

    __(") {\n");

    if (ptOption->iLoop) {
        _("    break;");
    } else {
        _("    return -1;");
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
        printf("    ");
    }

    return 0;
}

static int _(char *psBuf, ...)
{
    va_list	vaArgs;
	int		iRet;

    _s();

	va_start(vaArgs, psBuf);
	iRet = vprintf(psBuf, vaArgs);
	va_end(vaArgs);
    printf("\n");

	return iRet;
}

static void valueStringGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (ptOption->iLoop) {
        return;
    }

    _("/* [string] \"%s\" */", ptValue->psValue);

    _("if (iPos + %d > iMax) {", strlen(ptValue->psValue));
    _("    return -1;");
    _("}");
    if (1 == strlen(ptValue->psValue)) {
        _("psBuf[iPos] = '%s';", ptValue->psValue);
    } else {
        _("memcpy(psBuf+iPos, \"%s\", %d);", ptValue->psValue, strlen(ptValue->psValue));
    }
    _("iPos += %d;", strlen(ptValue->psValue));
    _("");
}

static void valueBitMapGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (ptOption->iLoop) {
        return;
    }

    _("/* [bitmap] */");
    unsigned int i = 0;
    for (i=1; i<=128; i++) {
        if (BITMAP_TEST(ptValue->psValue, i)) {
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

static void valueNodeStartGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (_LOOP_VAR == ptValue->iLoop) {
        if (0 == ptOption->iINum) {
            ptOption->iINum += 1;
            _("int i = 0;");
            _("for (i=0;;i++) {");
        } else {
            ptOption->iINum += 1;
            _("int i%d = 0;", ptOption->iINum);
            _("for (i%d=0;;i%d++) {", ptOption->iINum, ptOption->iINum);
        }

        f_Level += 1;
        ptOption->iLoop += 1;
        return;
    }

    if (ptValue->iLoop != 1) {
        if (ptValue->iLoop < 0) {
            ptOption->iLoop += 1;
        } else {
            if (!ptOption->iLoop) {
                if (0 == ptOption->iINum) {
                    ptOption->iINum += 1;
                    _("int i = 0;");
                    _("for (i=0; i<%d; i++) {", ptValue->iLoop);
                } else {
                    ptOption->iINum += 1;
                    _("int i%d = 0;", ptOption->iINum);
                    _("for (i%d=0; i%d<%d;i%d++) {", ptOption->iINum, ptOption->iINum, ptValue->iLoop, ptOption->iINum);
                }
                f_Level += 1;
            }
        }
    }
}

static void valueNodeEndGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (_LOOP_VAR == ptValue->iLoop) {
        ptOption->iLoop -= 1;
        f_Level -= 1;
        _("}");
        _("strInt2Str(i, %s.psValue, %s.iLen);", ptValue->psVar, ptValue->psVar);
        return;
    }

    if (ptValue->iLoop != 1) {
        if (ptValue->iLoop < 0) {
            ptOption->iLoop -= 1;
        } else {
            if (!ptOption->iLoop) {
                f_Level -= 1;
                _("}");
            }
        }
    }
}

static void valueLimitStartGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (ptValue->psVar != 0) {
        if (!ptOption->iPosTempInit) {
            ptOption->iPosTempInit = 1;
            _("int iPosTemp = 0;");
        }
        _("iPosTemp = iPos;");
    }

    if (ptValue->iLen <= 0) {
        return;
    }
}

static void valueLimitEndGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (ptValue->psVar != 0) {
        _("strInt2Str(iPos-iPosTemp, %s.psValue, %s.iLen);", ptValue->psVar, ptValue->psVar);
    }

    if (ptValue->iLen <= 0) {
        return;
    }
}

static char * locPrintVar(T_Value *ptValue, char *psVar)
{
    if (TYPE_VALUE_START != ptValue->eType) {
        return psVar;
    }

    if (0 == strcmp(ptValue->psValue, "Value")) {
        return ptValue->psValue;
    }

    if (NULL == psVar && 0 == strcmp(ptValue->psValue, "Var1")) {
        return ptValue->psValue;
    }

    return psVar;
}

static void valueValueStartGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    ptOption->iLoop += 1;
}

static void valueValueEndGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    ptOption->iLoop -= 1;
    _("iRet = getValue(\"%s\", psBuf + iPos, iMax);", ptOption->psName);
    _("if ( iRet < 0 ) {");
    _("    return -1;");
    _("}");
    _("iPos += iRet;");
    _("");
}

static void valueQuoteStartGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("/* [quote start] %s */", ptValue->psValue);
    _("T_VALUE %s;", ptValue->psValue);
    _("%s.psValue = psBuf+iPos;", ptValue->psValue);
    _("%s.iLen = 0;", ptValue->psValue);
    _("");
}

static void valueQuoteEndGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    _("/* [quote end] %s */", ptValue->psValue);
    _("%s.iLen = psBuf + iPos - %s.psValue;", ptValue->psValue, ptValue->psValue);
    _("");
}

static void valueNodeQuoteGen(T_Value *ptValue, T_Field_Option *ptOption)
{
    if (NULL != ptOption->psName) {
        _("iRet = getNode(\"%s\");", ptOption->psName);
        _("if ( iRet != 0 ) {");
        if (ptOption->iLoop) {
            _("    break;");
        } else {
            _("    return -3;");
        }
        _("}");
        _("");
    }

    _("iRet = %sGen(psBuf, iMax, iPos);", ptValue->psValue);
    _("if ( iRet < 0 ) {");
    _("    return -3;");
    _("}");
    _("iPos = iRet;");
}

/*-----------------------------  End ------------------------------------*/

