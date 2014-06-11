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
#define _DLEN_TINY_BUF  64
#define _DLEN_MINI_BUF  256
#define _DLEN_LARGE_BUF 1024
#define _DLEN_HUGE_BUF  10240

#define _DLEN_BITMAP     16
#define _NUM_BITMAP      _DLEN_BITMAP*8

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
    TYPE_NUM
} E_TYPE;

typedef struct {
    char *psValue;
    union {
        int iLoop;
        int iFlag;
        int iLen;
    };
    E_TYPE eType;
} T_Value;

typedef struct {
    char *psName;
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

static void valueStringDebug(T_Value *ptValue);
static void valueBitMapDebug(T_Value *ptValue);
static int storeChar(unsigned char caChar, unsigned char *psFrist, unsigned char *psLast);
static char * getBitMap(char *psBuf, int *piFlag, int *piLen);
static void valueNodeStartDebug(T_Value *ptValue);
static void valueNodeEndDebug(T_Value *ptValue);
static char * setLoop(T_Value *ptValue, H_LIST ptList, char *pCur);
static int getLoop(char *psBuf, int *piLen);
static int printfChar(unsigned char caFrist, unsigned char caLast, int *piFlag);
static int _s();
static int _(char *psBuf, ...);

static void valueStringGen(T_Value *ptValue);
static void valueBitMapGen(T_Value *ptValue);
static void valueNodeStartGen(T_Value *ptValue);
static void valueNodeEndGen(T_Value *ptValue);

static int fieldGenCodeParse(H_LIST hFieldList);
static int fieldGenCodeGen(H_LIST hFieldList);
static int locPrintVar(T_Value *ptValue);
static T_Field * filedParse(char *psName, char *psValue);
static int fieldGenCodeParseOne(T_Field *ptField);
static int fieldGenCodeGenOne(T_Field *ptField);

/*-------------------------  Global variable ----------------------------*/
FNC_DEBUG f_fncParseDebug[TYPE_NUM] = {
    valueStringDebug,
    valueBitMapDebug,
    valueNodeStartDebug,
    valueNodeEndDebug
};

FNC_DEBUG f_fncGenDebug[TYPE_NUM] = {
    valueStringGen,
    valueBitMapGen,
    valueNodeStartGen,
    valueNodeEndGen
};


/*-------------------------  Global functions ---------------------------*/
int main(int argc, char *argv[])
{
    _("#include <string.h>");
    _("#include <stdio.h>");
    _("");

    _("int parseValue(char *psName, char *psValue, int iLen)");
    _("{");
    _("    printf(\"%%s:%%.*s\\n\", psName, iLen, psValue);");
    _("    return 0;");
    _("}");
    _("");

    _("int genValue(char *psName, char *psValue, int iMax)");
    _("{");
    _("    memcpy(psValue, psName, strlen(psName));");
    _("    return strlen(psName);");
    _("}");
    _("");

    H_LIST hFieldList = listNew();
    listAdd(hFieldList, filedParse("test1", "(.{10})"));
    listAdd(hFieldList, filedParse("test2", "test2=(.{5})"));
    fieldGenCodeParse(hFieldList);
    fieldGenCodeGen(hFieldList);

    _("int main(int argc, char *argv[]) {");
    _("    printf(\"parse:%%d\\n\", locParse(\"1234567890test2=12345\"));");
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
    ptField->hList = listNew();
    locParse(psValue, ptField->hList);

    return ptField;
}

static int fieldGenCodeParse(H_LIST hFieldList)
{
    /* Parse */
    _("int locParse(char *psBuf)");
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
    _(" * FIELD:%s", ptField->psName);
    _(" */");
    LIST_LOOP(ptField->hList, f_fncParseDebug[((T_Value *)ptIter)->eType](ptIter));
    _s();__("iRet = parseValue(\"%s\"", ptField->psName);LIST_LOOP(ptField->hList, locPrintVar(ptIter));__(");\n");
    _("if ( iRet != 0 ) {");
    _("    return -1;");
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
    _(" * FIELD:%s", ptField->psName);
    _(" */");
    LIST_LOOP(ptField->hList, f_fncGenDebug[((T_Value *)ptIter)->eType](ptIter));

    return 0;
}

static int locParse(char *psBuf, H_LIST hList)
{
    T_Value *ptLastNode = NULL;
    char *pCur = psBuf;

    while ('\0'!=*pCur) {

        /* normal string */
        char *pStart = pCur;
        while (1) {
            char caTemp = *pCur;
            if (caTemp != '.' && caTemp != '[' && caTemp != '(' && caTemp != ')' && caTemp != '{' && caTemp != '\0') {
                pCur += 1;
                continue;
            }

            break;
        }

        if (pStart != pCur) {
            pCur = setLoop(valueNew(TYPE_STRING, pStart, (int)(pCur-pStart)), hList, pCur);
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
            
            char *psName = NULL;
            int iLen = 0;
            if ('?' != *(pCur+1) || ':' != *(pCur+2)) {
                psName = malloc(20);
                iLen = sprintf(psName, "Var%d", f_Index);
                f_Index += 1;
                pCur += 1;
            } else {
                pCur += 3;
            }
            ptLastNode = valueNew(TYPE_NODE_START, psName, iLen);
            listAdd(hList, ptLastNode);
            continue;
        }

        /*  ) */
        if (')' == *pCur) {
            if (NULL == ptLastNode) {
                return -1;
            }

            T_Value *ptLast = valueNew(TYPE_NODE_END, ptLastNode->psValue, -1);
            pCur = setLoop(ptLast, NULL, pCur+1);
            ptLastNode->iLoop = ptLast->iLoop;
            listAdd(hList, ptLast);
            ptLastNode = NULL;
            continue;
        }

        if ('\0' == *pCur) {
            break;
        }
    }

    return 0;
}

static char * setLoop(T_Value *ptValue, H_LIST hList, char *pCur)
{
    if ('*' != *pCur && '{' != *pCur) {
        listAdd(hList, ptValue);
        return pCur;
    }

    int iLoop = -1;
    int iLen = 0;
    if ('{' == *pCur) {
        iLoop = getLoop(pCur+1, &iLen);
    }

    if (NULL == hList) {
        ptValue->iLoop = iLoop;
    } else if ( iLoop != -1 && TYPE_BITMAP == ptValue->eType && NULL == ptValue->psValue) {
        ptValue->iLoop = iLoop;
        listAdd(hList, ptValue);
    } else {
        T_Value *ptStart = valueNew(TYPE_NODE_START, NULL, 0);
        T_Value *ptEnd = valueNew(TYPE_NODE_END, NULL, 0);

        ptStart->iLoop = iLoop;
        ptEnd->iLoop = iLoop;

        listAdd(hList, ptStart);
        listAdd(hList, ptValue);
        listAdd(hList, ptEnd);
    }

    return pCur + iLen + 1;
}

static int getLoop(char *psBuf, int *piLen)
{
    char sLen[_DLEN_TINY_BUF];
    memset(sLen, '\0', sizeof(sLen));

    int iLen = 0;
    while (psBuf[iLen] != '}') {
        sLen[iLen] = psBuf[iLen];
        iLen += 1;
    }
    *piLen = iLen+1;

    return atoi(sLen);
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

static void valueStringDebug(T_Value *ptValue)
{
    _("/* [string] \"%s\" */", ptValue->psValue);

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

static void valueNodeStartDebug(T_Value *ptValue)
{
    if (ptValue->iLoop != 1) {
        if (ptValue->iLoop < 0) {
            _("while (1) {", ptValue->iLoop);
            f_Loop = 1;
        } else {
            _("for (i=0; i<%d; i++) {", ptValue->iLoop);
        }
        f_Level += 1;
    }

    if (NULL != ptValue->psValue) {
        _("/* [get start][%d]%s */", ptValue->iLoop, ptValue->psValue);
        _("char *s%s = psBuf+iPos;", ptValue->psValue);
        _("");
    }
}

static void valueNodeEndDebug(T_Value *ptValue)
{
    if (NULL != ptValue->psValue) {
        _("/* [get end] %s */", ptValue->psValue);
        _("int iLen%s = psBuf + iPos - s%s;", ptValue->psValue, ptValue->psValue);
        _("");
    }

    if (ptValue->iLoop != 1) {
        f_Loop = 0;
        f_Level -= 1;
        _("}");
        _("");
    }
}

static void valueBitMapDebug(T_Value *ptValue)
{
    if (NULL == ptValue->psValue) {
        _("/* [bitmap][%d] . */", ptValue->iLoop);
        _("iPos += %d;", ptValue->iLoop);
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

    if (NULL != ptValue->psValue) {
        f_Loop += 1;
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

    if (NULL != ptValue->psValue) {
        f_Loop -= 1;
        _("iRet = genValue(\"%s\", psBuf + iPos, iMax);", f_psName);
        _("if ( iRet < 0 ) {");
        _("    return -1;");
        _("}");
        _("iPos += iRet;");
        _("");
    }
}

static int locPrintVar(T_Value *ptValue)
{
    if (TYPE_NODE_START != ptValue->eType || ptValue->psValue == NULL) {
        return 0;
    }

    __(", s%s, iLen%s", ptValue->psValue, ptValue->psValue);

    return 0;
}

/*-----------------------------  End ------------------------------------*/

