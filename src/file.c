#ifndef FILE_C
#define FILE_C

#include "file.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
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

void sendHeader(int stream_fileno, const char* dir_path);

long fileSize(const char* file_name) {
    struct stat file_stat;
    int error = stat(file_name, &file_stat);
    if (error < 0) {
        printf("Error getting the file size for %s\n", file_name);
        exit(-1);
    }
    return file_stat.st_size;
}

void concatenate_path(char* dest, const char* dir, const char* filename)
{
    size_t p_len = strlen(dir), f_len = strlen(filename); 
    dest[p_len + f_len + 1] = '\0';
    strcpy(dest, dir);
    dest[p_len] = '/';
    dest[p_len + 1] = '\0';
    strcat(dest, filename);
}

void sendFile(int stream_fileno, const char* file_path)
{
    printf("Starting datastream for file \"%s\"\n", file_path);
    if (stream_fileno <= 0) {
        printf("invalid file");
    	exit(-1);
    }
    
    int fno = open(file_path,O_RDONLY);
        
    char send_buffer[SEND_BUFFER_SIZE];
    bzero(send_buffer, SEND_BUFFER_SIZE);

    size_t n;
    do {
        bzero(send_buffer, SEND_BUFFER_SIZE);
        n = (size_t) read(fno, send_buffer, SEND_BUFFER_SIZE);
		printf("Read %ld bytes\n", n);
    	write(stream_fileno, send_buffer, n);
    } while (n > 0);
	printf("reddit\n");
    close(fno);
}

void sendDir(int stream_fileno, const char* dir_path)
{
    printf("sdir call for %d and %s\n", stream_fileno, dir_path);
    DIR* dir = opendir(dir_path);

    if (dir == NULL) {
        if (errno == ENOTDIR) {
            sendFile(stream_fileno, dir_path);
            return;
		} else {
			printf("Error for opening directory: %d\n", errno);
			exit(-1);
		}
    }

    struct dirent* f = readdir(dir);;
    while (f != NULL) {
        if (f->d_name[0] == '.') {
            f = readdir(dir);
            continue;
        }
        char ndir[strlen(dir_path) + strlen(f->d_name) + 1];
        concatenate_path(ndir, dir_path, f->d_name);
        sendDir(stream_fileno, ndir);
        f = readdir(dir);
    } 
    
    printf("closing directory\n");
    closedir(dir);
}

const char format[] = "DIR:\"%s\"\n";

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
            concatenate_path(nf_path, f_path, de->d_name);
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
    while (cur != NULL) {
        // printf("\"%s\" <%p> [%ld] -> ",cur->fname, cur, cur->file_size);
        printf("%s>",cur->fname);
        cur = cur->next;
    }
    printf("\n");
}


void sendHeader(int stream_fileno, const char* dir_path)
{
    trans_header header;
    known_file* first = malloc(sizeof(known_file));
    printf("\n POINTER %p \n", first);
    getAllFiles(dir_path, first);
    printf("\n POINTER %p \n", first);
    printFileChain(first);    
}

#endif