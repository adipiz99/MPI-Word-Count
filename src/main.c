#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include <unistd.h>
#include "mpi.h"
#include "../lib/wordManagement.h"

#define MASTER 0

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int tasks, myrank;
    MPI_Status status;
    MPI_Comm_size(MPI_COMM_WORLD, &tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Datatype filePartDatatype, wordDatatype;
    newFilePartDatatype(&filePartDatatype);
    newWordDatatype(&wordDatatype);

    if (myrank == MASTER)
        printf("[MASTER] Datatype pronti \n");

    if (argc < 2)
    {
        if (myrank == MASTER)
            printf("Usage: ./%s \"/path/to/folder\"\n", argv[0]);
        MPI_Finalize();
        return 0;
    }

    if (myrank == MASTER)
        printf("[MASTER] Gli argomenti ci sono, preparo lista \n");

    if (myrank == MASTER)
    {
        double allFilesSize = 0;
        GList *gFileList = createFileList(argv[1], &allFilesSize);
        printf("[MASTER] Lista pronta \n");

        if (gFileList == NULL)
        {
            printf("Error: No such file or directory!");
            MPI_Abort(MPI_COMM_WORLD, MPI_ERR_ARG);
            return 0;
        }

        printf("[MASTER] La cartella esiste \n");
        fflush(stdout);

        int portion = allFilesSize / tasks;
        int rest = (int) allFilesSize % tasks;
        printf("Resto %d\nPorzione %d\n", rest, portion);

        fileInfo *filePartPtr;
        filePart **jobs;
        GList *listPtr = gFileList;
        int foundFiles = g_list_length(gFileList);
        MPI_Alloc_mem(sizeof(filePart *) * tasks, MPI_INFO_NULL, &jobs);

        double assignedBytes, bytesToAssign = portion;
        double notAssignedBytes;
        int currentTask = 0, filePartCounter = 0, masterFilePartCount = 0;

        MPI_Request *requests;
        MPI_Alloc_mem(sizeof(MPI_Request) * (tasks - 1), MPI_INFO_NULL, &requests);
        MPI_Alloc_mem(sizeof(filePart) * foundFiles, MPI_INFO_NULL, &(jobs[currentTask]));

        printf("[MASTER] Tutto pronto per assegnare job \n");

        for (int i = 0; i < foundFiles; i++)
        {
            filePartPtr = (fileInfo *)listPtr->data;
            assignedBytes = 0;
            notAssignedBytes = filePartPtr->fileSize;

            while (notAssignedBytes > 0)
            {
                if (bytesToAssign == 0)
                { // Se non ho altri byte da assegnare al task corrente...
                    if (filePartCounter != 0)
                    { //... se ho almeno una parte di file che deve essere inviata...
                        if (currentTask != MASTER)
                        { //... e se il processo per cui sto calcolando cosa inviare non è il MASTER, allora invio.
                            MPI_Isend(jobs[currentTask], filePartCounter, filePartDatatype, currentTask, 0, MPI_COMM_WORLD, &(requests[currentTask - 1]));
                            printf("[MASTER] Job inviato al task %d \n", currentTask);
                            fflush(stdout);
                        }
                        else
                            masterFilePartCount = filePartCounter; // Altrimenti salvo il numero di parti da analizzare
                    }
                    bytesToAssign = portion;
                    printf("[MASTER] Job assegnato al task %d \n", currentTask);
                    fflush(stdout);
                    filePart inf;
                    for (int k = 0; k < filePartCounter; k++)
                    {
                        inf = jobs[currentTask][k];
                    }
                    currentTask++;
                    MPI_Alloc_mem(sizeof(filePart) * foundFiles, MPI_INFO_NULL, &(jobs[currentTask]));
                    filePartCounter = 0;
                }

                // Al MASTER do porzione e resto
                if (currentTask == MASTER && bytesToAssign == portion && rest > 0)
                    bytesToAssign = portion + rest;

                if (bytesToAssign >= notAssignedBytes)
                {
                    strncpy(jobs[currentTask][filePartCounter].filePath, filePartPtr->filePath, 300);
                    jobs[currentTask][filePartCounter].startPoint = assignedBytes;
                    jobs[currentTask][filePartCounter].endPoint = assignedBytes + notAssignedBytes;
                    assignedBytes += notAssignedBytes;
                    bytesToAssign -= notAssignedBytes;
                    notAssignedBytes = 0;
                    filePartCounter++;
                }
                else
                {
                    strncpy(jobs[currentTask][filePartCounter].filePath, filePartPtr->filePath, 300);
                    jobs[currentTask][filePartCounter].startPoint = assignedBytes;
                    jobs[currentTask][filePartCounter].endPoint = assignedBytes + bytesToAssign;
                    assignedBytes += bytesToAssign;
                    notAssignedBytes -= bytesToAssign;
                    bytesToAssign = 0;
                    filePartCounter++;
                }
            }
            listPtr = listPtr->next;
        }
        printf("[MASTER] Ultimo invio a %d \n", currentTask);
        filePart inf;
        for (int k = 0; k < filePartCounter; k++)
            inf = jobs[currentTask][k];
        // Ultimo invio
        MPI_Isend(jobs[currentTask], filePartCounter, filePartDatatype, currentTask, 0, MPI_COMM_WORLD, &(requests[currentTask - 1]));

        printf("[MASTER] Tutti i Job inviati \n");

        // Il MASTER esegue il proprio job
        int masterCountedWords = 0;
        filePart *masterFilePart = jobs[MASTER];
        word *masterWordArr = getWordOccurrencies(masterFilePart, masterFilePartCount, &masterCountedWords);
        printf("[MASTER] Array di word generato \n");

        MPI_Waitall((tasks - 1), requests, MPI_STATUSES_IGNORE);
        MPI_Free_mem(requests);

        printf("[MASTER] Job completato \n");

        MPI_Status recvStatus;
        int count, wordArrLength;
        word *wordArr;
        GHashTable *hashTable = g_hash_table_new(g_str_hash, g_str_equal);

        for (int i = 0; i < tasks; i++)
        {
            if (i != MASTER)
            {
                MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recvStatus);
                MPI_Get_count(&recvStatus, wordDatatype, &count);
                MPI_Alloc_mem(sizeof(word) * count, MPI_INFO_NULL, &wordArr);
                MPI_Recv(wordArr, count, wordDatatype, recvStatus.MPI_SOURCE, 0, MPI_COMM_WORLD, &recvStatus);
                printf("[MASTER] Ricevuto da %d \n", recvStatus.MPI_SOURCE);
                updateMasterHashTable(hashTable, wordArr, count);
            }
            else
            {
                updateMasterHashTable(hashTable, masterWordArr, masterCountedWords);
                MPI_Free_mem(masterWordArr);
            }
        }

        word *orderedWordArr = getWordArrayFromTable(hashTable, wordArrLength);
        sortByCount(orderedWordArr, 0, wordArrLength);
        printOutputCSV(orderedWordArr, wordArrLength);

        for (int i = 0; i < tasks - 1; i++)
        {
            MPI_Free_mem(jobs[i]);
        }
        MPI_Free_mem(jobs);
        freeFileList(gFileList, foundFiles);

        MPI_Free_mem(wordArr);
        free(orderedWordArr);
    }
    else
    {
        /*int partNum = 0, countedWords = 0;
        filePart *fileList = checkMessage(filePartDatatype, &partNum, status);
        word *wordArr = getWordOccurrencies(fileList, partNum, &countedWords);

        MPI_Send(wordArr, countedWords, wordDatatype, MASTER, 0, MPI_COMM_WORLD);
        MPI_Free_mem(fileList);
        MPI_Free_mem(wordArr);*/
    }

    MPI_Type_free(&filePartDatatype);
    MPI_Type_free(&wordDatatype);
    MPI_Finalize();
    return 0;
}