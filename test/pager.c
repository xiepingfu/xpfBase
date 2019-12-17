#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "pager.h"

Pager *NewPager()
{
    Pager *pager = (Pager *)xpfMalloc(sizeof(Pager));
    pager->nextPager = NULL;
    pager->prevPager = NULL;
    return pager;
}

FileManager *NewFileManager()
{
    FileManager *fileManager = (FileManager *)xpfMalloc(sizeof(FileManager));
    fileManager->fd = -1;
    fileManager->filename = NULL;
    fileManager->firstPager = NULL;
    fileManager->freePager = NULL;
    fileManager->numPager = 0;
    return fileManager;
}

RC xpfCreateFile(FileManager *fileManager, const char *filename)
{
    if (fileManager->filename != NULL)
        free(fileManager->filename);
    fileManager->filename = NewString(filename, strlen(filename));
    if (open(filename, O_RDONLY) != -1)
    {
        fprintf(stderr, "File %s had exist.\n", filename);
        return RC_EXIST_FILE;
    }
    if ((fileManager->fd = open(filename, O_CREAT)) == -1)
    {
        fprintf(stderr, "Create file %s fail.\n", filename);
        return RC_CREATE_FILE_FAIL;
    }
    close(fileManager->fd);
    return RC_OK;
}

RC xpfOpenFile(FileManager *fileManager, const char *filename, int flag)
{
    xpfCloseFile(fileManager);
    fileManager->filename = NewString(filename, strlen(filename));
    if ((fileManager->fd = open(filename, flag)) == -1)
    {
        fprintf(stderr, "Not found file: %s.\n", filename);
        return RC_NOFILE;
    }
    return RC_OK;
}

RC xpfCloseFile(FileManager *fileManager)
{
    if (fileManager == NULL)
        return RC_NULL_POINTER;

    if (fileManager->filename != NULL)
        free(fileManager->filename);
    if (fileManager->firstPager != NULL)
        free(fileManager->firstPager);
    if (fileManager->freePager != NULL)
        free(fileManager->freePager);
    close(fileManager->fd);
    free(fileManager);
    return RC_OK;
}

RC xpfWriteFile(FileManager *fileManager, const char *content, int n)
{

    return RC_OK;
}

RC xpfReadFile(FileManager *fileManager, char *content, int n);

off_t xpfSeekFile(FileManager *fileManager, off_t offset, int whence)
{
    if (fileManager == NULL)
        return RC_NULL_POINTER;
    if (fileManager->fd == -1)
        return RC_FILE_ERR;
    return lseek(fileManager->fd, offset, whence);
}