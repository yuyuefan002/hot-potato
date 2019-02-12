#include "server.h"
int init(const char *hostname, const char *port) {
  struct addrinfo hints, *servinfo, *p;
  int sockfd; // listen on sockfd
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  int rv;
  int yes = 1;
  if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(EXIT_FAILURE);
  }
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt failed\n");
      return EXIT_FAILURE;
    }
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server:bind\n");
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo); // all done with this structure
  if (p == NULL) {
    fprintf(stderr, "bind failed\n");
    exit(EXIT_FAILURE);
  }
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }
  return sockfd;
}
bool verify_port_num(char *port) {
  int num = atoi(port);
  if (num < 1024 || num > 65536) {
    return false;
  }
  return true;
}
bool verify_num_player(int num) { return num > 1; }
bool verify_num_hop(int num) { return num >= 0 && num <= 512; }
bool verify_args(char *port, int num_player, int num_hop) {
  if (verify_port_num(port) == false) {
    fprintf(stderr, "port number should be between 1024 and 65536\n");
    return false;
  }
  if (verify_num_player(num_player) == false) {
    fprintf(stderr, "player number should be larger than 1\n");
    return false;
  }
  if (verify_num_hop(num_hop) == false) {
    fprintf(stderr, "hop number should be between 0 and 512\n");
    return false;
  }
  return true;
}

void print_system_info(int player_num, int hop_num) {
  printf("Potato Ringmaster\n");
  printf("Players = %d\n", player_num);
  printf("Hops = %d\n", hop_num);
}
void print_start_info(int startplayer_num) {
  printf("Ready to start the game, sending potato to player <%d>\n",
         startplayer_num);
}
void print_trace(potato_t potato) { printf("Trace of potato:\n"); }

void print_player_ready_info(int player_num) {
  printf("Player %d is ready to play\n", player_num);
}

int wait_client_ready(int new_fd) {
  char buf[10] = "";
  int numbytes;

  while (strcmp(buf, "ready") != 0) {
    if ((numbytes = recv(new_fd, buf, 10 - 1, 0)) == -1) {
      perror("recv");
      return -1;
    }
    if (numbytes == 0) {
      fprintf(stderr, "server: socket %d hung up\n", new_fd);
      return -1;
    }
    buf[numbytes] = '\0';
  }
  return 0;
}
int send_client_id(int new_fd, int id, int total_num) {
  char str[10] = "";
  sprintf(str, "%d,%d", id, total_num);
  int size = sizeof(str);
  if (sendall(new_fd, str, &size) == -1) {
    perror("send");
    return -1;
  }
  return 0;
}

int sendall(int s, char *buf, int *len) {
  int total = 0;        // how many bytes we've sent
  int bytesleft = *len; // how many we have left to send
  int n;

  while (total < *len) {
    n = send(s, buf + total, bytesleft, 0);
    if (n == -1) {
      break;
    }
    total += n;
    bytesleft -= n;
  }

  *len = total; // return number actually sent here

  return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

int accept_new_connection(int listener, struct sockaddr_storage *remoteaddr,
                          int *fdmax, fd_set *master) {
  socklen_t addrlen;
  int newfd;
  addrlen = sizeof *remoteaddr;
  newfd = accept(listener, (struct sockaddr *)remoteaddr, &addrlen);

  if (newfd == -1) {
    perror("accept");
  } else {
    FD_SET(newfd, master); // add to master set
    if (newfd > *fdmax) {  // keep track of the max
      *fdmax = newfd;
    }
  }
  return newfd;
}
int send_neigh_info(int new_fd, int current_id, int num_players,
                    client_list_t client_list) {
  char str[30] = "";
  if (current_id == 1) {
    sprintf(str, "0:");
  } else if (current_id == num_players) {
    int ip = client_list.list[current_id - 2]->sin_addr.s_addr;
    int port = client_list.list[current_id - 2]->sin_port;
    int ip2 = client_list.list[0]->sin_addr.s_addr;
    int port2 = client_list.list[0]->sin_port;
    sprintf(str, "2:%d,%d:%d,%d", ip, port, ip2, port2);
  } else {
    int ip = client_list.list[current_id - 2]->sin_addr.s_addr;
    int port = client_list.list[current_id - 2]->sin_port;
    sprintf(str, "1:%d,%d", ip, port);
  }
  int size = sizeof(str);
  if (sendall(new_fd, str, &size) == -1) {
    perror("send");
    return -1;
  }
  return 0;
}
void set_up_connection(int new_fd, int current_id, int num_players,
                       client_list_t client_list) {
  if (wait_client_ready(new_fd) == -1) {
    fprintf(stderr, "fail to receive ready info from client\n");
    close(new_fd);
    exit(EXIT_FAILURE);
  }
  if (send_client_id(new_fd, current_id, num_players) == -1)
    fprintf(stderr, "fail to send info to client\n");
  if (send_neigh_info(new_fd, current_id, num_players, client_list) == -1)
    fprintf(stderr, "fail to set up connection with neighs\n");
}
void disconZombie(int nbytes, int i, fd_set *master) {
  if (nbytes == 0) {
    // connection closed
    printf("selectserver: socket %d hung up\n", i);
  } else {
    perror("recv");
  }
  close(i);          // bye!
  FD_CLR(i, master); // remove from master set
}
void updateClientList(client_list_t *client_list,
                      struct sockaddr_storage *remoteaddr) {
  (*client_list).list =
      realloc((*client_list).list,
              ((*client_list).size + 1) * sizeof(*(*client_list).list));
  client_info_t *temp = malloc(sizeof(**(*client_list).list));
  temp->sin_addr = ((struct sockaddr_in *)remoteaddr)->sin_addr;
  temp->sin_port = ((struct sockaddr_in *)remoteaddr)->sin_port;
  (*client_list).list[(*client_list).size++] = temp;
}
