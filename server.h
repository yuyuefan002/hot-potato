#ifndef __SERVER_H__
#define __SERVER_H__
#include "base.h"

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
// server
void verifyArgs(char *port, int num_player, int num_hop);
int init(const char *hostname, const char *port);
void printSysInfo(int num_players, int num_hops);
void waitingPlayer(int listener, int *fdmax, fd_set *master, int num_players);
void preparePotato(int fdmax, fd_set *master, int num_players);
void kickOff(int fdmax, int num_players, int num_hops);
char *runGame(int fdmax, fd_set *master);
void endGame(int fdmax, fd_set *master, const char *trace);

#endif
