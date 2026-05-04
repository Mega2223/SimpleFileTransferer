#ifndef NET_C
#define NET_C

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

// Returns fileno
int getSocketAsServer(int port){
    struct sockaddr_in self;
    self.sin_family = AF_INET;
    self.sin_port = htons(port);
    self.sin_addr.s_addr = INADDR_ANY;

    int bind_err = 0;
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0){
        printf("Error creating isocket");
        exit(-1);
    }

    bind_err = bind(sock,
        (const struct sockaddr *)(&self),
        sizeof(self));

    if(bind_err != 0){
        printf("Socket binding error: %d %d\n",bind_err,errno);
        exit(-1);
    }

    return sock;
}

int listenAtSocket(int sock)
{
    struct sockaddr_in sender;
    listen(sock,10);
    unsigned int c_size = sizeof(sender);
    int rsock = accept(sock, (struct sockaddr*) &sender ,&c_size);

    printf("Server socket connected to sender %d:%d\n", sender.sin_addr.s_addr, sender.sin_port);
    return rsock;
}

int getSocketAsClient(char* server_addr, uint16_t server_port){
    struct sockaddr_in server;

    int sock = socket(AF_INET,SOCK_STREAM,0);

    if(sock < 0){
        printf("Error creating isocket");
        exit(-1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr(server_addr);
    printf("Sending to %s with port %d\n",server_addr,server_port);

    int con_err = connect(sock, (struct sockaddr *) &server, sizeof server);

    if(con_err != 0){
        printf("Connection error: %d, %d\n", con_err, errno);
        return -1;
    }
    printf("Connected\n");

    return sock;
}

void closeSock(int fn) {
    close(fn);
}

int sigpipe_s = 0;

void onSigPipe(int s) {
    if (sigpipe_s == 0) {
        printf("Got SIGPIPE\n");
    }
    sigpipe_s = 1;
}

int sigpipe_status()
{
    return sigpipe_s;
}

#endif