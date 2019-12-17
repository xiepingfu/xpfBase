#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "parser_node.h"
#include "utils.h"

typedef struct Database
{
    char dbName[FILENAME_MAX];
    int tableNum;
    List *tableList;
} Database;

typedef struct Table
{
    char tableName[FILENAME_MAX];
    int dataNum;
    int dataSize;
    int fieldNum;
    List *fieldList;
} Table;

typedef struct Field
{
    char fieldName[STRING_SIZE];
    char fieldType[STRING_SIZE];
    char hasIndex;
} Field;

typedef struct FieldData
{
    char tableName[FILENAME_MAX];
    char fieldName[STRING_SIZE];
    char fieldType[STRING_SIZE];
    char value[STRING_SIZE];
    char hasIndex;
} FieldData;

typedef struct Column
{
    char tableName[STRING_SIZE];
    char fieldName[STRING_SIZE];
    char fieldType[STRING_SIZE];
    char nickName[STRING_SIZE];
    char hasIndex;
} Column;

typedef struct OrderBy
{
    char tableName[STRING_SIZE];
    char fieldName[STRING_SIZE];
    char asc; /*DESC, ASC*/
} OrderBy;

typedef struct SelectTable
{
    char dbName[FILENAME_MAX];
    char tableName[FILENAME_MAX];
    Table *table;
    List *sFieldList;
    List *sFieldDataList;
    ListNode *curNode;
} SelectTable;

typedef struct Update
{
    char tableName[FILENAME_MAX];
    char fieldName[FILENAME_MAX];
    char fieldType[STRING_SIZE];
    char newValue[STRING_SIZE];
} Update;

typedef struct XBParser
{
    Database *db;
    Node *tree;
    char *sql;
    char *lastSql;
    char *err;
    int clientfd;
} XBParser;

RC ParseSQL(XBParser *xbParser);

#endif