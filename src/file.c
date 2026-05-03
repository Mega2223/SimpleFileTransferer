#ifndef FILE_C
#define FILE_C

#include "file.h"
#include <asm-generic/errno-base.h>
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

void ensureHasPath(char* file_path)
{
    // printf("Ensuring path for %s\n", file_path);
    for (int i = 0; file_path[i] != '\0'; i++) {
        if (file_path[i] == '/') {
            file_path[i] = '\0';
            int res = mkdir(file_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (res == -1 && errno == EEXIST) {
                // Directory already exists
            }
            ensureHasPath(file_path);
            file_path[i] = '/';
        }
    }
}

void receiveFile(int r_stream_fileno, char* file_path, int bytes)
{
    ensureHasPath(file_path);
    int fstream = open(file_path, O_RDWR | O_TRUNC | O_CREAT);
    printf("fstream = %d\n", fstream);
    char rec_buffer[SEND_BUFFER_SIZE];
    int r = 1;
    while (bytes) {
        bzero(rec_buffer, sizeof(char));
        int n = read(r_stream_fileno, rec_buffer,
            bytes > SEND_BUFFER_SIZE ? SEND_BUFFER_SIZE : bytes);

        // printf("Reading \"%s\", [%d] bytes remaining.\n", file_path, bytes);
        // printf("Read [%d] bytes from a possible [%d].\n", n, bytes > SEND_BUFFER_SIZE ? SEND_BUFFER_SIZE : bytes);

        // if (n == 0 || n < 0 || errno) {
            // if (gotSigPipe) {
            //     printf("Got piped\n");
            //     break;
            // }
            // char w = '1';
            // write(n,&w,sizeof(w));
            // break;
        // }
        // printf("recv %d bytes\n", n);
        if (((char*)rec_buffer)[0] == '\0' && n == 0) {
            printf("got an EOF\n");
            break;
        }
        write(fstream, rec_buffer, n);
        bytes -= n;
    }
    close(fstream);
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

void sendFileHeader(int stream_fileno, known_file* k_file)
{
    int name_len = strlen(k_file->fname);
    write(stream_fileno, &k_file->file_size, sizeof(long));
    write(stream_fileno, &name_len, sizeof(int));
    write(stream_fileno, k_file->fname, name_len);
}

void recFileHeader(int read_stream, known_file* dest)
{
    long f_size; int name_len;
    read(read_stream, &f_size, sizeof(long));
    read(read_stream, &name_len, sizeof(int));
    char* name_buffer = malloc(name_len + 1);
    read(read_stream, name_buffer, name_len);
    name_buffer[name_len] = '\0';
    dest->fname = name_buffer;
    dest->fname_len = name_len;
    dest->file_size = f_size;
}

void sendHeader(int stream_fileno, trans_header* header)
{
    write(stream_fileno, &header->n_files, sizeof(int));
    known_file*current = header->files;
    while (current->file_size > 0) {
        sendFileHeader(stream_fileno, current);
        current = current->next;
    }
}

trans_header* recHeader(int read_stream)
{
    trans_header* ret = malloc(sizeof(trans_header));
    read(read_stream, &ret->n_files, sizeof(int));
    known_file* first = malloc(sizeof(known_file));
    known_file* cur = first;
    for (int i = 0; i < ret->n_files; i++) {
        recFileHeader(read_stream, cur);
        known_file* next = malloc(sizeof(known_file));
        next->next = NULL;
        next->fname = NULL;
        next->file_size = 0;
        cur->next = next;
        cur = next;
    }
    ret->files = first;
    printHeader(ret);
    return ret;
}

void sendDirectory(int stream_fileno, const char* dir_path)
{
    trans_header* header = malloc(sizeof(trans_header));
    known_file* first = malloc(sizeof(known_file));
    known_file* k_file = first;
    first->file_size = 0; first->fname = NULL;

    int n_files = 0.;
    getAllFiles(dir_path, k_file);
    while (k_file->file_size > 0) {
        k_file = k_file->next;
        n_files++;
    }
    header->n_files = n_files;
    header->files = first;

    sendHeader(stream_fileno, header);
    printHeader(header);

    k_file = first;
    while (k_file->file_size > 0) {
        sendFile(stream_fileno, k_file->fname, k_file->file_size);
        k_file = k_file->next;
    }
}

void recDirectory(int read_stream)
{
    trans_header* header = recHeader(read_stream);
    known_file* act = header->files;
    while (act->fname != NULL) {
        receiveFile(read_stream, act->fname, act->file_size);
        act = act->next;
    }
}

void printHeader(trans_header* header)
{
    printf("Header: Files[%d]\n", header->n_files);
    known_file* cur = header->files;
    while (cur->fname != NULL) {
        printf("File[%ld] %s\n",cur->file_size,cur->fname);
        cur = cur->next;
    }
}

#endif