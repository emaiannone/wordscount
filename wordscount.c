/*
    + Hypothesis 1: Each process sees the same files on the disk
    + Hypothesis 2: Each file has a single word per line (this constraint can be
   optionally relaxed)
   --- Part 1: Core algorithm---
    + Master fetch all files name and stores them.
    + Master reads all files, and get each lines number
    + Master reads all files, and get TOTAL lines number, say 1000.
    + Master make the chunks, so that each process receive an equal
   number of lines. Say we have 4 processes, each of them should work on 250
   lines, but they are distributed differenly in the files.
    + Master prepares sendCount and displs
    + Master scatters sendCount
    + Each process create and commit the custom datatype
    + Master scatters the chunks
    + Each process launches the wordsCount function its chunks, according to
   its work range. wordsCount access the file according to the given name and
   fetch all work lines. wordsCount return an list of structs like this: (word
   string - occurances)
    - Master receive all data and call a custom reduce function that merges the
   results of each process.
   --- Part 2: Local Beanchmark---
    - After everything has been tested, insert the time profiling code.
    - Run some benchmarks in local and write down these data into a file (not
   striclty required)
   --- Part 3: Building---
    - Make a script that launches the compilation and that launches the program
   multiple times, according to the strong and weak scalability requirements
   - Optionally, make different files, builded together with make. The make
   launch is done in the script
   --- Part 4: Cloud Benchmark---
    - Make another similar script that setups the whole cloud environment
    - Run some benchmarks on EC2 instances
   --- Part 5: Beanchmark Graph---
    - With the cloud benchmark file, make a Python script that plots the results
   according to the requirements
   --- Part 6: Pack everything---
    - Make a readme that summarizes: the problem, all the choices and rationale,
   all the necessary setup and consideration about benchmarks.
    - Put everything needed in a tar archive.
   ---Optional---
    - Try to make the slaves read file lines instead of master only and
   compare
*/

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mpi.h"
#define TAG 0
#define TIME
#define DEBUG
#define VERBOSE
#define LINE_LIMIT 65535
#define FILE_NAME_SIZE 256
#define DEFAULT_FILE_LOCATION "./files/"

// TODO SIstema queste strutture con alcune più intelligenti

typedef struct FileName {
  char fileName[FILE_NAME_SIZE];
  long int lineNumber;
} t_FileName;

typedef struct ChunkNode {
  t_FileName *fileName;
  long int startLine;
  long int endLine;
  struct ChunkNode *next;
} t_ChunkNode;

typedef struct Chunk {
  char fileName[FILE_NAME_SIZE];
  long int startLine;
  long int endLine;
} t_Chunk;

// Nel readme dire che l'uso di una hashmap sarebbe ottimale
typedef struct Word {
  char word[LINE_LIMIT];
  long int occurances;
} t_Word;

typedef struct WordNode {
  t_Word word;
  struct WordNode *next;
} t_WordNode;

// fileNames will be an array pointer to a list of FileNames; the return value
// is the file number
int getFiles(t_FileName **fileNames) {
  DIR *dir = NULL;
  dir = opendir(DEFAULT_FILE_LOCATION);
  if (dir == NULL) {
    return 0;
  }
  struct dirent *entry;

  // Count file number
  int count = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type != DT_DIR) {
      count++;
    }
  }
  closedir(dir);

  if (count == 0) {
    return 0;
  }
  dir = opendir(DEFAULT_FILE_LOCATION);
  if (dir == NULL) {
    return 0;
  }
  printf("Found %d files\n", count);
  *fileNames = (t_FileName *)calloc(count, sizeof(t_FileName));

  t_FileName *fileNamesPtr = *fileNames;
  while ((entry = readdir(dir)) != NULL) {
    // Only for files
    if (entry->d_type != DT_DIR) {
      // Initialization of the struct
      strcpy(fileNamesPtr->fileName, entry->d_name);
      fileNamesPtr->lineNumber = 0;
      fileNamesPtr++;
    }
  }
  closedir(dir);
  return count;
}

