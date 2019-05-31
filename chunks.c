#include "chunks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#define DEBUG

t_ChunkNode *buildChunkList(t_FileName *fileNames, int fileNumber,
                            unsigned long int totalLines, int workers) {
  unsigned long int stdChunkSize = totalLines / workers;
  int remainder = totalLines % workers;

  unsigned long int nextChunkSize = stdChunkSize;
  if (remainder > 0) {
    nextChunkSize++;
    remainder--;
  }
  t_FileName *fileNamesPtr = fileNames;
  t_ChunkNode *firstChunkPtr = NULL, *wordPtr = NULL;
  for (int i = 0; i < fileNumber; i++) {
    long int lastLine = -1;
    unsigned long int leftLines = fileNamesPtr->lineNumber;
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

#ifdef DEBUG
      printf("Created a chunk for file %s (%ld)\n", wordPtr->fileName->fileName,
             wordPtr->fileName->lineNumber);
      printf(
          "There are %ld lines in file %s. Next chunk will have %ld "
          "lines\n",
          leftLines, wordPtr->fileName->fileName, nextChunkSize);
#endif
      // Compute the amount of lines that can be used
      unsigned long int actualChunkSize;
      if (leftLines >= nextChunkSize) {
        actualChunkSize = nextChunkSize;
        nextChunkSize = stdChunkSize;
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
  if (nextChunkSize < stdChunkSize) {
    printf("There are some unassigned lines, results may be incorrect\n");
    return NULL;
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