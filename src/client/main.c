#include "../headers/utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char *address = NULL;
  char *f_path = NULL;
  char *o_path = NULL;

  int port = 0;
  int write_flag = 0;
  int read_flag = 0;
  int list_flag = 0;

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
      f_path = strdup(optarg);
      break;
    case 'o':
      o_path = strdup(optarg);
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
    printf(
        "Usage: %s -a <address> -p <port> [-f <file_path>] [-o <output_path>] "
        "[-w|-r|-l]\n",
        argv[0]);
    return 1;
  }

  if (!write_flag && !read_flag && !list_flag) {
    printf("You must specify an operation: -w, -r, or -l\n");
    return 1;
  }

  if (write_flag && read_flag || write_flag && list_flag ||
      read_flag && list_flag) {
    printf("You can only specify one operation: -w, -r, or -l\n");
    return 1;
  }

  // Miscellaneous checks
  if (write_flag) {
    // Check if -f path exists
    if (!file_exists(f_path)) {
      printf("The specified file does not exist.\n");
      return 1;
    }
  }

  int socket_fd = create_socket(address, port, 0);

  if (socket_fd == -1) {
    return 1;
  }

  if (write_flag) {
    // Use the same path for the output file if it was not specified
    if (!o_path) {
      o_path = strdup(f_path);
    }

    // Send the opcode of the operation, along with the file path
    char *request = malloc(strlen(o_path) + 3);
    sprintf(request, "%d;%s", WRITE_OPCODE, o_path);

    if (send(socket_fd, request, strlen(request), 0) == -1) {
      perror("send");
      return 1;
    }

    char response[BUFFER_SIZE];
    if (recv(socket_fd, response, BUFFER_SIZE, 0) == -1) {
      perror("recv");
      return 1;
    }

    // Check if the server responded with an error
    if (strcmp(response, "OK") != 0) {
      printf("The server responded with an error: %s\n", response);
      return 1;
    }

    // Open the file to send it to the server
    int file_fd = open(f_path, O_RDONLY);

    if (file_fd == -1) {
      perror("open");
      return 1;
    }

    char file_buffer[BUFFER_SIZE];

    int bytes_read = 0;

    while ((bytes_read = read(file_fd, file_buffer, BUFFER_SIZE)) > 0) {
      if (send(socket_fd, file_buffer, bytes_read, 0) == -1) {
        perror("send");
        return 1;
      }
    }

    close(file_fd);
    printf("File sent successfully.\n");
  } else if (read_flag) {
    // Use the same path for the output file if it was not specified
    if (!o_path) {
      o_path = strdup(f_path);
    }

    // Send the opcode of the operation, along with the file path
    char *request = malloc(strlen(f_path) + 3);
    sprintf(request, "%d;%s", READ_OPCODE, f_path);

    if (send(socket_fd, request, strlen(request), 0) == -1) {
      perror("send");
      return 1;
    }

    char response[BUFFER_SIZE];

    if (recv(socket_fd, response, BUFFER_SIZE, 0) == -1) {
      perror("recv");
      return 1;
    }

    // Check if the server responded with an error

    if (strcmp(response, "OK") != 0) {
      printf("The server responded with an error: %s\n", response);
      return 1;
    }

    // Open the file to write the received data
    int file_fd = open(o_path, O_CREAT | O_WRONLY, 0666);

    if (file_fd == -1) {
      perror("open");
      return 1;
    }

    // Send handshake indicating that the client is ready to receive the file
    if (send(socket_fd, "OK\0", 3, 0) == -1) {
      perror("send");
      return 1;
    }

    char file_buffer[BUFFER_SIZE];
    int bytes_read = 0;

    while ((bytes_read = recv(socket_fd, file_buffer, BUFFER_SIZE, 0)) > 0) {
      write(file_fd, file_buffer, bytes_read);
    }

    close(file_fd);
    close(socket_fd);
  } else if (list_flag) {
    // Send the opcode of the operation
    // And the directory to list
    char *request = malloc(strlen(f_path) + 3);
    sprintf(request, "%d;%s", LIST_OPCODE, f_path);

    if (send(socket_fd, request, strlen(request), 0) == -1) {
      perror("send");
      return 1;
    }

    char response[BUFFER_SIZE];
    recv(socket_fd, response, BUFFER_SIZE, 0);

    // Check if the server responded with an error
    if (strcmp(response, "OK") != 0) {
      printf("The server responded with an error: %s\n", response);
      return 1;
    }

    // Send a confirmation message to the server
    // to signal that the client is ready to receive the list
    send(socket_fd, "OK\0", 3, 0);

    char list_buffer[BUFFER_SIZE * 2];

    // Receive the list of files and directories
    while (recv(socket_fd, list_buffer, BUFFER_SIZE * 2, 0) > 0) {
      printf("%s", list_buffer);

      // Empty the buffer
      memset(list_buffer, 0, BUFFER_SIZE * 2);
    }

    close(socket_fd);
  }
}