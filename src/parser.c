#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "tokeniz.h"
#include "parser.h"
#include "utils.h"
#include "parser_node.h"
#include "manager.h"

Node *ParseAttrTypeList(XBParser *xbParser)
{
    Token ta = GetToken(xbParser);
    Token tb = GetToken(xbParser);
    Node *attrType = NULL;
    Node *attrTypeList = NULL;
    if (ta.n && tb.n && (ta.tokenType == TK_ID || ta.tokenType == VAL_STRING) && (tb.tokenType == TK_STRING || tb.tokenType == TK_INT || tb.tokenType == TK_FLOAT))
    {
        for (int i = 0; i < tb.n; ++i)
        {
            tb.sql[i] = toupper(tb.sql[i]);
        }
        attrType = NewAttrNode(NewString(ta.sql, ta.n), NewString(tb.sql, tb.n), NULL, 'n');
        ta = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_COMMA)
        {
            Node *nextAttrTypeList = ParseAttrTypeList(xbParser);
            attrTypeList = Prepend(attrType, nextAttrTypeList);
        }
        else
        {
            attrTypeList = NewListNode(attrType);
        }
    }
    return attrTypeList;
}

Node *ParseAttrList(XBParser *xbParser)
{
    Token ta = GetToken(xbParser);
    Node *attr = NULL;
    Node *attrList = NULL;
    if (ta.n && (ta.tokenType == TK_ID || ta.tokenType == VAL_STRING))
    {
        attr = NewAttrNode(NewString(ta.sql, ta.n), NULL, NULL, 'n');
        ta = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_COMMA)
        {
            Node *nextAttrList = ParseAttrList(xbParser);
            if (nextAttrList == NULL)
                return NULL;
            attrList = Prepend(attr, nextAttrList);
        }
        else
        {
            attrList = NewListNode(attr);
        }
    }
    return attrList;
}

Node *ParseValueList(XBParser *xbParser)
{
    Token ta = GetToken(xbParser);
    Node *value = NULL;
    Node *valueList = NULL;
    char valString[] = "STRING";
    char valInt[] = "INT";
    char valFloat[] = "FLOAT";
    if (ta.n && (ta.tokenType == TK_ID || ta.tokenType == VAL_STRING || ta.tokenType == VAL_INT || ta.tokenType == VAL_FLOAT))
    {
        switch (ta.tokenType)
        {
        case VAL_INT:
        {
            value = NewValueNode(NewString(valInt, strlen(valInt)), NewString(ta.sql, ta.n));
            break;
        }
        case VAL_FLOAT:
        {
            value = NewValueNode(NewString(valFloat, strlen(valFloat)), NewString(ta.sql, ta.n));
            break;
        }
        case VAL_STRING:
        default:
        {
            value = NewValueNode(NewString(valString, strlen(valString)), NewString(ta.sql, ta.n));
            break;
        }
        }
        ta = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_COMMA)
        {
            Node *nextValueList = ParseValueList(xbParser);
            valueList = Prepend(value, nextValueList);
        }
        else
        {
            valueList = NewListNode(value);
        }
    }
    return valueList;
}

/*CREATE TABLE tbname(attribution list);*/
RC ParseCreateTable(XBParser *xbParser)
{
    RC rc = RC_OK;
    int i = 0;
    Token ta, tb;
    ta = GetToken(xbParser);
    tb = GetToken(xbParser);
    Node *attrList = NULL;
    if (ta.n && tb.n && ta.tokenType == TK_CREATE && tb.tokenType == TK_TABLE)
    {
        ta = GetToken(xbParser);
        if (!ta.n || !(ta.tokenType == TK_ID || ta.tokenType == VAL_STRING))
        {
            return RC_SYNTAX_ERROR;
        }
        tb = GetToken(xbParser);
        if (!tb.n || tb.tokenType != TK_LPAREN)
        {
            return RC_SYNTAX_ERROR;
        }
        if ((attrList = ParseAttrTypeList(xbParser)) == NULL)
        {
            write(xbParser->clientfd, "Out of memory.\n", strlen("Out of memory.\n"));
            return RC_UNKNOW;
        }
        if (attrList == NULL)
            return RC_SYNTAX_ERROR;
        tb = GetToken(xbParser);
        if (!tb.n || tb.tokenType != TK_SEMICOLON)
        {
            return RC_SYNTAX_ERROR;
        }
        xbParser->tree = NewCreateTableNode(NewString(ta.sql, ta.n), attrList);
        if (xbParser->tree == NULL)
            return RC_SYNTAX_ERROR;
        if ((rc = ManagerCreateTable(xbParser)) != RC_OK)
        {
            return rc;
        }
        return RC_OK;
    }
    return RC_UNMATCH;
}

