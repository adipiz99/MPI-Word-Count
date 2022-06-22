#include <stdio.h>
#include <glib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "../lib/fileManagement.h"

filePart *newFilePart(double start_offset , double end_offset, char *path){

    filePart * chunk = malloc(sizeof(filePart));

    chunk->start_offset = start_offset;
    chunk->end_offset = end_offset;
    strncpy(chunk->path, path, 260);
    
    return chunk;

}

void newFilePartDatatype(MPI_Datatype *datatype){
    MPI_Datatype types[2];
    MPI_Aint displ[2], lowerBound, extent;
    int blocks[2];

    displ[0] = 0;
    blocks[0] = 300;
    types[0] = MPI_CHAR;
    
    MPI_Type_get_extent(MPI_CHAR, &lowerBound, &extent);

    displ[1] = 300 * extent;
    blocks[1] = 2;
    types[1] = MPI_DOUBLE;

    MPI_Type_create_struct(2, blocks, displ, types, datatype);
    MPI_Type_commit(datatype);
}

filePart *checkMessage(MPI_Datatype datatype, int  *partNum, MPI_Status status){
    *partNum = 0;
    MPI_Probe(MASTER, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, datatype, partNum);

    filePart *parts;
    MPI_Alloc_mem(sizeof(filePart)*(*partNum), MPI_INFO_NULL, &parts);
    MPI_Recv(parts, *partNum, datatype, MASTER, 0, MPI_COMM_WORLD, &status);
    return parts;
}

filePart *getInfo(char *path){
    struct stat stat;
    fileInfo *info = (fileInfo*) malloc(sizeof(fileInfo));

    if(stat(path, &stat) != -1){
        info->filePath = malloc(sizeof(char) * strlen(path) + 1);
        strncpy(info->filePath ,path , strlen(path)+1);
        info->fileSize = stat.st_size;
        return info;
    }
    return NULL;
}

GList *createFileList(char *filePaths, double *fileSizes){
    GList* fileList = NULL;
    DIR *dir;
    struct dirent *dirent;
    char absolutePath[300];
    char separators[]={'/','\0'};

    dir = opendir(filePaths);
    if (dir != NULL) {
        fileInfo *info;
        while ((dirent = readdir(dir)) != NULL) {
            if(dirent-> d_type != DT_DIR) {
                strncpy(absolutePath, filePaths, 300);
                strncat(absolutePath, separators, 2);
                strncat(absolutePath, dirent->d_name, strlen(dirent->d_name));
            
                info = getInfo(absolutePath);
                *fileSizes += info->fileSize;
                fileList = g_list_append(fileList, info);
            }
        }
        closedir(d);   
    }
    else if (ENOENT == errno) return NULL;
    return fileList;
}

void freeFileList(GList *list, int elemNum){

    GList *temp = list;
    for(int i=0; i<elemNum; i++){
            fileInfo *toFree = temp->data;
            free(toFree->path);
            free(temp->data);
            temp = temp->next;
    }
    g_list_free(list);
}