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
    + Master prepares sendCount and sendDispls
    + Master scatters sendCount
    + Each process create and commit the custom datatype
    + Master scatters the chunks
    + Each process launches the wordsCount function its chunks, according to
   its work range. wordsCount access the file according to the given name and
   fetch all work lines. wordsCount return an list of structs like this: (word
   string - occurances)
    + Master receive all data and call a custom reduce function that merges the
   results of each process.
    + Master prints all the results
    - Free main structures in the heap
   --- Part 2: Local Beanchmark---
    + After everything has been tested, insert the time profiling code.
    + Run some benchmarks in local and write down average of these data into a
   time log file in the CWD.
   --- Part 3: Building---
    + Insert ad additional CLI parameter that is the directory of file in order
   to easily change the directory of input files
    + Manage the error case where the CLI path does not esists
    + Insert ad additional CLI parameter that is the log file base name
    - Make a script that launches the compilation and that launches the program
   multiple times, according to the strong and weak scalability requirements.
   Record each time profiling into different time logs
    - Code cleanup and optimization
    - Use different source files, builded together with make. The
   make launch is done in the script
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
#define TIME
#define DEBUG
#define VERBOSE
#define LINE_LIMIT 1024
#define FILE_NAME_SIZE 256
#define DEFAULT_FILE_LOCATION "./files/"
#define DEFAULT_TIME_LOG_FILE "time_log.txt"

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
int getFilesName(t_FileName **fileNames, char *dirPath) {
  DIR *dir = NULL;
  dir = opendir(dirPath);
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
  dir = opendir(dirPath);
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

long int getLinesNumber(t_FileName *fileNames, int fileNumber, char *dirPath) {
  // Read all files using the fileNames
  t_FileName *fileNamesPtr = fileNames;
  long int totalLineNumber = 0;
  char *selectedFile;
  for (int i = 0; i < fileNumber; i++) {
    char *fileFullName = (char *)calloc(
        strlen(dirPath) + strlen(fileNamesPtr->fileName) + 1, sizeof(char));
    strcpy(fileFullName, dirPath);
    strcat(fileFullName, fileNamesPtr->fileName);
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

t_ChunkNode *buildChunkList(t_FileName *fileNames, int fileNumber,
                            long int totalLineNumber, int p) {
  long int standardChunkSize = totalLineNumber / p;
#ifdef VERBOSE
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
  t_ChunkNode *firstChunkPtr = NULL, *wordPtr = NULL;
  for (int i = 0; i < fileNumber; i++) {
    long int lastLine = -1;
    long int leftLines = fileNamesPtr->lineNumber;
    while (leftLines > 0) {
      // New chunk for this file. If it is the first, it will be the head
      if (wordPtr == NULL) {
        wordPtr = (t_ChunkNode *)malloc(sizeof(t_ChunkNode));
        firstChunkPtr = wordPtr;
      } else {
        wordPtr->next = (t_ChunkNode *)malloc(sizeof(t_ChunkNode));
        wordPtr = wordPtr->next;
      }
      wordPtr->fileName = fileNamesPtr;
      wordPtr->startLine = lastLine + 1;
      wordPtr->next = NULL;

      // Compute the amount of lines that can be used
      long int actualChunkSize;

#ifdef VERBOSE
      printf("Created a chunk for file %s (%ld)\n", wordPtr->fileName->fileName,
             wordPtr->fileName->lineNumber);
      printf(
          "There are %ld lines in file %s. Next chunk will have %ld "
          "lines\n",
          leftLines, wordPtr->fileName->fileName, nextChunkSize);
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
      wordPtr->endLine = lastLine;
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

int chunksToArray(t_Chunk **chunkArray, t_ChunkNode *chunkList) {
  // First, we need to count the number of chunks
  int chunkNumber = 0;
  t_ChunkNode *wordPtr = chunkList;
  while (wordPtr != NULL) {
    chunkNumber++;
    wordPtr = wordPtr->next;
  }
  *chunkArray = (t_Chunk *)calloc(chunkNumber, sizeof(t_Chunk));

  // Conversion
  wordPtr = chunkList;
  for (int i = 0; i < chunkNumber; i++) {
    strcpy((*chunkArray)[i].fileName, wordPtr->fileName->fileName);
    (*chunkArray)[i].startLine = wordPtr->startLine;
    (*chunkArray)[i].endLine = wordPtr->endLine;
    wordPtr = wordPtr->next;
  }
  return chunkNumber;
}

void printChunkArray(t_Chunk *chunkArray, int chunkNumber) {
  for (int i = 0; i < chunkNumber; i++) {
    t_Chunk chunk = chunkArray[i];
    printf("Chunk{%s of %ld lines (%ld to %ld)};\n", chunk.fileName,
           (chunk.endLine) - (chunk.startLine) + 1, chunk.startLine,
           chunk.endLine);
  }
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
#ifdef VERBOSE
  printf("sendDispls: ");
  printf("%d, ", sendDispls[0]);
#endif
  for (int i = 1; i < p; i++) {
    sendDispls[i] = sendCounts[i - 1] + sendDispls[i - 1];
#ifdef VERBOSE
    printf("%d, ", sendDispls[i]);
#endif
  }
#ifdef VERBOSE
  printf("\n\n");
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

void createWordDatatype(MPI_Datatype *newDatatype) {
  int blockNumber = 2;
  int blockLengths[] = {LINE_LIMIT, 1};
  long int displs[] = {0, LINE_LIMIT};
  MPI_Datatype types[] = {MPI_CHAR, MPI_INT64_T};
  MPI_Type_create_struct(blockNumber, blockLengths, displs, types, newDatatype);
  MPI_Type_commit(newDatatype);
}

long int wordscount(t_WordNode **myHistogramList, t_Chunk *myChunks,
                    int myChunkNumber, char *dirPath) {
  *myHistogramList = NULL;
  long int countedWords = 0;
  char wordBuf[LINE_LIMIT];

  for (int i = 0; i < myChunkNumber; i++) {
    // File opening
    char *fileFullName = (char *)calloc(
        strlen(dirPath) + strlen(myChunks[i].fileName) + 1, sizeof(char));
    strcpy(fileFullName, dirPath);
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

      // Look for the read word in the myHistogramList. If present, increase
      // occurences, otherwise append a new node
      if (*myHistogramList == NULL) {
        *myHistogramList = (t_WordNode *)malloc(sizeof(t_WordNode));
        strcpy((*myHistogramList)->word.word, wordBuf);
        (*myHistogramList)->word.occurances = 1;
        (*myHistogramList)->next = NULL;
      } else {
        t_WordNode *wordNodePtr = *myHistogramList;
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

int wordsToArray(t_Word **wordArray, t_WordNode *wordList) {
  // First, we need to count the number of words
  int chunkNumber = 0;
  t_WordNode *wordPtr = wordList;
  while (wordPtr != NULL) {
    chunkNumber++;
    wordPtr = wordPtr->next;
  }
  *wordArray = (t_Word *)calloc(chunkNumber, sizeof(t_Word));

  // Conversione
  wordPtr = wordList;
  for (int i = 0; i < chunkNumber; i++) {
    (*wordArray)[i] = wordPtr->word;
    wordPtr = wordPtr->next;
  }
  return chunkNumber;
}

void compactHistogram(t_WordNode **finalHistogramList,
                      t_Word *extendedHistogramArray, int extendedWordNumber) {
  t_WordNode *finalHistogramPtr;
  for (int i = 0; i < extendedWordNumber; i++) {
    // There are no words yet
    if (*finalHistogramList == NULL) {
      *finalHistogramList = malloc(sizeof(t_WordNode));
      strcpy((*finalHistogramList)->word.word, extendedHistogramArray[i].word);
      (*finalHistogramList)->word.occurances =
          extendedHistogramArray[i].occurances;
      (*finalHistogramList)->next = NULL;
    } else {
      // Look if this word already exist
      finalHistogramPtr = *finalHistogramList;
      while (strcmp(finalHistogramPtr->word.word,
                    extendedHistogramArray[i].word) != 0 &&
             finalHistogramPtr->next != NULL) {
        finalHistogramPtr = finalHistogramPtr->next;
      }
      // Not found
      if (strcmp(finalHistogramPtr->word.word,
                 extendedHistogramArray[i].word) != 0) {
        finalHistogramPtr->next = malloc(sizeof(t_WordNode));
        finalHistogramPtr = finalHistogramPtr->next;
        strcpy(finalHistogramPtr->word.word, extendedHistogramArray[i].word);
        finalHistogramPtr->word.occurances =
            extendedHistogramArray[i].occurances;
        finalHistogramPtr->next = NULL;
      } else {
        finalHistogramPtr->word.occurances +=
            extendedHistogramArray[i].occurances;
      }
    }
  }
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
  int *sendCount, *recvCounts, *sendDispls, *recvDispls;
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
    if (argc >= 3) {
      strcpy(timelogFile, argv[2]);
    }
  }

  // Master preprocessing
  if (rank == 0) {
#ifdef VERBOSE
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
      // TODO Correctly manage this error case, maybe broadcast a validity
      // signal? packed with the first and second scatter?
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

#ifdef VERBOSE
      printf("Master will send these chunks: ");
      printChunkArray(chunkArray, chunkNumber);
      printf("\n\n");
#endif

      // Prepare sendCounts and sendDispls for the scatter
      sendCount = (int *)calloc(p, sizeof(int));
      sendDispls = (int *)calloc(p, sizeof(int));
      prepareSendCountAndDispls(sendCount, sendDispls, chunkArray, chunkNumber,
                                totalLineNumber, p);
    }
  }
  // Everyone run here

  // If the master found at least one file, the algorithm can go on
  MPI_Bcast(&canStart, 1, MPI_INT, 0, MPI_COMM_WORLD);
  if (canStart) {
    scatterStartTime = MPI_Wtime();

    // Scatters sendCounts so that each process knows how many chunks expects
    int myChunkNumber;
    MPI_Scatter(sendCount, 1, MPI_INT, &myChunkNumber, 1, MPI_INT, 0,
                MPI_COMM_WORLD);

    // Scatter the chunks
    t_Chunk *myChunks = (t_Chunk *)calloc(myChunkNumber, sizeof(t_Chunk));
    MPI_Scatterv(chunkArray, sendCount, sendDispls, MPI_T_CHUNK, myChunks,
                 myChunkNumber, MPI_T_CHUNK, 0, MPI_COMM_WORLD);

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
#endif
#ifdef VERBOSE
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

#ifdef VERBOSE
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
    MPI_Gather(&myWordNumber, 1, MPI_INT, recvCounts, 1, MPI_INT, 0,
               MPI_COMM_WORLD);

    if (rank == 0) {
#ifdef VERBOSE
      printf("recvCounts: ");
      for (int i = 0; i < p; i++) {
        printf("%d, ", recvCounts[i]);
      }
      printf("\n");
#endif

      // TODO Questa creazione può essere messe in una
      // funzione a parte dato che è fatta ugualmente anche in un altro caso
      recvDispls[0] = 0;
#ifdef VERBOSE
      printf("recvDispls: ");
      printf("%d, ", recvDispls[0]);
#endif
      for (int i = 1; i < p; i++) {
        recvDispls[i] = recvCounts[i - 1] + recvDispls[i - 1];
#ifdef VERBOSE
        printf("%d, ", recvDispls[i]);
#endif
      }
#ifdef VERBOSE
      printf("\n\n");
#endif

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

#ifdef TIME
    gatheringTime = MPI_Wtime() - gatheringStartTime;
#endif

    if (rank == 0) {
#ifdef VERBOSE
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

    // Pack all time data and reduce them all into average
    double scatterTimeAvg = 0, wordcountTimeAvg = 0, gatheringTimeAvg = 0,
           compactTimeAvg = 0, globalTimeAvg = 0;
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
    if (rank == 0) {
      scatterTimeAvg /= p;
      wordcountTimeAvg /= p;
      gatheringTimeAvg /= p;
      compactTimeAvg /= p;
      globalTimeAvg /= p;
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

    // TODO Free everything!!!!!
    /*
    MPI_Type_free(&MPI_T_CHUNK);
    if (rank == 0) {
      free(fileNames);
      free(chunkArray);
      free(sendCount);
      free(sendDispls);
      fclose(fp);
    }
    */
  }
  MPI_Finalize();
  return 0;
}