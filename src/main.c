#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>

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

typedef enum APP_TYPE {
    RECEIVER = 0,
    SENDER = 1
} APP_TYPE;

int DEBUG = 0;

APP_TYPE SELF_TYPE = RECEIVER;
   
int main(int argc, char** argv){
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
        while (1) {
            int r = read(STDIN_FILENO,buff,sizeof(buff));
            write(socket, buff, r);
            bzero(buff, r);
        }

    } else {
        printf("Will listen at ip %s at port %d\n",DEST_ADDRESS,PORT_SERVER);
        int socket_f = getSocketAsServer(DEST_ADDRESS, PORT_SERVER);
        char buff[128];
        bzero(buff, sizeof(buff));

        for(int i = 0;; ++i){
            int n = read(socket_f, buff, sizeof(buff)-1);
            buff[n] = '\0';
            if(buff[n-1] == '\n') buff[n-1] = '\0';
            if(n <= 0){continue;}
            printf("recv[%d] \"%s\"\n",n,buff);            
        }
    }

    return 0;
}