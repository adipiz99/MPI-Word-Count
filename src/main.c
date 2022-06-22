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

    //Creazione del datatype per il chunk 
    MPI_Datatype filePartDatatype, wordDatatype;
    newFilePartDatatype(&filePartDatatype);
    newWordDatatype(&wordDatatype);

    if(argc<2){
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
    
        //Produzione dei chunk per ogni processo che non sia quello master
        fileInfo * filePartPtr;
        GList *listPtr = gFileList;
        int foundFiles = g_list_length(gFileList);

        //la size dei chunk per ogni processo può essere al più uguale al numero dei file
        //File_chunk ** chunks_to_send = malloc(sizeof(File_chunk *) * numtasks -1);
        filePart **jobs;
        MPI_Alloc_mem(sizeof(filePart*) * tasks, MPI_INFO_NULL , &jobs);

        double assignedBytes, bytesToAssign = 0;
        double notAssignedBytes;
        int currentTask = 0, filePartCounter = 0;
    
        //Istanzio array di request e status per l'invio e la ricezione asincrona
        MPI_Request* reqs;
        MPI_Alloc_mem(sizeof(MPI_Request) * (tasks - 1), MPI_INFO_NULL , &reqs);

        for(int i=0; i<foundFiles; i++){
            filePartPtr = (fileInfo*) listPtr->data;
            assignedBytes = 0;
            notAssignedBytes = filePartPtr->fileSize;

            while(notAssignedBytes > 0){
                if(bytesToAssign == 0){ //Se non ho altri byte da assegnare...
                    if(filePartCounter != 0){ //... se ho almeno una parte di file che deve essere inviata...
                        if(currentTask != MASTER){ //... e se il processo per cui sto calcolando cosa inviare non è il MASTER, allora invio.
                            MPI_Isend(jobs[currentTask], filePartCounter, filePartDatatype, currentTask, 0, MPI_COMM_WORLD, &(reqs[currentTask - 1]));
                        }
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
        MPI_Isend(jobs[currentTask], filePartCounter, filePartDatatype, currentTask, 0, MPI_COMM_WORLD, &(reqs[currentTask]));

        // SONO ARRIVATO QUI... IMPLEMENTARE WORKLOAD DEL MASTER (SALVATO IN jobs[0])

        MPI_Waitall(tasks - 1, reqs , MPI_STATUSES_IGNORE);

        //Mentre i worker lavorano il master può deallocare tutto ciò che non serve
        MPI_Free_mem(reqs);
        for(int i = 0; i< tasks -1; i++){
            MPI_Free_mem(jobs[i]);
        }
        MPI_Free_mem(jobs);
        freeFileList(gFileList, foundFiles);

        //Ricevere da tutti i figli
        MPI_Status recvStatus;
        int words_in_message, length, occ;
        Word_occurrence * occurrences;
        GHashTable* hash = g_hash_table_new(g_str_hash, g_str_equal);
        gpointer lookup;
    
        for(int i = 0; i < tasks -1; i++){

            
            MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recvStatus);
            MPI_Get_count(&recvStatus, wordtype, &words_in_message);
            MPI_Alloc_mem(sizeof(Word_occurrence ) * words_in_message, MPI_INFO_NULL , &occurrences);
            MPI_Recv(occurrences, words_in_message, wordtype, recvStatus.MPI_SOURCE, 0, MPI_COMM_WORLD, &recvStatus);
            for(int j=0; j< words_in_message; j++){
                lookup = g_hash_table_lookup(hash,occurrences[j].word);
                if(lookup == NULL){
                    g_hash_table_insert(hash,occurrences[j].word,GINT_TO_POINTER (occurrences[j].num));
                }
                else{
                    g_hash_table_insert(hash,occurrences[j].word,GINT_TO_POINTER (occurrences[j].num + GPOINTER_TO_INT(lookup)));
                }
            } 
        }

        char ** entries = (char **) g_hash_table_get_keys_as_array (hash , &length);
        FILE *out = fopen ("output.txt" , "w");
        Word_occurrence * to_order = malloc(sizeof(Word_occurrence) * length);
        for(int i = 0; i<length; i++){

            lookup = g_hash_table_lookup(hash,entries[i]); 
            occ =  GPOINTER_TO_INT(lookup);
            to_order[i].num = occ;
            strncpy(to_order[i].word, entries[i], sizeof(to_order[i].word));  
        }

        sort_occurrences(&to_order, length);

        fprintf(out, "\"Lessema\",\"Frequenza\"\n");
        for(int i = 0; i< length; i++){
            fprintf(out,"\"%s\",\"%d\"\n", to_order[i].word, to_order[i].num);
        }

        MPI_Free_mem(occurrences);
        free(to_order);
        free(entries);
    }
    else{
        int partNum = 0, countedWords = 0;
        filePart *fileList = checkMessage(filePartDatatype, status, &partNum);
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