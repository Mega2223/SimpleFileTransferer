#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "net.h"

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

void onSigPipe(int s);

int DEBUG = 0;
int CONNECTION_EX = 0;

APP_TYPE SELF_TYPE = RECEIVER;

int main(int argc, char **argv) {

    signal(SIGPIPE, onSigPipe);
  
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
    }

    if(SELF_TYPE == SENDER){
        int socket = getSocketAsClient(DEST_ADDRESS,PORT_SERVER);
        char buff[16];
        bzero(buff, sizeof(buff));
        while (socket > 0) {
            int r = read(STDIN_FILENO, buff, sizeof(buff));
            if(r == 0) continue;
            printf("writing\n");
            int w = write(socket, buff, r);
            printf("wrote[%d]\n", w);
            if (CONNECTION_EX) {
                printf("Connection ended abruptly");
                return -1;
            }
            bzero(buff, r);
        }

    } else {
        printf("Will listen at ip %s at port %d\n",DEST_ADDRESS,PORT_SERVER);
        int socket = getSocketAsServer(DEST_ADDRESS, PORT_SERVER);
        char buff[128];
        bzero(buff, sizeof(buff));

        while (socket > 0) {
            int n = read(socket, buff, sizeof(buff)-1);
            buff[n] = '\0';
            if(buff[n-1] == '\n') buff[n-1] = '\0';
            if (n == 0) continue;
            if (n < 0) {socket = -1; break;}
            printf("recv[%d] \"%s\"\n",n,buff);            
        }
    }
    printf("bye");
    return 0;
}

void onSigPipe(int s) {
    // printf("Sigpipe\n");
    CONNECTION_EX = 1;
}