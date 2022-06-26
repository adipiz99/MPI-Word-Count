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

char *getStrFromNode(fileNode *n){
    char *str = malloc(sizeof(char)*300);
    for(int i = 0; i < 300; i++){
        str[i] = n->path[i];
        if(str[i] == '\0') return str;
    }
    //strncpy(str, n->path, 300);
    return str;
}