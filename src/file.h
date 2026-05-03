#ifndef FILE_H
#define FILE_H

#include <stdlib.h>

void sendFile(int stream_fileno, const char* file_path);
void sendDir(int stream_fileno, const char* dir_path);

// void printFileChain(known_file* first);

#endif