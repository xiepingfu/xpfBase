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
#include "bptree.h"

#define DEBUG 1
#define TABLE_INFO_LINE_SIZE 256
#define TABLE_ATTR_SIZE 32

void FreeDatabase(Database *db);
RC DatabaseInfoFlush(Database *db);
Database *OpenDatabase(XBParser *xbParser, char *dbName);

RC CreateIndex(char *fielName)
{
    Bptree *tree = CreateBptree(fielName);
    CloseBptree(tree);
    return RC_OK;
}

RC ManagerUseDatabase(XBParser *xbParser, char *dbName)
{
    FreeDatabase(xbParser->db);
    xbParser->db = OpenDatabase(xbParser, dbName);
    if (xbParser->db == NULL)
    {
        return RC_NOFILE;
    }
    return RC_OK;
}

Database *NewDatabase(char *dbName)
{
    Database *db = (Database *)xpfMalloc(sizeof(Database));
    strcpy(db->dbName, dbName);
    db->tableNum = 0;
    db->tableList = NewList();
    return db;
}

void FreeTable(Table *table)
{
    if (table == NULL)
        return;
    FreeList(table->fieldList);
    free(table);
}

void FreeDatabase(Database *db)
{
    if (db == NULL)
        return;
    for (ListNode *i = db->tableList->first; i != NULL; i = i->next)
    {
        FreeTable(i->data);
        i->data = NULL;
    }
    FreeList(db->tableList);
}
Table *NewTable(char *tableName)
{
    Table *table = (Table *)xpfMalloc(sizeof(Table));
    strcpy(table->tableName, tableName);
    table->fieldList = NewList();
    table->dataNum = 0;
    Field *field = (Field *)xpfMalloc(sizeof(Field));
    strcpy(field->fieldName, "id");
    strcpy(field->fieldType, "INT");
    field->hasIndex = 1;
    ListNode *listNode = uNewListNode(field);
    ListAppend(table->fieldList, listNode);
    table->fieldNum = 1;
    table->dataSize = INT_SIZE;
    return table;
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
        fwrite(listNode->data, sizeof(Table), 1, file);
        ListNode *tmpListNode = ((Table *)listNode->data)->fieldList->first;
        while (tmpListNode != NULL)
        {
            fwrite((Field *)tmpListNode->data, sizeof(Field), 1, file);
            tmpListNode = tmpListNode->next;
        }
        listNode = listNode->next;
    }
    fclose(file);
}

Database *OpenDatabase(XBParser *xbParser, char *dbName)
{
    if (access(dbName, F_OK) != 0)
    {
        char msg[2048];
        sprintf(msg, "Not found database %s.", dbName);
        write(xbParser->clientfd, msg, strlen(msg));
        return NULL;
    }

    Database *db = (Database *)xpfMalloc(sizeof(Database));

    char prePath[FILENAME_MAX];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, dbName);
    strcat(prePath, "/");
    char dbInfoFilename[FILENAME_MAX];
    strcpy(dbInfoFilename, prePath);
    strcat(dbInfoFilename, "dbInfo");

    FILE *file = fopen(dbInfoFilename, "r");
    rewind(file);
    fread(db, sizeof(Database), 1, file);
    db->tableList = NewList();
    char tmpFilename[FILENAME_MAX];
    ListNode *listNode = NULL;
    for (int i = 0; i < db->tableNum; ++i)
    {
        Table *table = (Table *)xpfMalloc(sizeof(Table));
        fread(table, sizeof(Table), 1, file);
        table->fieldList = NewList();
        for (int j = 0; j < table->fieldNum; ++j)
        {
            Field *field = (Field *)xpfMalloc(sizeof(Field));
            fread(field, sizeof(Field), 1, file);
            ListNode *newListNode = uNewListNode(field);
            ListAppend(table->fieldList, newListNode);
        }
        listNode = uNewListNode(table);
        ListAppend(db->tableList, listNode);
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
        char msg[2048];
        sprintf(msg, "The database %s has exist.\n", dbName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }

    /* New database in memory and write it to dbInfoFile */
    Database *db = NewDatabase(dbName);
    DatabaseInfoFlush(db);

    xbParser->db = db;
    return RC_OK;
}

Field *NewField(char *fieldlName, char *fieldType, char hasIndex)
{
    Field *field = (Field *)xpfMalloc(sizeof(Field));
    strcpy(field->fieldName, fieldlName);
    strcpy(field->fieldType, fieldType);
    field->hasIndex = hasIndex;
    return field;
}

RC ManagerCreateTable(XBParser *xbParser)
{
    if (xbParser->db == NULL)
    {
        write(xbParser->clientfd, "USE Database firstly.\n", strlen("USE Database firstly.\n"));
        return RC_UNKNOW;
    }

    /* Is table had existed. */
    ListNode *listNode = xbParser->db->tableList->first;
    while (listNode != NULL)
    {
        if (strcmp(xbParser->tree->u.CreateTable.tableName, (char *)listNode->data) == 0)
        {
            char msg[2048];
            sprintf(msg, "Table %s had existed.\n", xbParser->tree->u.CreateTable.tableName);
            write(xbParser->clientfd, msg, strlen(msg));
            return RC_UNKNOW;
        }
        listNode = listNode->next;
    }

    /* New Table. */
    Table *table = NewTable(xbParser->tree->u.CreateTable.tableName);
    Node *fieldList = xbParser->tree->u.CreateTable.attrTypeList;
    while (fieldList != NULL)
    {
        Field *newField = NewField(fieldList->u.List.curr->u.Attr.attrName, fieldList->u.List.curr->u.Attr.type, 0);
        ListNode *newListNode = uNewListNode(newField);
        ListAppend(table->fieldList, newListNode);
        table->fieldNum += 1;

        int fieldSize = STRING_SIZE;
        if (strcmp(newField->fieldType, "INT") == 0)
        {
            fieldSize = INT_SIZE;
        }
        else if (strcmp(newField->fieldType, "FLOAT") == 0)
        {
            fieldSize = FLOAT_SIZE;
        }
        table->dataSize += fieldSize;

        fieldList = fieldList->u.List.next;
    }

    /* Create file. */
    char prePath[FILENAME_MAX], tableDataFilename[FILENAME_MAX], indexFilename[FILENAME_MAX];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, xbParser->db->dbName);
    strcat(prePath, "/");
    strcpy(tableDataFilename, prePath);
    strcat(tableDataFilename, xbParser->tree->u.CreateTable.tableName);
    strcpy(indexFilename, tableDataFilename);
    strcat(indexFilename, ".id.index");
    strcat(tableDataFilename, ".data");
    FILE *file = fopen(tableDataFilename, "w");
    fclose(file);

    CreateIndex(indexFilename);

    ListNode *newListNode = uNewListNode(table);
    ListAppend(xbParser->db->tableList, newListNode);
    xbParser->db->tableNum += 1;
    DatabaseInfoFlush(xbParser->db);
    return RC_OK;
}

RC IndexInsert(char *filename, int key, int data)
{
    Bptree *tree = OpenBptree(filename);
    BptreeInsert(tree, key, data);
    CloseBptree(tree);
}

