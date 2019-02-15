#ifndef __BASE_H__
#define __BASE_H__
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define BACKLOG 10      // how many pending connection queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once

int sendall(int s, char *buf, int *len);
char *append(const char *str1, const char *str2);

void substr(char *newstr, const char *buf, int start, int end);
void swap(int *a, int *b);
int accNewConnection(int listener, int *fdmax, fd_set *master);
void disconZombie(int nbytes, int i, fd_set *master);
void closeall(int fdmax, fd_set *master);
void interpIpPort(const char *buf, int start, int end, char *ip, char *port);
#endif
