#ifndef PARSER_H
#define PARSER_H

#include "parser_node.h"
#include "manager.h"
#include "utils.h"

typedef struct XBParser
{
    Database *db;
    Node *tree;
    char *sql;
    char *lastSql;
    char *err;
} XBParser;

RC ParseSQL(XBParser *xbParser);

#endif