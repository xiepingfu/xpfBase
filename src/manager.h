#ifndef MANAGER_H
#define MANAGER_H

#include "utils.h"
#include "parser_node.h"
#include "parser.h"
#include "tokeniz.h"

typedef struct Condition
{
    char leftTable[STRING_SIZE];
    char leftField[STRING_SIZE];
    char leftValue[STRING_SIZE];
    TokenType op;
    char rightTable[STRING_SIZE];
    char rightField[STRING_SIZE];
    char rightValue[STRING_SIZE];
} Condition;

typedef struct SelectField
{
    char fieldName[STRING_SIZE];
    char fieldType[STRING_SIZE];
    char hasIndex;
    TokenType op;
    char value[STRING_SIZE];
    int sort;
    List *dataList;
} SelectField;

RC ManagerCreateTable(XBParser *xbParser);
RC ManagerUseDatabase(XBParser *xbParser, char *dbName);
RC ManagerCreateDatabase(XBParser *xbParser, char *dbName);
RC ManagerInsert(XBParser *xbParser);
RC ManagerCreateIndex(XBParser *xbParser);
RC ManagerSelect(XBParser *xbParser, List *colList, List *tableList, List *expr, OrderBy *orderBy);
RC ManagerDropDatabase(XBParser *xbParser, char *dbName);
RC ManagerDropTable(XBParser *xbParser, char *tableName);
RC ManagerDropIndex(XBParser *xbParser, char *tableName, char *fieldName);
RC ManagerUpdate(XBParser *xbParser, Update *update, List *conditionList);
RC ManagerDelete(XBParser *xbParser, char *tableName, List *conditionList);
RC ManagerShow(XBParser *xbParser, char *name);

#endif