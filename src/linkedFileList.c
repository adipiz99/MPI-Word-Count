#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../lib/linkedFileList.h"

linkedFileList *newFileList()
{
    linkedFileList *l = malloc(sizeof(linkedFileList));
    l->size = 0;
    l->head = NULL;

    return l;
}

int isEmptyFileList(linkedFileList *l)
{
    if (l->head == NULL)
        return 1;
    return 0;
}

void addFileListEntry(linkedFileList *l, fileNode *n)
{
    n->next = l->head;
    l->head = n;
    l->size++;
}

int getFileListSize(linkedFileList *l)
{
    return l->size;
}