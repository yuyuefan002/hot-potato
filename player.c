#include "client.h"

int main(int argc, char *argv[]) {
  int sockfd;
  int userid = 0;
  int num_players = 0;
  if (argc != 3) {
    fprintf(stderr, "usage: player <machine_name> <port_num>\n");
    exit(1);
  }
  // set up server
  fd_set master; // master file descriptor list
  FD_ZERO(&master);
  int fdmax; // maximum file descriptor number
  int neigh[2];
  // connect server and get id
  sockfd = logIn((const char **)argv, &master, &fdmax, &userid, &num_players);
  // connect neighs
  connectNeighs(sockfd, &fdmax, &master, neigh, userid);

  // send ready msg to server
  readyForGame(sockfd);

  // transfer potato among players, send id to server if I receive potato
  playWithPotato(sockfd, fdmax, master, userid, neigh, num_players);

  // end the game
  closeall(fdmax, &master);
  return EXIT_SUCCESS;
}
