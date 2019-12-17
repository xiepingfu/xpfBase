#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "bptree.h"

#define DEBUG 0
#define NODE_SIZE ((unsigned long)4096)
#define NODE_EOF -1

typedef struct BptreeNode
{
    int self;
    int parent;
    char isLeaf;
    int size;
    int key[BPTREE_M];
    int dat[BPTREE_M + 1]; /* If isLeaf is true, the next point to data. Otherwise it point to node. */
    char del[BPTREE_M];
} BptreeNode;

BptreeNode *NewBptreeNode(Bptree *tree)
{
    tree->size += 1;
    BptreeNode *node = (BptreeNode *)malloc(NODE_SIZE);
    node->isLeaf = 0;
    node->parent = NODE_EOF;
    node->self = tree->size;
    node->size = 0;
    memset(node->del, 0, sizeof(node->del));
    for (int i = 0; i < BPTREE_M; ++i)
    {
        node->dat[i] = NODE_EOF;
    }
    return node;
}

BptreeNode *NodeSeek(Bptree *tree, int nodeSeek)
{
    if (nodeSeek == NODE_EOF)
    {
        return NULL;
    }

    BptreeNode *node = (BptreeNode *)malloc(NODE_SIZE);
    fseek(tree->file, NODE_SIZE * (nodeSeek), SEEK_SET);
    int n;
    if ((n = fread(node, NODE_SIZE, 1, tree->file)) != 1)
    {
        printf("Error!\n");
        return NULL;
    }
    return node;
}

void NodeFlush(Bptree *tree, BptreeNode *node)
{
    fseek(tree->file, NODE_SIZE * node->self, SEEK_SET);
    int n = fwrite(node, NODE_SIZE, 1, tree->file);
}

void BptreeFlush(Bptree *tree)
{
    rewind(tree->file);
    fwrite(tree, NODE_SIZE, 1, tree->file);
}

Bptree *CreateBptree(char *filename)
{
    Bptree *tree = (Bptree *)malloc(NODE_SIZE);
    tree->size = 0;
    tree->file = NULL;
    strcpy(tree->filename, filename);
    tree->root = NODE_EOF;

    FILE *file = fopen(tree->filename, "w");
    rewind(file);
    fwrite(tree, NODE_SIZE, 1, file);
    fclose(file);
    tree->file = fopen(tree->filename, "r+");
    return tree;
}

Bptree *OpenBptree(char *filename)
{
    Bptree *tree = (Bptree *)malloc(NODE_SIZE);
    FILE *file = fopen(filename, "r");
    rewind(file);
    fread(tree, NODE_SIZE, 1, file);
    fclose(file);

    tree->file = fopen(tree->filename, "r+");

    return tree;
}

void CloseBptree(Bptree *tree)
{
    BptreeFlush(tree);
    fclose(tree->file);
    free(tree);
}

/* Return the index that node->key[index] greater than key firstly. */
int GetIndex(BptreeNode *node, int key)
{
    int l = 0, r = node->size;
    int m;
    while (l < r)
    {
        m = l + (r - l) / 2;
        if (node->key[m] <= key)
        {
            l = m + 1;
        }
        else
        {
            r = m;
        }
    }
    return l;
}

