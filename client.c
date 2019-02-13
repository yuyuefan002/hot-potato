#include "client.h"

/*
  receive_potato
  This function decode potato get hop and minus hop
 */
void receive_potato(char *buf, int *hop, char *potato) {
  int i = 0;
  while (buf[i] != '\0') {
    *hop = *hop * 10 + buf[i] - '0';
    i++;
  }
  *hop -= 1;
  sprintf(potato, "%d", *hop);
}

/*
  printForwardInfo
  This function print info about sending message to whom
 */
void printForwardInfo(int my_id, int num_players, int dir) {
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

/*
  send_out_potato
  This function randomly pick a neigh and send out the potato
 */
void send_out_potato(int *neigh, fd_set master, char *msg, int player_id,
                     int num_players, int hop) {
  srand((unsigned int)time(NULL) + hop);
  int random = rand() % 2;
  int size = sizeof(msg);

  if (!FD_ISSET(neigh[random], &master)) {
    fprintf(stderr, "socket unset\n");
    exit(EXIT_FAILURE);
  }
  if (send(neigh[random], msg, size, 0) == -1) {
    perror("send");
    exit(EXIT_FAILURE);
  }
  printForwardInfo(player_id, num_players, random);
}
/*
  random_port
  This function generate a random port
 */
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

/*
  tell_master_my_listening_ip
  This function tells master where i am listening
 */
void tell_master_my_listening_ip(int sockfd, const char *ip, const char *port) {
  int len = strlen(ip) + strlen(port) + 1;
  char str[len];
  sprintf(str, "%s,%s", ip, port);
  send(sockfd, str, sizeof(str), 0);
}

/*
  init_rand_port
  Player listen on a random port
 */
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

/*
  initListener
  This function will listen on the given port
  update masterlist
  tell server where it listening
 */
int initListener(int sockfd, fd_set *master, int *fdmax, int player_id) {
  char hostname[128];
  gethostname(hostname, sizeof hostname);

  int listener = init_rand_port(hostname, player_id, sockfd);
  FD_SET(listener, master);
  // keep track of the biggest file descriptor
  if (listener > *fdmax)
    *fdmax = listener;
  return listener;
}

/*
  connectServer
  This function init socket and connect server

  Input:
  hostname: server ip
  port: server port
 */
int connectServer(const char *hostname, const char *port) {
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
  freeaddrinfo(servinfo);
  return sockfd;
}

/*
  decodeId
  This function decode user id and total user num

  Input:
  buf: the whole string contain id and total user num
*/
void decodeId(int *userid, int *total_num, const char *buf) {
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

/*
  logIn
  This function connect server and get back player's id

  Input:
  argv: contain server ip and port
  master: fd list
  fdmax: max fdmax
  userid,num_players
  Output:
  sockfd: the socket connecting server
 */
int logIn(const char **argv, fd_set *master, int *fdmax, int *userid,
          int *num_players) {
  int sockfd = connectServer(argv[1], argv[2]);
  FD_SET(sockfd, master);
  *fdmax = sockfd;
  char *str = "ready";
  send(sockfd, str, sizeof(str), 0);
  int numbytes;
  char buf[MAXDATASIZE];
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
    perror("recv");
    exit(1);
  }
  buf[numbytes] = '\0';
  decodeId(userid, num_players, buf);
  printf("Connected as player %d out of %d total players\n", *userid,
         *num_players);
  return sockfd;
}

/*
  connect2
  This function will connect one server and update master
 */
void connect2(const char *buf, int start, int end, fd_set *master, int *fdmax,
              int *neigh, int *neigh_num) {
  char ip[INET_ADDRSTRLEN];
  char port[10] = "";
  interpIpPort(buf, start, end, ip, port);

  int newfd = connectServer(ip, port);
  neigh[(*neigh_num)++] = newfd;
  FD_SET(newfd, master);
  if (newfd > *fdmax)
    *fdmax = newfd;
}
/*
  connectNeigh
  This function will connect one neigh of current player
 */
int connectNeigh(const char *buf, fd_set *master, int *fdmax, int *neigh,
                 int *neigh_num) {
  int i = 0;
  int neighs = 0;
  // get neighs
  while (buf[i] != ':') {
    neighs = neighs * 10 + buf[i] - '0';
    i++;
  }
  int left = 2 - neighs;

  // connect neighs
  while (neighs) {
    i++;
    int start = i;
    while (buf[i] != ':')
      i++;
    int end = i;
    connect2(buf, start, end, master, fdmax, neigh, neigh_num);
    neighs--;
  }
  return left;
}
/*
  connectNeighs
  This function will connect all two neighs
 */
void connectNeighs(int sockfd, int *fdmax, fd_set *master, int *neigh,
                   int userid) {
  // init as a listner
  int listener = initListener(sockfd, master, fdmax, userid);
  char buf[MAXDATASIZE];
  int numbytes;
  int neigh_num = 0;
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
    perror("recv");
    exit(1);
  }
  buf[numbytes] = '\0';
  int left;
  if ((left = connectNeigh(buf, master, fdmax, neigh, &neigh_num)) == 0) {
    close(listener);
    FD_CLR(listener, master);
  }
  // wait neighs connect
  while (left--) {
    neigh[neigh_num++] = accNewConnection(listener, fdmax, master);
  }
  close(listener);
  FD_CLR(listener, master);
  if (userid == 1) {
    swap(&neigh[0], &neigh[1]);
  }
}

/*
  tell_master_im_potato
  this function send msg to server indicate game end
 */
void tell_master_im_potato(int sockfd) {
  char msg[10] = "";
  sprintf(msg, "end");
  send(sockfd, msg, sizeof(msg), 0);
}

/*
  tell_master_i_receive_potato
  this function send its id to server indicate i receive potato
 */
void tell_master_i_receive_potato(int sockfd, int userid) {
  char id[10] = "";
  sprintf(id, "id:%d", userid);
  send(sockfd, id, sizeof id, 0);
  char ack[10] = "";
  while (strcmp(ack, "ack") != 0) {
    recv(sockfd, ack, sizeof ack, 0);
  }
}

/*
  playWithPotato
  This function transfer potato to its neigh, until hop of potato
  equal zero
 */
void playWithPotato(int sockfd, int fdmax, fd_set master, int userid,
                    int *neigh, int num_players) {
  int end = 0;
  while (end == 0) {
    fd_set read_fds = master; // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }
    // looking for data to read
    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        char buf[MAXDATASIZE];
        int nbytes;
        if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
          disconZombie(nbytes, i, &master);
        else if (strcmp(buf, "end") == 0)
          end = 1;
        else {
          // we got some data from a client
          char potato[10] = "";
          int hop = 0;
          receive_potato(buf, &hop, potato);
          tell_master_i_receive_potato(sockfd, userid);
          if (hop >= 0)
            send_out_potato(neigh, master, potato, userid, num_players, hop);
          else {
            tell_master_im_potato(sockfd);
            printf("I'm it\n");
          }
        }
      }
    }
  }
}

/*
  readyForGame
  This function send "ready" to server to indicate all of
  its connection are set

  Input:
  sockfd: server fd
 */
void readyForGame(int sockfd) {
  if (send(sockfd, "ready", sizeof("ready"), 0) == -1) {
    fprintf(stderr, "send fail\n");
    exit(EXIT_FAILURE);
  }
}
