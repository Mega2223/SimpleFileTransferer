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

#define SEND_BUFFER_SIZE 64

#define FILE_SEND_BEGIN = '\7'
#define FILE_SEND_ESCAPE = '\10'

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
        // bzero(send_buffer, SEND_BUFFER_SIZE);
        n = read(fno, send_buffer, SEND_BUFFER_SIZE);
        // printf("n=%d\n",n);
        // if(n <= 0) break;
		// printf("Read %d bytes\n", n);
    	// write(stream_fileno, send_buffer, n);
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

    struct dirent* f = readdir(dir);
    while (f != NULL) {
        if (f->d_name[0] == '.') {
            f = readdir(dir);
            continue;
        }
        int p_len = strlen(dir_path), f_len = strlen(f->d_name);
        
        char f_name[p_len + f_len + 2];
        f_name[p_len + f_len + 1] = '\0';

        strcpy(f_name, dir_path);
        f_name[p_len] = '/';
        f_name[p_len + 1] = '\0';
        strcat(f_name, f->d_name);

        printf("calling sdir:\n current call=\"%s\"\n next call:%s\n file name:%s\n",
            dir_path, f_name, f->d_name);
        sendDir(stream_fileno, f_name);
        f = readdir(dir);
    }
    printf("closing directory\n");
    closedir(dir);
}

#endif