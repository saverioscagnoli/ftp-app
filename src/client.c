#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_address;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(8080);

  if (inet_pton(AF_INET, "127.0.0.0", &server_address.sin_addr) <= 0) {
    printf("The specified address is invalid.\n");
    return -1;
  }

  if (connect(socket_fd, (struct sockaddr *)&server_address,
              sizeof(server_address)) < 0) {
    perror("connect");
    return -1;
  }

  send(socket_fd, "1\0", 2, 0);
}