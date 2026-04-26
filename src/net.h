#ifndef NET_H
#define NET_H

int getSocketAsServer(char* ip_addr, int port);

int getSocketAsClient(char* server_addr, int self_port, int server_port);

#endif