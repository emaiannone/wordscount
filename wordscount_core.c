#include "wordscount_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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