long int getLinesNumber(t_FileName *fileNames, int fileNumber) {
  // Read all files using the fileNames
  t_FileName *fileNamesPtr = fileNames;
  long int totalLineNumber = 0;
  char *selectedFile;
  for (int i = 0; i < fileNumber; i++) {
    char *fileFullName = (char *)calloc(
        strlen(DEFAULT_FILE_LOCATION) + strlen(fileNamesPtr->fileName) + 1,
        sizeof(char));
    strcpy(fileFullName, DEFAULT_FILE_LOCATION);
    strcat(fileFullName, fileNamesPtr->fileName);
    printf("Opening file %s\n", fileFullName);
    FILE *fp = fopen(fileFullName, "r");
    if (fp == NULL) {
      fprintf(stderr, "Error in opening file %s\n", fileNamesPtr->fileName);
    } else {
      char *buf = (char *)malloc(LINE_LIMIT);
      while (fgets(buf, LINE_LIMIT, fp) != NULL) {
        fileNamesPtr->lineNumber++;
        totalLineNumber++;
      }
      fclose(fp);
    }
    fileNamesPtr++;
  }
  return totalLineNumber;
}

void printLinesNumber(t_FileName *fileNames, int fileNumber) {
  t_FileName *fileNamesPtr = fileNames;
  for (int i = 0; i < fileNumber; i++) {
    printf("%s has %ld lines\n", fileNamesPtr->fileName,
           fileNamesPtr->lineNumber);
    fileNamesPtr++;
  }
}

t_ChunkNode *buildChunkList(t_FileName *fileNames, int fileNumber,
                            long int totalLineNumber, int p) {
  long int standardChunkSize = totalLineNumber / p;
#ifdef DEBUG
  printf("Each process should work on roughtly: %ld lines\n",
         standardChunkSize);
#endif
  int remainder = totalLineNumber % p;

  long int nextChunkSize = standardChunkSize;
  if (remainder > 0) {
    nextChunkSize++;
    remainder--;
  }
  t_FileName *fileNamesPtr = fileNames;
  t_ChunkNode *firstChunkPtr = NULL, *chunkPtr = NULL;
  for (int i = 0; i < fileNumber; i++) {
    long int lastLine = -1;
    long int leftLines = fileNamesPtr->lineNumber;
    while (leftLines > 0) {
      // New chunk for this file. If it is the first, it will be the head
      if (chunkPtr == NULL) {
        chunkPtr = (t_ChunkNode *)malloc(sizeof(t_ChunkNode));
        firstChunkPtr = chunkPtr;
      } else {
        chunkPtr->next = (t_ChunkNode *)malloc(sizeof(t_ChunkNode));
        chunkPtr = chunkPtr->next;
      }
      chunkPtr->fileName = fileNamesPtr;
      chunkPtr->startLine = lastLine + 1;
      chunkPtr->next = NULL;

      // Compute the amount of lines that can be used
      long int actualChunkSize;

#ifdef VERBOSE
      printf("Created a chunk for file %s (%ld)\n",
             chunkPtr->fileName->fileName, chunkPtr->fileName->lineNumber);
      printf(
          "There are %ld lines in file %s. Next chunk will have %ld "
          "lines\n",
          leftLines, chunkPtr->fileName->fileName, nextChunkSize);
#endif
      if (leftLines >= nextChunkSize) {
        actualChunkSize = nextChunkSize;
        nextChunkSize = standardChunkSize;
        if (remainder > 0) {
          nextChunkSize++;
          remainder--;
        }
      } else {
        actualChunkSize = leftLines;
        nextChunkSize -= leftLines;
      }
      lastLine += actualChunkSize;
      chunkPtr->endLine = lastLine;
      leftLines -= actualChunkSize;
    }
    // Go to next file
    fileNamesPtr++;
  }
  if (nextChunkSize < standardChunkSize) {
    printf("There are some unassigned lines...\n");
  }
  return firstChunkPtr;
}

int convertToArray(t_Chunk **chunkArray, t_ChunkNode *chunkList) {
  // First, we need to count the number of chunks
  int chunkNumber = 0;
  t_ChunkNode *chunkPtr = chunkList;
  while (chunkPtr != NULL) {
    chunkNumber++;
    chunkPtr = chunkPtr->next;
  }
  *chunkArray = (t_Chunk *)calloc(chunkNumber, sizeof(t_Chunk));

  // Conversione
  chunkPtr = chunkList;
  for (int i = 0; i < chunkNumber; i++) {
    strcpy((*chunkArray)[i].fileName, chunkPtr->fileName->fileName);
    (*chunkArray)[i].startLine = chunkPtr->startLine;
    (*chunkArray)[i].endLine = chunkPtr->endLine;
    chunkPtr = chunkPtr->next;
  }
  return chunkNumber;
}

void printChunkArray(t_Chunk *chunkArray, int chunkNumber) {
  for (int i = 0; i < chunkNumber; i++) {
    t_Chunk chunk = chunkArray[i];
    printf("Chunk{%s of %ld lines (%ld to %ld)}; ", chunk.fileName,
           (chunk.endLine) - (chunk.startLine) + 1, chunk.startLine,
           chunk.endLine);
  }
}

