#include "server.h"

int client_init(const char *hostname, const char *port) {
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and connect to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(servinfo); // all done with this structure
  return sockfd;
}
char *random_port(int player_id) {
  srand((unsigned int)time(NULL) + player_id);
  int random = 0;
  while (random <= 1024) {
    random = rand() % 65536;
  }
  char *port = malloc(10 * sizeof(char));
  sprintf(port, "%d", random);
  return port;
}
void tell_master_my_listening_ip(int sockfd, const char *ip, const char *port) {
  int len = strlen(ip) + strlen(port) + 1;
  char str[len];
  sprintf(str, "%s,%s", ip, port);
  send(sockfd, str, sizeof(str), 0);
}
int init_rand_port(const char *hostname, int player_id, int masterfd) {
  struct addrinfo hints, *servinfo;
  struct hostent *he = gethostbyname(hostname);
  struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;
  const char *ip = inet_ntoa(*addr_list[0]);
  int sockfd; // listen on sockfd
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  int rv;
  int yes = 1;
  char *port = NULL;

  while (1) {
    port = random_port(player_id);

    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      exit(EXIT_FAILURE);
    }
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                         servinfo->ai_protocol)) == -1) {
      perror("server: socket");
      exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt failed\n");
      exit(EXIT_FAILURE);
    }
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
      close(sockfd);
      perror("server:bind\n");
      free(port);
      continue;
    }
    break;
  }

  freeaddrinfo(servinfo); // all done with this structure
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }
  tell_master_my_listening_ip(masterfd, ip, port);
  free(port);
  return sockfd;
}

int init_listener_on_player(int sockfd, fd_set *master, int *fdmax,
                            int player_id) {
  char hostname[128];
  gethostname(hostname, sizeof hostname);

  int listener = init_rand_port(hostname, player_id, sockfd);
  FD_SET(listener, master);
  // keep track of the biggest file descriptor
  if (listener > *fdmax)
    *fdmax = listener;
  return listener;
}

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

void printSysInfo(int player_num, int hop_num) {
  printf("Potato Ringmaster\n");
  printf("Players = %d\n", player_num);
  printf("Hops = %d\n", hop_num);
}
void print_start_info(int startplayer_num) {
  printf("Ready to start the game, sending potato to player <%d>\n",
         startplayer_num);
}
void print_forward_info(int my_id, int num_players, int dir) {
  int neigh;
  if (dir == 0) {
    if (my_id == 1)
      neigh = num_players;
    else
      neigh = my_id - 1;
  } else {
    if (my_id == num_players)
      neigh = 1;
    else
      neigh = my_id + 1;
  }
  printf("Sending potato to %d\n", neigh);
}
char *append(const char *str1, const char *str2) {
  char *new;
  if ((new = malloc(strlen(str1) + strlen(str2) + 1)) != NULL) {
    new[0] = '\0';
    strcat(new, str1);
    strcat(new, str2);
  } else {
    perror("malloc\n");
    exit(EXIT_FAILURE);
  }
  return new;
}
char *receive_potato(char *buf, int *hop) {
  int i = 0;
  while (buf[i] != '\0') {
    *hop = *hop * 10 + buf[i] - '0';
    i++;
  }
  *hop -= 1;
  char hop_str[10];
  sprintf(hop_str, "%d", *hop);
  return append(hop_str, "");
}
void send_out_potato(int *neigh, int fdmax, fd_set master, char *msg,
                     int player_id, int num_players, int hop) {
  srand((unsigned int)time(NULL) + hop);
  int random = rand() % 2;
  int size = sizeof(msg);

  if (!FD_ISSET(neigh[random], &master)) {
    fprintf(stderr, "socket unset\n");
    exit(EXIT_FAILURE);
  }

  if (sendall(neigh[random], msg, &size) == -1) {
    perror("send");
    exit(EXIT_FAILURE);
  }
  print_forward_info(player_id, num_players, random);
}
void start_game(int fdmax, fd_set master, int num_players, int num_hops) {
  srand((unsigned int)time(NULL) + num_players);
  int random = rand() % num_players;
  char str[20] = "";
  sprintf(str, "%d", num_hops);
  int size = sizeof(str);
  printf("%d\n", fdmax - random);
  if (sendall(fdmax - random, str, &size) == -1) {
    perror("send");
    exit(EXIT_FAILURE);
  }
  print_start_info(num_players - random);
  /*for (int j = 0; j <= fdmax; j++) {
    // send to everyone!
    if (FD_ISSET(j, &master)) {
      printf("%d\n", j);
      // except the listener and ourselves

      if (send(j, "hi", sizeof("hi"), 0) == -1) {
        perror("send");
      }
    }
  }*/
}

