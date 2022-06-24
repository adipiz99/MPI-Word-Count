#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <glib.h>
#include "fileManagement.h"
#include "mpi.h"

typedef struct word
{
    int occurrencies;
    char text[50];
} word;

word *newWord(char *text, int count);

void sortByCount(word arr[], int l, int r);

void newWordDatatype(MPI_Datatype *datatype);

word *getWordOccurrencies(filePart *parts, int partNum, int *countedWords);

word *getWordArrayFromTable(GHashTable *hashTable, int wordArrLength);

void updateMasterHashTable(GHashTable *hashTable, word *wordArr, int wordNum);

void printOutputCSV(word *wordArr, int wordArrLength);