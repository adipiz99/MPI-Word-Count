#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../lib/linkedList.h"

#define LIST_ADDED 1
#define LIST_UPDATED 2

linkedList *newList()
{
    linkedList *l = malloc(sizeof(linkedList));
    l->size = 0;
    l->head = NULL;

    return l;
}

int isEmpty(linkedList *l)
{
    if (l->head == NULL)
        return 1;
    return 0;
}

void addListHead(linkedList *l, char *text, int occurrencies)
{
    node *n = malloc(sizeof(node));
    n->text = malloc(sizeof(char) * (strlen(text) + 1));

    strncpy(n->text, text, strlen(text) + 1);
    n->occurrencies = occurrencies;
    n->next = l->head;

    l->head = n;
    l->size++;
}

int updateListEntry(linkedList *l, char *text, int occurrencies)
{
    if (isEmpty(l))
    {
        addListHead(l, text, occurrencies);
        return LIST_ADDED;
    }

    node *p;
    for (p = l->head; p != NULL; p = p->next)
    {
        if (!strcmp(text, p->text))
        {
            p->occurrencies += occurrencies;
            return LIST_UPDATED;
        }
    }

    addListHead(l, text, occurrencies);
    return LIST_ADDED;
}

char *removeListHead(linkedList *l)
{
    if (isEmpty(l))
    {
        return NULL;
    }

    struct node *temp = l->head;
    char *text = malloc(sizeof(char) * strlen(temp->text));

    strncpy(text, temp->text, strlen(temp->text));
    l->head = temp->next;

    free(temp);
    l->size--;
    return text;
}

int getListSize(linkedList *l)
{
    return l->size;
}