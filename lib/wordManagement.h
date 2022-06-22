#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <glib.h>
#include "fileManagement.h"
#include "mpi.h"

typedef struct {
    char text[50];
    int occurrencies;
} word;

word *newWord(char *text, int count);

void sortByCount(word wordArr[], int start, int end);

void newWordDatatype(MPI_Datatype *datatype);

word *getWordOccurrencies(filePart *parts, int partNum, int *countedWords);

word *getWordArrayFromTable(GHashTable *hashTable, int wordArrLength);

void updateMasterHashTable(GHashTable *hashTable, word *wordArr, int wordNum);

void printOutputCSV(word *wordArr, int wordArrLength);