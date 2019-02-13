#include "base.h"

/*
  sendall
  This function can send() whatever length msg

  Input:
  s:socket fd
  buf:msg
  len: sizeof(buf)
 */
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

/*
  append
  This function append str2 to str1.Warning: you have to free the new str

  Input:
  str1,str2
  Output:
  new str:str1+str2
 */
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

/*
  substr
  This function generate substing warning, you have to free it

  Input:
  buf: whole string
  start, end

  Output:
  substring
 */
void substr(char *new, const char *buf, int start, int end) {
  int len = end - start;
  strncpy(new, buf + start, len);
  new[len] = '\0';
}

/*
  swap
  Only work for int.
 */
void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

/*
  closeall
  close all ongoing connection
 */
void closeall(int fdmax, fd_set *master) {
  for (int i = 0; i <= fdmax; i++) {
    // send to everyone!
    if (FD_ISSET(i, master)) {
      close(i);
      FD_CLR(i, master);
    }
  }
}

/*
  disconZombie
  Disconnect Zombie socket
 */
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

/*
  accNewconnection
  This function block until new connection come.
  accept new connection then update master
 */
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

/*
  interpIpPort
  This function interperate string into ip and port

  Input:
  buf: whole string
  start: start point
  end:
 */
void interpIpPort(const char *buf, int start, int end, char *ip, char *port) {
  int i = start;
  while (buf[i] != ',') {
    i++;
  }
  substr(ip, buf, start, i);
  substr(port, buf, i + 1, end);
}