void DataFlush(Table *table, char *filename, List *list, int offset)
{
    FILE *file = fopen(filename, "r+");
    fseek(file, table->dataSize * offset, SEEK_SET);
    for (ListNode *i = list->first; i != NULL; i = i->next)
    {
        FieldData *fData = i->data;
        int fieldSize = STRING_SIZE;
        if (fData->fieldType[0] == 'I')
        {
            fieldSize = INT_SIZE;
        }
        else if (fData->fieldType[0] == 'F')
        {
            fieldSize = FLOAT_SIZE;
        }
        fwrite(fData->value, fieldSize, 1, file);
    }
    fclose(file);
}

RC ManagerInsert(XBParser *xbParser) /*TODO: Memory leak.*/
{
    if (xbParser->db == NULL)
    {
        write(xbParser->clientfd, "USE Database firstly.\n", strlen("USE Database firstly.\n"));
        return RC_UNKNOW;
    }

    /* Is table exist. */
    char *tableName = xbParser->tree->u.Insert.tableName;
    ListNode *listNode = xbParser->db->tableList->first;
    Table *table = NULL;
    while (listNode != NULL)
    {
        if (strcmp(((Table *)listNode->data)->tableName, tableName) == 0)
        {
            table = (Table *)listNode->data;
            break;
        }
        listNode = listNode->next;
    }
    if (table == NULL)
    {
        char msg[2048];
        sprintf(msg, "Not found table %s.\n", tableName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }

    List *fieldDataList = NewList();
    ListNode *fieldListNode = table->fieldList->first;
    while (fieldListNode != NULL)
    {
        FieldData *fieldData = (FieldData *)xpfMalloc(sizeof(FieldData));
        strcpy(fieldData->fieldName, ((Field *)fieldListNode->data)->fieldName);
        strcpy(fieldData->fieldType, ((Field *)fieldListNode->data)->fieldType);
        strcpy(fieldData->value, "");
        fieldData->hasIndex = ((Field *)fieldListNode->data)->hasIndex;
        ListNode *tmpListNode = uNewListNode(fieldData);
        ListAppend(fieldDataList, tmpListNode);
        fieldListNode = fieldListNode->next;
    }

    ListNode *fieldDataListNode = fieldDataList->first->next;
    char *attrName = NULL;
    char *type = NULL;
    char *value = NULL;
    Node *valueList = xbParser->tree->u.Insert.valueList;
    Node *fieldList = xbParser->tree->u.Insert.attrList;
    if (fieldList == NULL)
    {
        while (valueList != NULL && fieldDataListNode != NULL)
        {
            char *value = valueList->u.List.curr->u.Attr.val;
            char *type = valueList->u.List.curr->u.Attr.type;
            if (strcmp(type, ((FieldData *)fieldDataListNode->data)->fieldType) == 0)
            {
                strcpy(((FieldData *)fieldDataListNode->data)->value, value);
            }
            else
            {
                break;
            }
            valueList = valueList->u.List.next;
            fieldDataListNode = fieldDataListNode->next;
        }
        if (valueList != NULL || fieldDataListNode != NULL)
        {
            write(xbParser->clientfd, "Syntax error.\n", strlen("Syntax error.\n"));
            return RC_SYNTAX_ERROR;
        }
    }
    else
    {
        while (valueList != NULL && fieldList != NULL)
        {
            attrName = fieldList->u.List.curr->u.Attr.attrName;
            value = valueList->u.List.curr->u.Attr.val;
            type = valueList->u.List.curr->u.Attr.type;
            fieldListNode = fieldDataList->first->next;
            while (fieldListNode != NULL)
            {
                if (strcmp(type, ((FieldData *)fieldListNode->data)->fieldType) == 0 && strcmp(attrName, ((FieldData *)fieldListNode->data)->fieldName) == 0)
                {
                    strcpy(((FieldData *)fieldListNode->data)->value, value);
                    break;
                }
                fieldListNode = fieldListNode->next;
            }
            valueList = valueList->u.List.next;
            fieldList = fieldList->u.List.next;
        }
        if (valueList != NULL && fieldList != NULL)
        {
            write(xbParser->clientfd, "Syntax error.\n", strlen("Syntax error.\n"));
            return RC_SYNTAX_ERROR;
        }
        fieldListNode = fieldDataList->first->next;
        while (fieldListNode != NULL)
        {
            if (((FieldData *)fieldListNode->data)->hasIndex == 1 && strlen(((FieldData *)fieldListNode->data)->value) == 0)
            {
                write(xbParser->clientfd, "Index error!\n", strlen("Index error!\n"));
                return RC_SYNTAX_ERROR;
            }
            fieldListNode = fieldListNode->next;
        }
    }

    fieldListNode = fieldDataList->first;

    strcpy(((FieldData *)fieldListNode->data)->value, IntToString(table->dataNum));

    int fieldSize = STRING_SIZE;
    char prePath[FILENAME_MAX], tableDataFilename[FILENAME_MAX], preIndexFilename[FILENAME_MAX], indexFilename[FILENAME_MAX];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, xbParser->db->dbName);
    strcat(prePath, "/");
    strcpy(tableDataFilename, prePath);
    strcat(tableDataFilename, tableName);
    strcat(tableDataFilename, ".data");
    strcpy(preIndexFilename, prePath);
    strcat(preIndexFilename, tableName);
    strcat(preIndexFilename, ".");

    FILE *file = fopen(tableDataFilename, "a");
    while (fieldListNode != NULL)
    {
        fieldSize = STRING_SIZE;
        if (strcmp(((FieldData *)fieldListNode->data)->fieldType, "INT") == 0)
        {
            fieldSize = INT_SIZE;
        }
        else if (strcmp(((FieldData *)fieldListNode->data)->fieldType, "FLOAT") == 0)
        {
            fieldSize = FLOAT_SIZE;
        }
        fwrite(((FieldData *)fieldListNode->data)->value, fieldSize, 1, file);
        if (((FieldData *)fieldListNode->data)->hasIndex)
        {
            strcpy(indexFilename, preIndexFilename);
            strcat(indexFilename, "id");
            strcat(indexFilename, ".index");
            IndexInsert(indexFilename, atoi(((FieldData *)fieldListNode->data)->value), table->dataNum);
        }
        fieldListNode = fieldListNode->next;
    }
    fclose(file);
    table->dataNum += 1;

    DatabaseInfoFlush(xbParser->db);

    /* TODO:free */
    return RC_OK;
}

List *DataSeek(Table *table, char *filename, int offset)
{
    List *list = NewList();
    ListNode *listNode = NULL;
    FILE *file = fopen(filename, "r");
    fseek(file, table->dataSize * offset, SEEK_SET);
    listNode = table->fieldList->first;
    int fieldSize = STRING_SIZE;
    char value[STRING_SIZE];
    while (listNode != NULL)
    {
        fieldSize = STRING_SIZE;
        if (((Field *)listNode->data)->fieldType[0] == 'I')
        {
            fieldSize = INT_SIZE;
        }
        else if (((Field *)listNode->data)->fieldType[0] == 'F')
        {
            fieldSize = FLOAT_SIZE;
        }
        fread(value, fieldSize, 1, file);
        FieldData *fieldData = (FieldData *)xpfMalloc(sizeof(FieldData));
        strcpy(fieldData->fieldName, ((Field *)listNode->data)->fieldName);
        strcpy(fieldData->fieldType, ((Field *)listNode->data)->fieldType);
        strcpy(fieldData->value, value);
        fieldData->hasIndex = ((Field *)listNode->data)->hasIndex;
        ListNode *newListNode = uNewListNode(fieldData);
        ListAppend(list, newListNode);
        listNode = listNode->next;
    }
    fclose(file);
    return list;
}

