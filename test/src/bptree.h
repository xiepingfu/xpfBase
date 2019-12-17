#ifndef BPTREE_H
#define BPTREE_H

#include <stdio.h>

#define BPTREE_M 452
#define MAX_FILENAME_SIZE 512

typedef struct Bptree
{
    char filename[MAX_FILENAME_SIZE];
    FILE *file;
    int size;
    int root;
} Bptree;

Bptree *CreateBptree(char *filename);
Bptree *OpenBptree(char *filename);
void CloseBptree(Bptree *tree);
void BptreeInsert(Bptree *tree, int key, int data);
int BptreeSearch(Bptree *tree, int key);
int BptreeDelete(Bptree *tree, int key);

#endif