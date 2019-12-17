#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

void *xpfMalloc(size_t size)
{
    void *t = malloc(size);
    if (t == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return t;
}

List *NewList()
{
    List *list = (List *)xpfMalloc(sizeof(List));
    list->first = NULL;
    list->last = NULL;
    list->size = 0;
    return list;
}

ListNode *uNewListNode(void *data)
{
    ListNode *listNode = (ListNode *)xpfMalloc(sizeof(ListNode));
    listNode->prev = NULL;
    listNode->next = NULL;
    listNode->data = data;
    return listNode;
}

void FreeList(List *list)
{
    if (list == NULL)
        return;
    ListNode *listNode = list->first;
    ListNode *tmp = NULL;
    while (listNode != NULL)
    {
        tmp = listNode;
        listNode = listNode->next;
        if (tmp->data != NULL)
            free(tmp->data);
        free(tmp);
    }
    free(list);
}

List *ListAppend(List *list, ListNode *listNode)
{
    if (list->size == 0)
    {
        list->first = listNode;
        list->last = listNode;
        list->size = 1;
    }
    else
    {
        listNode->prev = list->last;
        listNode->next = NULL;
        list->last->next = listNode;
        list->last = listNode;
        list->size += 1;
    }
    return list;
}

List *ListPrepend(ListNode *listNode, List *list)
{
    if (list->size == 0)
    {
        list->first = listNode;
        list->last = listNode;
        list->size = 1;
    }
    else
    {
        listNode->prev = NULL;
        listNode->next = list->first;
        list->first->prev = listNode;
        list->first = listNode;
        list->size += 1;
    }
    return list;
}

List *ListMerge(List *l1, List *l2)
{
    if (l1->size == 0)
    {
        l1->first = l2->first;
        l1->last = l2->last;
        l1->size = l2->size;
        return l1;
    }
    if (l2->size == 0)
    {
        return l1;
    }
    l1->last->next = l2->first;
    l2->first->prev = l1->last;
    l1->last = l2->last;
    l1->size += l2->size;
    return l1;
}

List *ListEraseListNode(List *list, ListNode *listNode)
{
    for (ListNode *i = list->first; i != NULL; i = i->next)
    {
        if (i == listNode)
        {
            if (i->prev == NULL)
            {
                list->first = i->next;
            }
            else
            {
                i->prev->next = i->next;
            }
            if (i->next == NULL)
            {
                list->last = i->prev;
            }
            else
            {
                i->next->prev = i->prev;
            }
            list->size -= 1;
            return list;
        }
    }
    return list;
}

List *ListEraseData(List *list, void *data)
{
    for (ListNode *i = list->first; i != NULL; i = i->next)
    {
        if (i->data == data)
        {
            return ListEraseListNode(list, i);
        }
    }
    return list;
}

int StringToInt(const char *s, int *r)
{
    *r = 0;
    int i = 0;
    if (s[0] == '-')
    {
        ++i;
    }
    for (; s[i] != 0; ++i)
    {
        if (s[i] < '0' || s[i] > '9')
            return -1;
        *r *= 10;
        *r += s[i] - '0';
    }
    if (s[0] == '-')
    {
        (*r) = -(*r);
    }
    return 0;
}

char *NewString(char *s, int n)
{
    char *string = (char *)xpfMalloc(sizeof(char) * (n + 1));
    StringCopy(string, s, n);
    string[n] = '\0';
    return string;
}

RC StringCopy(char *target, const char *source, int n)
{
    for (int i = 0; i < n; ++i)
    {
        target[i] = source[i];
    }
    target[n] = '\0';
    return RC_OK;
}

int StringCompare(char const *a, int la, char const *b, int lb)
{
    for (int i = 0; i < la && i < lb; ++i)
    {
        if (a[i] != b[i])
        {
            return a[i] - b[i];
        }
    }
    if (la != lb)
    {
        return la - lb;
    }
    return 0;
}

int StringCompareIC(char const *a, int la, char const *b, int lb)
{
    for (int i = 0; i < la && i < lb; ++i)
    {
        if (toupper(a[i]) != toupper(b[i]))
        {
            return a[i] - b[i];
        }
    }
    if (la != lb)
    {
        return la - lb;
    }
    return 0;
}

int StringCompareLength(const char *a, const char *b)
{
    int la = strlen(a), lb = strlen(b);
    if (la != lb)
    {
        return la - lb;
    }
    return StringCompare(a, la, b, lb);
}

char *IntToString(int x)
{
    char tmp[256];
    memset(tmp, 0, sizeof(tmp));
    int i = 0;
    int k = 0;
    if (x < 0)
    {
        k = 1;
        tmp[i] = '-';
        x = -x;
        ++i;
    }
    while (x)
    {
        tmp[i] = x % 10 + '0';
        ++i;
        x /= 10;
    }
    char t;
    int j = 0;
    for (int j = 0; j < i / 2; ++j)
    {
        t = tmp[i - 1 - j + k];
        tmp[i - 1 - j + k] = tmp[j + k];
        tmp[j + k] = t;
    }
    if (strlen(tmp) == 0)
    {
        tmp[0] = '0';
    }
    return NewString(tmp, strlen(tmp));
}