void InsertIntoNode(Bptree *tree, int nodeOffset, BptreeNode *left, BptreeNode *right, int key)
{
    if (nodeOffset == NODE_EOF)
    {
        /* New Root */
        BptreeNode *root = NewBptreeNode(tree);
        root->isLeaf = 0;
        root->parent = NODE_EOF;
        root->size = 1;
        root->key[0] = key;
        root->dat[0] = left->self;
        root->dat[1] = right->self;
        tree->root = root->self;
        left->parent = root->self;
        right->parent = root->self;
        NodeFlush(tree, root);
        free(root);
    }
    else
    {
        BptreeNode *node = NodeSeek(tree, nodeOffset);

        int index = GetIndex(node, key);
        node->dat[node->size + 1] = node->dat[node->size];
        for (int i = node->size; i > index; --i)
        {
            node->key[i] = node->key[i - 1];
            node->dat[i] = node->dat[i - 1];
            node->del[i] = node->del[i - 1];
        }
        node->key[index] = key;
        node->del[index] = 0;
        node->dat[index] = left->self;
        node->dat[index + 1] = right->self;
        node->size += 1;

        if (node->size == BPTREE_M)
        {
            int mid = (node->size + 1) / 2;
            BptreeNode *newRight = NewBptreeNode(tree);
            newRight->isLeaf = 0;
            newRight->parent = node->parent;
            for (int i = mid; i < node->size; ++i, ++(newRight->size))
            {
                newRight->key[newRight->size] = node->key[i];
                newRight->dat[newRight->size] = node->dat[i];
                newRight->del[newRight->size] = node->del[i];
                if (node->dat[i] == left->self)
                {
                    left->parent = newRight->self;
                }
                if (node->dat[i] == right->self)
                {
                    right->parent = newRight->self;
                }
            }
            newRight->dat[newRight->size] = node->dat[node->size];
            if (node->dat[node->size] == left->self)
            {
                left->parent = newRight->self;
            }
            if (node->dat[node->size] == right->self)
            {
                right->parent = newRight->self;
            }
            node->size = mid - 1;

            InsertIntoNode(tree, node->parent, node, newRight, newRight->key[0]);

            /* Update parent */
            BptreeNode *tmp = NULL;
            for (int i = 0; i <= newRight->size; ++i)
            {
                tmp = NodeSeek(tree, newRight->dat[i]);
                if (tmp != NULL)
                {
                    tmp->parent = newRight->self;
                    NodeFlush(tree, tmp);
                    free(tmp);
                }
            }
            NodeFlush(tree, node);
            NodeFlush(tree, newRight);
            free(newRight);
        }
        NodeFlush(tree, node);
        free(node);
    }
}

void InsertIntoLeafNode(Bptree *tree, BptreeNode *node, int key, int data)
{
    int index = GetIndex(node, key);
    for (int i = node->size; i > index; --i)
    {
        node->key[i] = node->key[i - 1];
        node->dat[i] = node->dat[i - 1];
        node->del[i] = node->del[i - 1];
    }
    node->key[index] = key;
    node->dat[index] = data;
    node->del[index] = 0;
    node->size += 1;

    if (node->size == BPTREE_M)
    {
        /* full */
        int mid = (node->size + 1) / 2;
        BptreeNode *right = NewBptreeNode(tree);
        right->isLeaf = 1;
        right->parent = node->parent;
        for (int i = mid; i < node->size; ++i, ++(right->size))
        {
            right->key[right->size] = node->key[i];
            right->dat[right->size] = node->dat[i];
            right->del[right->size] = node->del[i];
        }
        node->size = mid;
        InsertIntoNode(tree, node->parent, node, right, right->key[0]);
        NodeFlush(tree, node);
        NodeFlush(tree, right);
    }
    NodeFlush(tree, node);
    BptreeFlush(tree);
}

void BptreeInsert(Bptree *tree, int key, int data)
{
    BptreeNode *node = NodeSeek(tree, tree->root);
    while (node != NULL)
    {
        // DisplayNode(node);
        if (node->isLeaf)
        {
            return InsertIntoLeafNode(tree, node, key, data);
        }
        else
        {
            int index = GetIndex(node, key);
            if (index > node->size)
            {
                printf("Error!\n");
                return;
            }
            BptreeNode *tmp = node;
            node = NodeSeek(tree, node->dat[index]);
            free(tmp);
        }
    }

    /* New root */
    BptreeNode *root = NewBptreeNode(tree);
    root->isLeaf = 1;
    root->parent = NODE_EOF;
    root->size = 1;
    root->key[0] = key;
    root->dat[0] = data;
    tree->root = root->self;
    NodeFlush(tree, root);
    free(root);
    BptreeFlush(tree);
}

int BptreeSearch(Bptree *tree, int key)
{
    BptreeNode *node = NodeSeek(tree, tree->root);
    while (node != NULL)
    {
        // DisplayNode(node);
        int index = GetIndex(node, key);
        if (node->isLeaf)
        {
            for (int i = index; i >= 0; --i)
            {
                if (node->key[i] == key && !node->del[i])
                {
                    /* Found one */
                    return node->dat[i];
                }
            }
            break;
        }
        else if (index <= node->size)
        {
            BptreeNode *tmp = node;
            node = NodeSeek(tree, node->dat[index]);
            free(tmp);
        }
        else
        {
            break;
        }
    }
    printf("Not found key %d\n", key);
    return -1;
}

