#include "chunks.h"

typedef struct Word {
  char word[LINE_LIMIT];
  long int occurances;
} t_Word;

typedef struct WordNode {
  t_Word word;
  struct WordNode *next;
} t_WordNode;

long int wordscount(t_WordNode **myHistogramList, t_Chunk *myChunks,
                    int myChunkNumber, char *dirPath);
int wordsToArray(t_Word **wordArray, t_WordNode *wordList);
void compactHistogram(t_WordNode **finalHistogramList,
                      t_Word *extendedHistogramArray, int extendedWordNumber);