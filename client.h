#ifndef __CLIENT_H__
#define __CLIENT_H__
#include "base.h"
// player
const char *fetchIp(const char *hostname);
int logIn(const char *ip, const char *port, fd_set *master, int *fdmax,
          int *userid, int *num_players);
void connectNeighs(int sockfd, int *fdmax, fd_set *master, int *neigh,
                   int userid);
void readyForGame(int sockfd);
void playWithPotato(int sockfd, int fdmax, fd_set master, int userid,
                    int *neigh, int num_players);
#endif
