#ifndef UTILS_H
#define UTILS_H

#define STRING_SIZE 256
#define INT_SIZE 32
#define FLOAT_SIZE 32

typedef struct ListNode
{
    struct ListNode *prev;
    struct ListNode *next;
    void *data;
} ListNode;

typedef struct List
{
    ListNode *first;
    ListNode *last;
    int size;
} List;

typedef enum
{
    RC_OK,
    RC_UNKNOW,
    RC_TOKEN_ILLEGAL,
    RC_SYNTAX_ERROR,
    RC_UNMATCH,
    RC_NOFILE,
    RC_EXIST_FILE,
    RC_CREATE_FILE_FAIL,
    RC_NULL_POINTER,
    RC_FILE_ERR,
} RC;

int StringToInt(const char *s, int *r);
int StringCompare(char const *a, int la, char const *b, int lb);
int StringCompareIC(char const *a, int la, char const *b, int lb);
char *NewString(char *s, int n);
RC StringCopy(char *target, const char *source, int n);
void *xpfMalloc(size_t);
char *IntToString(int x);
int StringCompareLength(char const *a, char const *b);

List *NewList();
ListNode *uNewListNode(void *data);
void FreeList(List *list);
List *ListAppend(List *list, ListNode *listNode);
List *ListPrepend(ListNode *listNode, List *list);
List *ListMerge(List *l1, List *l2);
List *ListEraseListNode(List *list, ListNode *listNode);
List *ListEraseData(List *list, void *data);

#endif