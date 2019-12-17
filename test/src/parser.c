#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
    char valFloat[] = "Float";
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
            fprintf(stderr, "Out of memory.\n");
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
            fprintf(stderr, "RC_SYNTAX_ERROR.\n");
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

/*DROP INDEX indexName;*/
RC ParseDropIndex(XBParser *xbParser)
{
    return RC_UNMATCH;
}

/*UPDATE tableName SET filedName = newValue WHERE expr;*/
RC ParseUpdate(XBParser *xbParser)
{
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
    return RC_UNMATCH;
}

/*SELECT colList FROM tableList WHERE expr;*/
RC ParseSelect(XBParser *xbParser)
{
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
        ParseDropIndex,
        ParseUseDatabase,
        ParseUpdate,
        ParseInsert,
        ParseDelete,
        ParseSelect,
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
    return RC_SYNTAX_ERROR;
}