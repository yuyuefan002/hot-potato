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
  if (verify_args(port, num_players, num_hops) == false) {
    return EXIT_FAILURE;
  }

  // set up server
  fd_set master; // master file descriptor list
  FD_ZERO(&master);
  fd_set read_fds; // temp file descriptor list for select()
  int fdmax;       // maximum file descriptor number
  int listener;    // listen on listener, new connection on new_fd

  struct sockaddr_storage remoteaddr; // connector's address information
  char buf[256];                      // buffer for client data
  int nbytes;
  listener = init(hostname, port);
  print_system_info(num_players, num_hops);
  FD_SET(listener, &master);
  // keep track of the biggest file descriptor
  fdmax = listener; // so far, it's this one
  // init potato
  potato_t potato;
  potato.hop = num_hops;
  potato.trace = NULL;

  // build connection with each player and setting up the game
  int i, current_id = 1;
  client_list_t client_list; // run through the existing connections
  client_list.size = 0;
  client_list.list = NULL;
  for (;;) {
    read_fds = master; // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }
    // looking for data to read
    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // we got one!!
        if (i == listener) {
          // handle new connections
          int newfd =
              accept_new_connection(listener, &remoteaddr, &fdmax, &master);

          updateClientList(&client_list, &remoteaddr);
          printf("%ld,%d\n", client_list.size, current_id);
          set_up_connection(newfd, current_id, num_players, client_list);
          print_player_ready_info(current_id++);
          if (current_id > num_players) {
            close(listener);
            FD_CLR(listener, &master);
          }
        } else {
          // handle data from a client
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            // got error or connection closed by client
            disconZombie(nbytes, i, &master);
          } else {
            printf("%d\n", potato.hop);
            // we got some data from a client
          }
        } // END handle data from client
      }   // END got new incoming connection
    }     // END looping through file descriptors
  }       // END for(;;)--and you thought it would never end!

  return EXIT_SUCCESS;
  // initialize the potato and send it out
  // receive the potato
  // print a trace of the game and end the game
  // gabbage collection
}
