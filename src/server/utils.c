#include <arpa/inet.h>
#include <fcntl.h>
#include <grp.h>
#include <netinet/in.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

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

// This function joins two paths together. It allocates memory for the result
char *path_join(char *path_1, char *path_2) {
  // Adjust path_1 if it doesn't end with a slash
  size_t path_1_len = strlen(path_1);
  if (path_1[path_1_len - 1] != '/') {
    path_1_len++; // For the additional slash
  }

  // Adjust path_2 if it starts with ./
  size_t path_2_offset = 0;
  if (path_2[0] == '.' && path_2[1] == '/') {
    path_2_offset = 2; // Skip the ./
  }

  // Calculate the length of the final path
  size_t path_2_len = strlen(path_2) - path_2_offset;
  char *result =
      malloc(path_1_len + path_2_len + 1); // +1 for the null terminator
  if (result == NULL) {
    return NULL;
  }

  // Copy path_1 and append a slash if needed
  strcpy(result, path_1);
  if (path_1[path_1_len - 1] != '/') {
    strcat(result, "/");
  }

  // Concatenate the adjusted path_2
  strcat(result, path_2 + path_2_offset);
  return result;
}
