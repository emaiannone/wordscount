/*
    + Hypothesis 1: Each process sees the same files on the disk
    + Hypothesis 2: Each file has a single word per line (this constraint can be
   optionally relaxed)
    + Master fetch all files name and stores them.
    + Master reads all files, and get each lines number
    + Master reads all files, and get TOTAL lines number, say 1000.
    - Master make the partion of the work data, so each process receive an equal
   number of lines. Say we have 4 processes, each of them should work on 250
   lines, but they are distributed differenly in the files. Master should send
   the slaves (and to itself too) an array of structs like this: (a fileName
   where a process should work, starting line number, ending line number
   (inclusive)).
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

typedef struct Chunk {
  t_FileName *fileName;
  long int startLine;
  long int endLine;
  struct Chunk *next;
} t_Chunk;

// fileNames will be an array pointer to a list of FileNames; the return value
// is the file number
int getFiles(t_FileName **fileNames) {
  DIR *dir;
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
      fileNamesPtr += sizeof(t_FileName);
    }
  }
  return count;
}

int getLinesNumber(t_FileName *fileNames, int fileNumber) {
  // Read all files using the fileNames
  t_FileName *fileNamesPtr = fileNames;
  int totalLineNumber = 0;
  char *selectedFile;
  for (int i = 0; i < fileNumber; i++) {
    char *fileFullName = (char *)malloc(strlen(DEFAULT_FILE_LOCATION) +
                                        strlen(fileNamesPtr->fileName) + 1);
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
    }
    fileNamesPtr += sizeof(t_FileName);
  }
  return totalLineNumber;
}

void printLinesNumber(t_FileName *fileNames, int fileNumber) {
  t_FileName *fileNamesPtr = fileNames;
  for (int i = 0; i < fileNumber; i++) {
    printf("File %s has %ld lines\n", fileNamesPtr->fileName,
           fileNamesPtr->lineNumber);
    fileNamesPtr += sizeof(t_FileName);
  }
}

t_Chunk *buildChunkList(t_FileName *fileNames, int fileNumber,
                        int totalLineNumber, int p) {
  long int standardChunkSize = totalLineNumber / p;
#ifdef DEBUG
  printf("Standard chunk size: %ld\n", standardChunkSize);
#endif
  int remainder = totalLineNumber % p;
  t_FileName *fileNamesPtr = fileNames;
  t_Chunk *firstChunkPtr = NULL, *chunkPtr = NULL;
  for (int i = 0; i < fileNumber; i++) {
    // TODO Fix with remainder that increase the chunck Size by 1 for first
    // remainder chuncks
    long int lastLine = -1;
    long int leftLines = fileNamesPtr->lineNumber;
    while (leftLines > 0) {
      // New chunk for this file. If it is the first, it will be the head
      if (chunkPtr == NULL) {
        chunkPtr = malloc(sizeof(t_Chunk));
        firstChunkPtr = chunkPtr;
      } else {
        t_Chunk *precChunkPtr = chunkPtr;
        precChunkPtr->next = malloc(sizeof(t_Chunk));
        chunkPtr = precChunkPtr->next;
      }
      chunkPtr->fileName = malloc(sizeof(t_FileName));
      chunkPtr->fileName = fileNamesPtr;
      chunkPtr->startLine = lastLine + 1;

      // Compute the amount of lines that can be used
      long int chunkSize;
      if (leftLines > standardChunkSize) {
        chunkSize = standardChunkSize;
      } else {
        chunkSize = leftLines;
      }
      lastLine += chunkSize;
      chunkPtr->endLine = lastLine;
      leftLines -= chunkSize;
    }
    // Go to next file
    fileNamesPtr += sizeof(t_FileName);
  }
  return firstChunkPtr;
}

void printChunkList(t_Chunk *firstChunk) {
  t_Chunk *chunkPtr = firstChunk;
  while (chunkPtr != NULL) {
    printf("Chunk - %s (%ld) of %ld line (%ld to %ld)\n",
           chunkPtr->fileName->fileName, chunkPtr->fileName->lineNumber,
           chunkPtr->endLine - chunkPtr->startLine + 1, chunkPtr->startLine,
           chunkPtr->endLine);
    chunkPtr = chunkPtr->next;
  }
}

int main(int argc, char *argv[]) {
  // Init phase
  int rank, p;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  if (rank == 0) {
    // Fetch all files name and store them. Files are located in directory
    // "./files/", relative to the CWD (this is important in the readme)
    t_FileName *fileNames;
    int fileNumber = getFiles(&fileNames);
    if (fileNames == NULL) {
      printf("No files detected. Aborted...\n");
    } else {
      // Read all files and get their line number
      int totalLineNumber = getLinesNumber(fileNames, fileNumber);
#ifdef DEBUG
      printLinesNumber(fileNames, fileNumber);
      printf("Total line number: %d\n", totalLineNumber);
#endif

      // Prepare the chunk list
      t_Chunk *chunkList =
          buildChunkList(fileNames, fileNumber, totalLineNumber, p);
#ifdef VERBOSE
      printChunkList(chunkList);
#endif
      // Prepare sendCounts and displs
    }
    // Scatter sendCount values first

    // Scatter the work files
  } else {
  }
  MPI_Finalize();
  return 0;
}