typedef struct node
{
    char *text;
    int occurrencies;
    struct node *next;
} node;

typedef struct linkedList
{
    int size;
    node *head;
} linkedList;

linkedList *newList();
int isEmpty(linkedList *);
int updateListEntry(linkedList *, char *, int);
char *removeListHead(linkedList *);
int getListSize(linkedList *);