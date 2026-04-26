#include <stdio.h>
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

unsigned int PORT_LOCAL = 2000;
unsigned int PORT_DEST = 2000;

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
    }

    if(SELF_TYPE == SENDER){
        int socket = getSocketAsClient(DEST_ADDRESS,PORT_LOCAL,PORT_DEST);
        for (int i = 0; i < 1000; ++i) {
            char buf = '0' + (i % 10);
            printf("writing %c\n",buf);
            write(socket, &buf, sizeof(char));
        }

    } else {
        printf("Will listen\n");
        int socket_f = getSocketAsServer(DEST_ADDRESS, PORT_DEST);
        char buff[128];
        bzero(buff, sizeof(buff));

        for(int i = 0;; ++i){
            int n = read(socket_f, buff, sizeof(buff)-1);
            buff[n] = '\0';
            if(n == 0){continue;}
            printf("recv[%d] \"%s\"\n",n,buff);            
        }
    }

    return 0;
}