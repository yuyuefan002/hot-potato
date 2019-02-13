#include "server.h"
void tell_master_im_potato(int sockfd) {
  char msg[10] = "";
  sprintf(msg, "end");
  //  int size = sizeof(msg);
  send(sockfd, msg, sizeof(msg), 0);
}

void connect_neigh(const char *buf, int start, int end, fd_set *master,
                   int *fdmax, int *neigh, int *neigh_num) {
  char ip[INET_ADDRSTRLEN];
  char port[10] = "";
  interpIpPort(buf, start, end, ip, port);
  neigh[(*neigh_num)++] = connect_server(ip, port, master, fdmax);
  printf("connect ip:%s,port:%s success\n", ip, port);
}
int decode_neigh(const char *buf, fd_set *master, int *fdmax, int *neigh,
                 int *neigh_num) {
  int i = 0;
  int neighs = 0;
  while (buf[i] != ':') {
    neighs = neighs * 10 + buf[i] - '0';
    i++;
  }
  int left = 2 - neighs;
  while (neighs) {
    i++;
    int start = i;
    while (buf[i] != ':') {
      i++;
    }
    int end = i;
    connect_neigh(buf, start, end, master, fdmax, neigh, neigh_num);
    neighs--;
  }
  return left;
}
int main(int argc, char *argv[]) {
  int sockfd;
  char buf[MAXDATASIZE];
  int userid = 0;
  int num_players = 0;
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
  int neigh[2];
  int neigh_num = 0;
  // build connection with ringmaster
  const char *server = argv[1];
  const char *server_port = argv[2];

  sockfd = player_connect_master(server, server_port, &master, &fdmax, &userid,
                                 &num_players);

  // init as a listner

  listener = init_listener_on_player(sockfd, &master, &fdmax, userid);

  memset(buf, 0, sizeof(buf));
  int numbytes;
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
    perror("recv");
    exit(1);
  }
  buf[numbytes] = '\0';
  int left = decode_neigh(buf, &master, &fdmax, neigh, &neigh_num);
  if (left == 0) {
    close(listener);
    FD_CLR(listener, &master);
  }
  while (left) {
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
          neigh[neigh_num++] =
              accNewConnection(listener, &remoteaddr, &fdmax, &master);
          left--;
        } else {
          // handle data from a client
          int nbytes;
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            // got error or connection closed by client
            disconZombie(nbytes, i, &master);
          }
        } // END handle data from client
      }   // END got new incoming connection
    }     // END looping through file descriptors
  }
  close(listener);
  FD_CLR(listener, &master);
  if (userid == 1) {
    int temp = neigh[0];
    neigh[0] = neigh[1];
    neigh[1] = temp;
  }
  printf("ready for game\n");
  send(sockfd, "ready", sizeof("ready"), 0);
  int end = 0;
  while (end == 0) {
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
          accNewConnection(listener, &remoteaddr, &fdmax, &master);
          left--;
          if (left == 0) {
            close(listener);
            FD_CLR(listener, &master);
          }
        } else {
          // handle data from a client
          int nbytes;
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
            // got error or connection closed by client
            disconZombie(nbytes, i, &master);
          } else if (strcmp(buf, "end") == 0) {
            end = 1;
          } else {
            // we got some data from a client
            char *potato = NULL;
            int hop = 0;
            char id[10];
            sprintf(id, "id:%d", userid);
            send(sockfd, id, sizeof(id), 0);
            char ack[10] = "";
            while (strcmp(ack, "ack") != 0) {
              recv(sockfd, ack, sizeof ack, 0);
            }
            potato = receive_potato(buf, &hop);
            if (hop >= 0)
              send_out_potato(neigh, fdmax, master, potato, userid, num_players,
                              hop);
            else {
              tell_master_im_potato(sockfd);
              printf("I'm it\n");
            }
            free(potato);
          }
        } // END handle data from client
      }   // END got new incoming connection
    }     // END looping through file descriptors
  }       // END for(;;)--and you thought it would never end!

  closeall(fdmax, &master);
  return EXIT_SUCCESS;

  /*
  close(sockfd);

  return 0;*/
}