void print_trace(char *trace) {
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

int accNewConnection(int listener, int *fdmax, fd_set *master) {
  int newfd;
  struct sockaddr_storage remoteaddr; // connector's address information

  socklen_t addrlen = sizeof remoteaddr;
  if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) ==
      -1) {
    perror("accept");
    exit(EXIT_FAILURE);
  } else {
    FD_SET(newfd, master); // add to master set
    if (newfd > *fdmax) {  // keep track of the max
      *fdmax = newfd;
    }
    return newfd;
  }
}
int send_neigh_info(int new_fd, int current_id, int num_players,
                    client_list_t client_list) {
  char str[40] = "";
  if (current_id == 1) {
    sprintf(str, "0:");
  } else if (current_id == num_players) {
    struct in_addr ip = client_list.list[current_id - 2]->sin_addr;
    char ip1[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, ip1, INET_ADDRSTRLEN);
    int port = client_list.list[current_id - 2]->sin_port;
    ip = client_list.list[0]->sin_addr;
    char ip2[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, ip2, INET_ADDRSTRLEN);
    int port2 = client_list.list[0]->sin_port;
    sprintf(str, "2:%s,%d:%s,%d:", ip1, port, ip2, port2);
  } else {
    struct in_addr ip = client_list.list[current_id - 2]->sin_addr;
    char ip1[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, ip1, INET_ADDRSTRLEN);
    int port = client_list.list[current_id - 2]->sin_port;
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
  interpIpPort
  This function interperate string into ip and port

  Input:
  buf: whole string
  start: start point
  end:
 */
char *substr(const char *buf, int start, int end) {
  char *sub = malloc((end - start));
  int len = end - start;
  strncpy(sub, buf + start, len);
  sub[len] = '\0';
  return sub;
}
void interpIpPort(const char *buf, int start, int end, char *ip, char *port) {
  int i = start;
  while (buf[i] != ',') {
    i++;
  }
  const char *ip_tmp = substr(buf, start, i);
  const char *port_tmp = substr(buf, i + 1, end);
  sprintf(ip, "%s", ip_tmp);
  sprintf(port, "%s", port_tmp);
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

void player_decode_id(int *userid, int *total_num, const char *buf) {
  int i = 0;
  while (buf[i] != ',') {
    *userid = *userid * 10 + buf[i] - '0';
    i++;
  }
  i++;
  while (buf[i] != '\0') {
    *total_num = *total_num * 10 + buf[i] - '0';
    i++;
  }
}
int connect_server(const char *server, const char *server_port, fd_set *master,
                   int *fdmax) {
  int sockfd = client_init(server, server_port);
  FD_SET(sockfd, master);
  *fdmax = sockfd;
  return sockfd;
}
int player_connect_master(const char *server, const char *server_port,
                          fd_set *master, int *fdmax, int *userid,
                          int *num_players) {
  int sockfd = connect_server(server, server_port, master, fdmax);
  char *str = "ready";
  send(sockfd, str, sizeof(str), 0);
  int numbytes;
  char buf[MAXDATASIZE];
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
    perror("recv");
    exit(1);
  }
  buf[numbytes] = '\0';
  player_decode_id(userid, num_players, buf);
  printf("Connected as player %d out of %d total players\n", *userid,
         *num_players);
  return sockfd;
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
void end_game(int fdmax, fd_set master) {
  for (int i = 0; i <= fdmax; i++) {
    // send to everyone!
    if (FD_ISSET(i, &master)) {
      if (send(i, "end", sizeof("end"), 0) == -1) {
        perror("send\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}
void closeall(int fdmax, fd_set *master) {
  for (int i = 0; i <= fdmax; i++) {
    // send to everyone!
    if (FD_ISSET(i, master)) {
      close(i);
      FD_CLR(i, master);
    }
  }
}
