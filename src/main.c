#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "net.h"
#include "utils.h"

// TODO:
// - Configurações para predefinir a aplicação
// por exemplo nomear o melchior, etc.
// - Cópia recursiva
// - Procurar por hosts disponiveis
// - Escrita direta a partir do STDIN
// - Leitura para o STDOUT
// - Pensando alto: uma fila, caso o receptor não esteja online, guarde a requisição no chache
// e mande quando ele esteja, pensando ainda mais alto, seria ideal isso ser um serviço

unsigned int PORT_SERVER = 2000;
char* DEST_ADDRESS = "127.0.0.1";

typedef enum APP_TYPE { RECEIVER = 0, SENDER = 1 } APP_TYPE;

#define MAX_FILENAME_LEN 256

char fileName[MAX_FILENAME_LEN];

void onSigPipe(int s);

int DEBUG = 0;
int gotSigPipe = 0;

APP_TYPE SELF_TYPE = RECEIVER;

int main(int argc, char **argv) {
    signal(SIGPIPE, onSigPipe);
    bzero(fileName, sizeof(fileName));
  
    for(int a = 0; a < argc; ++a){
        if(strcmp(argv[a],"--sender") == 0){
            SELF_TYPE = SENDER;
        }
        if(strcmp(argv[a],"--ip") == 0){
            DEST_ADDRESS = argv[++a];
        }
        if(strcmp(argv[a],"--port") == 0){
            PORT_SERVER = atoi(argv[++a]);
        }
        if (strcmp(argv[a], "--file") == 0) {
            if (strlen(argv[a+1]) > MAX_FILENAME_LEN) {
                printf("File name way too large, max len is %d.\n", MAX_FILENAME_LEN);
                return -1;
            }
            strcpy(fileName, argv[++a]);
        }
    }

    if(SELF_TYPE == SENDER){
        printf("Sending file %s to host %s:%d\n",fileName,DEST_ADDRESS,PORT_SERVER);
        int socket = getSocketAsClient(DEST_ADDRESS, PORT_SERVER);
        if (socket <= 0) {
            return -1;
        }
        // FILE* f = fopen(fileName,"rb");
        int fno = open(fileName,O_RDONLY);
        
        char send_buffer[32];
        bzero(send_buffer, sizeof(send_buffer));

        int n = 1;
        while (n > 0 && !gotSigPipe) {
            n = read(fno, send_buffer, sizeof(send_buffer));
            printf("Read %d bytes: \"%s\"\n", n, send_buffer);
            write(socket, send_buffer, n);
            bzero(send_buffer, sizeof(send_buffer));
        }
        closeSock(socket);

    } else {
        printf("Will listen at ip %s at port %d\n", DEST_ADDRESS, PORT_SERVER);
        printf("Writing data to %s\n", fileName);
        
        int socket = getSocketAsServer(DEST_ADDRESS, PORT_SERVER);
        int file_to_write = open(fileName,O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
            
        void* rec_buffer = malloc(2048);
        bzero(rec_buffer, 2048);

        int n = 1;
        while (!gotSigPipe) {
            int n = read(socket, rec_buffer, sizeof(char));
            if (n == 0 || n < 0 || errno) {
                if (gotSigPipe) {
                    printf("Got piped\n");
                    break;
                }
                printf("errno=%d\n",errno);
            }
            printf("recv %d bytes\n", n);
            if (((int*)rec_buffer)[0] == 0 && n == 0) {
                printf("got an EOF\n");
                break;
            }
            write(file_to_write,rec_buffer,n);
            bzero(rec_buffer, sizeof(char));
        }
        close(file_to_write);
        closeSock(socket);
        free(rec_buffer);
    }
    printf("bye");
    return 0;
}

void onSigPipe(int s) {
    err("SIGPIPE");
    gotSigPipe = 1;
    // exit(SIGPIPE);
}