#include "server.h"

int main(int argc, char *argv[]) {
  int sockfd;
  char buf[MAXDATASIZE];
  int userid = 0;
  if (argc != 3) {
    fprintf(stderr, "usage: player <machine_name> <port_num>\n");
    exit(1);
  }
  // set up server
  fd_set master; // master file descriptor list
  FD_ZERO(&master);
  fd_set read_fds; // temp file descriptor list for select()
  int fdmax;       // maximum file descriptor number
  int listener;    // listen on listener, new connection on new_fd

  // build connection with ringmaster
  const char *server = argv[1];
  const char *server_port = argv[2];
  sockfd = player_connect_master(server, server_port, &master, &fdmax, &userid);

  // init as a listner
  listener = init_listener_on_player(&master, &fdmax, userid);
  printf("%d", sockfd);
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
    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // we got one!!
        if (i == listener) {
          // handle new connections
          struct sockaddr_storage remoteaddr; // connector's address information
          accept_new_connection(listener, &remoteaddr, &fdmax, &master);
          updateClientList(&client_list, &remoteaddr);
        } else {
          // handle data from a client
          int nbytes;
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            // got error or connection closed by client
            disconZombie(nbytes, i, &master);
          } else {
            // we got some data from a client
          }
        } // END handle data from client
      }   // END got new incoming connection
    }     // END looping through file descriptors
  }       // END for(;;)--and you thought it would never end!

  return EXIT_SUCCESS;

  /*
  close(sockfd);

  return 0;*/
}
