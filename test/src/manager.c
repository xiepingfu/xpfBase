#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parser_node.h"
#include "utils.h"
#include "manager.h"

#define TABLE_INFO_LINE_SIZE 256
#define TABLE_ATTR_SIZE 32

Node *ManagerGetTableNode(Node *db, char *tableName)
{
    Node *tableList = db->u.Database.tableList;
    Node *tableNode = NULL;
    int tableIsExist = 0;
    while (tableList != NULL)
    {
        if (strcmp(tableList->u.List.curr->u.Table.TableName, tableName) == 0)
        {
            tableNode = tableList->u.List.curr;
            break;
        }
        tableList = tableList->u.List.next;
    }
    return tableNode;
}

Node *ManagerInitTableNode(char *dbName, char *tableName)
{
    Node *tableNode = NULL;
    char prePath[STRING_SIZE];
    strcpy(prePath, dbName);
    strcat(prePath, "/");
    char tableInfoFilename[STRING_SIZE];
    strcpy(tableInfoFilename, prePath);
    strcat(tableInfoFilename, tableName);
    strcat(tableInfoFilename, ".info");
    int tableInfoFD;
    if ((tableInfoFD = open(tableInfoFilename, O_RDONLY)) == -1)
    {
        fprintf(stderr, "Can't open file %s.\n", tableInfoFilename);
        return tableNode;
    }
    char tableInfoLine[TABLE_INFO_LINE_SIZE];
    char *attrName, *typeName;
    char isIndex;
    Node *attrList = NULL;
    Node *attrNode = NULL;
    while (read(tableInfoFD, tableInfoLine, TABLE_INFO_LINE_SIZE) > 0)
    {
        isIndex = tableInfoLine[0];
        int l = 1, r = 0;
        while (l < TABLE_INFO_LINE_SIZE && tableInfoLine[l] != ' ')
        {
            ++l;
        }
        attrName = NewString(&tableInfoLine[1], l - 1);
        ++l;
        r = l;
        while (r < TABLE_INFO_LINE_SIZE && tableInfoLine[r] != '\0')
        {
            ++r;
        }
        typeName = NewString(&tableInfoLine[l], r - l);
        attrNode = NewAttrNode(attrName, typeName, NULL, isIndex);
        if (attrList == NULL)
        {
            attrList = NewListNode(attrNode);
        }
        else
        {
            attrList = Prepend(attrNode, attrList);
        }
    }
    close(tableInfoFD);
    tableNode = NewTableNode(tableName, attrList);
    return tableNode;
}

RC ManagerDatabaseIsLegal(XBParser *xbParser)
{
    if (access(xbParser->db->u.Database.dbName, R_OK | W_OK | F_OK) != 0)
    {
        fprintf(stderr, "The database %s was not found or has no access.\n", xbParser->db->u.Database.dbName);
        return RC_UNKNOW;
    }
    return RC_OK;
}

RC ManagerParserIsLegal(XBParser *xbParser, NodeKind nodeKind)
{
    if (xbParser == NULL || xbParser->tree == NULL)
    {
        fprintf(stderr, "NULL POINTER.\n");
        return RC_NULL_POINTER;
    }
    if (xbParser->db == NULL)
    {
        fprintf(stderr, "Please execute the 'USE DBNAME' command first.\n");
        return RC_NULL_POINTER;
    }
    if (xbParser->tree->nodeKind != nodeKind)
    {
        fprintf(stderr, "Unknow error.\n");
        return RC_UNKNOW;
    }
    return RC_OK;
}

RC ManagerUseDatabase(XBParser *xbParser, char *dbName)
{
    int fd;
    RC rc = RC_UNKNOW;

    /*Open dbInfo file.*/
    char prePath[STRING_SIZE];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, dbName);
    strcat(prePath, "/");
    int dbInfoFD;
    char dbInfoFilename[STRING_SIZE];
    strcpy(dbInfoFilename, prePath);
    strcat(dbInfoFilename, "dbInfo");
    if ((dbInfoFD = open(dbInfoFilename, O_RDONLY)) == -1)
    {
        fprintf(stderr, "Can't open file %s.", dbInfoFilename);
        return RC_UNKNOW;
    }

    /*Read dbInfo to init dbNode.*/
    Node *tableList = NULL;
    Node *table = NULL;
    char dbInfoLine[STRING_SIZE];
    int size = 0;
    while (read(dbInfoFD, dbInfoLine, STRING_SIZE) > 0)
    {
        table = ManagerInitTableNode(dbName, NewString(dbInfoLine, strlen(dbInfoLine)));
        if (tableList == NULL)
            tableList = NewListNode(table);
        else
            tableList = Prepend(table, tableList);
    }
    close(dbInfoFD);
    /*TODO: Free old xbParser->db.*/
    xbParser->db = NewDatabaseNode(dbName, tableList);

    return RC_OK;
}

