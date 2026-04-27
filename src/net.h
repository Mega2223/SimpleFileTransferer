#ifndef NET_H
#define NET_H

int getSocketAsServer(char* ip_addr, int port);

int getSocketAsClient(char *server_addr, int server_port);

void closeSock(int fn);

#endif