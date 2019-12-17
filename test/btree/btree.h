#ifndef BTREE_H
#define BTREE_H

#define BTREE_M 64

typedef struct DataListNode
{
    void *data;
    struct DataListNode *next;
} DataListNode;

typedef struct DataList
{
    int size;
    struct DataListNode *first;
    struct DataListNode *last;
} DataList;

typedef struct BtreeNode
{
    int size;
    void *data[BTREE_M + 1];             /**/
    void *key[BTREE_M + 1];              /**/
    int isDelete[BTREE_M + 1];           /* True if had deleted. */
    struct BtreeNode *next[BTREE_M + 2]; /**/
    struct BtreeNode *parent;            /**/
} BtreeNode;

typedef struct Btree
{
    int size;
    BtreeNode *root;
    int (*comparer)(const void *key1, const void *key2);
} Btree;

void *BtreeInsert(Btree *btree, BtreeNode *btreeNode, void *key, void *data, int isDelete, BtreeNode *left, BtreeNode *right);
Btree *NewBtree(int (*comparer)(const void *key1, const void *key2));
DataList *BtreeSearch(Btree *btree, BtreeNode *btreeNode, void *key);
void *BtreeMarkDelete(Btree *btree, BtreeNode *btreeNode, void *key);
void *BtreeSplite(Btree *btree, BtreeNode *btreeNode);

DataList *NewDataList();
DataListNode *NewDataListNode(void *data, DataListNode *next);
int DataListAppend(DataList *dataList, DataListNode *dataListNode);
int DataListPrepend(DataListNode *dataListNode, DataList *dataList);
DataList *DataListMerge(DataList *l1, DataList *l2);

#endif