Database *NewDatabase(char *dbName)
{
    Database *db = (Database *)xpfMalloc(sizeof(Database));
    strcpy(db->dbName, dbName);
    db->tableNum = 0;
    db->tableList = NewList();
    db->indexNum = 0;
    db->indexList = NewList();
    return db;
}

void FreeDatabase(Database *db)
{
    if (db == NULL)
        return;
    FreeList(db->tableList);
    FreeList(db->indexList);
    free(db);
}

RC DatabaseInfoFlush(Database *db)
{
    char prePath[FILENAME_MAX];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, db->dbName);
    strcat(prePath, "/");
    char dbInfoFilename[FILENAME_MAX];
    strcpy(dbInfoFilename, prePath);
    strcat(dbInfoFilename, "dbInfo");

    FILE *file = fopen(dbInfoFilename, "w");
    rewind(file);
    fwrite(db, sizeof(Database), 1, file);
    ListNode *listNode = db->tableList->first;
    while (listNode != NULL)
    {
        fwrite(listNode->data, FILENAME_MAX, 1, file);
        listNode = listNode->next;
    }
    listNode = db->indexList->first;
    while (listNode != NULL)
    {
        fwrite(listNode->data, FILENAME_MAX, 1, file);
        listNode = listNode->next;
    }
    fclose(file);
}

Database *OpenDatabase(char *dbName)
{
    Database *db = (Database *)xpfMalloc(sizeof(Database));

    char prePath[FILENAME_MAX];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, db->dbName);
    strcat(prePath, "/");
    char dbInfoFilename[FILENAME_MAX];
    strcpy(dbInfoFilename, prePath);
    strcat(dbInfoFilename, "dbInfo");

    FILE *file = fopen(dbInfoFilename, "r");
    rewind(file);
    fread(db, sizeof(Database), 1, file);
    db->tableList = NewList();
    db->indexList = NewList();
    char tmpFilename[FILENAME_MAX];
    ListNode *listNode = NULL;
    for (int i = 0; i < db->tableNum; ++i)
    {
        fread(tmpFilename, FILENAME_MAX, 1, file);
        listNode = uNewListNode(NewString(tmpFilename, strlen(tmpFilename)));
        ListAppend(db->tableList, listNode);
    }
    for (int i = 0; i < db->indexNum; ++i)
    {
        fread(tmpFilename, FILENAME_MAX, 1, file);
        listNode = uNewListNode(NewString(tmpFilename, strlen(tmpFilename)));
        ListAppend(db->indexList, listNode);
    }

    fclose(file);
    return db;
}

RC ManagerCreateDatabase(XBParser *xbParser, char *dbName)
{
    char mkdirCmd[FILENAME_MAX] = "mkdir ", exeCmd[FILENAME_MAX];

    /*Create database directory.*/
    strcpy(exeCmd, mkdirCmd);
    if (system(strcat(exeCmd, dbName)) != 0)
    {
        fprintf(stderr, "The database %s has exist.\n", dbName);
        return RC_UNKNOW;
    }

    /* New database in memory and write it to dbInfoFile */
    Database *db = NewDatabase(dbName);
    DatabaseInfoFlush(db);

    xbParser->db = db;
    return RC_OK;
}

