#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>

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

int INTERRUPTED = 0;

typedef enum APP_TYPE { RECEIVER = 0, SENDER = 1 } APP_TYPE;

#define MAX_FILENAME_LEN 256

char fileName[MAX_FILENAME_LEN] = ".";

int DEBUG = 0;
int PIPE = 0;
int EXEC_ONCE = 0;

APP_TYPE SELF_TYPE = RECEIVER;

void onSigInt(int i);

int main(int argc, char **argv) {
    signal(SIGPIPE, onSigPipe);
    signal(SIGINT, onSigInt);

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
        if(strcmp(argv[a],"--pipe") == 0){
            PIPE = 1;
        }
        if(strcmp(argv[a],"--once") == 0){
            EXEC_ONCE = 1;
        }
        if(strcmp(argv[a],"--net-debug") == 0){
            setNetworkDebug(1);
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
        int chdir_r = chdir(fileName);
        if (chdir_r != 0) {
            printf("Error opening %s, quitting.\n", fileName);
            return -1;
        }
        if (PIPE) {
            char buffer[32];
            for (;;) {
                int n = read(STDIN_FILENO, buffer, 32);
                write(socket, buffer, n);
            }
        } else {
            if(isDir(fileName)){
                DIR* dir = opendir(fileName);
                struct dirent* de = NULL;
                do {
                    if(strlen(de->d_name) == 0 || de->d_name[0] == '.') break;
                    size_t ll = strlen(fileName) + strlen(de->d_name) + 2;
                    char nfile_path[ll];
                    concatenatePath(nfile_path,fileName,de->d_name);
                } while (de != NULL);
            }
        }
        closeSock(socket);
    } else {
        ensureHasPath(fileName);
        mkdir(fileName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        int chdir_r = chdir(fileName);
        if (chdir_r != 0) {
            printf("Error opening %s, quitting.\n", fileName);
            return -1;
        }
        int serverSocket = getSocketAsServer(SELF_PORT);
        if (PIPE) {
            int transSocket = listenAtSocket(serverSocket);
            char buffer[32];
            for (;;) {
                int n = read(transSocket, buffer, 32);
                write(STDOUT_FILENO, buffer, n);
            }
            closeSock(transSocket);
        }
        while (!INTERRUPTED) {
            printf("Will listen at ip %s at port %d\n", DEST_ADDRESS, SELF_PORT);
            int transSocket = listenAtSocket(serverSocket);
            receiveFile(transSocket);
            closeSock(transSocket);
            if (EXEC_ONCE) break;
        }
    }
    printf("Quitting normally.\n");
    return 0;
}

void onSigInt(int i)
{
    INTERRUPTED = 1;
    exit(SIGINT);
}