/*CREATE DATABASE dbName;*/
RC ParseCreateDatabase(XBParser *xbParser)
{
    RC rc;
    Token ta = GetToken(xbParser);
    Token tb = GetToken(xbParser);
    if (ta.n && tb.n && ta.tokenType == TK_CREATE && tb.tokenType == TK_DATABASE)
    {
        ta = GetToken(xbParser);
        if (!ta.n || !(ta.tokenType == TK_ID || ta.tokenType == VAL_STRING))
        {
            write(xbParser->clientfd, "RC_SYNTAX_ERROR.\n", strlen("RC_SYNTAX_ERROR.\n"));
            return RC_SYNTAX_ERROR;
        }
        tb = GetToken(xbParser);
        if (!tb.n || tb.tokenType != TK_SEMICOLON)
        {
            return RC_SYNTAX_ERROR;
        }
        char *dbName = NewString(ta.sql, ta.n);
        if ((rc = ManagerCreateDatabase(xbParser, dbName)) != RC_OK)
        {
            return rc;
        }
        return RC_OK;
    }
    return RC_UNMATCH;
}

/*USE dbName;*/
RC ParseUseDatabase(XBParser *xbParser)
{
    RC rc;
    Token ta = GetToken(xbParser);
    Token tb;
    if (ta.n && ta.tokenType == TK_USE)
    {
        ta = GetToken(xbParser);
        if (!ta.n || !(ta.tokenType == TK_ID || ta.tokenType == VAL_STRING))
        {
            return RC_SYNTAX_ERROR;
        }
        tb = GetToken(xbParser);
        if (!tb.n || tb.tokenType != TK_SEMICOLON)
        {
            return RC_SYNTAX_ERROR;
        }
        char *dbName = NewString(ta.sql, ta.n);
        if ((rc = ManagerUseDatabase(xbParser, dbName)) != RC_OK)
        {
            return rc;
        }
        write(xbParser->clientfd, "Successfully open.\n", strlen("Successfully open.\n"));
        return RC_OK;
    }
    return RC_UNMATCH;
}

/*CREATE INDEX ON tableName (attrName);*/
RC ParseCreateIndex(XBParser *xbParser)
{
    RC rc = RC_UNMATCH;
    Token ta = GetToken(xbParser);
    Token tb = GetToken(xbParser);
    if (ta.n && tb.n && ta.tokenType == TK_CREATE && tb.tokenType == TK_INDEX)
    {
        tb = GetToken(xbParser);
        if (!tb.n || !(tb.tokenType == TK_ON))
        {
            return RC_SYNTAX_ERROR;
        }
        tb = GetToken(xbParser);
        char *tableName = NewString(tb.sql, tb.n);
        ta = GetToken(xbParser);
        if (!ta.n || !(ta.tokenType == TK_LPAREN))
        {
            return RC_SYNTAX_ERROR;
        }
        ta = GetToken(xbParser);
        if (!ta.n || !(ta.tokenType == TK_ID || ta.tokenType == VAL_STRING))
        {
            return RC_SYNTAX_ERROR;
        }
        char *attrName = NewString(ta.sql, ta.n);
        ta = GetToken(xbParser);
        tb = GetToken(xbParser);
        if (!ta.n || !tb.n || !(ta.tokenType == TK_RPAREN) || !(tb.tokenType == TK_SEMICOLON))
        {
            return RC_SYNTAX_ERROR;
        }
        xbParser->tree = NewCreateIndexNode(tableName, attrName);
        if ((rc = ManagerCreateIndex(xbParser)) != RC_OK)
        {
            return rc;
        }
        return RC_OK;
    }
    return RC_UNMATCH;
}

