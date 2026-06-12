#ifndef FILE_C
#define FILE_C

#include "file.h"
#include "net.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#define SEND_BUFFER_SIZE 64

typedef struct file_info {
    unsigned int namelen;
    char* name;
    int bytes;
} file_info;

void sendFileInfo(int stream, const char* filename, int bytes){
    file_info f;
    f.name = malloc(strlen(filename) + 1);
    strcpy(f.name,filename);
    f.bytes = bytes;
    f.namelen = strlen(filename);
    write(stream,&f.bytes,sizeof(f.bytes));
    write(stream,&f.namelen,sizeof(f.namelen));
    write(stream, f.name, f.namelen);
}

file_info* recFileInfo(int stream){
    file_info* ret = malloc(sizeof(file_info));
    read(stream,&ret->bytes,sizeof(ret->bytes));
    read(stream,&ret->namelen,sizeof(ret->namelen));
    char* name = malloc(ret->namelen+1);
    read(stream,name,ret->namelen);
    name[ret->namelen - 1] = '\0';
    ret->name = name;
    return ret;
}

int isDir(const char* fname){
    struct stat f_stat;
    stat(fname, &f_stat);
    return S_ISDIR(f_stat.st_mode);
}

int READ_BLOCK = 0, WRITE_BLOCK = 0;

ssize_t readExact(int fileno, void* buffer, size_t bytec)
{
    READ_BLOCK++;
    ssize_t rd = 0;
    while (rd < bytec) {
        rd += read(fileno, &buffer[rd], bytec-rd);
    }
    READ_BLOCK--;
    return rd;
}

ssize_t writeExact(int fileno, const void* buffer, size_t bytec)
{
    WRITE_BLOCK++;
    ssize_t written = 0;
    while (written < bytec) {
        written += write(fileno, &buffer[written], bytec-written);
    }
    WRITE_BLOCK--;
    return written;
}

long fileSize(const char* file_name)
{
    struct stat file_stat;
    int error = stat(file_name, &file_stat);
    if (error < 0) {
        printf("Error getting the file size for %s\n", file_name);
        exit(-1);
    }
    // printf("System reported %ld bytes for file %s.\n",file_stat.st_size,file_name);
    return file_stat.st_size;
}

void ensureHasPath(char* file_path)
{
    for (int i = 0; file_path[i] != '\0'; i++) {
        if (file_path[i] == '/') {
            file_path[i] = '\0';
            int res = mkdir(file_path, S_IRWXO | S_IRGRP | S_IRWXU);
            if (res == -1 && errno != EEXIST) {
                file_path[i] = '/';
                printf("Error creating directory \"%s\": %d, quitting.\n", file_path, errno);
                exit(-1);
            }
            ensureHasPath(file_path);
            file_path[i] = '/';
        }
    }
}

void concatenatePath(char* dest, const char* dir, const char* filename)
{
    size_t p_len = strlen(dir), f_len = strlen(filename); 
    dest[p_len + f_len + 2] = '\0';
    strcpy(dest, dir);
    dest[p_len] = '/';
    dest[p_len + 1] = '\0';
    strcat(dest, filename);
}

void sendFile(int stream_fileno, const char* file_path, long exp_bytes)
{
    printf("Starting datastream for file \"%s\".\n", file_path);
    if (stream_fileno <= 0) {
        printf("Invalid fileno object.\n");
    	exit(-1);
    }
    
    int fno = open(file_path,O_RDONLY);
    char send_buffer[SEND_BUFFER_SIZE];
    int sent = 0;
    sendFileInfo(stream_fileno,file_path,exp_bytes);

    ssize_t n = 1;
    while (n) {
        bzero(send_buffer, SEND_BUFFER_SIZE);
        n = read(fno, send_buffer, SEND_BUFFER_SIZE);
        if (n < 0) {
            printf("Reading error: %ld : %d", n, errno);
            exit(-1);
        }

        int written = 0;

        writeExact(stream_fileno, &send_buffer, n);
    	if (getNetworkDebug()){
            printBytes(send_buffer,SEND_BUFFER_SIZE,sent);
        }
        sent += n;
    }
    printf("[%s] read %d bytes, %ld expected\n", file_path, sent, exp_bytes);
    if (sent != exp_bytes) {
        printf("Number of bytes does not match header, quitting.\n");
        exit(-1);
    }
    close(fno);
}

void receiveFile(int r_stream_fileno)
{
    file_info* rec = recFileInfo(r_stream_fileno);

    ensureHasPath(rec->name);
    int fstream = open(rec->name, O_WRONLY | O_TRUNC | O_CREAT);
    char rec_buffer[SEND_BUFFER_SIZE];
    int bytes = rec->bytes;
    printf("Receiving %s[%d]\n", rec->name,bytes);

    while (bytes) {
        bzero(rec_buffer, sizeof(char));
        ssize_t n = readExact(r_stream_fileno, rec_buffer,
            bytes > SEND_BUFFER_SIZE ? SEND_BUFFER_SIZE : bytes);
        if (getNetworkDebug()){
            printBytes(rec_buffer,SEND_BUFFER_SIZE,n);
        }
        // printf("Reading \"%s\", [%d] bytes remaining.\n", file_path, bytes);
        // printf("Read [%d] bytes from a possible [%d].\n", n, bytes > SEND_BUFFER_SIZE ? SEND_BUFFER_SIZE : bytes);
        if (n <= 0) {
            char msg[] = "still alive?";
            writeExact(r_stream_fileno, msg, sizeof(msg));
        }
        if (gotSigPipe) {
            printf("Got SIGPIPE during transmittion, socket likely closed.\n");
            close(r_stream_fileno);
            exit(-1);
        }
        writeExact(fstream, rec_buffer, n);
        bytes -= n;
    }
    close(fstream);
    chmod(rec->name, S_IRWXU);
}

const char* SELF_PATH = ".";
#endif
