#include "wordManagement.h"

word *newWord(char *text, int count){
    word *word = malloc(sizeof(word));
    strncpy(word->text, word, 50);
    word->count = count;
    return word;
}

void newWordDatatype(MPI_Datatype *datatype){
    MPI_Datatype types[2];
    MPI_Aint displ[2], lowerBound, extent;
    int blocks[2];

    displ[0] = 0;
    blocks[0] = 50;
    types[0] = MPI_CHAR;

    MPI_Type_get_extent(MPI_CHAR, &lowerBound, &extent);

    displ[1] = 50 * extent;
    blocks[1] = 1;
    types[1] = MPI_INT;

    MPI_Type_create_struct(2, blocks, displ, types, datatype);
    MPI_Type_commit(datatype);
}

int isWordTerminator(char c){
    if(c == '\t' || c == '\r' || c == '\n' || c == ' ' || c == EOF) return 1;
    return 0;
}

void addOccurrency(GHashTable* hashTable, char *word, int *differentWords){
    gpointer gp;
    int n;

    gp = g_hash_table_lookup(hashTable, word);

    if(gp != NULL) n = GPOINTER_TO_INT(gp) + 1;
    else{
        n = 1;
        *differentWords += 1;
    }
    g_hash_table_insert(hashTable, word, GINT_TO_POINTER(n));
}

word *createArrayFromTable(GHashTable* hashTable){
    gpointer gp;
    word *wordArr;
    int keyNum, occurrencies;
    char *currWord, **keys = (char**) g_hash_table_get_keys_as_array(hashTable, &keyNum);
    
    MPI_Alloc_mem(sizeof(word) * keyNum, MPI_INFO_NULL , &wordArr);

    for (int i = 0; i < keyNum; i++){
        currWord = keys[i]; 
        strncpy(wordArr[i].text, currWord, 50);

        gp = g_hash_table_lookup(hashTable, currWord); 
        occurrencies =  GPOINTER_TO_INT(gp);
        wordArr[i].occurrencies = occurrencies;

        free(currWord);
    }
    free(keys);
    g_hash_table_destroy(hashTable);
 
    return wordArr;
}

word *getWordOccurrencies(filePart *parts, int partNum, int *countedWords) {
    FILE *file;
    filePart fp;
    word *wordArr;
    char currWord[50], currChar;
    int wordPtr = 0, wordNum = 0;

    GHashTable* hashTable = g_hash_table_new(g_str_hash, g_str_equal);
    
    for(int i = 0; i < partNum; i++){
        fp = parts[i];
        file = fopen(fp.filePath , "r");
        fseek(file, fp.startPoint , SEEK_CUR);

        if(ftell(file) != 0){ //Se non è l'inizio del file...
            fseek(file,  -1 , SEEK_CUR);
            currChar = fgetc(file);
            if(!isWordTerminator(currChar)){ //... e se il carattere che precede l'inizio della porzione non è un terminatore...
                while (!isWordTerminator(currChar)){
                    fseek(file,  -1 , SEEK_CUR); //... leggo all'indietro fino al primo terminatore.
                    currChar = fgetc(file);
                }
            }
        }
        
        int prevCharIsWordTerminator = 0;  
        while(ftell(file) < (fp.endPoint))
        {
            currWord[wordPtr] = fgetc(file);
            
            if((isalpha(currWord[wordPtr]) || isdigit(currWord[wordPtr])) && !prevCharIsWordTerminator){
                wordPtr++;  
            } 
            else if(isWordTerminator(currWord[wordPtr]) && !prevCharIsWordTerminator) {
                currWord[wordPtr] = '\0';

                for(int n = 0; currWord[n]; n++){
                    currWord[n] = tolower(currWord[n]);
                }
                
                addOccurrency(hashTable , currWord, &wordNum);
                wordPtr = 0;
                prevCharIsWordTerminator = 1;
            }
            else if(isWordTerminator(currWord[wordPtr]) && prevCharIsWordTerminator) {} //Ignoro i doppi terminatori
            else if((isalpha(currWord[wordPtr]) || isdigit(currWord[wordPtr])) && prevCharIsWordTerminator) {
                wordPtr++;
                prevCharIsWordTerminator = 0;
            }
        }
    }
    wordArr = createArrayFromTable(hashTable);
    *countedWords = wordNum; 

    return wordArr;
}

void sortByCount(word a[], int start, int end){
    int q;
    int elemNum = end - start;
    if (start < end) {
        q = (start + end)/2;
        sortByCount(a, start, q);
        sortByCount(a, q+1, end);
        merge(a, start, q, end, elemNum);
    }
    return;
}

void merge(word a[], int p, int q, int r, int elemNum) {
    int i, j, k=0;
    word b[elemNum];
    i = p;
    j = q+1;

    while (i<=q && j<=r) {
        if (a[i].occurrencies < a[j].occurrencies) {
            b[k] = a[i];
            i++;
        } else {
            b[k] = a[j];
            j++;
        }
        k++;
    }

    while (i <= q) {
        b[k] = a[i];
        i++;
        k++;
    }

    while (j <= r) {
        b[k] = a[j];
        j++;
        k++;
    }

    for (k=p; k<=r; k++) a[k] = b[k-p];
    return;
}