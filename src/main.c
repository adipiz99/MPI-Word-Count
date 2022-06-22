#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include <unistd.h>
#include "mpi.h"
#include "fileManagement.h"
#include "wordManagement.h"

#define MASTER 0

int main (int argc, char *argv[]){
    MPI_Init(&argc,&argv);
    int tasks, myrank;
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Datatype filePartDatatype, wordDatatype;
    newFilePartDatatype(&filePartDatatype);
    newWordDatatype(&wordDatatype);

    if(argc < 2){
        printf("Usage: ./%s \"/path/to/folder\"", argv[0]);
        MPI_Finalize();
        return 0;
    }

    if(myrank == MASTER){
        double allFilesSize = 0;
        GList * gFileList = createFileList(argv[1], &allFilesSize);

        if(gFileList == NULL){
            printf("Error: No such file or directory!");
            MPI_Finalize();
            return 0;
        }
        double portion = (allFilesSize / tasks) + 0.0;
        int rest = (int) allFilesSize % tasks;
    
        fileInfo *filePartPtr;
        filePart **jobs;
        GList *listPtr = gFileList;
        int foundFiles = g_list_length(gFileList);
        MPI_Alloc_mem(sizeof(filePart*) * tasks, MPI_INFO_NULL , &jobs);

        double assignedBytes, bytesToAssign = 0;
        double notAssignedBytes;
        int currentTask = 0, filePartCounter = 0, masterFilePartCount = 0;
    
        MPI_Request *requests;
        MPI_Alloc_mem(sizeof(MPI_Request) * (tasks - 1), MPI_INFO_NULL , &requests);

        for(int i=0; i<foundFiles; i++){
            filePartPtr = (fileInfo*) listPtr->data;
            assignedBytes = 0;
            notAssignedBytes = filePartPtr->fileSize;

            while(notAssignedBytes > 0){
                if(bytesToAssign == 0){ //Se non ho altri byte da assegnare...
                    if(filePartCounter != 0){ //... se ho almeno una parte di file che deve essere inviata...
                        if(currentTask != MASTER){ //... e se il processo per cui sto calcolando cosa inviare non è il MASTER, allora invio.
                            MPI_Isend(jobs[currentTask], filePartCounter, filePartDatatype, currentTask, 0, MPI_COMM_WORLD, &(requests[currentTask - 1]));
                        }
                        else masterFilePartCount = filePartCounter; //Altrimenti salvo il numero di parti da analizzare
                    }
                    bytesToAssign = portion;
                    currentTask++; 
                    MPI_Alloc_mem(sizeof(filePart)* foundFiles, MPI_INFO_NULL , &(jobs[currentTask]));
                    filePartCounter = 0;
                }
                
                //All'ultimo do porzione e resto
                if(currentTask == (tasks - 1) && rest > 0) bytesToAssign = portion + rest;

                
                if(notAssignedBytes <= bytesToAssign){
                    jobs[currentTask][filePartCounter].start_offset = assignedBytes;
                    jobs[currentTask][filePartCounter].end_offset = assignedBytes + notAssignedBytes;

                    strncpy(jobs[currentTask][filePartCounter].path , filePartPtr->path, 300);

                    filePartCounter++;
                    bytesToAssign -= notAssignedBytes;
                    notAssignedBytes = 0;
                }
                else {
                    jobs[currentTask][filePartCounter].start_offset = assignedBytes;
                    jobs[currentTask][filePartCounter].end_offset = assignedBytes + notAssignedBytes;

                    strncpy(jobs[currentTask][filePartCounter].path , filePartPtr->path, 300);

                    filePartCounter++;
                    assignedBytes += bytesToAssign;
                    notAssignedBytes -= bytesToAssign;
                    bytesToAssign = 0;
                }   
            }
            listPtr = listPtr->next;
        }
        
        //Ultimo invio
        MPI_Isend(jobs[currentTask], filePartCounter, filePartDatatype, currentTask, 0, MPI_COMM_WORLD, &(requests[currentTask-1]));

        //Il MASTER esegue il proprio job
        int masterCountedWords = 0;
        filePart *masterFilePart = jobs[MASTER];
        word *masterWordArr = getWordOccurrencies(masterFilePart, masterFilePartCount, &masterCountedWords);

        MPI_Waitall((tasks - 1), requests , MPI_STATUSES_IGNORE);
        MPI_Free_mem(requests);

        MPI_Status recvStatus;
        int count, wordArrLength;
        word *wordArr;
        GHashTable *hashTable = g_hash_table_new(g_str_hash, g_str_equal);
    
        for(int i = 0; i < tasks; i++){
            if(i != MASTER){
                MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recvStatus);
                MPI_Get_count(&recvStatus, wordDatatype, &count);
                MPI_Alloc_mem(sizeof(word) * count, MPI_INFO_NULL , &wordArr);
                MPI_Recv(wordArr, count, wordDatatype, recvStatus.MPI_SOURCE, 0, MPI_COMM_WORLD, &recvStatus);

                updateMasterHashTable(hashTable, wordArr, count);
            }
            else {
                updateMasterHashTable(hashTable, masterWordArr, masterCountedWords);
                MPI_Free_mem(masterWordArr);
            }
        }

        word *orderedWordArr = getWordArrayFromTable(hashTable, wordArrLength);    
        sortByCount(orderedWordArr, 0, wordArrLength);
        printOutputCSV(orderedWordArr, wordArrLength);

        for(int i = 0; i< tasks -1; i++){
            MPI_Free_mem(jobs[i]);
        }
        MPI_Free_mem(jobs);
        freeFileList(gFileList, foundFiles);

        MPI_Free_mem(wordArr);
        free(orderedWordArr);
    }
    else{
        int partNum = 0, countedWords = 0;
        filePart *fileList = checkMessage(filePartDatatype, &partNum, status);
        word *wordArr = getWordOccurrencies(fileList, partNum, &countedWords);

        MPI_Send(wordArr, countedWords, wordDatatype, MASTER, 0, MPI_COMM_WORLD);
        MPI_Free_mem(fileList);
        MPI_Free_mem(wordArr);        
    }

    MPI_Type_free(&filePartDatatype);
    MPI_Type_free(&wordDatatype);
    MPI_Finalize();
    return 0;
}