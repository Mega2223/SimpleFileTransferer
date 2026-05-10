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

typedef struct known_file {
    size_t fname_len;
    long file_size;
    char* fname;
    struct known_file* next;
} known_file;

typedef struct trans_header {
    int file_count;
    known_file* files;
} trans_header;

known_file* gen_kfile()
{
    known_file* ret = malloc(sizeof(known_file));
    bzero(ret, sizeof(known_file));
    ret->next = NULL;
    ret->file_size = 0;
    ret->fname = NULL;
    ret->fname_len = 0;
    return ret;
};

ssize_t readExact(int fileno, void* buffer, size_t bytec)
{
    ssize_t rd = 0;
    while (rd < bytec) {
        rd += read(fileno, &buffer[rd], bytec-rd);
    }
    return rd;
}

ssize_t writeExact(int fileno, const void* buffer, size_t bytec)
{
    ssize_t written = 0;
    while (written < bytec) {
        written += write(fileno, &buffer[written], bytec-written);
    }
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
    printf("System reported %ld bytes for file %s.\n",file_stat.st_size,file_name);
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

    ssize_t n = 1;
    while (n) {
        bzero(send_buffer, SEND_BUFFER_SIZE);
        n = read(fno, send_buffer, SEND_BUFFER_SIZE);
        if (n < 0) {
            printf("Reading error: %ld : %d", n, errno);
            exit(-1);
        }
        // printf("Read %ld bytes at %s\n", n, file_path);
        int written = 0;
        // while (written < n) {
        //     char send_loc = send_buffer[written];
        //     written += safeWrite(stream_fileno, &send_loc, n - written);
        // }
        writeExact(stream_fileno, &send_buffer, n);
    	
        sent += n;
    }
    printf("[%s] read %d bytes, %ld expected\n", file_path, sent, exp_bytes);
    if (sent != exp_bytes) {
        printf("Number of bytes does not match header, quitting.\n");
        exit(-1);
    }
    close(fno);
}

void receiveFile(int r_stream_fileno, char* file_path, int bytes)
{
    ensureHasPath(file_path);
    int fstream = open(file_path, O_WRONLY | O_TRUNC | O_CREAT);
    char rec_buffer[SEND_BUFFER_SIZE];
    printf("Receiving %s[%d]\n", file_path,bytes);
    while (bytes) {
        bzero(rec_buffer, sizeof(char));
        ssize_t n = readExact(r_stream_fileno, rec_buffer,
            bytes > SEND_BUFFER_SIZE ? SEND_BUFFER_SIZE : bytes);
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
        // if (((char*)rec_buffer)[0] == '\0' && n == 0) {
        //     printf("got an EOF\n");
        //     break;
        // }
        writeExact(fstream, rec_buffer, n);
        bytes -= n;
    }
    close(fstream);
    chmod(file_path, S_IRWXU);
}

const char* SELF_PATH = ".";

int getAllFiles(const char* f_path, known_file* next_file)
{
    printf("At call %s\n", f_path);
    if (f_path == NULL) {
        f_path = SELF_PATH;
    }
    DIR* dir = opendir(f_path);
    if (dir != NULL) {
        // Is directory
        int file_count = 0;
        struct dirent* de = NULL;
        do {
            de = readdir(dir);
            if (de == NULL || de->d_name[0] == '.')
                continue;
            char nf_path[strlen(f_path) + strlen(de->d_name) + 2 + 2];
            concatenatePath(nf_path, f_path, de->d_name);
            printf("looking file \"%s\" (\"%s\",\"%s\")\n", nf_path, f_path, de->d_name);
            int f_files = getAllFiles(nf_path, next_file);
            if (f_files == -1){
                file_count++;
                printf("Nextpo %p %p\n", next_file, next_file->next);
                next_file = next_file->next;
            } else {
                for (int i = 0; i < f_files; i++) { // This is so dumb i love it
                    printf("Exchanging %p to %p\n", next_file, next_file->next);
                    next_file = next_file->next;
                }
            }
        } while (de != NULL);
        printf("File getter call for (%s) returning %d\n",f_path,file_count);
        return file_count;
    } else if (errno == ENOTDIR) {
        // Is file
        printf("%s am file\n",f_path);
        if (next_file->fname != NULL || next_file->fname_len != 0 || next_file->file_size != 0) {
            printf("Corruption issue for file %s \n", f_path);
            if (next_file->fname != NULL) {
                printf("fname = %p\n", next_file->fname);
                printf("(%s)\n", next_file->fname);
            }
            if (next_file->fname_len != 0) {
                printf("fname_len = %ld\n",next_file->fname_len);
            }
            if (next_file->file_size != 0) {
                printf("fsize = %ld\n",next_file->file_size);
            }

        } else if (fileSize(f_path) == 0) {
            return 0;
        }
        char* n_name = malloc(strlen(f_path) + 1);
        strcpy(n_name, f_path);
        next_file->fname = n_name;
        next_file->fname_len = strlen(n_name);
        next_file->file_size = fileSize(n_name);
        next_file->next = gen_kfile();
        printf("Setting %p [%ld] to file \"%s\", new pointer: %p\n",
            next_file, next_file->file_size,
            next_file->fname,
            next_file->next);
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
        cur = cur->next;
    }
    printf("\n");
}

