/*
    + Hypothesis 1: Each process sees the same files on the disk
    + Hypothesis 2: Each file has a single word per line (this constraint can be
   optionally relaxed)
    + Master fetch all files name and stores them.
    + Master reads all files, and get each lines number
    + Master reads all files, and get TOTAL lines number, say 1000.
    + Master make the chunks, so that each process receive an equal
   number of lines. Say we have 4 processes, each of them should work on 250
   lines, but they are distributed differenly in the files.
    + Master prepares sendCount and displs
    - Master scatters sendCount
    - Master scatters the chunks
    - Each process launch the wordsCount function on each file, according to its
   work range. wordsCount access the file according to the given name and fetch
   all work lines. wordsCount return an array of structs like this: (word string
   - occurances)
    - Among all the calls a process made, it should merge all the returned
   structs, creating a single array of structs. Then it sends back to the master
    - Master receive all data and call a custom reduce function that is the
   same merging function used by each process.
    - After everything has been tested, insert the time profiling code.
    - Run some benchmarks in local and record everything in a file (not striclty
   required)
    - Try to make the slaves read file lines instead of master only and compare
    - Make a script that launches the compilation and that launches the program
   multiple times, according to the strong and weak scalability requirements
   - Optionally, make different files, builded together with make. The make
   launch is done in the script
    - Run some benchmarks on EC2 instances and record everything in a file
   (strinctly required)
    - Make another similar script that setups the whole cloud environment
    - With the cloud benchmark file, make a Python script that plots the results
   according to the requirements
    - Make a readme that summarizes: the problem, all the choices and rationale,
   all the necessary setup and consideration about benchmarks
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
#define LINE_LIMIT 655535
#define FILE_NAME_SIZE 256
#define DEFAULT_FILE_LOCATION "./files/"

// TODO Creare una struttura fatta da nome file e intero numero linee

typedef struct FileName {
  char *fileName;
  long int lineNumber;
} t_FileName;

typedef struct ChunkNode {
  t_FileName *fileName;
  long int startLine;
  long int endLine;
  struct ChunkNode *next;
} t_ChunkNode;

typedef struct Chunk {
  char *fileName;
  long int startLine;
  long int endLine;
} t_Chunk;

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
      fileNamesPtr->fileName = (char *)malloc(FILE_NAME_SIZE);
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
  printf("Standard chunk size: %ld\n", standardChunkSize);
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
      printf("New chunk for file %s (%ld)\n", chunkPtr->fileName->fileName,
             chunkPtr->fileName->lineNumber);

      // Compute the amount of lines that can be used
      long int actualChunkSize;
      printf("leftLines: %ld, nextChunkSize: %ld\n", leftLines, nextChunkSize);
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
    (*chunkArray)[i].fileName = chunkPtr->fileName->fileName;
    (*chunkArray)[i].startLine = chunkPtr->startLine;
    (*chunkArray)[i].endLine = chunkPtr->endLine;
    chunkPtr = chunkPtr->next;
  }
  return chunkNumber;
}

void printChunkArray(t_Chunk *chunkArray, int chunkNumber) {
  for (int i = 0; i < chunkNumber; i++) {
    t_Chunk chunk = chunkArray[i];
    printf("Chunk - %s of %ld lines (%ld to %ld)\n", chunk.fileName,
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
#ifdef DEBUG
    printf("Process %d will receive %d chunk(s)\n", i, chunkToSend);
#endif

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
  for (int i = 1; i < p; i++) {
    displs[i] = sendCounts[i - 1] + displs[i - 1];
  }
}

int main(int argc, char *argv[]) {
  // Init phase
  int rank, p;
  int *sendCount, *displs;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  if (rank == 0) {
    // Fetch all files name and store them. Files are located in directory
    // "./files/", relative to the CWD (this is important in the readme)
    t_FileName *fileNames = NULL;
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

      // Convert list into array because it is easier to scatter
      t_Chunk *chunkArray;
      int chunkNumber = convertToArray(&chunkArray, chunkList);

#ifdef VERBOSE
      printf("\n");
      printChunkArray(chunkArray, chunkNumber);
      printf("\n");
#endif

      // Prepare sendCounts and displs for the scatter
      sendCount = (int *)calloc(p, sizeof(int));
      displs = (int *)calloc(p, sizeof(int));
      prepareSendCountAndDispls(sendCount, displs, chunkArray, chunkNumber,
                                totalLineNumber, p);

      // TODO Free everything in the end
      free(fileNames);
      free(chunkArray);
    }
  }

  // TODO Scatter sendCount values first. Reuse simple_word_count solution

  // Scatter the work files

  MPI_Finalize();
  return 0;
}