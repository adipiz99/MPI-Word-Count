#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../lib/fileManagement.h"

void newFilePartDatatype(MPI_Datatype *datatype)
{
    MPI_Datatype types[2];
    MPI_Aint displ[2], lowerBound, extent;
    int blocks[2];

    displ[0] = 0;
    blocks[1] = 300;
    types[1] = MPI_CHAR;

    MPI_Type_get_extent(MPI_DOUBLE, &lowerBound, &extent);

    displ[1] = 2 * extent; // 300 * extent;
    blocks[0] = 2;
    types[0] = MPI_DOUBLE;

    MPI_Type_create_struct(2, blocks, displ, types, datatype);
    MPI_Type_commit(datatype);
}

filePart *checkMessage(MPI_Datatype datatype, int *partNum, MPI_Status status)
{
    *partNum = 0;
    MPI_Probe(MASTER, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, datatype, partNum);

    filePart *parts;
    MPI_Alloc_mem(sizeof(filePart) * (*partNum), MPI_INFO_NULL, &parts);
    MPI_Recv(parts, *partNum, datatype, MASTER, 0, MPI_COMM_WORLD, &status);
    return parts;
}

fileNode *createFileNode(char *path)
{
    struct stat filestat;
    fileNode *node = (fileNode *)malloc(sizeof(fileNode));

    if (stat(path, &filestat) != -1)
    {
        node->path = malloc(sizeof(char) * strlen(path) + 1);
        strncpy(node->path, path, strlen(path) + 1);
        node->size = filestat.st_size;
        return node;
    }
    return NULL;
}

linkedFileList *createFileList(char *filePaths, double *allFileSize)
{
    linkedFileList *fileList = newFileList();
    DIR *dir;
    struct dirent *dirent;
    char absolutePath[300];
    char sep1 = '/', sep2 = '\0';

    dir = opendir(filePaths);
    if (dir != NULL)
    {
        while ((dirent = readdir(dir)) != NULL)
        {
            if (dirent->d_type != DT_DIR)
            {
                strncpy(absolutePath, filePaths, 300);
                strncat(absolutePath, &sep1, 1);
                strncat(absolutePath, &sep2, 1);
                strncat(absolutePath, dirent->d_name, strlen(dirent->d_name));

                fileNode *info = createFileNode(absolutePath);
                *allFileSize += info->size;
                addFileListEntry(fileList, info);
            }
        }
        closedir(dir);
    }
    else if (ENOENT == errno)
        return NULL;
    return fileList;
}

void freeFileList(linkedFileList *list)
{
    for (fileNode *p = list->head; p != NULL; p = p->next)
    {
        free(p->path);
    }
    free(list);
}