RC ManagerCreateIndex(XBParser *xbParser)
{
    if (xbParser->db == NULL)
    {
        write(xbParser->clientfd, "USE Database firstly.\n", strlen("USE Database firstly.\n"));
        return RC_UNKNOW;
    }
    char *tableName = xbParser->tree->u.CreateIndex.tableName;
    char *fieldName = xbParser->tree->u.CreateIndex.attrname;

    /* Is table exist? Is field exist? Is index exist? */
    Table *table = NULL;
    ListNode *listNode = xbParser->db->tableList->first;
    while (listNode != NULL)
    {
        if (strcmp(((Table *)listNode->data)->tableName, tableName) == 0)
        {
            table = listNode->data;
            break;
        }
        listNode = listNode->next;
    }
    if (table == NULL)
    {
        char msg[2048];
        sprintf(msg, "Not found table %s.\n", tableName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }
    listNode = table->fieldList->first;
    Field *field = NULL;
    while (listNode != NULL)
    {
        if (strcmp(((Field *)listNode->data)->fieldName, fieldName) == 0)
        {
            field = listNode->data;
            break;
        }
        listNode = listNode->next;
    }
    if (field == NULL)
    {
        char msg[2048];
        sprintf(msg, "Not found field %s.\n", fieldName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }
    if (field->hasIndex)
    {
        char msg[2048];
        sprintf(msg, "Index has existed.\n");
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }
    if (field->fieldType[0] != 'I')
    {
        char msg[2048];
        sprintf(msg, "Only int type can create index.\n");
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }

    /* Create index */
    field->hasIndex = 1;
    char prePath[FILENAME_MAX], indexFilename[FILENAME_MAX], dataFilename[FILENAME_MAX];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, xbParser->db->dbName);
    strcat(prePath, "/");
    strcpy(indexFilename, prePath);
    strcat(indexFilename, tableName);
    strcpy(dataFilename, indexFilename);
    strcat(dataFilename, ".data");
    strcat(indexFilename, ".");
    strcat(indexFilename, fieldName);
    strcat(indexFilename, ".index");
    CreateIndex(indexFilename);

    List *fieldDataList = NULL;
    ListNode *fieldDataListNode = NULL;
    for (int i = 0; i < table->dataNum; ++i)
    {
        fieldDataList = DataSeek(table, dataFilename, i);
        fieldDataListNode = fieldDataList->first;
        while (fieldDataListNode != NULL)
        {
            if (strcmp(((FieldData *)fieldDataListNode->data)->fieldName, fieldName) == 0)
            {
                break;
            }
            fieldDataListNode = fieldDataListNode->next;
        }
        if (fieldDataListNode == NULL)
        {
            char msg[2048];
            sprintf(msg, "Not found field %s.\n", fieldName);
            write(xbParser->clientfd, msg, strlen(msg));
            return RC_UNKNOW;
        }
        int key = atoi(((FieldData *)fieldDataListNode->data)->value);
        IndexInsert(indexFilename, key, i);
        FreeList(fieldDataList);
    }

    DatabaseInfoFlush(xbParser->db);

    return RC_OK;
}

List *GetSelectTable(XBParser *xbParser, List *tableList)
{
    List *list = NewList();
    char *tableVis[32];
    int tableVisIndex = 0;
    for (ListNode *i = tableList->first; i != NULL; i = i->next)
    {
        int k = 0;
        for (int j = 0; j < tableVisIndex; ++j)
        {
            if (strcmp(tableVis[j], (char *)i->data) == 0)
            {
                k = 1;
                break;
            }
        }
        if (k)
        {
            continue;
        }

        for (ListNode *j = xbParser->db->tableList->first; j != NULL; j = j->next)
        {
            Table *t = j->data;
            if (strcmp(t->tableName, (char *)i->data) == 0)
            {
                SelectTable *sTable = (SelectTable *)xpfMalloc(sizeof(SelectTable));
                sTable->sFieldList = NewList();
                sTable->sFieldDataList = NewList();
                sTable->table = t;
                strcpy(sTable->tableName, t->tableName);
                strcpy(sTable->dbName, xbParser->db->dbName);
                ListNode *newListNode = uNewListNode(sTable);
                ListAppend(list, newListNode);
                tableVis[tableVisIndex] = (char *)i->data;
                tableVisIndex += 1;
                k = 1;
                break;
            }
        }
        if (!k)
        {
            char msg[2048];
            sprintf(msg, "Not found table %s.\n", (char *)i->data);
            write(xbParser->clientfd, msg, strlen(msg));
            FreeList(list);
            return NULL;
        }
    }

    return list;
}

RC GetSelectField(XBParser *xbParser, List *sTableList, List *colList)
{
    if (colList->size == 0) /* STAR */
    {
        for (ListNode *j = sTableList->first; j != NULL; j = j->next)
        {
            SelectTable *sTable = j->data;
            for (ListNode *k = sTable->table->fieldList->first; k != NULL; k = k->next)
            {
                Field *field = k->data;
                Column *column = (Column *)xpfMalloc(sizeof(Column));
                strcpy(column->fieldName, field->fieldName);
                strcpy(column->tableName, sTable->tableName);
                strcpy(column->nickName, column->tableName);
                strcat(column->nickName, ".");
                strcat(column->nickName, field->fieldName);
                ListAppend(colList, uNewListNode(column));
            }
        }
    }

    for (ListNode *i = colList->first; i != NULL; i = i->next)
    {
        int n = 0;
        Column *column = i->data;
        int ctl = strlen(column->tableName);
        for (ListNode *j = sTableList->first; j != NULL; j = j->next)
        {
            SelectTable *sTable = j->data;
            if (ctl == 0)
            {
                for (ListNode *k = sTable->table->fieldList->first; k != NULL; k = k->next)
                {
                    Field *field = k->data;
                    if (strcmp(column->fieldName, field->fieldName) == 0)
                    {
                        strcpy(column->tableName, sTable->tableName);
                        strcpy(column->fieldType, field->fieldType);
                        column->hasIndex = field->hasIndex;
                        ++n;
                    }
                    if (n > 1)
                    {
                        break;
                    }
                }
            }
            else
            {
                if (strcmp(column->tableName, sTable->tableName) == 0)
                {
                    for (ListNode *k = sTable->table->fieldList->first; k != NULL; k = k->next)
                    {
                        Field *field = k->data;
                        if (strcmp(column->fieldName, field->fieldName) == 0)
                        {
                            strcpy(column->tableName, sTable->tableName);
                            strcpy(column->fieldType, field->fieldType);
                            column->hasIndex = field->hasIndex;
                            ++n;
                        }
                        if (n > 1)
                        {
                            break;
                        }
                    }
                }
            }
        }
        if (n == 0)
        {
            char msg[2048];
            sprintf(msg, "Not found field %s in all tables.\n", column->fieldName);
            write(xbParser->clientfd, msg, strlen(msg));
            return RC_UNKNOW;
        }
        else if (n > 1)
        {
            char msg[2048];
            sprintf(msg, "There are field %s in defferent tables.\n", column->fieldName);
            write(xbParser->clientfd, msg, strlen(msg));
            return RC_UNKNOW;
        }
        else
        {
            n = 0;
            for (ListNode *j = sTableList->first; j != NULL; j = j->next)
            {
                SelectTable *sTable = j->data;
                if (strcmp(sTable->tableName, column->tableName) == 0)
                {
                    SelectField *sField = (SelectField *)xpfMalloc(sizeof(SelectField));
                    strcpy(sField->fieldName, column->fieldName);
                    strcpy(sField->fieldType, column->fieldType);
                    sField->hasIndex = column->hasIndex;
                    sField->op = TK_SEMICOLON;
                    sField->sort = 0;
                    sField->dataList = NewList();
                    strcpy(sField->value, "");
                    ListAppend(sTable->sFieldList, uNewListNode(sField));
                    ++n;
                    break;
                }
            }
            if (!n)
            {
                char msg[2048];
                sprintf(msg, "Not found field %s in all tables.\n", column->fieldName);
                write(xbParser->clientfd, msg, strlen(msg));
                return RC_UNKNOW;
            }
        }
    }
    return RC_OK;
}

RC CheckConditionList(XBParser *xbParser, List *sTableList, List *conditionList)
{
    for (ListNode *i = conditionList->first; i != NULL; i = i->next)
    {
        Condition *condition = i->data;
        if (strlen(condition->leftValue) == 0 && strlen(condition->rightValue) == 0)
        {
            char msg[2048];
            sprintf(msg, "Does not support such conditions.\n");
            write(xbParser->clientfd, msg, strlen(msg));
            return RC_UNKNOW;
        }

        if (strlen(condition->leftValue) != 0)
        {
            for (ListNode *j = sTableList->first; j != NULL; j = j->next)
            {
                SelectTable *sTable = j->data;
                for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                {
                    SelectField *sField = k->data;
                    if (strcmp(condition->rightField, sField->fieldName) == 0 && (strlen(condition->rightTable) == 0 || strcmp(condition->rightTable, sTable->tableName) == 0))
                    {
                        strcpy(sField->value, condition->leftValue);
                        switch (condition->op)
                        {
                        case TK_EQ:
                        case TK_NE:
                        {
                            sField->op = condition->op;
                            break;
                        }
                        case TK_LE:
                        {
                            sField->op = TK_GT;
                            break;
                        }
                        case TK_LT:
                        {
                            sField->op = TK_GE;
                            break;
                        }
                        case TK_GT:
                        {
                            sField->op = TK_LE;
                            break;
                        }
                        case TK_GE:
                        {
                            sField->op = TK_LT;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                        }
                    }
                }
            }
        }
        else
        {
            for (ListNode *j = sTableList->first; j != NULL; j = j->next)
            {
                SelectTable *sTable = j->data;
                for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                {
                    SelectField *sField = k->data;
                    if (strcmp(condition->leftField, sField->fieldName) == 0 && (strlen(condition->leftTable) == 0 || strcmp(condition->leftTable, sTable->tableName) == 0))
                    {
                        strcpy(sField->value, condition->rightValue);
                        sField->op = condition->op;
                    }
                }
            }
        }
    }
    return RC_OK;
}

RC CheckOrderBy(XBParser *xbParser, List *sTableList, OrderBy *orderby)
{
    int n = 0;
    for (ListNode *i = sTableList->first; i != NULL; i = i->next)
    {
        SelectTable *sTable = i->data;
        for (ListNode *j = sTable->sFieldList->first; j != NULL; j = j->next)
        {
            SelectField *sField = j->data;
            if (strcmp(sField->fieldName, orderby->fieldName) == 0 && (strlen(orderby->tableName) == 0 || strcmp(orderby->tableName, sTable->tableName) == 0))
            {
                if (orderby->asc)
                {
                    sField->sort = 1;
                }
                else
                {
                    sField->sort = -1;
                }
                ++n;
            }
        }
    }
    if (n == 0)
    {
        char msg[2048];
        sprintf(msg, "Not found field %s in all tables.\n", orderby->fieldName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }
    else if (n > 1)
    {
        char msg[2048];
        sprintf(msg, "There are field %s in defferent tables.\n", orderby->fieldName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }
    return RC_OK;
}

void SelectTableQuery(SelectTable *sTable)
{
    SelectField *useIndex = NULL;
    for (ListNode *i = sTable->sFieldList->first; i != NULL; i = i->next)
    {
        SelectField *sField = i->data;
        if (sField->op == TK_EQ && sField->hasIndex)
        {
            useIndex = sField;
            break;
        }
    }
    char prePath[FILENAME_MAX], tableDataFilename[FILENAME_MAX], preIndexFilename[FILENAME_MAX];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, sTable->dbName);
    strcat(prePath, "/");
    strcpy(tableDataFilename, prePath);
    strcat(tableDataFilename, sTable->tableName);
    strcpy(preIndexFilename, tableDataFilename);
    strcat(tableDataFilename, ".data");
    strcat(preIndexFilename, ".");
    if (useIndex == NULL)
    {
        char indexFilename[FILENAME_MAX];
        strcpy(indexFilename, preIndexFilename);
        strcat(indexFilename, "id.index");

        Bptree *tree = OpenBptree(indexFilename);

        for (int i = 0; i < sTable->table->dataNum; ++i)
        {
            List *list = BptreeSearch(tree, i);
            for (ListNode *j = list->first; j != NULL; j = j->next)
            {
                List *fdList = DataSeek(sTable->table, tableDataFilename, *((int *)j->data));
                int ok = 1;
                for (ListNode *fd = fdList->first; fd != NULL; fd = fd->next)
                {
                    if (!ok)
                    {
                        break;
                    }
                    FieldData *fData = fd->data;

                    for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                    {
                        SelectField *sField = k->data;
                        if (strcmp(fData->fieldType, sField->fieldType) != 0 || strcmp(fData->fieldName, sField->fieldName) != 0)
                        {
                            continue;
                        }

                        if (sField->op == TK_SEMICOLON)
                        {
                            continue;
                        }
                        else if (sField->op == TK_EQ)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) != atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) != atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) != 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_NE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) == atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) == atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) == 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) > atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) > atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) > 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) >= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) >= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) >= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) < atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) < atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) < 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) <= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) <= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) <= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                    }
                }
                if (ok)
                {
                    List *dataList = NewList();
                    for (ListNode *fd = fdList->first; fd != NULL; fd = fd->next)
                    {
                        FieldData *fData = fd->data;
                        for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                        {
                            SelectField *sField = k->data;
                            if (strcmp(sField->fieldName, fData->fieldName) == 0)
                            {
                                FieldData *fieldData = (FieldData *)xpfMalloc(sizeof(FieldData));
                                strcpy(fieldData->fieldName, fData->fieldName);
                                strcpy(fieldData->tableName, fData->tableName);
                                strcpy(fieldData->value, fData->value);
                                ListAppend(dataList, uNewListNode(fieldData));
                                break;
                            }
                        }
                    }
                    ListAppend(sTable->sFieldDataList, uNewListNode(dataList));
                }
                FreeList(fdList);
            }
            FreeList(list);
        }
        CloseBptree(tree);
    }
    else
    {
        char indexFilename[FILENAME_MAX];
        strcpy(indexFilename, preIndexFilename);
        strcat(indexFilename, useIndex->fieldName);
        strcat(indexFilename, ".index");

        Bptree *tree = OpenBptree(indexFilename);
        int key = atoi(useIndex->value);
        List *tmpList = BptreeSearch(tree, key);
        CloseBptree(tree);

        strcpy(indexFilename, preIndexFilename);
        strcat(indexFilename, "id.index");
        tree = OpenBptree(indexFilename);

        for (ListNode *i = tmpList->first; i != NULL; i = i->next)
        {
            List *list = BptreeSearch(tree, *((int *)i->data));
            for (ListNode *j = list->first; j != NULL; j = j->next)
            {
                List *fdList = DataSeek(sTable->table, tableDataFilename, *((int *)j->data));
                int ok = 1;
                for (ListNode *fd = fdList->first; fd != NULL; fd = fd->next)
                {
                    if (!ok)
                    {
                        break;
                    }
                    FieldData *fData = fd->data;
                    for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                    {
                        SelectField *sField = k->data;
                        if (strcmp(fData->fieldType, sField->fieldType) != 0 || strcmp(fData->fieldName, sField->fieldName) != 0)
                        {
                            continue;
                        }

                        if (sField->op == TK_SEMICOLON)
                        {
                            continue;
                        }
                        else if (sField->op == TK_EQ)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) != atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) != atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) != 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_NE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) == atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) == atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) == 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) > atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) > atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) > 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) >= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) >= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) >= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) < atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) < atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) < 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) <= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) <= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) <= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                    }
                }
                if (ok)
                {
                    List *dataList = NewList();
                    for (ListNode *fd = fdList->first; fd != NULL; fd = fd->next)
                    {
                        FieldData *fData = fd->data;
                        for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                        {
                            SelectField *sField = k->data;
                            if (strcmp(sField->fieldName, fData->fieldName) == 0)
                            {
                                FieldData *fieldData = (FieldData *)xpfMalloc(sizeof(FieldData));
                                strcpy(fieldData->fieldName, fData->fieldName);
                                strcpy(fieldData->tableName, fData->tableName);
                                strcpy(fieldData->value, fData->value);
                                ListAppend(dataList, uNewListNode(fieldData));
                                break;
                            }
                        }
                    }
                    ListAppend(sTable->sFieldDataList, uNewListNode(dataList));
                }
                FreeList(fdList);
            }
            FreeList(list);
        }
        CloseBptree(tree);
        FreeList(tmpList);
    }
}