void sendFileHeader(int stream_fileno, known_file* k_file)
{
    int name_len = strlen(k_file->fname);
    int written = 0, expected = sizeof(long) + sizeof(int) + name_len;
    written += writeExact(stream_fileno, &k_file->file_size, sizeof(long));
    written += writeExact(stream_fileno, &name_len, sizeof(int));
    written += writeExact(stream_fileno, k_file->fname, name_len);
    if (expected != written) {
        printf("WARNING: Sending discrepancy at %s: [%d/%d]\n", k_file->fname,written,expected);
        exit(-1);
    }
}

void recFileHeader(int read_stream, known_file* dest)
{
    long f_size;
    int name_len, expected_bytes = sizeof(long) + sizeof(int), received_bytes = 0;
    received_bytes += readExact(read_stream, &f_size, sizeof(long));
    received_bytes += readExact(read_stream, &name_len, sizeof(int));
    char* name_buffer = malloc(name_len + 1);
    // expected_bytes += name_len;
    if (name_len > 0) {
        printf("Taking %d bytes\n", name_len);
        int name_r = readExact(read_stream, name_buffer, name_len);
        if (name_r != name_len) {
            printf("Namelen discrepancy :p [%d/%d]\n", name_r, name_len);
            name_buffer[name_len] = '\0';
            printf("s = \"%s\"",name_buffer);
            exit(-2);
        }
        name_buffer[name_len] = '\0';
        printf("Ack file %s\n", name_buffer);
    }
    if (expected_bytes != received_bytes) {
        printf("WARNING: Receive discrepancy: [%d/%d]\n", received_bytes, expected_bytes);
        exit(-1);
    }
    dest->fname = name_buffer;
    dest->fname_len = name_len;
    dest->file_size = f_size;
}

void sendHeader(int stream_fileno, trans_header* header)
{
    writeExact(stream_fileno, &header->file_count, sizeof(int));
    known_file*current = header->files;
    while (current->file_size > 0) {
        sendFileHeader(stream_fileno, current);
        current = current->next;
    }
}

trans_header* recHeader(int read_stream)
{
    trans_header* ret = malloc(sizeof(trans_header));
    readExact(read_stream, &ret->file_count, sizeof(int));
    known_file* first = gen_kfile();
    known_file* cur = first;
    for (int i = 0; i < ret->file_count; i++) {
        recFileHeader(read_stream, cur);
        known_file* next = gen_kfile();
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

void sendDirectory(int stream_fileno)
{
    trans_header* header = malloc(sizeof(trans_header));
    known_file* first = gen_kfile();
    known_file* k_file = first;
    first->file_size = 0; first->fname = NULL;

    int n_files = 0.;
    getAllFiles(NULL , k_file);
    while (k_file != NULL) {
        k_file = k_file->next;
        n_files++;
    }
    header->file_count = n_files - 1;
    header->files = first;

    printHeader(header);
    sendHeader(stream_fileno, header);

    k_file = first;
    while (k_file->next != NULL) {
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
    printf("Header: Files=%d\n", header->file_count);
    known_file* cur = header->files;
    int c = 0;
    while (cur != NULL) {
        printf("File[%d][%ld] %s[%ld]\n",++c,cur->file_size,cur->fname,cur->fname_len);
        cur = cur->next;
    }
    printf("End header.\n");
}

#endif