#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser_node.h"
#include "tokeniz.h"
#include "utils.h"

#define MAXNODE 128

Node *NewNode(NodeKind nodeKind)
{
    Node *node = (Node *)xpfMalloc(sizeof(Node));
    node->nodeKind = nodeKind;
    return node;
}

Node *NewListNode(Node *node)
{
    Node *list = NewNode(NK_LIST);
    list->u.List.curr = node;
    list->u.List.next = NULL;
    list->u.List.numChilds = 0;
    return list;
}

Node *NewCreateTableNode(char *tableName, Node *attrTypeList)
{
    Node *node = NewNode(NK_CREATETABLE);
    node->u.CreateTable.tableName = tableName;
    node->u.CreateTable.attrTypeList = attrTypeList;
    return node;
}

Node *NewAttrNode(char *attrName, char *type, char *val, char isIndex)
{
    Node *node = NewNode(NK_ATTRTYPE);
    node->u.Attr.attrName = attrName;
    node->u.Attr.type = type;
    node->u.Attr.val = val;
    node->u.Attr.isIndex = isIndex;
    return node;
}

Node *Append(Node *list, Node *node)
{
    Node *newList = NewNode(NK_LIST);
    newList->u.List.curr = node;
    newList->u.List.next = NULL;
    Node *lasList = list;
    while (lasList->u.List.next != NULL)
    {
        lasList = lasList->u.List.next;
    }
    lasList->u.List.next = newList;
    return list;
}

Node *Prepend(Node *node, Node *list)
{
    Node *newList = NewNode(NK_LIST);
    newList->u.List.curr = node;
    newList->u.List.next = list;
    newList->u.List.numChilds = list->u.List.numChilds + 1;
    return newList;
}

Node *NewInsertNode(char *tableName, Node *attrList, Node *valueList)
{
    Node *node = NewNode(NK_INSERT);
    node->u.Insert.tableName = tableName;
    node->u.Insert.attrList = attrList;
    node->u.Insert.valueList = valueList;
    return node;
}

Node *NewTableNode(char *tableName, Node *attrList)
{
    Node *node = NewNode(NK_TABLE);
    node->u.Table.TableName = tableName;
    node->u.Table.attrList = attrList;
    return node;
}

Node *NewDatabaseNode(char *dbName, Node *tableList)
{
    Node *node = NewNode(NK_DATABASE);
    node->u.Database.dbName = dbName;
    node->u.Database.tableList = tableList;
    return node;
}

Node *NewValueNode(char *type, char *val)
{
    Node *node = NewNode(NK_VALUE);
    node->u.Attr.type = type;
    node->u.Attr.val = val;
    return node;
}

Node *NewCreateIndexNode(char *tableName, char *attrName)
{
    Node *node = NewNode(NK_CREATEINDEX);
    node->u.CreateIndex.tableName = tableName;
    node->u.CreateIndex.attrname = attrName;
    return node;
}