List *SelectTableListQuery(List *sTableList, List *colList)
{
    List *result = NewList();
    return result;
}

void CartesianProduct(XBParser *xbParser, ListNode *sTableNode, List *sTableList, List *colList)
{
    if (sTableNode == NULL)
    {
        for (ListNode *j = colList->first; j != NULL; j = j->next)
        {
            int ok = 1;
            Column *column = j->data;
            for (ListNode *i = sTableList->first; i != NULL && ok; i = i->next)
            {
                SelectTable *sTable = i->data;
                if (strcmp(sTable->tableName, column->tableName) == 0)
                {
                    for (ListNode *k = ((List *)sTable->curNode->data)->first; k != NULL; k = k->next)
                    {
                        FieldData *fd = k->data;
                        if (strcmp(fd->fieldName, column->fieldName) == 0)
                        {
                            ok = 0;
                            char msg[2048];
                            sprintf(msg, "%s\t", fd->value);
                            write(xbParser->clientfd, msg, strlen(msg));
                            break;
                        }
                    }
                }
            }
        }
        write(xbParser->clientfd, "\n", strlen("\n"));
        return;
    }
    SelectTable *sTable = sTableNode->data;
    ListNode *nextsTableNode = sTableNode->next;
    for (ListNode *i = sTable->sFieldDataList->first; i != NULL; i = i->next)
    {
        sTable->curNode = i;
        CartesianProduct(xbParser, nextsTableNode, sTableList, colList);
    }
}

