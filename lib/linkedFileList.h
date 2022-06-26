typedef struct fileNode
{
    char *path;
    int size;
    struct fileNode *next;
} fileNode;

typedef struct linkedFileList
{
    int size;
    struct fileNode *head;
} linkedFileList;

linkedFileList *newFileList();
int isEmptyFileList(linkedFileList *);
void addFileListEntry(linkedFileList *, fileNode *);
int getFileListSize(linkedFileList *);
char *getStrFromNode(fileNode *);