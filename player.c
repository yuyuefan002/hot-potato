#include "server.h"
void connect_neigh(const char *buf, int start, int end, fd_set *master,
                   int *fdmax) {
  char ip[INET_ADDRSTRLEN];
  char port[10] = "";
  interpret_ip(buf, start, end, ip, port);
  connect_server(ip, port, master, fdmax);
  printf("connect ip:%s,port:%s success\n", ip, port);
}
void decode_neigh(const char *buf, fd_set *master, int *fdmax) {
  int i = 0;
  int neighs = 0;
  while (buf[i] != ':') {
    neighs = neighs * 10 + buf[i] - '0';
    i++;
  }
  while (neighs) {
    i++;
    int start = i;
    while (buf[i] != ':') {
      i++;
    }
    int end = i;
    connect_neigh(buf, start, end, master, fdmax);
    neighs--;
  }
}
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

  listener = init_listener_on_player(sockfd, &master, &fdmax, userid);

  memset(buf, 0, sizeof(buf));
  int numbytes;
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
    perror("recv");
    exit(1);
  }
  buf[numbytes] = '\0';
  decode_neigh(buf, &master, &fdmax);
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
