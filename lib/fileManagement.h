#include"mpi.h"
#include"glib.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PACK_SIZE 8192
#define MASTER 0

typedef struct fileInfo{
    char *filePath;
    double fileSize;
} fileInfo;

typedef struct filePart {
    char filePath[300];
    double startPoint;
    double endPoint;
} filePart;

filePart *newFilePart(double start_offset , double end_offset, char *path);

void newFilePartDatatype(MPI_Datatype *datatype);

filePart *checkMessage(MPI_Datatype datatype, int  *partNum, MPI_Status status);

filePart *getInfo(char *path);

GList *createFileList(char *filePaths, double *fileSizes);

void freeFileList(GList *list, int elemNum);