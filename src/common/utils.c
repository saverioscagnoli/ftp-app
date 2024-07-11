#include "../headers/utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// This function creates a directory even if the parent directories do not
// exist.
int mkdir_p(const char *path) {
  char temp[256];
  char *p = NULL;
  size_t len;

  snprintf(temp, sizeof(temp), "%s", path);
  len = strlen(temp);
  if (temp[len - 1] == '/')
    temp[len - 1] = 0;
  for (p = temp + 1; *p; p++)
    if (*p == '/') {
      *p = 0;
      mkdir(temp, S_IRWXU);
      *p = '/';
    }
  mkdir(temp, S_IRWXU);

  return 0;
}

// This function joins two paths together.
// Example: path_join("/tmp", "file.txt") -> "/tmp/file.txt"
char *path_join(char *path_1, char *path_2) {
  char *result = malloc(strlen(path_1) + strlen(path_2) + 2);
  if (result == NULL) {
    return NULL;
  }
  strcpy(result, path_1);

  // Check if the path_1 ends with a slash
  if (result[strlen(result) - 1] != '/') {
    strcat(result, "/");
  }

  strcat(result, path_2);
  return result;
}

// Utility function to check if a file exists
// Returns 1 if the file exists, 0 otherwise
int file_exists(const char *path) {
  struct stat buffer;
  return stat(path, &buffer) == 0;
}

// Utility function to check if a path is a folder
int is_dir(const char *path) {
  struct stat s;
  if (stat(path, &s) == 0) {
    if (S_ISDIR(s.st_mode)) {
      return 1;
    } else {
      return 0;
    }
  } else {
    // error, unable to get file status
    return -1;
  }
}