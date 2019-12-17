#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

typedef enum
{
    XB_AF_NO,
    XB_AF_MIN,
    XB_AF_COUNT,
    XB_AF_SUM,
    XB_AF_AVG,
    XB_AF_MAX,
} AggFun;

typedef enum
{
    OP_NO,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE
} Operator;

typedef enum
{
    NK_NONE,
    NK_DATABASE,
    NK_CREATEDATABASE,
    NK_CREATETABLE,
    NK_CREATEINDEX,
    NK_DROPTABLE,
    NK_DROPINDEX,
    NK_LOAD,
    NK_SET,
    NK_HELP,
    NK_PRINT,
    NK_QUERY,
    NK_INSERT,
    NK_DELETE,
    NK_UPDATE,
    NK_RELATTR,
    NK_CONDITION,
    NK_RELATTR_OR_VALUE,
    NK_ATTRTYPE,
    NK_VALUE,
    NK_RELATION,
    NK_STATISTICS,
    NK_LIST,
    NK_DISTRIBUTE,
    NK_TABLE,
} NodeKind;

typedef struct Node
{
    NodeKind nodeKind;

    union {
        /* System Manage component nodes */
        /* Create table node */
        struct
        {
            char *tableName;
            struct Node *attrTypeList;
        } CreateTable;

        /* Create index node */
        struct
        {
            char *tableName;
            char *attrname;
        } CreateIndex;

        /* Drop index node */
        struct
        {
            char *tableName;
            char *attrName;
        } DropIndex;

        /* Drop table node */
        struct
        {
            char *tableName;
        } DropTable;

        /* Load node */
        struct
        {
            char *tableName;
            char *filename;
        } Load;

        /* Set node */
        struct
        {
            char *paramName;
            char *string;
        } Set;

        /* Help node */
        struct
        {
            char *tableName;
        } Help;

        /* Print node */
        struct
        {
            char *tableName;
        } Print;

        /* Query node */
        struct
        {
            struct Node *tableAttrList;
            struct Node *tableList;
            struct Node *conditionList;
            struct Node *orderTableAttr;
            struct Node *groupTableattr;
        } Query;

        /* Insert node */
        struct
        {
            char *tableName;
            struct Node *attrList;
            struct Node *valueList;
        } Insert;

        /* Delete node */
        struct
        {
            char *tableName;
            struct Node *conditionList;
        } Delete;

        /* Update node */
        struct
        {
            char *tableName;
            struct Node *tableAttr;
            struct Node *tableValue;
            struct Node *conditionList;
        } Update;

        /* Command support nodes */
        /* Aggregation functions for table attribute node */
        struct
        {
            AggFun func;
            char *tableName;
            char *attrName;
        } AggTableAttr;

        /* Condition node */
        struct
        {
            struct Node *leftTableAttr;
            Operator op;
            struct Node *rightTableAttr;
            struct Node *rightValue;
        } Condition;

        /* Attribution node */
        struct
        {
            char *attrName;
            char *type;
            char *val;
            char isIndex;
        } Attr;

        /* Database node */
        struct
        {
            char *dbName;
            struct Node *tableList;
        } Database;

        /* Table node */
        struct
        {
            char *TableName;
            struct Node *attrList;
        } Table;

        /* List node */
        struct
        {
            struct Node *curr;
            struct Node *next;
            int numChilds; /* The number of childs. */
        } List;
    } u;
} Node;

Node *NewNode(NodeKind nodeKind);
Node *Prepend(Node *node, Node *list);
Node *NewListNode(Node *node);
Node *NewCreateTableNode(char *tableName, Node *attrTypeList);
Node *NewAttrNode(char *attrName, char *type, char *val, char isIndex);
Node *NewInsertNode(char *tableName, Node *attrList, Node *valueList);
Node *NewTableNode(char *tableName, Node *attrList);
Node *NewDatabaseNode(char *dbName, Node *tableList);
Node *NewValueNode(char *type, char *val);
Node *NewCreateIndexNode(char *tableName, char *attrName);

#endif