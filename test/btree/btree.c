#include <string.h>
#include <stdlib.h>
#include "utils.h"
#include "btree.h"

BtreeNode *NewBtreeNode()
{
    BtreeNode *node = (BtreeNode *)xpfMalloc(sizeof(BtreeNode));
    memset(node, 0, sizeof(BtreeNode));
    for (int i = 0; i < BTREE_M + 1; ++i)
    {
        node->key[i] = NULL;
        node->isDelete[i] = 0;
        node->data[i] = NULL;
        node->next[i] = NULL;
    }
    node->next[BTREE_M + 1] = NULL;
    return node;
}

Btree *NewBtree(int (*comparer)(const void *key1, const void *key2))
{
    Btree *btree = (Btree *)xpfMalloc(sizeof(Btree));
    btree->comparer = comparer;
    btree->root = NewBtreeNode();
    btree->size = 1;
    return btree;
}

DataList *NewDataList()
{
    DataList *dataList = (DataList *)xpfMalloc(sizeof(DataList));
    dataList->size = 0;
    dataList->first = NULL;
    dataList->last = NULL;
    return dataList;
}

DataListNode *NewDataListNode(void *data, DataListNode *next)
{
    DataListNode *dataListNode = (DataListNode *)xpfMalloc(sizeof(DataListNode));
    dataListNode->next = next;
    dataListNode->data = data;
    return dataListNode;
}

int DataListAppend(DataList *dataList, DataListNode *dataListNode)
{
    if (dataList->size == 0)
    {
        dataList->last = dataListNode;
        dataList->first = dataListNode;
    }
    else
    {
        dataList->last->next = dataListNode;
        dataList->last = dataListNode;
    }
    dataList->size++;
    return dataList->size;
}

int DataListPrepend(DataListNode *dataListNode, DataList *dataList)
{
    dataListNode->next = dataList->first;
    dataList->first = dataListNode;
    dataList->size++;
    return dataList->size;
}

DataList *DataListMerge(DataList *l1, DataList *l2)
{
    if (l1->size == 0)
    {
        l1->first = l2->first;
        l1->last = l2->last;
        l1->size = l2->size;
        return l1;
    }
    if (l2->size > 0)
    {
        l1->last->next = l2->first;
        l1->last = l2->last;
        l1->size += l2->size;
        return l1;
    }
    return l1;
}

int SearchInNode(Btree *btree, BtreeNode *btreeNode, void *key, int *cp)
{
    int newIndex = 0;
    while (newIndex < btreeNode->size && ((cp = btree->comparer(btreeNode->key[newIndex], key)) >= 0))
    {
        ++newIndex;
    }
    return newIndex;
}

void *InsertIntoNode(Btree *btree, BtreeNode *btreeNode, void *key, void *data, int isDelete, BtreeNode *left, BtreeNode *right)
{
    BtreeNode *currNode = btreeNode;
    int newIndex = 0, cp;
    if (currNode != NULL)
    {
        newIndex = 0;
        while (newIndex < currNode->size && (cp = btree->comparer(currNode->key[newIndex], key) <= 0))
        {
            ++newIndex;
        }
        currNode->next[currNode->size + 1] = currNode->next[currNode->size];
        for (int i = currNode->size; i > newIndex; --i)
        {
            currNode->key[i] = currNode->key[i - 1];
            currNode->data[i] = currNode->data[i - 1];
            currNode->next[i] = currNode->next[i - 1];
            currNode->isDelete[i] = currNode->isDelete[i - 1];
        }
        currNode->key[newIndex] = key;
        currNode->data[newIndex] = data;
        currNode->isDelete[newIndex] = 0;
        currNode->next[newIndex] = left;
        // if (currNode == left)
        // {
        //     printf("==\n");
        // }
        if (left != NULL)
            left->parent = currNode;
        if (right != NULL)
        {
            currNode->next[newIndex + 1] = right;
            right->parent = currNode;
        }
        currNode->size++;
        if (currNode->size == BTREE_M)
        {
            return BtreeSplite(btree, currNode);
        }
    }
    return NULL;
}

