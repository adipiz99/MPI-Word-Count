#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
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
        linkedFileList *fileList = createFileList(argv[1], &allFilesSize);
        printf("[MASTER] Lista pronta \n");

        if (fileList == NULL)
        {
            printf("Error: No such file or directory!");
            MPI_Abort(MPI_COMM_WORLD, MPI_ERR_ARG);
            return 0;
        }

        printf("[MASTER] La cartella esiste \n");
        fflush(stdout);

        int portion = allFilesSize / tasks;
        int rest = (int)allFilesSize % tasks;
        printf("Resto %d\nPorzione %d\n", rest, portion);

        filePart **jobs;
        int foundFiles = fileList->size;
        MPI_Alloc_mem(sizeof(filePart *) * tasks, MPI_INFO_NULL, &jobs);

        double assignedBytes, bytesToAssign = portion;
        double notAssignedBytes;
        int currentTask = 0, filePartCounter = 0, masterFilePartCount = 0;

        MPI_Request *requests;
        MPI_Alloc_mem(sizeof(MPI_Request) * (tasks - 1), MPI_INFO_NULL, &requests);
        MPI_Alloc_mem(sizeof(filePart) * foundFiles, MPI_INFO_NULL, &(jobs[currentTask]));

        printf("[MASTER] Tutto pronto per assegnare job \n");

        fileNode *n = fileList->head;
        for (int i = 0; i < foundFiles; i++)
        {
            assignedBytes = 0;
            notAssignedBytes = n->size;

            while (notAssignedBytes > 0)
            {
                if (bytesToAssign == 0)
                { // Se non ho altri byte da assegnare al task corrente...
                    if (filePartCounter != 0)
                    { //... se ho almeno una parte di file che deve essere inviata...
                        if (currentTask != MASTER)
                        { //... e se il processo per cui sto calcolando cosa inviare non è il MASTER, allora invio.
                            MPI_Isend(jobs[currentTask], filePartCounter, filePartDatatype, currentTask, 0, MPI_COMM_WORLD, &(requests[currentTask - 1]));
                            for(int k = 0; k < filePartCounter; k++){
                                printf("[MASTER] task %d\n\
                                    \tFilepath: %s\n\
                                    \tstartPoint: %f\n\
                                    \tendPoint: %f\n", currentTask, jobs[currentTask][k].filePath, jobs[currentTask][k].startPoint, jobs[currentTask][k].endPoint);fflush(stdout);
                            }
                        }
                        else
                            masterFilePartCount = filePartCounter; // Altrimenti salvo il numero di parti da analizzare
                    }
                    bytesToAssign = portion;
                    currentTask++;
                    MPI_Alloc_mem(sizeof(filePart) * foundFiles, MPI_INFO_NULL, &(jobs[currentTask]));
                    filePartCounter = 0;
                }

                // Al MASTER do porzione e resto
                if (currentTask == MASTER && bytesToAssign == portion && rest > 0)
                    bytesToAssign = portion + rest;

                printf("Comincio a vedere se entra\n");fflush(stdout);

                if (bytesToAssign >= notAssignedBytes)
                {
                    printf("entra\n");fflush(stdout);
                    printf("filename: %s\n", n->path);fflush(stdout);
                    char *c = getStrFromNode(n);
                    strncpy(jobs[currentTask][filePartCounter].filePath, c, 300);
                    //strncpy(jobs[currentTask][filePartCounter].filePath, n->path, 300);
                    printf("copiato\n");fflush(stdout);
                    jobs[currentTask][filePartCounter].startPoint = assignedBytes;
                    jobs[currentTask][filePartCounter].endPoint = assignedBytes + notAssignedBytes;
                    assignedBytes += notAssignedBytes;
                    bytesToAssign -= notAssignedBytes;
                    notAssignedBytes = 0;
                    filePartCounter++;
                    printf("Fatto\n");fflush(stdout);
                }
                else
                {
                    printf("non entra\n");fflush(stdout);
                    strncpy(jobs[currentTask][filePartCounter].filePath, n->path, 300);
                    jobs[currentTask][filePartCounter].startPoint = assignedBytes;
                    jobs[currentTask][filePartCounter].endPoint = assignedBytes + bytesToAssign;
                    assignedBytes += bytesToAssign;
                    notAssignedBytes -= bytesToAssign;
                    bytesToAssign = 0;
                    filePartCounter++;
                }
            }
            n = n->next;
        }
        // Ultimo invio
        MPI_Isend(jobs[currentTask], filePartCounter, filePartDatatype, currentTask, 0, MPI_COMM_WORLD, &(requests[currentTask - 1]));

        printf("[MASTER] Tutti i Job inviati \n");

        // Il MASTER esegue il proprio job
        int masterCountedWords = 0;
        filePart *masterFilePart = jobs[MASTER];
        word *masterWordArr = getWordOccurrencies(masterFilePart, masterFilePartCount, &masterCountedWords);
        for(int i = 0; i < masterCountedWords; i++)
        printf("word: %s, occ: %d \n", masterWordArr[i].text, masterWordArr[i].occurrencies);fflush(stdout);
        printf("[MASTER] Array di word generato \n");
        fflush(stdout);

        MPI_Waitall((tasks - 1), requests, MPI_STATUSES_IGNORE);
        MPI_Free_mem(requests);

        printf("[MASTER] Job completato \n");
        fflush(stdout);

        MPI_Status recvStatus;
        int count, wordArrLength;
        word *wordArr;
        linkedList *list = newList();

        for (int i = 0; i < tasks; i++)
        {
            if (i != MASTER)
            {
                printf("[MASTER] Probe per task %d \n", i);
                fflush(stdout);
                MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recvStatus);
                MPI_Get_count(&recvStatus, wordDatatype, &count);
                MPI_Alloc_mem(sizeof(word) * count, MPI_INFO_NULL, &wordArr);
                MPI_Recv(wordArr, count, wordDatatype, recvStatus.MPI_SOURCE, 0, MPI_COMM_WORLD, &recvStatus);
                printf("[MASTER] Ricevuto da %d \n", recvStatus.MPI_SOURCE);
                fflush(stdout);
                updateMasterList(list, wordArr, count);
                printf("[MASTER] Aggiornamento hashtable completato per task %d \n", recvStatus.MPI_SOURCE);
                fflush(stdout);
            }
            else
            {
                printf("[MASTER] Aggiornamento hashtable MASTER \n");
                updateMasterList(list, masterWordArr, masterCountedWords);
                MPI_Free_mem(masterWordArr);
                printf("[MASTER] Operazione completata \n");
            }
        }
        printf("[MASTER] Comincio a ordinare \n");
        word *orderedWordArr = getWordArrayFromList(list, &wordArrLength);
        printf("[MASTER] Array completo preso \n");
        sortByCount(orderedWordArr, 0, wordArrLength);
        printf("[MASTER] Sorting completato \n");
        printf("[MASTER] Creazione file \n");
        printOutputCSV(orderedWordArr, wordArrLength);
        printf("[MASTER] File creato \n");

        for (int i = 0; i < tasks - 1; i++)
        {
            MPI_Free_mem(jobs[i]);
        }
        MPI_Free_mem(jobs);
        printf("[MASTER] free jobs \n");

        freeFileList(fileList);

        MPI_Free_mem(wordArr);
        free(orderedWordArr);
    }
    else
    {
        int partNum = 0, countedWords = 0;
        filePart *fileList = checkMessage(filePartDatatype, &partNum, status);
        /*for(int i = 0; i < partNum; i++)
            printf("[TASK %d] Filepath: %s\n\
            \tstartPoint: %f\n\
            \tendPoint: %f\n", myrank, fileList[i].filePath, fileList[i].startPoint, fileList[i].endPoint);*/
        word *wordArr = getWordOccurrencies(fileList, partNum, &countedWords);
        printf("[TASK %d] Occorrenze prese \n", myrank);
        fflush(stdout);

        printf("[TASK %d] Send in corso \n", myrank);
        fflush(stdout);
        MPI_Send(wordArr, countedWords, wordDatatype, MASTER, 0, MPI_COMM_WORLD);
        printf("[TASK %d] Send completata \n", myrank);
        fflush(stdout);
        MPI_Free_mem(fileList);
        MPI_Free_mem(wordArr);
    }

    MPI_Type_free(&filePartDatatype);
    MPI_Type_free(&wordDatatype);
    MPI_Finalize();
    return 0;
}