void prepareSendCountAndDispls(int *sendCounts, int *displs,
                               t_Chunk *chunkArray, int chunkNumber,
                               int totalLineNumber, int p) {
  long int standardChunkSize = totalLineNumber / p;
  int remainder = totalLineNumber % p;
  long int nextChunkSize = standardChunkSize;
  if (remainder > 0) {
    nextChunkSize++;
    remainder--;
  }

  // Prepare send count array
  int chunkToSend = 0;
  long int accumulatedChunkSize = 0;
  int j = 0;
  for (int i = 0; i < p; i++) {
    // Accumulate the needed chunks
    while (j < chunkNumber && accumulatedChunkSize < nextChunkSize) {
      chunkToSend++;
      accumulatedChunkSize +=
          (chunkArray[i].endLine) - (chunkArray[i].startLine) + 1;
      j++;
    }
    // This shouldn't happen. If yes, there is a bug to fix!
    if (accumulatedChunkSize < nextChunkSize) {
      fprintf(stderr, "Error: bad work splitting. Aborting\n");
      return;
    }
    sendCounts[i] = chunkToSend;

    // Prepare everything for next process
    chunkToSend = 0;
    accumulatedChunkSize = 0;
    nextChunkSize = standardChunkSize;
    if (remainder > 0) {
      nextChunkSize++;
      remainder--;
    }
  }

  // Prepare displacement arrays
  displs[0] = 0;
#ifdef VERBOSE
  printf("displs array:\n");
  printf("dipls[%d]: %d\n", 0, displs[0]);
#endif
  for (int i = 1; i < p; i++) {
    displs[i] = sendCounts[i - 1] + displs[i - 1];
#ifdef VERBOSE
    printf("dipls[%d]: %d\n", i, displs[i]);
#endif
  }
#ifdef VERBOSE
  printf("\n");
#endif
}

void createChunkDatatype(MPI_Datatype *newDatatype) {
  int blockNumber = 3;
  int blockLengths[] = {FILE_NAME_SIZE, 1, 1};
  long int displs[] = {0, FILE_NAME_SIZE, FILE_NAME_SIZE + 8};
  MPI_Datatype types[] = {MPI_CHAR, MPI_INT64_T, MPI_INT64_T};
  MPI_Type_create_struct(blockNumber, blockLengths, displs, types, newDatatype);
  MPI_Type_commit(newDatatype);
}

long int wordscount(t_WordNode **histogram, t_Chunk *myChunks,
                    int myChunkNumber) {
  *histogram = NULL;
  long int countedWords = 0;
  char wordBuf[LINE_LIMIT];

  for (int i = 0; i < myChunkNumber; i++) {
    // File opening
    char *fileFullName = (char *)calloc(
        strlen(DEFAULT_FILE_LOCATION) + strlen(myChunks[i].fileName) + 1,
        sizeof(char));
    strcpy(fileFullName, DEFAULT_FILE_LOCATION);
    strcat(fileFullName, myChunks[i].fileName);
    FILE *fp = fopen(fileFullName, "r");

    // Add offset according to start line
    for (int j = 0; j < myChunks[i].startLine; j++) {
      fgets(wordBuf, LINE_LIMIT, fp);
    }

    // Terminate when every lines has been counter or file ends
    int wordsFile = myChunks[i].endLine - myChunks[i].startLine + 1;
    for (int j = 0; j < wordsFile && fgets(wordBuf, LINE_LIMIT, fp) != NULL;
         j++) {
      // Replace \n with \0 in order to have fewer chars
      int wordLength = strlen(wordBuf);
      if (*(wordBuf + wordLength - 1) == '\n') {
        *(wordBuf + wordLength - 1) = '\0';
      }

      // Look for the read word in the histogram. If present, increase
      // occurences, otherwise append a new node
      if (*histogram == NULL) {
        *histogram = (t_WordNode *)malloc(sizeof(t_WordNode));
        strcpy((*histogram)->word.word, wordBuf);
        (*histogram)->word.occurances = 1;
        (*histogram)->next = NULL;
      } else {
        t_WordNode *wordNodePtr = *histogram;
        while (strcmp(wordNodePtr->word.word, wordBuf) != 0 &&
               wordNodePtr->next != NULL) {
          wordNodePtr = wordNodePtr->next;
        }
        if (strcmp(wordNodePtr->word.word, wordBuf) != 0) {  // Not found
          wordNodePtr->next = (t_WordNode *)malloc(sizeof(t_WordNode));
          wordNodePtr = wordNodePtr->next;
          strcpy(wordNodePtr->word.word, wordBuf);
          wordNodePtr->word.occurances = 1;
          wordNodePtr->next = NULL;
        } else {  // Found
          wordNodePtr->word.occurances++;
        }
      }
    }
    fclose(fp);
    free(fileFullName);
    countedWords += wordsFile;
  }
  return countedWords;
}

