/*
  Annotare nel readme:
    - l'uso di una hashmap sarebbe ottimale
    - si è valutato l'uso della lista con un uso
       intelligente di sendDispls, tuttavia nel momento in cui unprocesso
       doveva ricere due o più chunk li avrebbe presi sequenzialmente,
       accedendo ad aree non allocate
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
    + Master prepares sendCounts and sendDispls
    + Master scatters sendCounts
    + Each process create and commit the custom datatype
    + Master scatters the chunks
    + Each process launches the wordsCount function its chunks, according to
   its work range. wordsCount access the file according to the given name and
   fetch all work lines. wordsCount return an list of structs like this: (word
   string - occurances)
    + Master receive all data and call a custom reduce function that merges the
   results of each process.
    + Master prints all the results
    + Free main structures in the heap
   --- Part 2: Local Beanchmark---
    + After everything has been tested, insert the time profiling code.
    + Run some benchmarks in local and write down average of these data into a
   time log file in the CWD.
    + Create a full sequential solution when np=1. A full sequential solution
   only calls MPI_Init(), MPI_Finalize() e MPI_Wtime().
   --- Part 3: Building---
    + Insert ad additional CLI parameter that is the directory of file in order
   to easily change the directory of input files
    + Manage the error case where the CLI path does not esists
    + Insert ad additional CLI parameter that is the log file base name
    + Use different source files, builded together with make.
    + Code cleanup and optimization: split main into different functions
    + Change long int into unsigned long int and adapt code
    + Change run_all.sh to allow the two CLI parameters: dirPath and
  timeLogFile.
    + Make a benchmark script that launches make and mpirun
   multiple times, according to the strong and weak scalability requirements.
   --- Part 4: Cloud Benchmark---
    - Make another similar script that setups the whole cloud environment
    - Run some benchmarks on EC2 instances
   --- Part 5: Beanchmark Plots---
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mpi.h"
#include "wordscount_core.h"
#define TIME
//#define DEBUG
#define DEFAULT_FILE_LOCATION "./files/"
#define DEFAULT_TIME_LOG_FILE "time_log"

void createChunkDatatype(MPI_Datatype *newDatatype) {
  int blockNumber = 3;
  int blockLengths[] = {FILE_NAME_SIZE, 1, 1};
  long int displs[] = {0, FILE_NAME_SIZE, FILE_NAME_SIZE + 8};
  MPI_Datatype types[] = {MPI_CHAR, MPI_INT64_T, MPI_INT64_T};
  MPI_Type_create_struct(blockNumber, blockLengths, displs, types, newDatatype);
  MPI_Type_commit(newDatatype);
}

void createWordDatatype(MPI_Datatype *newDatatype) {
  int blockNumber = 2;
  int blockLengths[] = {LINE_LIMIT, 1};
  long int displs[] = {0, LINE_LIMIT};
  MPI_Datatype types[] = {MPI_CHAR, MPI_INT64_T};
  MPI_Type_create_struct(blockNumber, blockLengths, displs, types, newDatatype);
  MPI_Type_commit(newDatatype);
}

void printChunkArray(t_Chunk *chunkArray, int chunkNumber) {
  for (int i = 0; i < chunkNumber; i++) {
    t_Chunk chunk = chunkArray[i];
    printf("Chunk{%s of %ld lines (%ld to %ld)};\n", chunk.fileName,
           (chunk.endLine) - (chunk.startLine) + 1, chunk.startLine,
           chunk.endLine);
  }
}

void prepareSendCounts(int *sendCounts, t_Chunk *chunkArray, int chunkArraySize,
                       int totalLines, int p) {
  unsigned long int standardChunkSize = totalLines / p;
  int remainder = totalLines % p;

  // Index for chunks
  int j = 0;
  for (int i = 0; i < p; i++) {  // For each process
    // Prepare everything for this process
    int chunkToSend = 0;
    unsigned long int accumulatedChunkSize = 0;
    unsigned long int nextChunkSize = standardChunkSize;
    if (remainder > 0) {
      nextChunkSize++;
      remainder--;
    }

    // Accumulate the needed chunks
    while (j < chunkArraySize && accumulatedChunkSize < nextChunkSize) {
      chunkToSend++;
      accumulatedChunkSize +=
          (chunkArray[j].endLine) - (chunkArray[j].startLine) + 1;
      j++;
    }

    // This shouldn't happen. If yes, there is a bug to fix!
    if (accumulatedChunkSize < nextChunkSize) {
      printf("Error: bad work splitting for process %d. Aborting\n", i);
      printf("Accumulated %ld lines, but required %ld\n", accumulatedChunkSize,
             nextChunkSize);
      return;
    }
    sendCounts[i] = chunkToSend;
  }
}

void prepareDispls(int *displs, int *counts, int size) {
  displs[0] = 0;
  for (int i = 1; i < size; i++) {
    displs[i] = counts[i - 1] + displs[i - 1];
  }
}

int main(int argc, char *argv[]) {
  double globalStartTime = MPI_Wtime();
  double initStartTime, scatterStartTime, wordcountStartTime,
      gatheringStartTime, compactStartTime;
  double initTime, scatterTime, wordcountTime, gatheringTime, compactTime;

  int rank, p;
  MPI_Datatype MPI_T_CHUNK, MPI_T_WORD;

  char dirPath[FILE_NAME_SIZE], timelogFilePath[FILE_NAME_SIZE];
  t_FileName *fileNames;
  int canStart;

  t_Chunk *chunkArray;
  int *sendCounts, *recvCounts, *sendDispls, *recvDispls;
  t_Word *extendedHistogramArray;
  int extendedWordNumber;

  // Init phase
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  // Create the custom MPI datatypes
  createChunkDatatype(&MPI_T_CHUNK);
  createWordDatatype(&MPI_T_WORD);

  // File directory path and time log file path from CLI if specified (default
  // is files directory in CWD)
  strcpy(dirPath, DEFAULT_FILE_LOCATION);
  strcpy(timelogFilePath, DEFAULT_TIME_LOG_FILE);
  if (argc >= 2) {
    char *basePath = malloc(2 + sizeof(argv[1]) + 1 + 1);
    strcpy(basePath, "./");
    strcat(basePath, argv[1]);
    strcat(basePath, "/");
    strcpy(dirPath, basePath);
    free(basePath);
    if (argc >= 3) {
      strcpy(timelogFilePath, argv[2]);
    }
  }

  // Master preprocessing
  if (rank == 0) {
#ifdef DEBUG
    int typeSize;
    MPI_Type_size(MPI_T_CHUNK, &typeSize);
    printf("MPI_T_CHUNK will be %d bytes long\n", typeSize);
    MPI_Type_size(MPI_T_WORD, &typeSize);
    printf("MPI_T_WORD will be %d bytes long\n", typeSize);
    printf("\nReading files...\n");
#endif

    initStartTime = MPI_Wtime();
    // No problem for now
    canStart = 1;
    // Fetch all files name
    fileNames = NULL;
    int fileNumber = getFilesName(&fileNames, dirPath);
    // Check if program can stop earlier through canStart
    if (fileNames == NULL || fileNumber == 0) {
      canStart = 0;
      printf("No files found in directory %s\n", dirPath);
    } else {
      // Read each file and get the total lines
      unsigned long int totalLines =
          getLinesNumber(fileNames, fileNumber, dirPath);

#ifdef DEBUG
      printf("Found %d files:\n", fileNumber);
      t_FileName *fileNamesPtr = fileNames;
      for (int i = 0; i < fileNumber; i++) {
        printf("%s has %ld lines\n", fileNamesPtr->fileName,
               fileNamesPtr->lineNumber);
        fileNamesPtr++;
      }
      printf("Total lines: %ld\n", totalLines);
      printf("Each process will work on roughtly %ld lines\n\n",
             totalLines / p);
#endif

      // Prepare the chunk list and convert it into an array: easier to scatter
      t_ChunkNode *chunkList =
          buildChunkList(fileNames, fileNumber, totalLines, p);
      if (chunkList == NULL) {
        canStart = 0;
      } else {
        int chunkArraySize = chunksToArray(&chunkArray, chunkList);
#ifdef DEBUG
        printf("\nMaster will send these chunks:\n");
        printChunkArray(chunkArray, chunkArraySize);
        printf("\n");
#endif
        // Prepare sendCounts and sendDispls for the scatter
        sendCounts = (int *)calloc(p, sizeof(int));
        sendDispls = (int *)calloc(p, sizeof(int));
        prepareSendCounts(sendCounts, chunkArray, chunkArraySize, totalLines,
                          p);
        prepareDispls(sendDispls, sendCounts, p);
#ifdef DEBUG
        printf("Master will use these arrays for scattering:\n");
        printf("Send counts: ");
        for (int i = 0; i < p; i++) {
          printf("%d, ", sendCounts[i]);
        }
        printf("\nSend displs: ");
        for (int i = 0; i < p; i++) {
          printf("%d, ", sendDispls[i]);
        }
        printf("\n\n");
#endif
      }
    }
#ifdef TIME
    initTime = MPI_Wtime() - initStartTime;
#endif
  }
  // --------------------------Everyone runs here----------------------------
  // If the master found at least one file, the algorithm can go on
  if (p != 1) {
    MPI_Bcast(&canStart, 1, MPI_INT, 0, MPI_COMM_WORLD);
  }
  if (canStart) {
    scatterStartTime = MPI_Wtime();
    int myChunkNumber;
    t_Chunk *myChunks;
    if (p != 1) {
      // Scatters sendCounts so that each process knows how many chunks expects
      MPI_Scatter(sendCounts, 1, MPI_INT, &myChunkNumber, 1, MPI_INT, 0,
                  MPI_COMM_WORLD);
      // Scatter the chunks
      myChunks = (t_Chunk *)calloc(myChunkNumber, sizeof(t_Chunk));
      MPI_Scatterv(chunkArray, sendCounts, sendDispls, MPI_T_CHUNK, myChunks,
                   myChunkNumber, MPI_T_CHUNK, 0, MPI_COMM_WORLD);
    } else {
      myChunkNumber = sendCounts[0];
      myChunks = chunkArray;
    }
#ifdef TIME
    scatterTime = MPI_Wtime() - scatterStartTime;
#endif
#ifdef DEBUG
    printf("P%d) Received:\n", rank);
    printChunkArray(myChunks, myChunkNumber);
    printf("\n");
#endif

    wordcountStartTime = MPI_Wtime();
    // Each process launches wordcounts procedure on its chunks
    t_WordNode *myHistogramList;
    unsigned long int countedWords =
        wordscount(&myHistogramList, myChunks, myChunkNumber, dirPath);
#ifdef TIME
    wordcountTime = MPI_Wtime() - wordcountStartTime;
#endif
    // Convert histogram list into an array
    t_Word *myHistogramArray;
    int myWordNumber = wordsToArray(&myHistogramArray, myHistogramList);
#ifdef DEBUG
    printf("P%d) My histogram (%ld): ", rank, countedWords);
    for (int i = 0; i < myWordNumber; i++) {
      printf("%s(%ld); ", myHistogramArray[i].word,
             myHistogramArray[i].occurances);
    }
    printf("\n\n");
#endif

    gatheringStartTime = MPI_Wtime();
    // Gather all wordNumbers
    if (p != 1) {
      if (rank == 0) {
        recvCounts = (int *)calloc(p, sizeof(int));
      }
      MPI_Gather(&myWordNumber, 1, MPI_INT, recvCounts, 1, MPI_INT, 0,
                 MPI_COMM_WORLD);
      if (rank == 0) {
        // TODO Questa creazione può essere messa in una
        // funzione a parte dato che è fatta ugualmente anche in un altro caso
        recvDispls = (int *)calloc(p, sizeof(int));
        prepareDispls(recvDispls, recvCounts, p);
#ifdef DEBUG
        printf("Master will use these arrays for gathering:\n");
        printf("Recv counts: ");
        for (int i = 0; i < p; i++) {
          printf("%d, ", recvCounts[i]);
        }
        printf("\nRecv displs: ");
        for (int i = 0; i < p; i++) {
          printf("%d, ", recvDispls[i]);
        }
        printf("\n\n");
#endif
        // Sum recvCounts for the typeSize of extendedHistogramArray
        extendedWordNumber = 0;
        for (int i = 0; i < p; i++) {
          extendedWordNumber += recvCounts[i];
        }
        // Prepare the extended array
        extendedHistogramArray =
            (t_Word *)calloc(extendedWordNumber, sizeof(t_Word));
      }
      // Gather all histograms into extended array
      MPI_Gatherv(myHistogramArray, myWordNumber, MPI_T_WORD,
                  extendedHistogramArray, recvCounts, recvDispls, MPI_T_WORD, 0,
                  MPI_COMM_WORLD);
    } else {
      // Shortcut that avoids gathering
      extendedHistogramArray = myHistogramArray;
      extendedWordNumber = myWordNumber;
    }
#ifdef TIME
    gatheringTime = MPI_Wtime() - gatheringStartTime;
#endif

    if (rank == 0) {
#ifdef DEBUG
      printf("Master received this extended histogram: ");
      for (int i = 0; i < extendedWordNumber; i++) {
        printf("%s(%ld); ", extendedHistogramArray[i].word,
               extendedHistogramArray[i].occurances);
      }
      printf("\n\n");
#endif
      // Compact the final histogram
      compactStartTime = MPI_Wtime();
      t_WordNode *finalHistogramList = NULL;
      compactHistogram(&finalHistogramList, extendedHistogramArray,
                       extendedWordNumber);
#ifdef TIME
      compactTime = MPI_Wtime() - compactStartTime;
#endif

      t_WordNode *finalHistogramPtr = finalHistogramList;
      printf("Master compacted the histogram: ");
      while (finalHistogramPtr != NULL) {
        printf("%s(%ld); ", finalHistogramPtr->word.word,
               finalHistogramPtr->word.occurances);
        finalHistogramPtr = finalHistogramPtr->next;
      }
      printf("\n\n");
    }

    // Free everything
    MPI_Type_free(&MPI_T_CHUNK);
    MPI_Type_free(&MPI_T_WORD);
    free(myHistogramArray);
    free(myChunks);
    if (rank == 0) {
      free(fileNames);
      free(sendCounts);
      free(sendDispls);
      // For p == 1 chunkArray buffer is pointed by myChunks
      if (p != 1) {
        free(recvCounts);
        free(recvDispls);
        free(chunkArray);
        free(extendedHistogramArray);
      }
    }

#ifdef TIME
    // Print parallel part times
    double globalTime = MPI_Wtime() - globalStartTime;
    if (p != 1) {
      printf("P%d) Scattering, %f\n", rank, scatterTime);
    }
    printf("P%d) Wordscount, %f\n", rank, wordcountTime);
    if (p != 1) {
      printf("P%d) Gathering, %f\n", rank, gatheringTime);
    }
    printf("P%d) Global, %f\n\n", rank, globalTime);

    // Reduce all time data as average
    double scatterTimeAvg = 0, wordcountTimeAvg = 0, gatheringTimeAvg = 0,
           globalTimeAvg = 0;
    if (p != 1) {
      MPI_Reduce(&scatterTime, &scatterTimeAvg, 1, MPI_DOUBLE, MPI_SUM, 0,
                 MPI_COMM_WORLD);
      MPI_Reduce(&wordcountTime, &wordcountTimeAvg, 1, MPI_DOUBLE, MPI_SUM, 0,
                 MPI_COMM_WORLD);
      MPI_Reduce(&gatheringTime, &gatheringTimeAvg, 1, MPI_DOUBLE, MPI_SUM, 0,
                 MPI_COMM_WORLD);
      MPI_Reduce(&globalTime, &globalTimeAvg, 1, MPI_DOUBLE, MPI_SUM, 0,
                 MPI_COMM_WORLD);
      scatterTimeAvg /= p;
      wordcountTimeAvg /= p;
      gatheringTimeAvg /= p;
      globalTimeAvg /= p;
    } else {
      scatterTimeAvg = scatterTime;
      wordcountTimeAvg = wordcountTime;
      gatheringTimeAvg = gatheringTime;
      globalTimeAvg = globalTime;
    }

    if (rank == 0) {
      // Print average times
      printf("Init, %f\n", initTime);
      if (p != 1) {
        printf("Average Scattering, %f\n", scatterTimeAvg);
      }
      printf("Average Wordscount, %f\n", wordcountTimeAvg);
      if (p != 1) {
        printf("Average Gathering, %f\n", gatheringTimeAvg);
      }
      printf("Compaction, %f\n", compactTime);
      printf("Average Global, %f\n", globalTimeAvg);

      // Write average times
      FILE *fp = fopen(timelogFilePath, "a");
      fprintf(fp, "---%d process(es) in %s directory---\n", p, dirPath);
      fprintf(fp, "Init, %f\n", initTime);
      if (p != 1) {
        fprintf(fp, "Average Scattering, %f\n", scatterTimeAvg);
      }
      fprintf(fp, "Average Wordscount, %f\n", wordcountTimeAvg);
      if (p != 1) {
        fprintf(fp, "Average Gathering, %f\n", gatheringTimeAvg);
      }
      fprintf(fp, "Compaction, %f\n", compactTime);
      fprintf(fp, "Average Global, %f\n", globalTimeAvg);
      fclose(fp);
    }
#endif
  } else {
    printf("P%d) Aborting...\n", rank);
  }
  MPI_Finalize();
  return 0;
}