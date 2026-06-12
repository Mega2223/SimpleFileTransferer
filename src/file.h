#ifndef FILE_H
#define FILE_H

#include <stdlib.h>

typedef struct file_info file_info;

void sendFile(int stream_fileno, const char* file_path, long exp_bytes);

void receiveFile(int r_stream_fileno);

void concatenatePath(char* dest, const char* dir, const char* filename);

int isDir(const char* fname);

void ensureHasPath(char* file_path);

#endif