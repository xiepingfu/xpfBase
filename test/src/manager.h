#ifndef MANAGER_H
#define MANAGER_H

#include "utils.h"
#include "parser_node.h"
#include "parser.h"

typedef struct Database
{
    char dbName[FILENAME_MAX];
    int tableNum;
    List *tableList;
    int indexNum;
    List *indexList;
} Database;

RC ManagerCreateTable(XBParser *xbParser);
RC ManagerUseDatabase(XBParser *xbParser, char *dbName);
RC ManagerCreateDatabase(XBParser *xbParser, char *dbName);
RC ManagerInsert(XBParser *xbParser);
RC ManagerCreateIndex(XBParser *xbParser);

#endif