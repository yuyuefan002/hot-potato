#include "potato.h"
#include "server.h"
#include <signal.h>
void sigchld_handler() {
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: ringmaster <port_num> <num_players> <num_hops>");
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
  int sockfd, new_fd; // listen on sockfd, new connection on new_fd
  socklen_t sin_size;
  struct sigaction sa;
  struct sockaddr_storage client_addr; // connector's address information
  sockfd = init(hostname, port);
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    return EXIT_FAILURE;
  }
  // accept connection
  socklen_t addr_size = sizeof(client_addr);
  new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
  // build connection with each player and setting up the game
  while (1) { // main accept() loop
    sin_size = sizeof(client_addr);
    new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }
    char s[INET6_ADDRSTRLEN];
    inet_ntop(client_addr.ss_family,
              get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
    printf("server: got connection from %s\n", s);

    if (!fork()) {   // this is the child process
      close(sockfd); // child doesn't need the listener
      if (send(new_fd, "Hello, world!", 13, 0) == -1)
        perror("send");
      close(new_fd);
      exit(0);
    }
    close(new_fd); // parent doesn't need this
  }
  return EXIT_SUCCESS;
  // initialize the potato and send it out
  // receive the potato
  // print a trace of the game and end the game
  // gabbage collection
}
