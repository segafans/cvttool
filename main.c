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

typedef void (*FNC_DEBUG)(T_Value *ptValue);
static int f_Level = 0;
static int f_Index = 1;
static int f_Loop = 0;
static char *f_psName = NULL;

/*---------------------- Local function declaration ---------------------*/
static int locParse(char *psBuf, H_LIST hList);
static T_Value * valueNew(E_TYPE eType, char *psValue, int iLen);

static int storeChar(unsigned char caChar, unsigned char *psFrist, unsigned char *psLast);
static char * getBitMap(char *psBuf, int *piFlag, int *piLen);
static char * setLoop(T_Value *ptValue, H_LIST ptList, char *pCur);
static char * getVarName(char *psBuf, int iLen);
static int getLoop(char *psBuf, int *piLen, E_TYPE *eStart, E_TYPE *eEnd);
static int printfChar(unsigned char caFrist, unsigned char caLast, int *piFlag);
static int _s();
static int _(char *psBuf, ...);
static char * setLoopNode(T_Value *ptStartNode, T_Value *ptEndNode, H_LIST hList, char *pCur);

static void valueStringParse(T_Value *ptValue);
static void valueBitMapParse(T_Value *ptValue);
static void valueNodeStartParse(T_Value *ptValue);
static void valueNodeEndParse(T_Value *ptValue);
static void valueValueStartParse(T_Value *ptValue);
static void valueValueEndParse(T_Value *ptValue);
static void valueLimitStartParse(T_Value *ptValue);
static void valueLimitEndParse(T_Value *ptValue);

static void valueStringGen(T_Value *ptValue);
static void valueBitMapGen(T_Value *ptValue);
static void valueNodeStartGen(T_Value *ptValue);
static void valueNodeEndGen(T_Value *ptValue);
static void valueValueStartGen(T_Value *ptValue);
static void valueValueEndGen(T_Value *ptValue);
static void valueLimitStartGen(T_Value *ptValue);
static void valueLimitEndGen(T_Value *ptValue);

static int fieldGenCodeParse(H_LIST hFieldList);
static int fieldGenCodeGen(H_LIST hFieldList);
static char * locPrintVar(T_Value *ptValue, char *psVar);
static T_Field * filedParse(char *psName, char *psValue);
static int fieldGenCodeParseOne(T_Field *ptField);
static int fieldGenCodeGenOne(T_Field *ptField);

/*-------------------------  Global variable ----------------------------*/
FNC_DEBUG f_fncParseDebug[TYPE_NUM] = {
    valueStringParse,
    valueBitMapParse,
    valueNodeStartParse,
    valueNodeEndParse,
    valueValueStartParse,
    valueValueEndParse,
    valueLimitStartParse,
    valueLimitEndParse
};

FNC_DEBUG f_fncGenDebug[TYPE_NUM] = {
    valueStringGen,
    valueBitMapGen,
    valueNodeStartGen,
    valueNodeEndGen,
    valueValueStartGen,
    valueValueEndGen,
    valueLimitStartGen,
    valueLimitEndGen
};


/*-------------------------  Global functions ---------------------------*/
int main(int argc, char *argv[])
{
    _("#include <string.h>");
    _("#include <stdio.h>");
    _("");

    _("int setValue(char *psName, char *psValue, int iLen)");
    _("{");
    _("    printf(\"%%s:%%.*s\\n\", psName, iLen, psValue);");
    _("    return 0;");
    _("}");
    _("");

    _("int getValue(char *psName, char *psValue, int iMax)");
    _("{");
    _("    memcpy(psValue, psName, strlen(psName));");
    _("    return strlen(psName);");
    _("}");
    _("");

    H_LIST hFieldList = listNew();
    listAdd(hFieldList, filedParse("test1", "(?:.* *)<10>"));
    listAdd(hFieldList, filedParse("test2", "(.{10})"));
    listAdd(hFieldList, filedParse("test3", "([0-9]{3})(?:<Value>.*)<$1>"));
    listAdd(hFieldList, filedParse("test4", "(.{$LEN})"));
    listAdd(hFieldList, filedParse("test5", "test2=(.{5})"));
    listAdd(hFieldList, filedParse("test6", "([a-zA-Z]{20})"));
    fieldGenCodeParse(hFieldList);
    fieldGenCodeGen(hFieldList);

    _("int main(int argc, char *argv[]) {");
    _("    char *psInBuf = \"1234567890test2=12345\";");
    _("    printf(\"parse:%%d\\n\", locParse(psInBuf, strlen(psInBuf)));");
    _("    char sBuf[100];");
    _("    memset(sBuf, '\\0', sizeof(sBuf));");
    _("    int iMax = sizeof(sBuf);");
    _("    printf(\"gen:%%d\\n\", locGen(sBuf, iMax));");
    _("    printf(\"%%s\\n\", sBuf);");
    _("}");

    return 0;
}

