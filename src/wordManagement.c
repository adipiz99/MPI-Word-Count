#include "../lib/wordManagement.h"

word *newWord(char *text, int count)
{
    word *word = malloc(sizeof(word));
    strncpy(word->text, text, 50);
    word->occurrencies = count;
    return word;
}

void newWordDatatype(MPI_Datatype *datatype)
{
    MPI_Datatype types[2];
    MPI_Aint displ[2], lowerBound, extent;
    int blocks[2];

    displ[0] = 0;
    blocks[1] = 50;
    types[1] = MPI_CHAR;

    MPI_Type_get_extent(MPI_INT, &lowerBound, &extent);

    displ[1] = extent;
    blocks[0] = 1;
    types[0] = MPI_INT;

    MPI_Type_create_struct(2, blocks, displ, types, datatype);
    MPI_Type_commit(datatype);
}

int isWordTerminator(char c)
{
    if (c == '\t' || c == '\r' || c == '\n' || c == ' ' || c == EOF)
        return 1;
    return 0;
}

void addOccurrency(linkedList *list, char *word, int *differentWords)
{
    int opStatus = updateListEntry(list, word, 1);

    if (opStatus == 1) // Nuovo nodo
        *differentWords += 1;
}

word *getWordOccurrencies(filePart *parts, int partNum, int *countedWords)
{
    FILE *file;
    filePart fp;
    word *wordArr;
    char currWord[50], currChar;
    int wordPtr = 0, wordCount = 0;

    linkedList *list = newList();

    for (int i = 0; i < partNum; i++)
    {
        fp = parts[i];
        file = fopen(fp.filePath, "r");
        fseek(file, fp.startPoint, SEEK_CUR);

        if (ftell(file) != 0)
        { // Se non è l'inizio del file...
            fseek(file, -1, SEEK_CUR);
            currChar = fgetc(file);

            if (!isWordTerminator(currChar))
            { //... e se il carattere che precede l'inizio della porzione non è un terminatore...
                while (!isWordTerminator(currChar))
                {
                    fseek(file, -2, SEEK_CUR); //... leggo all'indietro fino al primo terminatore.
                    currChar = fgetc(file);
                }
            }
        }

        int prevCharIsWordTerminator = 0;
        while (ftell(file) < (fp.endPoint))
        {
            currWord[wordPtr] = fgetc(file);

            if ((isalpha(currWord[wordPtr]) || isdigit(currWord[wordPtr])) && !prevCharIsWordTerminator)
            {
                wordPtr++;
            }
            else if (isWordTerminator(currWord[wordPtr]) && !prevCharIsWordTerminator)
            {
                currWord[wordPtr] = '\0';

                for (int n = 0; currWord[n]; n++)
                {
                    currWord[n] = tolower(currWord[n]);
                }

                addOccurrency(list, currWord, &wordCount);
                wordPtr = 0;
                prevCharIsWordTerminator = 1;
            }
            else if (isWordTerminator(currWord[wordPtr]) && prevCharIsWordTerminator)
            {
            } // Ignoro i doppi terminatori
            else if ((isalpha(currWord[wordPtr]) || isdigit(currWord[wordPtr])) && prevCharIsWordTerminator)
            {
                wordPtr++;
                prevCharIsWordTerminator = 0;
            }
        }
    }
    wordArr = getWordArrayFromList(list, countedWords);
    return wordArr;
}

word *getWordArrayFromList(linkedList *list, int *wordArrLength)
{
    word *wordArr;
    MPI_Alloc_mem(sizeof(word) * list->size, MPI_INFO_NULL, &wordArr);
    *wordArrLength = list->size;

    int i = 0;
    for (node *n = list->head; n != NULL; n = n->next, i++)
    {
        strncpy(wordArr[i].text, n->text, 50);
        wordArr[i].occurrencies = n->occurrencies;
    }

    return wordArr;
}

void updateMasterList(linkedList *list, word *wordArr, int wordNum)
{
    for (int i = 0; i < wordNum; i++)
    {
        updateListEntry(list, wordArr[i].text, wordArr[i].occurrencies);
    }
}

void merge(word arr[], int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    /* create temp arrays */
    word L[n1], R[n2];

    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    /* Merge the temp arrays back into arr[l..r]*/
    i = 0; // Initial index of first subarray
    j = 0; // Initial index of second subarray
    k = l; // Initial index of merged subarray
    while (i < n1 && j < n2)
    {
        if (L[i].occurrencies <= R[j].occurrencies)
        {
            arr[k] = L[i];
            i++;
        }
        else
        {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    /* Copy the remaining elements of L[], if there
    are any */
    while (i < n1)
    {
        arr[k] = L[i];
        i++;
        k++;
    }

    /* Copy the remaining elements of R[], if there
    are any */
    while (j < n2)
    {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void sortByCount(word arr[], int l, int r)
{
    if (l < r)
    {
        // Same as (l+r)/2, but avoids overflow for
        // large l and h
        int m = l + (r - l) / 2;

        // Sort first and second halves
        sortByCount(arr, l, m);
        sortByCount(arr, m + 1, r);

        merge(arr, l, m, r);
    }
}

void printOutputCSV(word *wordArr, int wordArrLength)
{
    FILE *csvFile = fopen("results.csv", "w+");
    fprintf(csvFile, "Word, Occurrencies\n");
    for (int i = (wordArrLength - 1); i >= 0; i--)
    {
        fprintf(csvFile, "%s, %d\n", wordArr[i].text, wordArr[i].occurrencies);
    }
}