void *BtreeSplite(Btree *btree, BtreeNode *btreeNode)
{
    BtreeNode *curNode = btreeNode;
    BtreeNode *parent = curNode->parent;
    BtreeNode *newRightNode = NewBtreeNode();
    int midIndex = BTREE_M / 2;
    // while (midIndex > 0 && btree->comparer(curNode->key[midIndex], curNode->key[midIndex - 1]) == 0)
    //     --midIndex;
    void *midKey = curNode->key[midIndex];
    void *midData = curNode->data[midIndex];
    void *midNext = curNode->next[midIndex];
    int midIsDelete = curNode->isDelete[midIndex];

    for (int i = midIndex + 1, j = 0; i < BTREE_M; ++i, ++j)
    {
        newRightNode->key[j] = curNode->key[i];
        newRightNode->data[j] = curNode->data[i];
        newRightNode->next[j] = curNode->next[i];
        newRightNode->isDelete[j] = curNode->isDelete[i];
        curNode->key[i] = NULL;
        curNode->data[i] = NULL;
        curNode->next[i] = NULL;
        curNode->isDelete[i] = 0;
    }
    curNode->next[midIndex] = NULL;
    curNode->size = midIndex;
    newRightNode->size = BTREE_M - midIndex - 1;
    printf("size = %d\n", curNode->size + newRightNode->size);

    // printf("New left node:\n");
    // for (int i = 0; i < curNode->size; ++i)
    // {
    //     printf("key: %s, data: %s, next: %d\n", (char *)curNode->key[i], (char *)curNode->data[i], curNode->next[i]);
    // }

    // printf("New right node:\n");
    // for (int i = 0; i < newRightNode->size; ++i)
    // {
    //     printf("key: %s, data: %s, next: %d\n", (char *)newRightNode->key[i], (char *)newRightNode->data[i], newRightNode->next[i]);
    // }

    if (parent == NULL) /*New root.*/
    {
        BtreeNode *newRoot = NewBtreeNode();
        newRoot->key[0] = midKey;
        newRoot->data[0] = midData;
        newRoot->isDelete[0] = midIsDelete;
        newRoot->next[0] = curNode;
        newRoot->next[1] = newRightNode;
        curNode->parent = newRoot;
        newRightNode->parent = newRoot;
        newRoot->size = 1;
        btree->root = newRoot;
        btree->size += 2;
    }
    else
    {
        btree->size += 1;
        // printf("%ld, %ld\n", parent, curNode);
        if (curNode == parent)
        {
            printf("==\n");
        }
        if (newRightNode == parent)
        {
            printf("==\n");
        }
        return InsertIntoNode(btree, parent, midKey, midData, midIsDelete, curNode, newRightNode);
    }
    return NULL;
}

void *BtreeInsert(Btree *btree, BtreeNode *btreeNode, void *key, void *data, int isDelete, BtreeNode *left, BtreeNode *right)
{
    BtreeNode *currNode = btreeNode;
    int newIndex = 0, cp;
    if (currNode != NULL)
    {
        newIndex = 0;
        while (newIndex < currNode->size && (cp = btree->comparer(currNode->key[newIndex], key) <= 0))
        {
            ++newIndex;
        }
        {
            if (currNode->next[newIndex] == NULL)
            {
                currNode->next[currNode->size + 1] = currNode->next[currNode->size];
                for (int i = currNode->size; i > newIndex; --i)
                {
                    currNode->key[i] = currNode->key[i - 1];
                    currNode->data[i] = currNode->data[i - 1];
                    currNode->next[i] = currNode->next[i - 1];
                    currNode->isDelete[i] = currNode->isDelete[i - 1];
                }
                currNode->key[newIndex] = key;
                currNode->data[newIndex] = data;
                currNode->isDelete[newIndex] = 0;
                currNode->next[newIndex] = left;
                // if (currNode == left)
                // {
                //     printf("==\n");
                // }
                if (left != NULL)
                    left->parent = currNode;
                if (right != NULL)
                {
                    currNode->next[newIndex + 1] = right;
                    right->parent = currNode;
                }
                currNode->size++;
                if (currNode->size == BTREE_M)
                {
                    return BtreeSplite(btree, currNode);
                }
            }
            else
            {
                currNode = currNode->next[newIndex];
                // if (currNode == left)
                // {
                //     printf("==\n");
                // }
                return BtreeInsert(btree, currNode, key, data, isDelete, left, right);
            }
        }
    }
    return NULL;
}

void *BtreeMarkDelete(Btree *btree, BtreeNode *btreeNode, void *key)
{
    if (btreeNode != NULL)
    {
        int index = 0, cp;
        while (index < btreeNode->size && ((cp = btree->comparer(btreeNode->key[index], key)) < 0))
        {
            ++index;
        }
        BtreeMarkDelete(btree, btreeNode->next[index], key);
        for (int i = index; i < btreeNode->size && btree->comparer(btreeNode->key[i], key) == 0; ++i)
        {
            btreeNode->isDelete[i] = 1;
            BtreeMarkDelete(btree, btreeNode->next[i + 1], key);
        }
    }
    return NULL;
}

DataList *BtreeSearch(Btree *btree, BtreeNode *btreeNode, void *key)
{
    DataList *dataList = NewDataList();
    DataList *subList = NULL;
    if (btreeNode != NULL)
    {
        int index = 0, cp;
        while (index < btreeNode->size && ((cp = btree->comparer(btreeNode->key[index], key)) < 0))
        {
            ++index;
        }
        subList = BtreeSearch(btree, btreeNode->next[index], key);
        printf("i = %d, subList->size = %d, index = %d, btreeNode->size = %d\n", -1, subList->size, index, btreeNode->size);
        DataListMerge(dataList, subList);
        for (int i = index; i < btreeNode->size && btree->comparer(btreeNode->key[i], key) == 0; ++i)
        {
            if (btreeNode->isDelete[i] == 0)
                DataListAppend(dataList, NewDataListNode(btreeNode->data[i], NULL));
            subList = BtreeSearch(btree, btreeNode->next[i + 1], key);
            if (btree->root == btreeNode)
            {
                printf("i = %d, subList->size = %d\n", i, subList->size);
            }
            DataListMerge(dataList, subList);
        }
    }
    return dataList;
}