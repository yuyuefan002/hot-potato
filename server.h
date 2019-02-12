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
#include <time.h>
#include <unistd.h>
#define BACKLOG 10      // how many pending connection queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once
struct _client_info_t {
  struct in_addr sin_addr;
  unsigned short int sin_port;
};
struct _client_list_t {
  struct _client_info_t **list;
  size_t size;
};
typedef struct _client_info_t client_info_t;
typedef struct _client_list_t client_list_t;
bool verify_args(char *port, int num_player, int num_hop);
int init(const char *hostname, const char *port);
int client_init(const char *hostname, const char *port);
void set_up_connection(int new_fd, int current_id, int num_players,
                       client_list_t client_list);
int wait_client_ready(int new_fd);
int send_client_id(int new_fd, int id, int total_num);
void print_system_info(int player_num, int hop_num);
void print_player_ready_info(int player_num);
int sendall(int s, char *buf, int *len);
int accept_new_connection(int listener, struct sockaddr_storage *remoteaddr,
                          int *fdmax, fd_set *master);
void disconZombie(int nbytes, int i, fd_set *master);
void updateClientList(client_list_t *client_list,
                      struct sockaddr_storage *remoteaddr);
int init_listener_on_player(fd_set *master, int *fdmax, int player_id);
int player_connect_master(const char *server, const char *server_port,
                          fd_set *master, int *fdmax, int *userid);
#endif