/*-------------------------  Local functions ----------------------------*/
static T_Field * filedParse(char *psName, char *psValue)
{
    T_Field * ptField = malloc(sizeof(T_Field));
    memset(ptField, '\0', sizeof(T_Field));

    ptField->psName = strdup(psName);
    ptField->psValue = strdup(psValue);
    ptField->hList = listNew();

    f_Index = 1;
    locParse(psValue, ptField->hList);

    return ptField;
}

static int fieldGenCodeParse(H_LIST hFieldList)
{
    /* Parse */
    _("int locParse(char *psBuf, int iMax)");
    _("{");
    f_Level = 1;
    _("int iPos = 0;");
    _("int i = 0;");
    _("int iRet = 0;");
    _("");

    LIST_LOOP(hFieldList, fieldGenCodeParseOne(ptIter));

    _("return 0;");
    f_Level = 0;
    _("}");
    _("");
    
    return 0;
}

static int fieldGenCodeParseOne(T_Field *ptField)
{
    _("/*");
    _(" * FIELD:%s VALUE:%s", ptField->psName, ptField->psValue);
    _(" */");
    _("{");
    f_Level += 1;
    LIST_LOOP(ptField->hList, f_fncParseDebug[((T_Value *)ptIter)->eType](ptIter));

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

static int fieldGenCodeGen(H_LIST hFieldList)
{
    /* Gen */
    _("int locGen(char *psBuf, int iMax)");
    _("{");
    f_Level = 1;
    _("int iPos = 0;");
    _("int i = 0;");
    _("int iRet = 0;");
    _("");

    LIST_LOOP(hFieldList, fieldGenCodeGenOne(ptIter));

    _("return iPos;");
    f_Level = 0;
    _("}");
    _("");

    return 0;
}

static int fieldGenCodeGenOne(T_Field *ptField)
{
    f_psName = ptField->psName;

    _("/*");
    _(" * FIELD:%s VALUE:%s", ptField->psName, ptField->psValue);
    _(" */");
    _("{");
    f_Level += 1;
    LIST_LOOP(ptField->hList, f_fncGenDebug[((T_Value *)ptIter)->eType](ptIter));
    f_Level -= 1;
    _("}");
    _("");

    return 0;
}

static int locParse(char *psBuf, H_LIST hList)
{
    T_Value *ptLastNode = NULL;
    T_Value *ptLastValue = NULL;
    char *pCur = psBuf;

    while ('\0'!=*pCur) {

        /* normal string */
        char *pStart = pCur;
        while (1) {
            char caTemp = *pCur;
            if ('.' == caTemp || '[' == caTemp || '(' == caTemp || ')' == caTemp || '\0' == caTemp ) {
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
            if (ptLastNode != NULL) {
                return -1;
            }

            ptLastNode = valueNew(TYPE_NODE_START, NULL, 0);
            listAdd(hList, ptLastNode);

            if ('?' != pCur[1] || ':' != pCur[2]) {
                if (ptLastValue != NULL) {
                    return -1;
                }

                char *psName = malloc(20);
                int iLen = 0;
                iLen = sprintf(psName, "Var%d", f_Index);
                ptLastValue = valueNew(TYPE_VALUE_START, psName, iLen);
                listAdd(hList, ptLastValue);

                f_Index += 1;
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

            if (NULL == ptLastNode) {
                return -1;
            }

            T_Value *ptNodeEnd = valueNew(TYPE_NODE_END, NULL, 0);
            pCur = setLoopNode(ptLastNode, ptNodeEnd, hList, pCur+1);
            ptLastNode = NULL;
            continue;
        }

        if ('\0' == *pCur) {
            break;
        }
    }

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
        psVarName = getVarName(pCur, iLen);
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
        psVarName = getVarName(pCur, iLen);
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

static char * getVarName(char *psBuf, int iLen)
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
    } else {
        psVarName = malloc(iLen-3+1);
        memcpy(psVarName, psBuf+2, iLen-3);
    }

    return psVarName;
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

static void valueStringParse(T_Value *ptValue)
{
    _("/* [string] \"%s\" */", ptValue->psValue);
    _("if ( iPos + %d > iMax ) {", strlen(ptValue->psValue));
    if (f_Loop) {
        _("    break;");
    } else {
        _("    return -1;");
    }
    _("}");

    if (1 == strlen(ptValue->psValue)) {
        _("if ('%c' != psBuf[iPos]) {", ptValue->psValue[0]);
    } else {
        _("if (0 != memcmp(psBuf+iPos, \"%s\", %ld)) {", ptValue->psValue, strlen(ptValue->psValue));
    }

    if (f_Loop) {
        _("    break;");
    } else {
        _("    return -1;");
    }
    _("}");
    _("iPos += %ld;", strlen(ptValue->psValue));

    _("");
}

static void valueNodeStartParse(T_Value *ptValue)
{
    if (NULL != ptValue->psValue) {
        _("/* %s */", ptValue->psValue);
    }

    if (ptValue->iLoop != 1) {
        if (ptValue->iLoop < 0) {
            _("while (1) {", ptValue->iLoop);
            f_Loop = 1;
        } else {
            _("for (i=0; i<%d; i++) {", ptValue->iLoop);
        }
        f_Level += 1;
    }
}

static void valueNodeEndParse(T_Value *ptValue)
{
    if (NULL != ptValue->psValue) {
        _("/* %s */", ptValue->psValue);
    }

    if (ptValue->iLoop != 1) {
        f_Loop = 0;
        f_Level -= 1;
        _("}");
        _("");
    }
}

static void valueLimitStartParse(T_Value *ptValue)
{
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

    if (ptValue->iLen <= 0) {
        return;
    }

    _("iMaxTemp = iMax;");
    _("iMax = iPos + %d;", ptValue->iLen);
    _("");
}

static void valueLimitEndParse(T_Value *ptValue)
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

static void valueValueStartParse(T_Value *ptValue)
{
    _("/* [value start] %s */", ptValue->psValue);
    _("T_POC_VALUE %s;", ptValue->psValue);
    _("%s.psValue = psBuf+iPos;", ptValue->psValue);
    _("%s.iLen = 0;", ptValue->psValue);
    _("");
}

static void valueValueEndParse(T_Value *ptValue)
{
    _("/* [value end] %s */", ptValue->psValue);
    _("%s.iLen = psBuf + iPos - %s.psValue;", ptValue->psValue, ptValue->psValue);
    _("");
}

static void valueBitMapParse(T_Value *ptValue)
{
    if (NULL == ptValue->psValue) {
        _("/* [bitmap][%d] . */", ptValue->iLoop);
        if (_LOOP_VAR == ptValue->iLoop) {
            if (0 == memcmp(ptValue->psVar, "Var", 3)) {
                _("int iLen = strStr2Int(%s.psValue, %s.iLen);", ptValue->psVar, ptValue->psVar);
            } else {
                _("int iLen = POC_GetValueInt32(\"%s\");", ptValue->psVar);
            }
            _("if ( iPos + iLen > iMax ) {");
        } else {
            _("if ( iPos + %d > iMax ) {", ptValue->iLoop);
        }
        if (f_Loop) {
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
    if (f_Loop) {
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

    if (f_Loop) {
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

static void valueStringGen(T_Value *ptValue)
{
    if (f_Loop) {
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

static void valueBitMapGen(T_Value *ptValue)
{
    if (f_Loop) {
        return;
    }

    _("/* [bitmap] not do */");
    _("");
}

static void valueNodeStartGen(T_Value *ptValue)
{
    if (ptValue->iLoop != 1) {
        if (ptValue->iLoop < 0) {
            f_Loop += 1;
        } else {
            if (!f_Loop) {
                _("for (i=0; i<%d; i++) {", ptValue->iLoop);
                f_Level += 1;
            }
        }
    }
}

static void valueNodeEndGen(T_Value *ptValue)
{
    if (ptValue->iLoop != 1) {
        if (ptValue->iLoop < 0) {
            f_Loop -= 1;
        } else {
            if (!f_Loop) {
                f_Level -= 1;
                _("}");
            }
        }
    }
}

static void valueLimitStartGen(T_Value *ptValue)
{
    if (ptValue->iLen <= 0) {
        return;
    }

    _("iMaxTemp = iMax;");
    _("iMax = iPos + %d;", ptValue->iLen);
    _("");
}

static void valueLimitEndGen(T_Value *ptValue)
{
    if (ptValue->iLen <= 0) {
        return;
    }

    _("iMax = iMaxTemp;");
    _("");
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

static void valueValueStartGen(T_Value *ptValue)
{
    f_Loop += 1;
}

static void valueValueEndGen(T_Value *ptValue)
{
    f_Loop -= 1;
    _("iRet = getValue(\"%s\", psBuf + iPos, iMax);", f_psName);
    _("if ( iRet < 0 ) {");
    _("    return -1;");
    _("}");
    _("iPos += iRet;");
    _("");
}

/*-----------------------------  End ------------------------------------*/

