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
  printf("Players = <%d>\n", player_num);
  printf("Hops = <%d>\n", hop_num);
}
void print_start_info(int startplayer_num) {
  printf("Ready to start the game, sending potato to player <%d>\n",
         startplayer_num);
}
void print_trace(potato_t potato) { printf("Trace of potato:\n"); }