/*DROP INDEX tableName(fieldName);*/
RC ParseDropIndex(XBParser *xbParser)
{
    Token ta = GetToken(xbParser);
    Token tb = GetToken(xbParser);
    char tableName[FILENAME_MAX];
    char fieldName[FILENAME_MAX];
    if (ta.n && ta.tokenType == TK_DROP && tb.n && tb.tokenType == TK_INDEX)
    {
        ta = GetToken(xbParser);
        tb = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_ID && tb.n && tb.tokenType == TK_LPAREN)
        {
            StringCopy(tableName, ta.sql, ta.n);
        }
        else
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
        ta = GetToken(xbParser);
        tb = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_ID && tb.n && tb.tokenType == TK_RPAREN)
        {
            StringCopy(fieldName, ta.sql, ta.n);
        }
        else
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
        tb = GetToken(xbParser);
        if (tb.n && tb.tokenType == TK_SEMICOLON)
        {
            return ManagerDropIndex(xbParser, tableName, fieldName);
        }
        else
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
    }
    return RC_UNMATCH;
}

/*DROP DATABASE dbName;*/
RC ParseDropDataBase(XBParser *xbParser)
{
    Token ta = GetToken(xbParser);
    Token tb = GetToken(xbParser);
    if (ta.n && ta.tokenType == TK_DROP && tb.n && tb.tokenType == TK_DATABASE)
    {
        ta = GetToken(xbParser);
        tb = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_ID && tb.n && tb.tokenType == TK_SEMICOLON)
        {
            char dbName[FILENAME_MAX];
            StringCopy(dbName, ta.sql, ta.n);
            return ManagerDropDatabase(xbParser, dbName);
        }
        else
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
    }
    return RC_UNMATCH;
}

/*DROP TABLE tableName;*/
RC ParseDropTable(XBParser *xbParser)
{
    Token ta = GetToken(xbParser);
    Token tb = GetToken(xbParser);
    if (ta.n && ta.tokenType == TK_DROP && tb.n && tb.tokenType == TK_TABLE)
    {
        ta = GetToken(xbParser);
        tb = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_ID && tb.n && tb.tokenType == TK_SEMICOLON)
        {
            char tableName[FILENAME_MAX];
            StringCopy(tableName, ta.sql, ta.n);
            return ManagerDropTable(xbParser, tableName);
        }
        else
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
    }
    return RC_UNMATCH;
}