RC ManagerSelect(XBParser *xbParser, List *colList, List *tableList, List *conditionList, OrderBy *orderBy)
{
    if (xbParser->db == NULL)
    {
        write(xbParser->clientfd, "USE Database firstly.\n", strlen("USE Database firstly.\n"));
        return RC_UNKNOW;
    }

    /*Check tableList*/
    List *sTableList = GetSelectTable(xbParser, tableList);
    if (sTableList == NULL)
    {
        return RC_UNKNOW;
    }

    /*Check colList*/
    if (GetSelectField(xbParser, sTableList, colList) != RC_OK)
    {
        FreeList(sTableList);
        return RC_UNKNOW;
    }

    /*CheckconditionList*/
    if (conditionList->size != 0)
    {
        if (CheckConditionList(xbParser, sTableList, conditionList) != RC_OK)
        {
            FreeList(sTableList);
            return RC_UNKNOW;
        }
    }

    /*Check orderBy*/
    if (strlen(orderBy->fieldName) != 0)
    {
        if (CheckOrderBy(xbParser, sTableList, orderBy) != RC_OK)
        {
            FreeList(sTableList);
            return RC_UNKNOW;
        }
    }

    /*Query*/
    for (ListNode *i = sTableList->first; i != NULL; i = i->next)
    {
        SelectTableQuery(i->data);
        ((SelectTable *)i->data)->curNode = ((SelectTable *)i->data)->sFieldDataList->first;
    }

    /*Cartesian product And Print*/
    char msg[2048];
    for (ListNode *i = colList->first; i != NULL; i = i->next)
    {
        Column *column = i->data;
        sprintf(msg, "%s\t", column->nickName);
        write(xbParser->clientfd, msg, strlen(msg));
    }
    write(xbParser->clientfd, "\n", strlen("\n"));

    CartesianProduct(xbParser, sTableList->first, sTableList, colList);

    return RC_OK;
}

RC RemoveFile(char *filename)
{
    char exeCmd[FILENAME_MAX] = "rm ";
    strcat(exeCmd, filename);
    if (system(exeCmd) != 0)
    {
        return RC_UNKNOW;
    }
    return RC_OK;
}

RC ManagerDropDatabase(XBParser *xbParser, char *dbName)
{
    xbParser->db = OpenDatabase(xbParser, dbName);
    if (xbParser->db == NULL)
    {
        return RC_NOFILE;
    }
    char exeCmd[FILENAME_MAX] = "rm -r ./";
    strcat(exeCmd, dbName);
    if (system(exeCmd) != 0)
    {

        write(xbParser->clientfd, "System error!\n", strlen("System error!\n"));
        return RC_UNKNOW;
    }
    return RC_OK;
}

