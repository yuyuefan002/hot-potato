#include "server.h"

int main(int argc, char **argv) {

  if (argc != 4) {
    fprintf(stderr, "Usage: ringmaster <port_num> <num_players> <num_hops>\n");
    return EXIT_FAILURE;
  }

  // take arguments
  const char *hostname = NULL;
  char *port = argv[1];
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);
  char *trace = NULL;
  verifyArgs(port, num_players, num_hops);

  // set up server
  fd_set master; // master file descriptor list
  FD_ZERO(&master);
  int fdmax;
  int listener;

  listener = init(hostname, port);
  FD_SET(listener, &master);
  fdmax = listener;

  printSysInfo(num_players, num_hops);

  // build connection between master&player, player&neigh
  waitingPlayer(listener, &fdmax, &master, num_players);

  // waiting all connect be set up
  preparePotato(fdmax, &master, num_players);

  // send potato to random player
  kickOff(fdmax, num_players, num_hops);

  // run game, track the potato
  trace = runGame(fdmax, &master);

  // take potato back, clean all connections
  endGame(fdmax, &master, trace);

  free(trace);
  return EXIT_SUCCESS;
}
