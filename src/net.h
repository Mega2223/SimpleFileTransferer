#ifndef NET_H
#define NET_H

void setNetworkDebug(int level);

void printBytes(void* loc, int bytes, int offset);

int getNetworkDebug();

int getSocketAsServer(int port);

int getSocketAsClient(char *server_addr, int server_port);

void closeSock(int fn);

void onSigPipe(int s);

int sigpipe_status();

int listenAtSocket(int sock);

#define gotSigPipe (sigpipe_status())

#endif