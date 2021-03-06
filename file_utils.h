#define FILE_NAME_SIZE 256
#define LINE_LIMIT 1024

typedef struct FileName {
  char fileName[FILE_NAME_SIZE];
  unsigned long int lineNumber;
} t_FileName;

int getFilesName(t_FileName **fileNames, char *dirPath);
unsigned long int getLinesNumber(t_FileName *fileNames, int fileNumber, char *dirPath);