int BptreeDelete(Bptree *tree, int key)
{
    int delNum = 0;
    BptreeNode *node = NodeSeek(tree, tree->root);
    while (node != NULL)
    {
        int index = GetIndex(node, key);
        if (node->isLeaf)
        {
            for (int i = index; i >= 0 && node->key[i] >= key; --i)
            {
                if (node->key[i] == key && !node->del[i])
                {
                    /* Mark it as deleted. */
                    delNum += 1;
                    node->del[i] = 1;
                }
            }
            NodeFlush(tree, node);
            break;
        }
        else if (index <= node->size)
        {
            BptreeNode *tmp = node;
            node = NodeSeek(tree, node->dat[index]);
            free(tmp);
        }
        else
        {
            break;
        }
    }
    return delNum;
}

#if DEBUG == 1

void DisplayNode(Node *node)
{
    printf("============================================\n");
    printf("self: %d, ", node->self);
    printf("isLeaf: %d, ", node->isLeaf);
    printf("size: %d\n", node->size);
    for (int i = 0; i < node->size; ++i)
    {
        printf("key: %d, dat: %d\n", node->key[i], node->dat[i]);
    }
    printf("============================================\n");
}

void DisplayLeaf(Bptree *tree, int offset, int *dataSize)
{
    Node *node = NodeSeek(tree, offset);
    if (node == NULL)
        return;
    if (node->isLeaf)
    {
        DisplayNode(node);
        (*dataSize) += node->size;
    }
    else
    {
        for (int i = 0; i <= node->size; ++i)
        {
            DisplayLeaf(tree, node->dat[i], dataSize);
        }
    }
    free(node);
}

void DispalySpace(int n)
{
    for (int i = 0; i < n; ++i)
        printf("\t");
}

void DisplayArch(Bptree *tree, int offset, int deep)
{
    Node *node = NodeSeek(tree, offset);
    if (node == NULL)
        return;
    DispalySpace(deep);
    printf("#%d %d:", offset, node->parent);
    for (int i = 0; i < node->size; ++i)
    {
        printf("%d, ", node->key[i]);
    }
    printf("\n");
    if (node->isLeaf)
    {
    }
    else
    {
        for (int i = 0; i <= node->size; ++i)
        {
            DisplayArch(tree, node->dat[i], deep + 1);
        }
    }
    free(node);
}

void DisplayTree(Bptree *tree)
{
    // printf("============================================\n");
    // printf("filename: %s, size: %d, root: %d\n", tree->filename, tree->size, tree->root);
    // int dataSize = 0;
    // DisplayLeaf(tree, tree->root, &dataSize);
    // printf("dataSize: %d\n", dataSize);
    printf("============================================\n");
    DisplayArch(tree, tree->root, 0);
    printf("============================================\n");
}

int main()
{
    printf("%ld, %ld\n", sizeof(Bptree), sizeof(Node));
    int op;
    printf("0:create\n1:open\n2:close\n3:insert\n4:search\n5:quit\n6:test insert\n7:dispalyTree\n8:Delete\n");
    Bptree *tree;
    while (scanf("%d", &op) != EOF)
    {
        switch (op)
        {
        case 0:
        {
            char filename[256];
            scanf("%s", filename);
            tree = CreateBptree(filename);
            break;
        }
        case 1:
        {
            char filename[256];
            scanf("%s", filename);
            tree = OpenBptree(filename);
            DisplayTree(tree);
            break;
        }
        case 2:
        {
            CloseBptree(tree);
            break;
        }
        case 3:
        {
            int key, data;
            scanf("%d%d", &key, &data);
            BptreeInsert(tree, key, data);
            DisplayTree(tree);
            break;
        }
        case 4:
        {
            int key, data;
            scanf("%d", &key);
            data = BptreeSearch(tree, key);
            printf("%d\n", data);
            break;
        }
        case 5:
        {
            return 0;
            break;
        }
        case 6:
        {
            int n;
            scanf("%d", &n);
            srand(n * 1997);
            for (int i = 0; i < n; ++i)
            {
                int key = rand() % (19970227);
                int data = key;
                if (i == 17)
                {
                    printf("# %d\n", i);
                }
                BptreeInsert(tree, key, data);
            }
            DisplayTree(tree);
            break;
        }
        case 7:
        {
            DisplayTree(tree);
            break;
        }
        case 8:
        {
            int key, data;
            scanf("%d", &key);
            data = BptreeDelete(tree, key);
            printf("Had deleted %d.\n", data);
            break;
        }
        default:
            break;
        }
        printf("0:create\n1:open\n2:close\n3:insert\n4:search\n5:quit\n6:test insert\n7:dispalyTree\n8:Delete\n");
    }
    return 0;
}

#endif /* #if DEBUG == 1 */