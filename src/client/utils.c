#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>

// This function creates a socket and binds it to the specified address and
// port. Returns the file descriptor of the socket. If an error occurs, it
// returns -1.
int create_socket(char *address, int port, int is_server) {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_address;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  if (inet_pton(AF_INET, address, &server_address.sin_addr) <= 0) {
    printf("The specified address is invalid.\n");
    return -1;
  }
  if (is_server) {
    if (bind(socket_fd, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0) {
      perror("bind");
      return -1;
    }
  } else {
    if (connect(socket_fd, (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0) {
      perror("connect");
      return -1;
    }
  }

  return socket_fd;
}