RC ManagerDropTable(XBParser *xbParser, char *tableName)
{
    if (xbParser->db == NULL)
    {
        write(xbParser->clientfd, "USE Database firstly.\n", strlen("USE Database firstly.\n"));
        return RC_UNKNOW;
    }

    Table *table = NULL;
    int found = 0;
    char prePath[FILENAME_MAX];
    strcpy(prePath, xbParser->db->dbName);
    strcat(prePath, "/");

    for (ListNode *i = xbParser->db->tableList->first; i != NULL; i = i->next)
    {
        table = i->data;
        if (strcmp(table->tableName, tableName) == 0)
        {
            char filename[FILENAME_MAX];
            strcpy(filename, prePath);
            strcat(filename, table->tableName);
            strcat(filename, ".data");
            if (RemoveFile(filename) != RC_OK)
            {
                write(xbParser->clientfd, "System error.\n", strlen("System error.\n"));
            }

            for (ListNode *j = table->fieldList->first; j != NULL; j = j->next)
            {
                Field *field = j->data;
                if (field->hasIndex)
                {
                    strcpy(filename, prePath);
                    strcat(filename, table->tableName);
                    strcat(filename, ".");
                    strcat(filename, field->fieldName);
                    strcat(filename, ".index");
                    RemoveFile(filename);
                }
            }
            break;
        }
    }
    if (table != NULL)
    {
        ListEraseData(xbParser->db->tableList, table);
        FreeTable(table);
        DatabaseInfoFlush(xbParser->db);
    }
    else
    {
        char msg[2048];
        sprintf(msg, "Not found table %s in database %s.\n", tableName, xbParser->db->dbName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }
    return RC_OK;
}

RC ManagerDropIndex(XBParser *xbParser, char *tableName, char *fieldName)
{
    if (xbParser->db == NULL)
    {
        write(xbParser->clientfd, "USE Database firstly.\n", strlen("USE Database firstly.\n"));
        return RC_UNKNOW;
    }

    int found = 0;
    char prePath[FILENAME_MAX];
    strcpy(prePath, xbParser->db->dbName);
    strcat(prePath, "/");

    for (ListNode *i = xbParser->db->tableList->first; i != NULL; i = i->next)
    {
        Table *table = i->data;
        if (strcmp(table->tableName, tableName) == 0)
        {
            for (ListNode *j = table->fieldList->first; j != NULL; j = j->next)
            {
                Field *field = j->data;
                if (field->hasIndex && strcmp(fieldName, field->fieldName) == 0)
                {
                    found = 1;
                    char filename[FILENAME_MAX];
                    strcpy(filename, prePath);
                    strcat(filename, table->tableName);
                    strcat(filename, ".");
                    strcat(filename, field->fieldName);
                    strcat(filename, ".index");
                    RemoveFile(filename);
                    field->hasIndex = 0;
                    DatabaseInfoFlush(xbParser->db);
                    break;
                }
            }
        }
    }
    if (!found)
    {
        char msg[2048];
        sprintf(msg, "Not found index %s(%s).\n", tableName, fieldName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }
    return RC_OK;
}

int UpdateTableQuery(SelectTable *sTable, Update *update)
{
    int udNum = 0;
    SelectField *useIndex = NULL;
    for (ListNode *i = sTable->sFieldList->first; i != NULL; i = i->next)
    {
        SelectField *sField = i->data;
        if (sField->op == TK_EQ && sField->hasIndex)
        {
            useIndex = sField;
            break;
        }
    }
    char prePath[FILENAME_MAX], tableDataFilename[FILENAME_MAX], preIndexFilename[FILENAME_MAX];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, sTable->dbName);
    strcat(prePath, "/");
    strcpy(tableDataFilename, prePath);
    strcat(tableDataFilename, sTable->tableName);
    strcpy(preIndexFilename, tableDataFilename);
    strcat(tableDataFilename, ".data");
    strcat(preIndexFilename, ".");
    if (useIndex == NULL)
    {
        char indexFilename[FILENAME_MAX];
        strcpy(indexFilename, preIndexFilename);
        strcat(indexFilename, "id.index");

        Bptree *tree = OpenBptree(indexFilename);

        for (int i = 0; i < sTable->table->dataNum; ++i)
        {
            List *list = BptreeSearch(tree, i);
            for (ListNode *j = list->first; j != NULL; j = j->next)
            {
                List *fdList = DataSeek(sTable->table, tableDataFilename, *((int *)j->data));
                FieldData *uData = NULL;
                int ok = 1;
                for (ListNode *fd = fdList->first; fd != NULL; fd = fd->next)
                {
                    if (!ok)
                    {
                        break;
                    }
                    FieldData *fData = fd->data;
                    for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                    {
                        SelectField *sField = k->data;
                        if (strcmp(fData->fieldName, update->fieldName) == 0 && strcmp(fData->fieldType, update->fieldType) == 0)
                        {
                            uData = fData;
                        }
                        if (strcmp(fData->fieldType, sField->fieldType) != 0 || strcmp(fData->fieldName, sField->fieldName) != 0)
                        {
                            continue;
                        }

                        if (sField->op == TK_SEMICOLON)
                        {
                            continue;
                        }
                        else if (sField->op == TK_EQ)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) != atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) != atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) != 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_NE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) == atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) == atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) == 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) > atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) > atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) > 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) >= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) >= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) >= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) < atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) < atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) < 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) <= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) <= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) <= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                    }
                }
                if (ok)
                {
                    udNum += 1;
                    memset(uData->value, 0, sizeof(uData->value));
                    strcpy(uData->value, update->newValue);
                    DataFlush(sTable->table, tableDataFilename, fdList, *((int *)j->data));
                }
                FreeList(fdList);
            }
            FreeList(list);
        }
        CloseBptree(tree);
    }
    else
    {
        char indexFilename[FILENAME_MAX];
        strcpy(indexFilename, preIndexFilename);
        strcat(indexFilename, useIndex->fieldName);
        strcat(indexFilename, ".index");

        Bptree *tree = OpenBptree(indexFilename);
        int key = atoi(useIndex->value);
        List *tmpList = BptreeSearch(tree, key);
        CloseBptree(tree);

        strcpy(indexFilename, preIndexFilename);
        strcat(indexFilename, "id.index");
        tree = OpenBptree(indexFilename);

        for (ListNode *i = tmpList->first; i != NULL; i = i->next)
        {
            List *list = BptreeSearch(tree, *((int *)i->data));
            for (ListNode *j = list->first; j != NULL; j = j->next)
            {
                List *fdList = DataSeek(sTable->table, tableDataFilename, *((int *)j->data));
                FieldData *uData = NULL;
                int ok = 1;
                for (ListNode *fd = fdList->first; fd != NULL; fd = fd->next)
                {
                    if (!ok)
                    {
                        break;
                    }
                    FieldData *fData = fd->data;
                    for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                    {
                        SelectField *sField = k->data;
                        if (strcmp(fData->fieldName, update->fieldName) == 0 && strcmp(fData->fieldType, update->fieldType) == 0)
                        {
                            uData = fData;
                        }
                        if (strcmp(fData->fieldType, sField->fieldType) != 0 || strcmp(fData->fieldName, sField->fieldName) != 0)
                        {
                            continue;
                        }

                        if (sField->op == TK_SEMICOLON)
                        {
                            continue;
                        }
                        else if (sField->op == TK_EQ)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) != atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) != atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) != 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_NE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) == atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) == atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) == 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) > atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) > atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) > 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) >= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) >= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) >= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) < atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) < atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) < 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) <= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) <= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) <= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                    }
                }
                if (ok)
                {
                    udNum += 1;
                    memset(uData->value, 0, sizeof(uData->value));
                    strcpy(uData->value, update->newValue);
                    DataFlush(sTable->table, tableDataFilename, fdList, *((int *)j->data));
                }
                FreeList(fdList);
            }
            FreeList(list);
        }
        CloseBptree(tree);
        FreeList(tmpList);
    }
    return udNum;
}

