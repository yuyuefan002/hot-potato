#ifndef __SERVER_H__
#define __SERVER_H__
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
void setConnection(int new_fd, int current_id, int num_players,
                   client_list_t *client_list);
int wait_client_ready(int new_fd);

void printSysInfo(int num_players, int num_hops);
int sendall(int s, char *buf, int *len);
int accNewConnection(int listener, int *fdmax, fd_set *master);
void disconZombie(int nbytes, int i, fd_set *master);
int init_listener_on_player(int sockfd, fd_set *master, int *fdmax,
                            int player_id);
int connectServer(const char **argv, fd_set *master, int *fdmax, int *userid,
                  int *num_players);
void interpIpPort(const char *buf, int start, int end, char *ip, char *port);

void send_out_potato(int *neigh, int fdmax, fd_set master, char *msg,
                     int player_id, int num_players, int hop);
char *receive_potato(char *buf, int *hop);
char *updateTrace(char *buf, char *trace);
void closeall(int fdmax, fd_set *master);

void waitingPlayer(int listener, int *fdmax, fd_set *master, int num_players);
void preparePotato(int fdmax, fd_set *master, int num_players);
void kickOff(int fdmax, int num_players, int num_hops);
char *runGame(int fdmax, fd_set *master);
void endGame(int fdmax, fd_set *master, const char *trace);
void connectNeighs(int sockfd, int *fdmax, fd_set *master, int *neigh,
                   int userid);
void playWithPotato(int sockfd, int fdmax, fd_set master, int userid,
                    int *neigh, int num_players);
void readyForGame(int sockfd);
#endif
