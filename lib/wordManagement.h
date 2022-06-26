#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "fileManagement.h"
#include "linkedList.h"
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

word *getWordArrayFromList(linkedList *list, int *wordArrLength);

void updateMasterList(linkedList *list, word *wordArr, int wordNum);

void printOutputCSV(word *wordArr, int wordArrLength);