#ifndef FILE_C
#define FILE_C

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

#define FILE_SEND_BEGIN '\7'
#define FILE_SEND_ESCAPE '\10'

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
    int p_len = strlen(dir), f_len = strlen(filename); 
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

    int n;
    do {
        bzero(send_buffer, SEND_BUFFER_SIZE);
        n = read(fno, send_buffer, SEND_BUFFER_SIZE);
		printf("Read %d bytes\n", n);
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

void sendHeader(int stream_fileno, const char* dir_path)
{
    DIR* dir = opendir(dir_path);
    int name_len = strlen(dir_path);
    if (dir != NULL) {
        int bsize = name_len + strlen(format) + 1;
        char buffer[bsize];
        bzero(buffer, bsize);
        sprintf(buffer, format, dir_path);
        write(stream_fileno, buffer, strlen(buffer));
        
        struct dirent* f = readdir(dir);
        while (f != NULL) {
            if (f->d_name[0] == '.') {
                f = readdir(dir);
                continue;
            }
            char ndir[name_len + strlen(f->d_name) + 1];
            concatenate_path(ndir, dir_path, f->d_name);
            sendHeader(stream_fileno, ndir);
            f = readdir(dir);
        }

    } else if (errno == ENOTDIR) {
        write(stream_fileno, "FSIZE=", 5);
        long fsize = fileSize(dir_path);
        char fsize_s[128];
        bzero(fsize_s, 128);
        sprintf(fsize_s, "%ld", fsize);
        write(stream_fileno, fsize_s, strlen(fsize_s));
        write(stream_fileno, "\n", 1);

    } else {
		printf("Error when reading file \"%s\" for header: %d\n", dir_path, errno);
		exit(-1);
	}
}

#endif