int main(int argc, char *argv[]) {
  int rank, p;
  t_FileName *fileNames;
  t_Chunk *chunkArray;
  int *sendCount, *displs;
  MPI_Datatype MPI_T_CHUNK;

  // Init phase
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  // Master preprocessing
  if (rank == 0) {
    // Fetch all files name and store them. Files are located in directory
    // "./files/", relative to the CWD (this is important in the readme)
    fileNames = NULL;
    int fileNumber = getFiles(&fileNames);
    if (fileNames == NULL) {
      printf("No files detected. Aborted...\n");
    } else {
      // Read all files and get their line number
      long int totalLineNumber = getLinesNumber(fileNames, fileNumber);
      printf("%ld\n", totalLineNumber);

#ifdef DEBUG
      printf("\n");
      printLinesNumber(fileNames, fileNumber);
      printf("Total line number: %ld\n", totalLineNumber);
      printf("\n");
#endif

      // Prepare the chunk list
      t_ChunkNode *chunkList =
          buildChunkList(fileNames, fileNumber, totalLineNumber, p);

      // TODO Is this conversion necessary? MPI_Type_create_hindexed()?

      // Aggiungere nel readme: si è valutato l'uso della lista con un uso
      // intelligente di displs, tuttavia nel momento in cui unprocesso doveva
      // ricere due o più chunk li avrebbe presi sequenzialmente, accedendo ad
      // aree non allocate

      // Convert list into array because it is easier to scatter
      int chunkNumber = convertToArray(&chunkArray, chunkList);

#ifdef VERBOSE
      printf("\nMaster will send these chunks: ");
      printChunkArray(chunkArray, chunkNumber);
      printf("\n\n");
#endif

      // Prepare sendCounts and displs for the scatter
      sendCount = (int *)calloc(p, sizeof(int));
      displs = (int *)calloc(p, sizeof(int));
      prepareSendCountAndDispls(sendCount, displs, chunkArray, chunkNumber,
                                totalLineNumber, p);
    }
  }

  // Scatters the sendCounts so that each process knows how many data expects
  int myChunkNumber;
  MPI_Scatter(sendCount, 1, MPI_INT, &myChunkNumber, 1, MPI_INT, 0,
              MPI_COMM_WORLD);

  // Manage the custom MPI datatype
  createChunkDatatype(&MPI_T_CHUNK);

#ifdef DEBUG
  if (rank == 0) {
    int size;
    MPI_Type_size(MPI_T_CHUNK, &size);
    printf("The new datatype will be %d bytes long\n", size);
  }
#endif

  // Scatter the chunks
  t_Chunk *myChunks = (t_Chunk *)calloc(myChunkNumber, sizeof(t_Chunk));
  MPI_Scatterv(chunkArray, sendCount, displs, MPI_T_CHUNK, myChunks,
               myChunkNumber, MPI_T_CHUNK, 0, MPI_COMM_WORLD);

#ifdef DEBUG
  printf("\nProcess %d has received: ", rank);
  printChunkArray(myChunks, myChunkNumber);
#endif

  // Each process launches wordcounts procedure on its chunks
  t_WordNode *histogram;
  long int countedWords = wordscount(&histogram, myChunks, myChunkNumber);

#ifdef DEBUG
  printf("\nProcess %d counted %ld words\n", rank, countedWords);
#endif
#ifdef VERBOSE
  t_WordNode *wordNodePtr = histogram;
  while (wordNodePtr != NULL) {
    printf("Process %d counted \"%s\" for %ld times\n", rank,
           wordNodePtr->word.word, wordNodePtr->word.occurances);
    wordNodePtr = wordNodePtr->next;
  }
#endif

  // TODO Convert Words list into an array?

  // TODO Reduce

  // TODO Free everything in the end
  /*
  MPI_Type_free(&MPI_T_CHUNK);
  if (rank == 0) {
    free(fileNames);
    free(chunkArray);
    free(sendCount);
    free(displs);
  }
  */

  MPI_Finalize();
  return 0;
}