/*SHOW [TABLES, tableName];*/
RC ParseShow(XBParser *xbParesr)
{
    Token ta = GetToken(xbParesr);
    char name[FILENAME_MAX];
    if (ta.n && ta.tokenType == TK_SHOW)
    {
        ta = GetToken(xbParesr);
        if (ta.n && ta.tokenType == TK_ID)
        {
            StringCopy(name, ta.sql, ta.n);
        }
        else
        {
            write(xbParesr->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
        ta = GetToken(xbParesr);
        if (ta.n && ta.tokenType == TK_SEMICOLON)
        {
            return ManagerShow(xbParesr, name);
        }
        else
        {
            write(xbParesr->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
    }
    return RC_UNMATCH;
}

List *GetExpr(XBParser *xbParesr)
{
    List *list = NewList();
    Token ta = GetToken(xbParesr);
    char tmp[1024], str[1024];
    while (ta.n && ta.tokenType != TK_ORDER && ta.tokenType != TK_SEMICOLON)
    {
        Condition *condition = (Condition *)xpfMalloc(sizeof(Condition));
        strcpy(condition->leftField, "");
        strcpy(condition->leftTable, "");
        strcpy(condition->leftValue, "");
        strcpy(condition->rightField, "");
        strcpy(condition->rightTable, "");
        strcpy(condition->rightValue, "");

        if (ta.n && (ta.tokenType == VAL_STRING || ta.tokenType == VAL_FLOAT || ta.tokenType == VAL_INT))
        {
            StringCopy(condition->leftValue, ta.sql, ta.n);
            ta = GetToken(xbParesr);
        }
        else if (ta.n && ta.tokenType == TK_ID)
        {
            StringCopy(condition->leftField, ta.sql, ta.n);
            ta = GetToken(xbParesr);
            if (ta.n && ta.tokenType == TK_DOT)
            {
                ta = GetToken(xbParesr);
                if (ta.n && ta.tokenType == TK_ID)
                {
                    strcpy(condition->leftTable, condition->leftField);
                    StringCopy(condition->leftField, ta.sql, ta.n);
                    ta = GetToken(xbParesr);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            break;
        }

        if (ta.n && (ta.tokenType == TK_LT || ta.tokenType == TK_LE || ta.tokenType == TK_GT || ta.tokenType == TK_GE || ta.tokenType == TK_EQ || ta.tokenType == TK_NE))
        {
            condition->op = ta.tokenType;
            ta = GetToken(xbParesr);
        }
        else
        {
            break;
        }

        if (ta.n && (ta.tokenType == VAL_STRING || ta.tokenType == VAL_FLOAT || ta.tokenType == VAL_INT))
        {
            StringCopy(condition->rightValue, ta.sql, ta.n);
            ta = GetToken(xbParesr);
        }
        else if (ta.n && ta.tokenType == TK_ID)
        {
            StringCopy(condition->rightField, ta.sql, ta.n);
            ta = GetToken(xbParesr);
            if (ta.n && ta.tokenType == TK_DOT)
            {
                ta = GetToken(xbParesr);
                if (ta.n && ta.tokenType == TK_ID)
                {
                    strcpy(condition->rightTable, condition->rightField);
                    StringCopy(condition->rightField, ta.sql, ta.n);
                    ta = GetToken(xbParesr);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            break;
        }

        ListNode *listNode = uNewListNode(condition);
        ListAppend(list, listNode);

        if (ta.n && ta.tokenType == TK_AND)
        {
            ta = GetToken(xbParesr);
        }
        else
        {
            break;
        }
    }
    return list;
}

/*UPDATE tableName SET filedName = newValue WHERE expr;*/
RC ParseUpdate(XBParser *xbParser)
{
    Token ta = GetToken(xbParser), tb;
    char tableName[FILENAME_MAX];
    Update update;
    if (ta.n && ta.tokenType == TK_UPDATE)
    {
        ta = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_ID)
        {
            StringCopy(update.tableName, ta.sql, ta.n);
        }
        else
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }

        tb = GetToken(xbParser);
        if (!(tb.n && tb.tokenType == TK_SET))
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }

        ta = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_ID)
        {
            StringCopy(update.fieldName, ta.sql, ta.n);
        }
        else
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }

        tb = GetToken(xbParser);
        if (!(tb.n && tb.tokenType == TK_EQ))
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }

        ta = GetToken(xbParser);
        if (ta.n && (ta.tokenType == VAL_STRING || ta.tokenType == VAL_INT || ta.tokenType == VAL_FLOAT))
        {
            StringCopy(update.newValue, ta.sql, ta.n);
            if (ta.tokenType == VAL_STRING)
            {
                strcpy(update.fieldType, "STRING");
            }
            else if (ta.tokenType == VAL_INT)
            {
                strcpy(update.fieldType, "INT");
            }
            else if (ta.tokenType == VAL_FLOAT)
            {
                strcpy(update.fieldType, "FLOAT");
            }
        }
        else
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }

        tb = GetToken(xbParser);

        List *conditionList = GetExpr(xbParser);
        return ManagerUpdate(xbParser, &update, conditionList);
    }
    return RC_UNMATCH;
}

/*INSERT INTO tbName (fileds list) VALUES(values list);*/
RC ParseInsert(XBParser *xbParser)
{
    RC rc = RC_UNMATCH;
    Token ta = GetToken(xbParser);
    Token tb = GetToken(xbParser);
    Node *attrList = NULL;
    Node *valueList = NULL;
    if (ta.n && tb.n && ta.tokenType == TK_INSERT && tb.tokenType == TK_INTO)
    {
        ta = GetToken(xbParser);
        if (!ta.n || !(ta.tokenType == TK_ID || ta.tokenType == TK_STRING))
        {
            return RC_SYNTAX_ERROR;
        }
        tb = GetToken(xbParser);
        if (!tb.n || !(tb.tokenType == TK_VALUES || tb.tokenType == TK_LPAREN))
        {
            return RC_SYNTAX_ERROR;
        }
        if (tb.tokenType == TK_LPAREN)
        {
            if ((attrList = ParseAttrList(xbParser)) == NULL)
            {
                return RC_SYNTAX_ERROR;
            }
            tb = GetToken(xbParser);
        }
        if (tb.tokenType == TK_VALUES)
        {
            tb = GetToken(xbParser);
            if (!tb.n || !(tb.tokenType == TK_LPAREN))
            {
                return RC_SYNTAX_ERROR;
            }
            if ((valueList = ParseValueList(xbParser)) == NULL)
            {
                return RC_SYNTAX_ERROR;
            }
        }
        tb = GetToken(xbParser);
        if (!tb.n || tb.tokenType != TK_SEMICOLON)
        {
            return RC_SYNTAX_ERROR;
        }
        xbParser->tree = NewInsertNode(NewString(ta.sql, ta.n), attrList, valueList);
        if ((rc = ManagerInsert(xbParser)) != RC_OK)
        {
            return rc;
        }
        return RC_OK;
    }
    return RC_UNMATCH;
}

/*DELETE FROM tableName WHERE expr;*/
RC ParseDelete(XBParser *xbParser)
{
    Token ta = GetToken(xbParser);
    Token tb = GetToken(xbParser);
    char tableName[FILENAME_MAX];
    List *conditionList = NULL;
    if (ta.n && ta.tokenType == TK_DELETE)
    {
        if (!(tb.n && tb.tokenType == TK_FROM))
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
        ta = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_ID)
        {
            StringCopy(tableName, ta.sql, ta.n);
        }
        else
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }

        tb = GetToken(xbParser);
        conditionList = GetExpr(xbParser);
        if (conditionList == NULL)
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
        return ManagerDelete(xbParser, tableName, conditionList);
    }
    return RC_UNMATCH;
}

