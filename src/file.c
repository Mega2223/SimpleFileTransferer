#ifndef FILE_C
#define FILE_C

#include "file.h"
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

typedef struct known_file {
    size_t fname_len;
    long file_size;
    char* fname;
    struct known_file* next;
} known_file;

typedef struct trans_header {
    int n_files;
    known_file* files;
} trans_header;

long fileSize(const char* file_name)
{
    struct stat file_stat;
    int error = stat(file_name, &file_stat);
    if (error < 0) {
        printf("Error getting the file size for %s\n", file_name);
        exit(-1);
    }
    return file_stat.st_size;
}

void concatenatePath(char* dest, const char* dir, const char* filename)
{
    size_t p_len = strlen(dir), f_len = strlen(filename); 
    dest[p_len + f_len + 1] = '\0';
    strcpy(dest, dir);
    dest[p_len] = '/';
    dest[p_len + 1] = '\0';
    strcat(dest, filename);
}

void sendFile(int stream_fileno, const char* file_path, int n_bytes)
{
    printf("Starting datastream for file \"%s\"\n", file_path);
    if (stream_fileno <= 0) {
        printf("Invalid fileno object");
    	exit(-1);
    }
    
    int fno = open(file_path,O_RDONLY);
        
    char send_buffer[SEND_BUFFER_SIZE];

    int sent = 0;

    ssize_t n = 1;
    while (n) {
        bzero(send_buffer, SEND_BUFFER_SIZE);
        n = read(fno, send_buffer, SEND_BUFFER_SIZE);
        if (n < 0) {
            printf("Reading error: %ld : %d", n, errno);
            break;
        }
		// printf("Read %ld bytes at %s\n", n, file_path);
    	write(stream_fileno, send_buffer, (ssize_t) n);
        sent += n;
    }
	printf("[%s] read %d bytes, %d expected\n",file_path,sent,n_bytes);
    close(fno);
}

int getAllFiles(const char* f_path, known_file* next_file)
{
    printf("at call %s\n", f_path);
    DIR* dir = opendir(f_path);
    if (dir != NULL) {
        // Is dir
        int file_count = 0;
        struct dirent* de = NULL;
        do {
            de = readdir(dir);
            if (de == NULL || de->d_name[0] == '.')
                continue;
            char nf_path[strlen(f_path) + strlen(de->d_name) + 1];
            concatenatePath(nf_path, f_path, de->d_name);
            printf("looking file \"%s\" (\"%s\",\"%s\")\n", nf_path, f_path, de->d_name);
            int f_files = getAllFiles(nf_path, next_file);
            if (f_files == -1){
                file_count++;
                next_file = next_file->next;
            } else {
                for (int i = 0; i < f_files; i++) { // This is so dumb i love it
                    printf("Exchanging %p to %p\n", next_file, next_file->next);
                    next_file = next_file->next;
                }
            }
        } while (de != NULL);
        printf("call (%s) returning %d\n",f_path,file_count);
        return file_count;
    } else if (errno == ENOTDIR) {
        // Is file
        char* n_name = malloc(strlen(f_path) + 1);
        bzero(n_name, strlen(f_path) + 1);
        strcpy(n_name, f_path);
        next_file->fname = n_name;
        next_file->file_size = fileSize(n_name);
        next_file->next = malloc(sizeof(known_file));
        next_file->next->file_size = 0;
        next_file->next->next = NULL;
        printf("setting %p to file %s, next %p\n",next_file,n_name,next_file->next);
        return -1;
    } else {
        printf("Error for file %s\n", f_path);
        return -2;
	}
}

void printFileChain(known_file* first)
{
    known_file* cur = first;
    printf("\n");
    while (cur != NULL && cur->file_size > 0) {
        printf("\"%s\" [%ld] -> ",cur->fname, cur->file_size);
        // printf("%s>",cur->fname);
        cur = cur->next;
    }
    printf("\n");
}


void sendDirectory(int stream_fileno, const char* dir_path)
{
    trans_header* header = malloc(sizeof(trans_header));
    known_file* k_file = malloc(sizeof(known_file));
    k_file->file_size = 0;
    getAllFiles(dir_path, k_file);
    while (k_file->file_size > 0) {
        sendFile(stream_fileno, k_file->fname, k_file->file_size);
        k_file = k_file->next;
    }
    printFileChain(k_file);    
}

#endif