#include "file_utils.h"

typedef struct ChunkNode {
  t_FileName *fileName;
  unsigned long int startLine;
  unsigned long int endLine;
  struct ChunkNode *next;
} t_ChunkNode;

typedef struct Chunk {
  char fileName[FILE_NAME_SIZE];
  unsigned long int startLine;
  unsigned long int endLine;
} t_Chunk;

t_ChunkNode *buildChunkList(t_FileName *fileNames, int fileNumber,
                            unsigned long int totalLineNumber, int p);
int chunksToArray(t_Chunk **chunkArray, t_ChunkNode *chunkList);
void printChunkArray(t_Chunk *chunkArray, int chunkNumber);