RC ManagerCreateTable(XBParser *xbParser)
{
    RC rc = RC_UNKNOW;
    if ((rc = ManagerParserIsLegal(xbParser, NK_CREATETABLE)) != RC_OK)
    {
        return rc;
    }
    if ((rc = ManagerDatabaseIsLegal(xbParser)) != RC_OK)
    {
        return rc;
    }

    char exeCmd[STRING_SIZE];
    char rmCmd[STRING_SIZE] = "rm ";

    char prePath[STRING_SIZE];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, xbParser->db->u.Database.dbName);
    strcat(prePath, "/");

    /*Create table dataFile and infoFile.*/
    int tableDataFD, tableInfoFD;
    char tableDataFilename[STRING_SIZE], tableInfoFileName[STRING_SIZE];

    strcpy(tableDataFilename, prePath);
    strcat(tableDataFilename, xbParser->tree->u.CreateTable.tableName);
    strcat(tableDataFilename, ".data");

    strcpy(tableInfoFileName, prePath);
    strcat(tableInfoFileName, xbParser->tree->u.CreateTable.tableName);
    strcat(tableInfoFileName, ".info");

    if ((tableDataFD = open(tableDataFilename, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1)
    {
        fprintf(stderr, "Table %s has exist.\n", tableDataFilename);
        return RC_UNKNOW;
    }
    close(tableDataFD);

    if ((tableInfoFD = open(tableInfoFileName, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1)
    {
        fprintf(stderr, "Table %s has exist.\n", tableDataFilename);
        strcpy(exeCmd, rmCmd);
        strcat(exeCmd, tableDataFilename);
        if (system(exeCmd) != 0)
        {
            fprintf(stderr, "Unknow error.\n");
        }
        return RC_UNKNOW;
    }

    /*Check legality.*/
    char attrTable[TABLE_ATTR_SIZE][STRING_SIZE];
    int attrTableIndex = 0;
    Node *attrList = xbParser->tree->u.CreateTable.attrTypeList;
    while (attrList != NULL && attrList->nodeKind == NK_LIST && attrList->u.List.curr->nodeKind == NK_ATTRTYPE)
    {
        if (strlen(attrList->u.List.curr->u.Attr.type) + strlen(attrList->u.List.curr->u.Attr.attrName) + 2 > TABLE_INFO_LINE_SIZE)
        {
            fprintf(stderr, "Field name %s too long.\n", attrList->u.List.curr->u.Attr.attrName);
            return RC_UNKNOW;
        }
        for (int i = 0; i < attrTableIndex; ++i)
        {
            if (strcmp(attrList->u.List.curr->u.Attr.attrName, attrTable[attrTableIndex]) == 0)
            {
                fprintf(stderr, "There two field name %s.\n", attrTable[attrTableIndex]);
                return RC_UNKNOW;
            }
        }
        if (attrTableIndex >= TABLE_ATTR_SIZE)
        {
            fprintf(stderr, "The number of fields cannot exceed %d.\n", TABLE_ATTR_SIZE);
            return RC_UNKNOW;
        }
        strcpy(attrTable[attrTableIndex], attrList->u.List.curr->u.Attr.attrName);
        ++attrTableIndex;
        attrList = attrList->u.List.next;
    }

    /*Write attribution to the tableInfoFile.*/
    attrList = xbParser->tree->u.CreateTable.attrTypeList;
    char tableInfoLine[TABLE_INFO_LINE_SIZE];
    while (attrList != NULL && attrList->nodeKind == NK_LIST && attrList->u.List.curr->nodeKind == NK_ATTRTYPE)
    {
        memset(tableInfoLine, 0, sizeof(tableInfoLine));
        strcpy(tableInfoLine, "n"); /* 'n' means no index. 'y' means has Index.*/
        strcat(tableInfoLine, attrList->u.List.curr->u.Attr.attrName);
        strcat(tableInfoLine, " ");
        strcat(tableInfoLine, attrList->u.List.curr->u.Attr.type);
        write(tableInfoFD, tableInfoLine, TABLE_INFO_LINE_SIZE);
        attrList = attrList->u.List.next;
    }
    close(tableInfoFD);

    /*Update dbInfo file.*/
    int dbInfoFD;
    char dbInfoFilename[STRING_SIZE];
    strcpy(dbInfoFilename, prePath);
    strcat(dbInfoFilename, "dbInfo");
    if ((dbInfoFD = open(dbInfoFilename, O_WRONLY | O_APPEND)) == -1)
    {
        fprintf(stderr, "Can't open file %s.", dbInfoFilename);
        return RC_UNKNOW;
    }
    write(dbInfoFD, xbParser->tree->u.CreateTable.tableName, STRING_SIZE);
    close(dbInfoFD);

    ManagerUseDatabase(xbParser, xbParser->db->u.Database.dbName);
    return RC_OK;
}

RC ManagerInsert(XBParser *xbParser) /*TODO: Memory leak.*/
{
    RC rc = RC_UNKNOW;
    if ((rc = ManagerParserIsLegal(xbParser, NK_INSERT)) != RC_OK)
    {
        return rc;
    }
    if ((rc = ManagerDatabaseIsLegal(xbParser)) != RC_OK)
    {
        return rc;
    }

    char prePath[STRING_SIZE];
    strcpy(prePath, xbParser->db->u.Database.dbName);
    strcat(prePath, "/");

    /*Is table exist. Get table node. */
    char *tableName = xbParser->tree->u.Insert.tableName;
    Node *tableNode = NULL;
    if ((tableNode = ManagerGetTableNode(xbParser->db, tableName)) == NULL)
    {
        fprintf(stderr, "Not found table %s.\n", tableName);
        return RC_UNKNOW;
    }

    /*Is attribution and type legal. Format attribution and value to pair.*/
    int attrValueListIndex = 0;
    Node *valueList = xbParser->tree->u.Insert.valueList;

    Node *attrList = xbParser->tree->u.Insert.attrList;
    Node *tempAttrTypeList = NULL;
    int isLegal = 0;

    Node attrValueList[TABLE_ATTR_SIZE];
    for (int i = 0; i < TABLE_ATTR_SIZE; ++i)
    {
        attrValueList[i].nodeKind = NK_NONE;
    }
    //tempAttrTypeList = tableAttrList;
    tempAttrTypeList = tableNode->u.Table.attrList;
    while (tempAttrTypeList != NULL)
    {
        attrValueList[attrValueListIndex].u.Attr.attrName = tempAttrTypeList->u.List.curr->u.Attr.attrName;
        attrValueList[attrValueListIndex].u.Attr.type = tempAttrTypeList->u.List.curr->u.Attr.type;
        attrValueList[attrValueListIndex].u.Attr.val = "";
        ++attrValueListIndex;
        tempAttrTypeList = tempAttrTypeList->u.List.next;
    }

    if (attrList != NULL)
    {
        char *attrName = NULL;
        char *type = NULL;
        char *value = NULL;
        while (attrList != NULL && valueList != NULL)
        {
            attrName = attrList->u.List.curr->u.Attr.attrName;
            value = valueList->u.List.curr->u.Attr.val;
            type = valueList->u.List.curr->u.Attr.type;
            isLegal = 0;
            for (int i = attrValueListIndex - 1; i >= 0; --i)
            {
                if (attrValueList[i].nodeKind == NK_NONE &&
                    StringCompareIC(attrValueList[i].u.Attr.attrName, strlen(attrValueList[i].u.Attr.attrName), attrName, strlen(attrName)) == 0 &&
                    StringCompareIC(attrValueList[i].u.Attr.type, strlen(attrValueList[i].u.Attr.type), type, strlen(type)) == 0)
                {
                    attrValueList[i].nodeKind = NK_ATTRTYPE;
                    attrValueList[i].u.Attr.val = value;
                    isLegal = 1;
                    break;
                }
            }
            if (!isLegal)
            {
                fprintf(stderr, "There is no such field %s or the field appears multiple times.\n", attrName);
                return RC_UNKNOW;
            }
            attrList = attrList->u.List.next;
            valueList = valueList->u.List.next;
        }
        if (attrList != NULL || valueList != NULL)
        {
            fprintf(stderr, "Syntax error.\n");
            return RC_SYNTAX_ERROR;
        }
    }
    else
    {
        Node *valueList = xbParser->tree->u.Insert.valueList;
        int i;
        for (i = attrValueListIndex - 1; i >= 0 && valueList != NULL; --i)
        {
            char *value = valueList->u.List.curr->u.Attr.val;
            char *type = valueList->u.List.curr->u.Attr.type;
            if (attrValueList[i].nodeKind == NK_NONE &&
                StringCompareIC(attrValueList[i].u.Attr.type, strlen(attrValueList[i].u.Attr.type), type, strlen(type)) == 0)
            {
                attrValueList[i].nodeKind = NK_ATTRTYPE;
                attrValueList[i].u.Attr.val = value;
            }
            else
            {
                fprintf(stderr, "Syntax error.\n");
                return RC_SYNTAX_ERROR;
            }
            valueList = valueList->u.List.next;
        }
        if (i >= 0 || valueList != NULL)
        {
            fprintf(stderr, "Syntax error.\n");
            return RC_SYNTAX_ERROR;
        }
    }

    /*Write to the table data file.*/
    char tableDataFilename[STRING_SIZE];
    strcpy(tableDataFilename, prePath);
    strcat(tableDataFilename, tableName);
    strcat(tableDataFilename, ".data");
    int tableDataFD;
    if ((tableDataFD = open(tableDataFilename, O_WRONLY | O_APPEND)) == -1)
    {
        fprintf(stderr, "Can't open file %s.\n", tableDataFilename);
        return RC_UNKNOW;
    }
    int attrSize;
    for (int i = 0; i < attrValueListIndex; ++i)
    {
        switch (attrValueList[i].u.Attr.type[0])
        {
        case 'I':
        case 'i':
        {
            attrSize = INT_SIZE;
            break;
        }
        case 'F':
        case 'f':
        {
            attrSize = FLOAT_SIZE;
            break;
        }
        default:
        {
            attrSize = STRING_SIZE;
            break;
        }
        }
        write(tableDataFD, attrValueList[i].u.Attr.val, attrSize);
        write(STDOUT_FILENO, attrValueList[i].u.Attr.val, strlen(attrValueList[i].u.Attr.val));
    }
    close(tableDataFD);
the_end:
    /*Free memroy.*/
    return RC_OK;
}

RC ManagerCreateIndex(XBParser *xbParser)
{
    RC rc;
    if ((rc = ManagerParserIsLegal(xbParser, NK_CREATEINDEX)) != RC_OK)
    {
        return rc;
    }
    if ((rc = ManagerDatabaseIsLegal(xbParser)) != RC_OK)
    {
        return rc;
    }

    Node *tableNode = NULL;
    if ((tableNode = ManagerGetTableNode(xbParser->db, xbParser->tree->u.CreateIndex.tableName)) == NULL)
    {
        fprintf(stderr, "Can't found table %s.\n", xbParser->tree->u.CreateIndex.tableName);
        return RC_UNKNOW;
    }

    /* Is legal attrname. */
    Node *attrList = tableNode->u.Table.attrList;
    int isLegal = 0;
    while (attrList != NULL)
    {
        if (strcmp(attrList->u.List.curr->u.Attr.attrName, xbParser->tree->u.CreateIndex.attrname) == 0)
        {
            isLegal = 1;
            attrList->u.List.curr->u.Attr.isIndex = 'y';
            break;
        }
        attrList = attrList->u.List.next;
    }
    if (!isLegal)
    {
        fprintf(stderr, "No field %s in table %s.\n", xbParser->tree->u.CreateIndex.attrname, xbParser->tree->u.CreateIndex.tableName);
        return RC_UNKNOW;
    }

    /*Create index file.*/
    char prePath[STRING_SIZE];
    strcpy(prePath, xbParser->db->u.Database.dbName);
    strcat(prePath, "/");
    char indexFilename[STRING_SIZE];
    strcpy(indexFilename, prePath);
    strcat(indexFilename, xbParser->tree->u.CreateIndex.tableName);
    strcat(indexFilename, ".");
    strcat(indexFilename, xbParser->tree->u.CreateIndex.attrname);
    strcat(indexFilename, ".index");
    int indexFD;
    if ((indexFD = open(indexFilename, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1)
    {
        fprintf(stderr, "Can't create file %s.\n", indexFilename);
        return RC_UNKNOW;
    }
    close(indexFD);

    /*Modify table information file.*/
    char tableInfoFilename[STRING_SIZE];
    strcpy(tableInfoFilename, prePath);
    strcat(tableInfoFilename, xbParser->tree->u.CreateIndex.tableName);
    strcat(tableInfoFilename, ".info");
    int fdTableInfo;
    if ((fdTableInfo = open(tableInfoFilename, O_RDWR)) == -1)
    {
        fprintf(stderr, "Can't open file %s.\n", tableInfoFilename);
        return RC_UNKNOW;
    }
    lseek(fdTableInfo, attrList->u.List.numChilds * STRING_SIZE, SEEK_SET);
    write(fdTableInfo, "y", 1);
    close(fdTableInfo);

    return RC_OK;
}