/* End of token FROM.*/
List *GetColList(XBParser *xbParser)
{
    List *list = NewList();
    Token ta = GetToken(xbParser);
    if (ta.n && ta.tokenType == TK_STAR)
    {
        ta = GetToken(xbParser);
        if (ta.n && ta.tokenType == TK_FROM)
        {
            return list;
        }
        else
        {
            FreeList(list);
            return NULL;
        }
    }
    while (ta.n && (ta.tokenType == TK_ID || ta.tokenType == VAL_STRING))
    {
        Column *column = (Column *)xpfMalloc(sizeof(Column));
        strcpy(column->tableName, "");
        StringCopy(column->fieldName, ta.sql, ta.n);
        strcpy(column->nickName, column->fieldName);
        Token tb = GetToken(xbParser);
        if (tb.n && tb.tokenType == TK_DOT)
        {
            tb = GetToken(xbParser);
            if (tb.n && tb.tokenType == TK_ID)
            {
                StringCopy(column->tableName, ta.sql, ta.n);
                StringCopy(column->fieldName, tb.sql, tb.n);
                strcpy(column->nickName, column->fieldName);
                tb = GetToken(xbParser);
            }
            else
            {
                FreeList(list);
                return NULL;
            }
        }

        if (tb.n && tb.tokenType == TK_AS)
        {
            tb = GetToken(xbParser);
            if (tb.n && (tb.tokenType == TK_ID || tb.tokenType == VAL_STRING))
            {
                StringCopy(column->nickName, tb.sql, tb.n);
                tb = GetToken(xbParser);
            }
            else
            {
                FreeList(list);
                return NULL;
            }
        }
        else if (tb.n && (tb.tokenType == TK_ID || tb.tokenType == VAL_STRING))
        {
            StringCopy(column->nickName, tb.sql, tb.n);
            tb = GetToken(xbParser);
        }

        if (tb.n && (tb.tokenType == TK_COMMA || tb.tokenType == TK_FROM))
        {

            ListNode *listNode = uNewListNode(column);
            ListAppend(list, listNode);
            if (tb.tokenType == TK_FROM)
            {
                break;
            }
            ta = GetToken(xbParser);
        }
        else
        {
            FreeList(list);
            return NULL;
        }
    }
    return list;
}

