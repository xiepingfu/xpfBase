#ifndef TOKENIZ_H
#define TOKENIZ_H

#include "parser.h"

typedef enum
{
    TK_DATABASE,
    TK_CREATE,
    TK_DROP,
    TK_TABLE,
    TK_INDEX,
    TK_LOAD,
    TK_HELP,
    TK_USE,
    TK_EXIT,
    TK_PRINT,
    TK_SET,
    TK_AND,
    TK_INTO,
    TK_VALUES,
    TK_SELECT,
    TK_FROM,
    TK_WHERE,
    TK_ORDER,
    TK_GROUP,
    TK_BY,
    TK_DESC,
    TK_ASC,
    TK_INSERT,
    TK_DELETE,
    TK_UPDATE,
    TK_MAX,
    TK_MIN,
    TK_AVG,
    TK_SUM,
    TK_COUNT,
    TK_RESET,
    TK_BUFFER,
    TK_ON,
    TK_OFF,
    TK_STRING,
    TK_COMMENT,
    TK_SPACE,     /*' ' OR '\t' OR '\n' OR '\r'*/
    TK_LT,        /*<*/
    TK_LE,        /*<=*/
    TK_GT,        /*>*/
    TK_GE,        /*>=*/
    TK_EQ,        /*== OR =*/
    TK_NE,        /*!=*/
    TK_EOF,       /*EOF*/
    TK_LPAREN,    /*'('*/
    TK_RPAREN,    /*')'*/
    TK_COMMA,     /*','*/
    TK_SEMICOLON, /*';'*/
    TK_ATTR_TYPE, /*Attribute type*/
    TK_STAR,      /*'*'*/
    TK_DOT,       /*'.'*/
    TK_AS,
    TK_ILLEGAL,
    TK_MINUS,
    TK_SLASH,
    TK_PLUS,
    TK_OR,
    TK_ID, /*Wild match*/
    TK_FLOAT,
    TK_INT,
    TK_SHOW,

    VAL_STRING, /*"STRING" OR 'STRING'*/
    VAL_INT,    /*(+|-)[0-9]*/
    VAL_FLOAT   /*(+|-)[0-9].[0-9]*/
} TokenType;

typedef struct Token
{
    char *sql;
    int n;
    TokenType tokenType;
} Token;

Token GetToken(XBParser *xbParser);

#endif