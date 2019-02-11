#ifndef __SERVER_H__
#define __SERVER_H__
#include "potato.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define BACKLOG 10 // how many pending connection queue will hold
bool verify_args(char *port, int num_player, int num_hop);
int init(const char *hostname, const char *port);
void set_up_connection(int new_fd, int current_id, int num_players);
int wait_client_ready(int new_fd);
int send_client_id(int new_fd, int id, int total_num);
void print_system_info(int player_num, int hop_num);
void print_player_ready_info(int player_num);
int sendall(int s, char *buf, int *len);
int accept_new_connection(int listener, struct sockaddr_storage *remoteaddr,
                          int *fdmax, fd_set *master);
#endif
