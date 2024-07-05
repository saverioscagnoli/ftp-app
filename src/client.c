#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char const *argv[]) {

  int read_flag = 0;
  int write_flag = 0;

  char *address = NULL;
  int port = 0;
  char *local_path = NULL;
  char *remote_path = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-a") == 0) {
      if (i + 1 < argc) {
        address = strdup(argv[++i]);
      }
    } else if (strcmp(argv[i], "-p") == 0) {
      if (i + 1 < argc) {
        port = atoi(argv[++i]);
      }
    } else if (strcmp(argv[i], "-f") == 0) {
      if (i + 1 < argc) {
        local_path = strdup(argv[++i]);
      }
    } else if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        remote_path = strdup(argv[++i]);
      }
    } else if (strcmp(argv[i], "-r") == 0) {
      read_flag = 0;
    } else if (strcmp(argv[i], "-w") == 0) {
      write_flag = 1;
    } else {
      printf("Unknown argument: %s\n", argv[i]);
      return 0;
    }
  }

  int socket_d = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_address;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  if (inet_pton(AF_INET, address, &(server_address.sin_addr)) <= 0) {
    printf("The specified address is invalid.\n");
    return -1;
  }

  int status = connect(socket_d, (struct sockaddr *)&server_address,
                       sizeof(server_address));

  if (status == -1) {
    printf("There was an error while trying to connect.\n");
  } else {
    if (write_flag) {
      int file_d = open(local_path, O_RDONLY);

      if (file_d < 0) {
        perror("Failed to open the file");
        return -1;
      }

      send(socket_d, remote_path, strlen(remote_path) + 1, 0);

      char buffer[BUFFER_SIZE];
      ssize_t bytes_read;

      while ((bytes_read = read(file_d, buffer, BUFFER_SIZE)) > 0) {
        send(socket_d, buffer, bytes_read, 0);
      }

      close(file_d);
    } else {
      char strData[255];

      recv(socket_d, strData, sizeof(strData), 0);

      printf("Message: %s\n", strData);
    }
  }

  close(socket_d);

  return 0;
}