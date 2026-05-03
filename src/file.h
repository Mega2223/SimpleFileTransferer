#ifndef FILE_H
#define FILE_H

#include <stdlib.h>

typedef struct trans_header trans_header;
typedef struct known_file known_file;

void sendFile(int stream_fileno, const char* file_path, int n_bytes);
void sendDirectory(int stream_fileno, const char* dir_path);

void recDirectory(int read_stream);

void receiveFile(int r_stream_fileno, char* file_path, int bytes);

void printHeader(trans_header* header);

void ensureHasPath(char* file_path);

#endif