RC ManagerUpdate(XBParser *xbParser, Update *update, List *conditionList)
{
    if (xbParser->db == NULL)
    {
        write(xbParser->clientfd, "USE Database firstly.\n", strlen("USE Database firstly.\n"));
        return RC_UNKNOW;
    }

    SelectTable *sTable = NULL;
    for (ListNode *i = xbParser->db->tableList->first; i != NULL; i = i->next)
    {
        Table *table = i->data;
        if (strcmp(update->tableName, table->tableName) == 0)
        {
            sTable = (SelectTable *)xpfMalloc(sizeof(SelectTable));
            sTable->sFieldList = NewList();
            sTable->sFieldDataList = NewList();
            sTable->table = table;
            strcpy(sTable->tableName, table->tableName);
            strcpy(sTable->dbName, xbParser->db->dbName);

            for (ListNode *j = table->fieldList->first; j != NULL; j = j->next)
            {
                Field *field = j->data;
                SelectField *sField = (SelectField *)xpfMalloc(sizeof(SelectField));
                strcpy(sField->fieldName, field->fieldName);
                strcpy(sField->fieldType, field->fieldType);
                sField->hasIndex = field->hasIndex;
                sField->op = TK_SEMICOLON;
                sField->sort = 0;
                sField->dataList = NewList();
                strcpy(sField->value, "");
                for (ListNode *k = conditionList->first; k != NULL; k = k->next)
                {
                    Condition *condition = k->data;
                    if (strlen(condition->leftValue) == 0 && strlen(condition->rightValue) == 0)
                    {
                        write(xbParser->clientfd, "Does not support such conditions.\n", strlen("Does not support such conditions.\n"));
                        return RC_UNKNOW;
                    }
                    if (strlen(condition->leftValue) != 0)
                    {
                        if (strcmp(condition->rightField, sField->fieldName) == 0 && (strlen(condition->rightTable) == 0 || strcmp(condition->rightTable, sTable->tableName) == 0))
                        {
                            strcpy(sField->value, condition->leftValue);
                            switch (condition->op)
                            {
                            case TK_EQ:
                            case TK_NE:
                            {
                                sField->op = condition->op;
                                break;
                            }
                            case TK_LE:
                            {
                                sField->op = TK_GT;
                                break;
                            }
                            case TK_LT:
                            {
                                sField->op = TK_GE;
                                break;
                            }
                            case TK_GT:
                            {
                                sField->op = TK_LE;
                                break;
                            }
                            case TK_GE:
                            {
                                sField->op = TK_LT;
                                break;
                            }
                            default:
                            {
                                break;
                            }
                            }
                        }
                    }
                    else
                    {
                        if (strcmp(condition->leftField, sField->fieldName) == 0 && (strlen(condition->leftTable) == 0 || strcmp(condition->leftTable, sTable->tableName) == 0))
                        {
                            strcpy(sField->value, condition->rightValue);
                            sField->op = condition->op;
                        }
                    }
                }
                ListAppend(sTable->sFieldList, uNewListNode(sField));
            }
            break;
        }
    }
    if (sTable == NULL)
    {
        char msg[2048];
        sprintf(msg, "Not found table %s.\n", update->tableName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }

    char msg[2048];
    int udNum = UpdateTableQuery(sTable, update);
    sprintf(msg, "%d items successfully updated.\n", udNum);
    write(xbParser->clientfd, msg, strlen(msg));
    return RC_OK;
}

int DeleteTableQuery(SelectTable *sTable)
{
    int delNum = 0;
    SelectField *useIndex = NULL;
    for (ListNode *i = sTable->sFieldList->first; i != NULL; i = i->next)
    {
        SelectField *sField = i->data;
        if (sField->op == TK_EQ && sField->hasIndex)
        {
            useIndex = sField;
            break;
        }
    }
    char prePath[FILENAME_MAX], tableDataFilename[FILENAME_MAX], preIndexFilename[FILENAME_MAX];
    memset(prePath, 0, sizeof(prePath));
    strcpy(prePath, sTable->dbName);
    strcat(prePath, "/");
    strcpy(tableDataFilename, prePath);
    strcat(tableDataFilename, sTable->tableName);
    strcpy(preIndexFilename, tableDataFilename);
    strcat(tableDataFilename, ".data");
    strcat(preIndexFilename, ".");
    if (useIndex == NULL)
    {
        char indexFilename[FILENAME_MAX];
        strcpy(indexFilename, preIndexFilename);
        strcat(indexFilename, "id.index");

        Bptree *tree = OpenBptree(indexFilename);

        for (int i = 0; i < sTable->table->dataNum; ++i)
        {
            List *list = BptreeSearch(tree, i);
            for (ListNode *j = list->first; j != NULL; j = j->next)
            {
                List *fdList = DataSeek(sTable->table, tableDataFilename, *((int *)j->data));
                int ok = 1;
                for (ListNode *fd = fdList->first; fd != NULL; fd = fd->next)
                {
                    if (!ok)
                    {
                        break;
                    }
                    FieldData *fData = fd->data;
                    for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                    {
                        SelectField *sField = k->data;
                        if (strcmp(fData->fieldType, sField->fieldType) != 0 || strcmp(fData->fieldName, sField->fieldName) != 0)
                        {
                            continue;
                        }

                        if (sField->op == TK_SEMICOLON)
                        {
                            continue;
                        }
                        else if (sField->op == TK_EQ)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) != atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) != atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) != 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_NE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) == atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) == atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) == 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) > atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) > atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) > 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) >= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) >= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) >= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) < atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) < atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) < 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) <= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) <= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) <= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                    }
                }
                if (ok)
                {
                    delNum += BptreeDelete(tree, *((int *)j->data));
                }
                FreeList(fdList);
            }
            FreeList(list);
        }
        CloseBptree(tree);
    }
    else
    {
        char indexFilename[FILENAME_MAX];
        strcpy(indexFilename, preIndexFilename);
        strcat(indexFilename, useIndex->fieldName);
        strcat(indexFilename, ".index");

        Bptree *tree = OpenBptree(indexFilename);
        int key = atoi(useIndex->value);
        List *tmpList = BptreeSearch(tree, key);
        CloseBptree(tree);

        strcpy(indexFilename, preIndexFilename);
        strcat(indexFilename, "id.index");
        tree = OpenBptree(indexFilename);

        for (ListNode *i = tmpList->first; i != NULL; i = i->next)
        {
            List *list = BptreeSearch(tree, *((int *)i->data));
            for (ListNode *j = list->first; j != NULL; j = j->next)
            {
                List *fdList = DataSeek(sTable->table, tableDataFilename, *((int *)j->data));
                int ok = 1;
                for (ListNode *fd = fdList->first; fd != NULL; fd = fd->next)
                {
                    if (!ok)
                    {
                        break;
                    }
                    FieldData *fData = fd->data;
                    for (ListNode *k = sTable->sFieldList->first; k != NULL; k = k->next)
                    {
                        SelectField *sField = k->data;
                        if (strcmp(fData->fieldType, sField->fieldType) != 0 || strcmp(fData->fieldName, sField->fieldName) != 0)
                        {
                            continue;
                        }

                        if (sField->op == TK_SEMICOLON)
                        {
                            continue;
                        }
                        else if (sField->op == TK_EQ)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) != atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) != atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) != 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_NE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) == atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) == atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) == 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) > atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) > atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) > 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_LT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) >= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) >= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) >= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GE)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) < atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) < atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) < 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                        else if (sField->op == TK_GT)
                        {
                            if (fData->fieldType[0] == 'I')
                            {
                                if (atoi(fData->value) <= atoi(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else if (fData->fieldType[0] == 'F')
                            {
                                if (atof(fData->value) <= atof(sField->value))
                                {
                                    ok = 0;
                                }
                            }
                            else
                            {
                                if (strcmp(fData->value, sField->value) <= 0)
                                {
                                    ok = 0;
                                }
                            }
                        }
                    }
                }
                if (ok)
                {
                    delNum += BptreeDelete(tree, *((int *)j->data));
                }
                FreeList(fdList);
            }
            FreeList(list);
        }
        CloseBptree(tree);
        FreeList(tmpList);
    }
    return delNum;
}

