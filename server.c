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

void verifyArgs(char *port, int num_player, int num_hop) {
  int num = atoi(port);
  if (num < 1024 || num > 65536) {
    fprintf(stderr, "port num should between 1024 and 65536\n");
    exit(EXIT_FAILURE);
  }
  if (num_player <= 1) {
    fprintf(stderr, "player number should be larger than 1\n");
    exit(EXIT_FAILURE);
  }
  if (num_hop < 0 || num_hop > 512) {
    fprintf(stderr, "hop number should be between 0 and 512\n");
    exit(EXIT_FAILURE);
  }
}

void printSysInfo(int num_players, int num_hops) {
  printf("Potato Ringmaster\n");
  printf("Players = %d\n", num_players);
  printf("Hops = %d\n", num_hops);
}
void printStartInfo(int player) {
  printf("Ready to start the game, sending potato to player %d\n", player);
}

void printTrace(const char *trace) {
  printf("Trace of potato:\n");
  printf("%s\n", trace);
}

void printPlayerReadyInfo(int player_num) {
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
  char str[20] = "";
  sprintf(str, "%d,%d", id, total_num);
  int size = sizeof(str);
  if (sendall(new_fd, str, &size) == -1) {
    perror("send");
    return -1;
  }
  return 0;
}

int send_neigh_info(int new_fd, int current_id, int num_players,
                    client_list_t client_list) {
  char str[40] = "";
  if (current_id == 0) {
    sprintf(str, "0:");
  } else if (current_id == num_players - 1) {
    struct in_addr ip = client_list.list[current_id - 1]->sin_addr;
    char ip1[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, ip1, INET_ADDRSTRLEN);
    int port = client_list.list[current_id - 1]->sin_port;
    ip = client_list.list[0]->sin_addr;
    char ip2[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, ip2, INET_ADDRSTRLEN);
    int port2 = client_list.list[0]->sin_port;
    sprintf(str, "2:%s,%d:%s,%d:", ip1, port, ip2, port2);
  } else {
    struct in_addr ip = client_list.list[current_id - 1]->sin_addr;
    char ip1[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, ip1, INET_ADDRSTRLEN);
    int port = client_list.list[current_id - 1]->sin_port;
    sprintf(str, "1:%s,%d:", ip1, port);
  }
  if (send(new_fd, str, sizeof(str), 0) == -1) {
    perror("send");
    return -1;
  }
  return 0;
}
void updateClientList(client_list_t *client_list, const char *ip,
                      const char *port) {
  (*client_list).list =
      realloc((*client_list).list,
              ((*client_list).size + 1) * sizeof(*(*client_list).list));
  client_info_t *temp = malloc(sizeof(**(*client_list).list));
  inet_pton(AF_INET, ip, &(temp->sin_addr)); // IPv4
  temp->sin_port = atoi(port);
  (*client_list).list[(*client_list).size++] = temp;
}
char *wait_client_listener(int new_fd) {
  char *buf = malloc(20);
  int numbytes;
  if ((numbytes = recv(new_fd, buf, 20 - 1, 0)) == -1) {
    perror("recv");
    exit(EXIT_FAILURE);
  }
  if (numbytes == 0) {
    fprintf(stderr, "server: socket %d hung up\n", new_fd);
    exit(EXIT_FAILURE);
  }
  buf[numbytes] = '\0';
  return buf;
}

/*
  setConnection
  This function set up connection between master and player, player and its
  neigh
  Input:
  new_fd: player socket fd
  current_id: player id determine who are neighs
  num_players: total player
  client_list: player info
*/
void setConnection(int new_fd, int current_id, int num_players,
                   client_list_t *client_list) {
  if (wait_client_ready(new_fd) == -1) {
    fprintf(stderr, "fail to receive ready info from client\n");
    exit(EXIT_FAILURE);
  }
  if (send_client_id(new_fd, current_id, num_players) == -1) {
    fprintf(stderr, "fail to send info to client\n");
    exit(EXIT_FAILURE);
  }
  char *buf = wait_client_listener(new_fd);
  char ip[INET_ADDRSTRLEN];
  char port[10] = "";

  interpIpPort(buf, 0, strlen(buf), ip, port);
  updateClientList(client_list, ip, port);
  if (send_neigh_info(new_fd, current_id, num_players, *client_list) == -1) {
    fprintf(stderr, "fail to set up connection with neighs\n");
    exit(EXIT_FAILURE);
  }
  free(buf);
}

char *updateTrace(char *buf, char *trace) {
  int i = 3;
  int id = 0;
  while (buf[i] != '\0') {
    id = id * 10 + buf[i] - '0';
    i++;
  }
  char id_str[5];
  char *temp = trace;
  if (trace != NULL) {
    sprintf(id_str, ",%d", id);
    trace = append(trace, id_str);
    free(temp);
  } else {
    sprintf(id_str, "%d", id);
    trace = append(id_str, "");
  }
  return trace;
}

void waitingPlayer(int listener, int *fdmax, fd_set *master, int num_players) {
  int current_id = 0;
  client_list_t client_list; // run through the existing connections
  client_list.size = 0;
  client_list.list = NULL;
  while (current_id < num_players) {
    // handle new connections
    int newfd = accNewConnection(listener, fdmax, master);
    setConnection(newfd, current_id, num_players, &client_list);
    printPlayerReadyInfo(current_id++);
  }
  close(listener);
  FD_CLR(listener, master);
  for (size_t i = 0; i < client_list.size; i++) {
    free(client_list.list[i]);
  }
  free(client_list.list);
}
/*
  preparePotato
  This function wait until all play send "ready"

  Input:
  fdmax:
  master:
  numplayers:
 */
void preparePotato(int fdmax, fd_set *master, int num_players) {
  int ready = 0;
  while (ready < num_players) {
    fd_set read_fds = *master;
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }
    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // we got one!!
        int nbytes;
        char buf[MAXDATASIZE];
        if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
          // got error or connection closed by client
          disconZombie(nbytes, i, master);
        else if (strcmp(buf, "ready") == 0)
          ready++;
      }
    }
  }
}
void kickOff(int fdmax, int num_players, int num_hops) {
  srand((unsigned int)time(NULL) + num_players);
  int random = rand() % num_players;
  char str[20] = "";
  sprintf(str, "%d", num_hops);
  int size = sizeof(str);
  if (sendall(fdmax - random, str, &size) == -1) {
    perror("send");
    exit(EXIT_FAILURE);
  }
  printStartInfo(num_players - 1 - random);
}
char *runGame(int fdmax, fd_set *master) {
  int end = 0;
  char *trace = NULL;
  while (end == 0) {
    fd_set read_fds = *master;
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }
    // looking for data to read
    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // we got one!!
                                    // handle data from a client
        char buf[MAXDATASIZE];
        int nbytes;
        if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
          // got error or connection closed by client
          disconZombie(nbytes, i, master);
        } else {
          if (buf[0] == 'i' && buf[1] == 'd' && buf[2] == ':') {
            send(i, "ack", sizeof("ack"), 0);
            trace = updateTrace(buf, trace);
          }
          if (strcmp(buf, "end") == 0)
            end = 1;
        }
      }
    }
  }
  return trace;
}
void endGame(int fdmax, fd_set *master, const char *trace) {
  for (int i = 0; i <= fdmax; i++) {
    // send to everyone!
    if (FD_ISSET(i, master)) {
      if (send(i, "end", sizeof("end"), 0) == -1) {
        perror("send\n");
        exit(EXIT_FAILURE);
      }
    }
  }
  printTrace(trace);
  closeall(fdmax, master);
}
