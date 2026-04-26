#ifndef NET_C
#define NET_C

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

// Returns fileno
int getSocketAsServer(char* ip_addr, int port){
    struct sockaddr_in self, dest;

    int sock = socket(AF_INET,SOCK_STREAM,0);

    self.sin_family = AF_INET;
    self.sin_port = port;
    self.sin_addr.s_addr = inet_addr("127.0.0.1");

    int bind_err = bind(sock,
        (const struct sockaddr *)(&self),
        sizeof(self));

    if(bind_err != 0){
        const char* errmsg = "Socket binding error";
        err(errmsg);
        exit(-1);
    }

    listen(sock,1);
    unsigned int c_size = sizeof(dest);
    sock = accept(sock, (struct sockaddr*) &dest ,&c_size);

    info("Server socket connected\n");

    return sock;
}

int getSocketAsClient(char* server_addr, int server_port){
    struct sockaddr_in self, server;

    int sock = socket(AF_INET,SOCK_STREAM,0);

    server.sin_family = AF_INET;
    server.sin_port = server_port;
    server.sin_addr.s_addr = inet_addr(server_addr);
    printf("Sending to %s\n",server_addr);

    int con_err = connect(sock, (struct sockaddr *) &server, sizeof server);

    if(con_err != 0){
        err("Connection error\n");
        return -1;
    }

    return sock;
}

#endif