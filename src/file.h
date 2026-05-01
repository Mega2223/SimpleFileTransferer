#ifndef FILE_H
#define FILE_H

void sendFile(int stream_fileno, const char* file_path);
void sendDir(int stream_fileno, const char* dir_path);

#endif