List *GetTableList(XBParser *xbParser)
{
    List *list = NewList();
    Token ta = GetToken(xbParser);
    while (ta.n && (ta.tokenType == TK_ID || ta.tokenType == VAL_STRING))
    {
        ListNode *listNode = uNewListNode(NewString(ta.sql, ta.n));
        ta = GetToken(xbParser);
        if (ta.n && (ta.tokenType == TK_COMMA || ta.tokenType == TK_SEMICOLON || ta.tokenType == TK_WHERE))
        {
            ListAppend(list, listNode);
            if (ta.tokenType == TK_SEMICOLON || ta.tokenType == TK_WHERE)
            {
                break;
            }

            ta = GetToken(xbParser);
        }
        else
        {
            FreeList(list);
            return NULL;
        }
    }
    return list;
}

OrderBy GetOrderBy(XBParser *xbParser)
{
    OrderBy orderBy;
    orderBy.asc = 1;
    strcpy(orderBy.fieldName, "");
    strcpy(orderBy.tableName, "");
    Token ta = GetToken(xbParser);
    if (ta.n && ta.tokenType == TK_BY)
    {
        ta = GetToken(xbParser);
        if (ta.n && (ta.tokenType == TK_ID || ta.tokenType == VAL_STRING))
        {
            StringCopy(orderBy.fieldName, ta.sql, ta.n);
            ta = GetToken(xbParser);
            if (ta.n && ta.tokenType == TK_DOT)
            {
                strcpy(orderBy.tableName, orderBy.fieldName);
                ta = GetToken(xbParser);
                if (ta.n && (ta.tokenType == TK_ID || ta.tokenType == VAL_STRING))
                {
                    StringCopy(orderBy.fieldName, ta.sql, ta.n);
                    ta = GetToken(xbParser);
                }
            }
            if (ta.n && (ta.tokenType == TK_ASC || ta.tokenType == TK_DESC))
            {
                if (ta.tokenType == TK_DESC)
                {
                    orderBy.asc = 0;
                }
                ta = GetToken(xbParser);
            }
            if (ta.n && ta.tokenType == TK_SEMICOLON)
            {
                return orderBy;
            }
        }
    }
    return orderBy;
}

/*SELECT colList FROM tableList WHERE expr [ ORDER BY col [DESC, ASC] ];*/
/*expr: VALUE [>, >=, <, <=, =, ==, !=] VALUE [ AND ] expr*/
/*VALUE: [ table.field, INT, FLOAT, STRING ]*/
RC ParseSelect(XBParser *xbParser)
{
    RC rc = RC_UNMATCH;
    Token ta = GetToken(xbParser);
    if (ta.n && ta.tokenType == TK_SELECT)
    {
        List *colList = GetColList(xbParser);
        if (colList == NULL)
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
        List *tableList = GetTableList(xbParser);
        if (tableList == NULL)
        {
            write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
            return RC_SYNTAX_ERROR;
        }
        List *conditionList = GetExpr(xbParser);
        OrderBy orderBy = GetOrderBy(xbParser);
        return ManagerSelect(xbParser, colList, tableList, conditionList, &orderBy);
    }
    return RC_UNMATCH;
}

RC ParseSQL(XBParser *xbParser)
{
    RC rc = RC_OK;
    RC(*functionTable[])
    (XBParser *) = {
        ParseCreateTable,
        ParseCreateIndex,
        ParseCreateDatabase,
        ParseDropIndex,
        ParseDropDataBase,
        ParseDropTable,
        ParseUseDatabase,
        ParseUpdate,
        ParseInsert,
        ParseDelete,
        ParseSelect,
        ParseShow,
    };
    int n = sizeof(functionTable) / sizeof(functionTable[0]);
    for (int i = 0; i < n; ++i)
    {
        xbParser->lastSql = xbParser->sql;
        if ((rc = functionTable[i](xbParser)) == RC_OK)
        {
            return rc;
        }
    }
    write(xbParser->clientfd, "Syntax error!\n", strlen("Syntax error!\n"));
    return RC_SYNTAX_ERROR;
}