RC ManagerDelete(XBParser *xbParser, char *tableName, List *conditionList)
{
    if (xbParser->db == NULL)
    {
        write(xbParser->clientfd, "USE Database firstly.\n", strlen("USE Database firstly.\n"));
        return RC_UNKNOW;
    }

    SelectTable *sTable = NULL;
    for (ListNode *i = xbParser->db->tableList->first; i != NULL; i = i->next)
    {
        Table *table = i->data;
        if (strcmp(tableName, table->tableName) == 0)
        {
            sTable = (SelectTable *)xpfMalloc(sizeof(SelectTable));
            sTable->sFieldList = NewList();
            sTable->sFieldDataList = NewList();
            sTable->table = table;
            strcpy(sTable->tableName, table->tableName);
            strcpy(sTable->dbName, xbParser->db->dbName);

            for (ListNode *j = table->fieldList->first; j != NULL; j = j->next)
            {
                Field *field = j->data;
                SelectField *sField = (SelectField *)xpfMalloc(sizeof(SelectField));
                strcpy(sField->fieldName, field->fieldName);
                strcpy(sField->fieldType, field->fieldType);
                sField->hasIndex = field->hasIndex;
                sField->op = TK_SEMICOLON;
                sField->sort = 0;
                sField->dataList = NewList();
                strcpy(sField->value, "");
                for (ListNode *k = conditionList->first; k != NULL; k = k->next)
                {
                    Condition *condition = k->data;
                    if (strlen(condition->leftValue) == 0 && strlen(condition->rightValue) == 0)
                    {
                        write(xbParser->clientfd, "Does not support such conditions.\n", strlen("Does not support such conditions.\n"));
                        return RC_UNKNOW;
                    }
                    if (strlen(condition->leftValue) != 0)
                    {
                        if (strcmp(condition->rightField, sField->fieldName) == 0 && (strlen(condition->rightTable) == 0 || strcmp(condition->rightTable, sTable->tableName) == 0))
                        {
                            strcpy(sField->value, condition->leftValue);
                            switch (condition->op)
                            {
                            case TK_EQ:
                            case TK_NE:
                            {
                                sField->op = condition->op;
                                break;
                            }
                            case TK_LE:
                            {
                                sField->op = TK_GT;
                                break;
                            }
                            case TK_LT:
                            {
                                sField->op = TK_GE;
                                break;
                            }
                            case TK_GT:
                            {
                                sField->op = TK_LE;
                                break;
                            }
                            case TK_GE:
                            {
                                sField->op = TK_LT;
                                break;
                            }
                            default:
                            {
                                break;
                            }
                            }
                        }
                    }
                    else
                    {
                        if (strcmp(condition->leftField, sField->fieldName) == 0 && (strlen(condition->leftTable) == 0 || strcmp(condition->leftTable, sTable->tableName) == 0))
                        {
                            strcpy(sField->value, condition->rightValue);
                            sField->op = condition->op;
                        }
                    }
                }
                ListAppend(sTable->sFieldList, uNewListNode(sField));
            }
            break;
        }
    }
    if (sTable == NULL)
    {
        char msg[2048];
        sprintf(msg, "Not found table %s.\n", tableName);
        write(xbParser->clientfd, msg, strlen(msg));
        return RC_UNKNOW;
    }
    char msg[2048];
    int delNum = DeleteTableQuery(sTable);
    sprintf(msg, "%d items successfully deleted.\n", delNum);
    write(xbParser->clientfd, msg, strlen(msg));
    return RC_OK;
}

RC ManagerShow(XBParser *xbParser, char *name)
{
    if (xbParser->db == NULL)
    {
        write(xbParser->clientfd, "USE Database firstly.\n", strlen("USE Database firstly.\n"));
        return RC_UNKNOW;
    }

    char msg[4096];
    if (StringCompareIC(name, strlen(name), "TABLES", strlen("TABLES")) == 0)
    {
        for (ListNode *i = xbParser->db->tableList->first; i != NULL; i = i->next)
        {
            Table *table = i->data;
            sprintf(msg, "%s\n", table->tableName);
            write(xbParser->clientfd, msg, strlen(msg));
        }
    }
    else
    {
        int found = 0;
        for (ListNode *i = xbParser->db->tableList->first; i != NULL; i = i->next)
        {
            Table *table = i->data;
            if (strcmp(name, table->tableName) == 0)
            {
                found = 1;
                sprintf(msg, "Field\tType\tHas Index\t\n");
                write(xbParser->clientfd, msg, strlen(msg));
                for (ListNode *j = table->fieldList->first; j != NULL; j = j->next)
                {
                    Field *field = j->data;
                    sprintf(msg, "%s\t%s\t%d\t\n", field->fieldName, field->fieldType, field->hasIndex);
                    write(xbParser->clientfd, msg, strlen(msg));
                }
            }
        }
        if (!found)
        {
            sprintf(msg, "Not found table %s.\n", name);
            write(xbParser->clientfd, msg, strlen(msg));
            return RC_UNKNOW;
        }
    }
    return RC_OK;
}