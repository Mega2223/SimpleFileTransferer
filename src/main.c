#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "net.h"
#include "file.h"

// TODO:
// - Configurações para predefinir a aplicação
// por exemplo nomear o melchior, etc.
// - Cópia recursiva (Feito)
// - Procurar por hosts disponiveis
// - Escrita direta a partir do STDIN
// - Leitura para o STDOUT
// - Pensando alto: uma fila, caso o receptor não esteja online, guarde a requisição no chache
// e mande quando ele esteja, pensando ainda mais alto, seria ideal isso ser um serviço

unsigned int SELF_PORT = 2000;
char* DEST_ADDRESS = "127.0.0.1";

typedef enum APP_TYPE { RECEIVER = 0, SENDER = 1 } APP_TYPE;

#define MAX_FILENAME_LEN 256

char fileName[MAX_FILENAME_LEN] = ".";

int DEBUG = 0;

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
            SELF_PORT = (unsigned int) atoi(argv[++a]);
        }
        if (strcmp(argv[a], "--file") == 0) {
            if (strlen(argv[a+1]) > MAX_FILENAME_LEN) {
                printf("File name way too large, max len is %d.\n", MAX_FILENAME_LEN);
                return -1;
            }
            strcpy(fileName, argv[++a]);
        }
    }

    if (SELF_TYPE == SENDER) {
        printf("Sending \"%s\" to host %s:%d\n",fileName,DEST_ADDRESS,SELF_PORT);
        int socket = getSocketAsClient(DEST_ADDRESS, SELF_PORT);
        if (socket <= 0) {
            return -1;
        }
        sendDirectory(socket, fileName);
        closeSock(socket);
    } else {
        ensureHasPath(fileName);
        mkdir(fileName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        chdir(fileName);
        printf("Will listen at ip %s at port %d\n", DEST_ADDRESS, SELF_PORT);
        int socket = getSocketAsServer(SELF_PORT);
        recDirectory(socket);
        closeSock(socket);
    }
    printf("Quitting normally.\n");
    return 0;
}
