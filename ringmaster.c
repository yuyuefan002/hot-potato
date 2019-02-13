#include "potato.h"
#include "server.h"
#include <signal.h>
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
  if (verify_args(port, num_players, num_hops) == false) {
    return EXIT_FAILURE;
  }

  // set up server
  fd_set master; // master file descriptor list
  FD_ZERO(&master);
  fd_set read_fds;
  int fdmax;
  int listener;

  char buf[256]; // buffer for client data
  int nbytes;

  listener = init(hostname, port);
  FD_SET(listener, &master);
  fdmax = listener;

  printSysInfo(num_players, num_hops);
  // build connection with each player and setting up the game
  int i, current_id = 1;
  client_list_t client_list; // run through the existing connections
  client_list.size = 0;
  client_list.list = NULL;
  while (current_id <= num_players) {
    // handle new connections
    int newfd = accNewConnection(listener, &fdmax, &master);
    setConnection(newfd, current_id, num_players, &client_list);
    printPlayerReadyInfo(current_id++);
  }
  close(listener);
  FD_CLR(listener, &master);
  int ready = 0;
  while (ready < num_players) {
    read_fds = master;
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }
    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // we got one!!

        if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
          // got error or connection closed by client
          disconZombie(nbytes, i, &master);
        } else {
          if (strcmp(buf, "ready") == 0)
            ready++;
        }
      }
    }
  }
  printf("all player connected\n");
  start_game(fdmax, master, num_players, num_hops);
  int end = 0;
  while (end == 0) {
    read_fds = master; // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }
    // looking for data to read
    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // we got one!!
        if (i != listener) {
          // handle data from a client
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            // got error or connection closed by client
            disconZombie(nbytes, i, &master);
          } else {
            printf("%s\n", buf);
            if (buf[0] == 'i' && buf[1] == 'd' && buf[2] == ':') {
              send(i, "ack", sizeof("ack"), 0);
              trace = updateTrace(buf, trace);
            }
            if (strcmp(buf, "end") == 0)
              end = 1;

            // we got some data from a client
          }
        } // END handle data from client
      }   // END got new incoming connection
    }     // END looping through file descriptors
  }       // END for(;;)--and you thought it would never end!
  end_game(fdmax, master);
  print_trace(trace);
  closeall(fdmax, &master);
  for (size_t i = 0; i < client_list.size; i++) {
    free(client_list.list[i]);
  }
  free(client_list.list);
  free(trace);
  return EXIT_SUCCESS;
  // initialize the potato and send it out
  // receive the potato
  // print a trace of the game and end the game
  // gabbage collection
}
