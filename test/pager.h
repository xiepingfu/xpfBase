#ifndef PAGER_H
#define PAGER_H

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#define PAGER_SIZE 4096
#define PAGER_HEADER_SIZE 96
#define PAGER_BODY_SIZE (PAGER_SIZE - PAGER_HEADER_SIZE)

typedef struct Pager
{
    struct Pager *prevPager;
    struct Pager *nextPager;
} Pager;

typedef struct FileManager
{
    char *filename;           /*Filename for this file.*/
    int fd;                   /*File discriptor for this file.*/
    int numPager;             /*Number of pager in this file.*/
    int inMemoryPagers;       /*The number of pagers that had in memory.*/
    struct Pager *firstPager; /*The first pager that had the content.*/
    struct Pager *freePager;  /*The first pager that had nothing.*/
} FileManager;

RC xpfCreateFile(FileManager *fileManager, const char *filename);
RC xpfOpenFile(FileManager *fileManager, const char *filename, int flag);
RC xpfCloseFile(FileManager *fileManager);
RC xpfWriteFile(FileManager *fileManager, const char *content, int n);
RC xpfReadFile(FileManager *fileManager, char *content, int n);
off_t xpfSeekFile(FileManager *fileManager, off_t offset, int whence);

#endif