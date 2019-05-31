#include "file_utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// fileNames is an array pointer to FileNames; return value is file number
int getFilesName(t_FileName **fileNames, char *dirPath) {
  // Count files number if directory exists
  DIR *dir = opendir(dirPath);
  if (dir == NULL) {
    return 0;
  }
  struct dirent *entry;
  int fileNumber = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type != DT_DIR) {  // Only for files
      fileNumber++;
    }
  }
  closedir(dir);

  // If there is at least one file, get their name
  if (fileNumber == 0) {
    return 0;
  }
  dir = opendir(dirPath);
  if (dir == NULL) {
    return 0;
  }
  *fileNames = (t_FileName *)calloc(fileNumber, sizeof(t_FileName));

  t_FileName *fileNamesPtr = *fileNames;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type != DT_DIR) {
      // Initialization of the struct
      strcpy(fileNamesPtr->fileName, entry->d_name);
      fileNamesPtr->lineNumber = 0;
      fileNamesPtr++;
    }
  }
  closedir(dir);
  return fileNumber;
}

long int getLinesNumber(t_FileName *fileNames, int fileNumber, char *dirPath) {
  // Read all files using the fileNames
  t_FileName *fileNamesPtr = fileNames;
  long int totalLineNumber = 0;
  for (int i = 0; i < fileNumber; i++) {
    char *fileFullName = (char *)calloc(
        strlen(dirPath) + strlen(fileNamesPtr->fileName) + 1, sizeof(char));
    strcpy(fileFullName, dirPath);
    strcat(fileFullName, fileNamesPtr->fileName);
    FILE *fp = fopen(fileFullName, "r");
    if (fp == NULL) {
      printf("Error in opening file %s\n", fileNamesPtr->fileName);
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