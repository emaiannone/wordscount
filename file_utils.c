#include "file_utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// fileNames is an array pointer to FileNames; return value is file number
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