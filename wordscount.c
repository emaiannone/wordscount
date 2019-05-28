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
    - Code cleanup and optimization!
    - Make a script that launches make and mpirun
   multiple times, according to the strong and weak scalability requirements.
   Record each time profiling into different time logs
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mpi.h"
#include "wordscount_core.h"
#define TIME
#define DEBUG
#define DEFAULT_FILE_LOCATION "./files/"
#define DEFAULT_TIME_LOG_FILE "time_log.txt"

// Nel readme dire che l'uso di una hashmap sarebbe ottimale

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

void prepareSendCountAndDispls(int *sendCounts, int *sendDispls,
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
  sendDispls[0] = 0;
#ifdef DEBUG
  printf("sendDispls: ");
  printf("%d, ", sendDispls[0]);
#endif
  for (int i = 1; i < p; i++) {
    sendDispls[i] = sendCounts[i - 1] + sendDispls[i - 1];
#ifdef DEBUG
    printf("%d, ", sendDispls[i]);
#endif
  }
#ifdef DEBUG
  printf("\n\n");
#endif
}

int main(int argc, char *argv[]) {
  double globalStartTime = MPI_Wtime();
  double initStartTime, scatterStartTime, wordcountStartTime,
      gatheringStartTime, compactStartTime;
  double initTime, scatterTime, wordcountTime, gatheringTime, compactTime;
  int rank, p;
  char dirPath[FILE_NAME_SIZE], timelogFile[FILE_NAME_SIZE];
  int canStart;
  t_FileName *fileNames;
  t_Chunk *chunkArray;
  int *sendCounts, *recvCounts, *sendDispls, *recvDispls;
  t_Word *extendedHistogramArray;
  int extendedWordNumber;
  MPI_Datatype MPI_T_CHUNK, MPI_T_WORD;

  // Init phase
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  // Create the custom MPI datatypes
  createChunkDatatype(&MPI_T_CHUNK);
  createWordDatatype(&MPI_T_WORD);

  // File directory from CLI if specified
  strcpy(dirPath, DEFAULT_FILE_LOCATION);
  strcpy(timelogFile, DEFAULT_TIME_LOG_FILE);
  if (argc >= 2) {
    char *basePath = malloc(2 + sizeof(argv[1]) + 1 + 1);
    strcpy(basePath, "./");
    strcat(basePath, argv[1]);
    strcat(basePath, "/");
    strcpy(dirPath, basePath);
    free(basePath);
    if (argc >= 3) {
      strcpy(timelogFile, argv[2]);
    }
  }

  // Master preprocessing
  if (rank == 0) {
#ifdef DEBUG
    int size;
    MPI_Type_size(MPI_T_CHUNK, &size);
    printf("MPI_T_CHUNK will be %d bytes long\n", size);
    MPI_Type_size(MPI_T_WORD, &size);
    printf("MPI_T_WORD will be %d bytes long\n", size);
#endif

    initStartTime = MPI_Wtime();
    // Fetch all files name and store them. Files are located in directory
    // "./files/", relative to the CWD (this is important in the readme)
    fileNames = NULL;
    int fileNumber = getFilesName(&fileNames, dirPath);
    if (fileNames == NULL) {
      canStart = 0;
      printf("No files found in directory %s\n", dirPath);
    } else {
      canStart = 1;
      // Read all files and get their line number
      long int totalLineNumber = getLinesNumber(fileNames, fileNumber, dirPath);

#ifdef DEBUG
      t_FileName *fileNamesPtr = fileNames;
      for (int i = 0; i < fileNumber; i++) {
        printf("%s has %ld lines\n", fileNamesPtr->fileName,
               fileNamesPtr->lineNumber);
        fileNamesPtr++;
      }
      printf("Total lines: %ld\n\n", totalLineNumber);
#endif

      // Prepare the chunk list
      t_ChunkNode *chunkList =
          buildChunkList(fileNames, fileNumber, totalLineNumber, p);

      // TODO Is this conversion necessary? MPI_Type_create_hindexed()?
      // Aggiungere nel readme: si è valutato l'uso della lista con un uso
      // intelligente di sendDispls, tuttavia nel momento in cui unprocesso
      // doveva ricere due o più chunk li avrebbe presi sequenzialmente,
      // accedendo ad aree non allocate

      // Convert list into array because it is easier to scatter
      int chunkNumber = chunksToArray(&chunkArray, chunkList);

#ifdef DEBUG
      printf("Master will send these chunks: ");
      printChunkArray(chunkArray, chunkNumber);
      printf("\n\n");
#endif

      // Prepare sendCounts and sendDispls for the scatter
      sendCounts = (int *)calloc(p, sizeof(int));
      sendDispls = (int *)calloc(p, sizeof(int));
      prepareSendCountAndDispls(sendCounts, sendDispls, chunkArray, chunkNumber,
                                totalLineNumber, p);
    }
  }
  // Everyone run here

  // TODO add code for p == 1 that avoid MPI_Calls and resuse the same solution

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
    printf("%d) Received: ", rank);
    printChunkArray(myChunks, myChunkNumber);
    printf("\n");
#endif

    wordcountStartTime = MPI_Wtime();
    // Each process launches wordcounts procedure on its chunks
    t_WordNode *myHistogramList;
    long int countedWords =
        wordscount(&myHistogramList, myChunks, myChunkNumber, dirPath);

#ifdef TIME
    wordcountTime = MPI_Wtime() - wordcountStartTime;
#endif
#ifdef DEBUG
    printf("%d) Counted %ld words\n", rank, countedWords);
    t_WordNode *wordNodePtr = myHistogramList;
    while (wordNodePtr != NULL) {
      printf("%d) counted \"%s\" for %ld times\n", rank, wordNodePtr->word.word,
             wordNodePtr->word.occurances);
      wordNodePtr = wordNodePtr->next;
    }
#endif

    // Convert histogram list into an array
    t_Word *myHistogramArray;
    int myWordNumber = wordsToArray(&myHistogramArray, myHistogramList);

#ifdef DEBUG
    printf("%d) Histogram: ", rank);
    for (int i = 0; i < myWordNumber; i++) {
      printf("%s(%ld); ", myHistogramArray[i].word,
             myHistogramArray[i].occurances);
    }
    printf("\n\n");
#endif

    gatheringStartTime = MPI_Wtime();
    // Gather all wordNumbers
    if (rank == 0) {
      recvCounts = (int *)calloc(p, sizeof(int));
      recvDispls = (int *)calloc(p, sizeof(int));
    }
    if (p != 1) {
      MPI_Gather(&myWordNumber, 1, MPI_INT, recvCounts, 1, MPI_INT, 0,
                 MPI_COMM_WORLD);
    } else {
      myWordNumber = recvCounts[0];
    }

    // TODO Fondere questo grande if per il master con quello di poco sopra?
    if (rank == 0) {
#ifdef DEBUG
      printf("recvCounts: ");
      for (int i = 0; i < p; i++) {
        printf("%d, ", recvCounts[i]);
      }
      printf("\n");
#endif

      // TODO Questa creazione può essere messa in una
      // funzione a parte dato che è fatta ugualmente anche in un altro caso
      recvDispls[0] = 0;
#ifdef DEBUG
      printf("recvDispls: ");
      printf("%d, ", recvDispls[0]);
#endif
      for (int i = 1; i < p; i++) {
        recvDispls[i] = recvCounts[i - 1] + recvDispls[i - 1];
#ifdef DEBUG
        printf("%d, ", recvDispls[i]);
#endif
      }
#ifdef DEBUG
      printf("\n\n");
#endif
    }

    if (p != 1) {
      if (rank == 0) {
        // Sum recvCounts for the size of extendedHistogramArray
        extendedWordNumber = 0;
        for (int i = 0; i < p; i++) {
          extendedWordNumber += recvCounts[i];
        }
        extendedHistogramArray =
            (t_Word *)calloc(extendedWordNumber, sizeof(t_Word));
      }
      // Gather all histograms
      MPI_Gatherv(myHistogramArray, myWordNumber, MPI_T_WORD,
                  extendedHistogramArray, recvCounts, recvDispls, MPI_T_WORD, 0,
                  MPI_COMM_WORLD);
    } else {
      extendedHistogramArray = myHistogramArray;
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

      compactStartTime = MPI_Wtime();

      // Compact the final histogram
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

#ifdef TIME
    // Print and write time profiling
    double globalTime = MPI_Wtime() - globalStartTime;
    if (rank == 0) {
      double initTime = MPI_Wtime() - initStartTime;
      printf("0) Init phase took %f seconds\n", initTime);
    }

    printf("%d) Scattering phase took %f seconds\n", rank, scatterTime);
    printf("%d) Wordcount phase took %f seconds\n", rank, wordcountTime);
    printf("%d) Gathering phase took %f seconds\n", rank, gatheringTime);
    if (rank == 0) {
      printf("0) Compaction phase took %f seconds\n", compactTime);
    }
    printf("%d) Everything terminated in %f seconds\n", rank, globalTime);

    // Reduce all time data as average
    double scatterTimeAvg = 0, wordcountTimeAvg = 0, gatheringTimeAvg = 0,
           compactTimeAvg = 0, globalTimeAvg = 0;
    if (p != 1) {
      MPI_Reduce(&scatterTime, &scatterTimeAvg, 1, MPI_DOUBLE, MPI_SUM, 0,
                 MPI_COMM_WORLD);
      MPI_Reduce(&wordcountTime, &wordcountTimeAvg, 1, MPI_DOUBLE, MPI_SUM, 0,
                 MPI_COMM_WORLD);
      MPI_Reduce(&gatheringTime, &gatheringTimeAvg, 1, MPI_DOUBLE, MPI_SUM, 0,
                 MPI_COMM_WORLD);
      MPI_Reduce(&compactTime, &compactTimeAvg, 1, MPI_DOUBLE, MPI_SUM, 0,
                 MPI_COMM_WORLD);
      MPI_Reduce(&globalTime, &globalTimeAvg, 1, MPI_DOUBLE, MPI_SUM, 0,
                 MPI_COMM_WORLD);
      scatterTimeAvg /= p;
      wordcountTimeAvg /= p;
      gatheringTimeAvg /= p;
      compactTimeAvg /= p;
      globalTimeAvg /= p;
    } else {
      scatterTimeAvg = scatterTime;
      wordcountTimeAvg = wordcountTime;
      gatheringTimeAvg = gatheringTime;
      compactTimeAvg = compactTime;
      globalTimeAvg = globalTime;
    }
    if (rank == 0) {
      // Print to stdout
      printf("Average Scattering time %f\n", scatterTimeAvg);
      printf("Average Words count time %f\n", wordcountTimeAvg);
      printf("Average Gathering time %f\n", gatheringTimeAvg);
      printf("Average Compaction time %f\n", compactTimeAvg);
      printf("Average Global time %f\n", globalTimeAvg);

      // Write to file
      FILE *fp = fopen(timelogFile, "w");
      fprintf(fp, "Average Scattering time %f\n", scatterTimeAvg);
      fprintf(fp, "Average Words count time %f\n", wordcountTimeAvg);
      fprintf(fp, "Average Gathering time %f\n", gatheringTimeAvg);
      fprintf(fp, "Average Compaction time %f\n", compactTimeAvg);
      fprintf(fp, "Average Global time %f\n", globalTimeAvg);
      fclose(fp);
    }
#endif

    // Free everything
    MPI_Type_free(&MPI_T_CHUNK);
    MPI_Type_free(&MPI_T_WORD);
    free(myHistogramArray);
    free(myChunks);
    if (rank == 0) {
      free(fileNames);
      free(sendCounts);
      free(sendDispls);
      free(recvCounts);
      free(recvDispls);
      // For p==1 chunkArray buffer is pointed by myChunks
      if (p != 1) {
        free(chunkArray);
        free(extendedHistogramArray);
      }
    }
  }
  MPI_Finalize();
  return 0;
}