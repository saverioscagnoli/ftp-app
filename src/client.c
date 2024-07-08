#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
  int read_flag = 0;
  int write_flag = 0;
  int list_flag = 0;

  char *address = NULL;
  int port = 0;
  char *local_path = NULL;
  char *remote_path = NULL;

  int opt;

  while ((opt = getopt(argc, argv, "a:p:f:o:rwl")) != -1) {
    switch (opt) {
    case 'a':
      address = strdup(optarg);
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'f':
      local_path = strdup(optarg);
      break;
    case 'o':
      remote_path = strdup(optarg);
      break;
    case 'w':
      write_flag = 1;
      break;
    case 'r':
      read_flag = 1;
      break;
    case 'l':
      list_flag = 1;
      break;
    default:
      printf("Unknown argument: %c\n", opt);
      return 0;
    }
  }

  if (!address || !port) {
    printf("Address and port are required.\n");
    return 0;
  }

  if (read_flag && write_flag || read_flag && list_flag ||
      write_flag && list_flag) {
    printf("Only one of read, write, or list flags can be specified.\n");
    return 0;
  }

  if (!read_flag && !write_flag && !list_flag) {
    printf("One of read, write, or list flags must be specified.\n");
    return 0;
  }

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  if (inet_pton(AF_INET, address, &server_address.sin_addr) <= 0) {
    printf("The specified address is invalid.\n");
    return -1;
  }

  if (connect(socket_fd, (struct sockaddr *)&server_address,
              sizeof(server_address)) < 0) {
    perror("connect");
    return -1;
  }

  char request_buffer[BUFFER_SIZE];

  if (write_flag) {
    if (!remote_path) {
      remote_path = strdup(local_path);
    }

    sprintf(request_buffer, "WRITE TO %s", remote_path);

    char confirmation_buffer[BUFFER_SIZE];
    send(socket_fd, request_buffer, strlen(request_buffer), 0);

    recv(socket_fd, confirmation_buffer, BUFFER_SIZE, 0);

    printf("The server responded with: %s\n", confirmation_buffer);

    if (strcmp(confirmation_buffer, "OK") != 0) {
      printf("The server did not confirm the write operation.\n");
      return -1;
    }

    FILE *file = fopen(local_path, "r");

    if (!file) {
      perror("fopen");
      return -1;
    }

    while (1) {
      int bytes_read = fread(request_buffer, 1, BUFFER_SIZE, file);

      if (bytes_read <= 0) {
        break;
      }

      send(socket_fd, request_buffer, bytes_read, 0);
    }

    fclose(file);

  } else if (read_flag) {
    sprintf(request_buffer, "READ FROM %s", local_path);

    send(socket_fd, request_buffer, strlen(request_buffer), 0);

    char confirmation_buffer[BUFFER_SIZE];
    recv(socket_fd, confirmation_buffer, BUFFER_SIZE, 0);

    if (strcmp(confirmation_buffer, "OK") != 0) {
      printf("The server did not confirm the read operation.\n");
      return -1;
    }

    FILE *file = fopen(remote_path, "w");

    if (!file) {
      perror("fopen");
      return -1;
    }

    char file_buffer[BUFFER_SIZE];

    while (1) {
      int bytes_received = recv(socket_fd, file_buffer, BUFFER_SIZE, 0);

      if (bytes_received < 0) {
        perror("recv");
        close(socket_fd);

        return 1;
      }

      if (bytes_received == 0) {
        break;
      }

      fwrite(file_buffer, 1, bytes_received, file);
    }

    fclose(file);

    printf("File received and written to %s\n", remote_path);
  } else if (list_flag) {
    sprintf(request_buffer, "LIST %s", local_path);

    send(socket_fd, request_buffer, strlen(request_buffer), 0);

    char confirmation_buffer[BUFFER_SIZE];
    recv(socket_fd, confirmation_buffer, BUFFER_SIZE, 0);

    send(socket_fd, "OK\0", 3, 0);

    char list_buffer[BUFFER_SIZE];

    if (strcmp(confirmation_buffer, "OK") == 0) {

      while (1) {
        int bytes_received = recv(socket_fd, list_buffer, BUFFER_SIZE, 0);
        list_buffer[bytes_received] = '\0';

        if (bytes_received < 0) {
          perror("recv");
          close(socket_fd);
          return 1;
        }

        if (bytes_received == 0) {
          break;
        }

        printf("%s", list_buffer);
      }

      close(socket_fd);
    } else {
      printf("The server did not confirm the list operation.\n");
    }
  }

  return 0;
}
