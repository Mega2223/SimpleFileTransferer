#ifndef FILE_H
#define FILE_H

#include <stdlib.h>

void sendFile(int stream_fileno, const char* file_path, int n_bytes);
void sendDirectory(int stream_fileno, const char* dir_path);

void receiveFile(int r_stream_fileno, char* file_path, int bytes);


#endif