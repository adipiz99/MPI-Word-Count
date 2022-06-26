#include "mpi.h"
#include "linkedFileList.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PACK_SIZE 8192
#define MASTER 0

typedef struct filePart
{
    double startPoint;
    double endPoint;
    char *filePath;
} filePart;

void newFilePartDatatype(MPI_Datatype *datatype);

filePart *checkMessage(MPI_Datatype datatype, int *partNum, MPI_Status status);

fileNode *createFileNode(char *path);

linkedFileList *createFileList(char *filePaths, double *allFileSize);

void freeFileList(linkedFileList *list);