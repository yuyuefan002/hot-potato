#include "client.h"
#define MAXDATASIZE 100 // max number of bytes we can get at once

void decodeID(int *userid, int *total_num, const char *buf) {
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
int main(int argc, char *argv[]) {
  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  int userid = 0;
  if (argc != 3) {
    fprintf(stderr, "usage: player <machine_name> <port_num>\n");
    exit(1);
  }
  const char *hostname = argv[1];
  const char *port = argv[2];
  sockfd = init(hostname, port);
  char *str = "ready";
  send(sockfd, str, sizeof(str), 0);
  if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
    perror("recv");
    exit(1);
  }
  buf[numbytes] = '\0';
  int total_num = 0;
  decodeID(&userid, &total_num, buf);
  printf("Connected as player %d out of %d total players", userid, total_num);

  close(sockfd);

  return 0;
}
