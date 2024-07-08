#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

#define WRITE_OPCODE = 0
#define READ_OPCODE = 1
#define LIST_OPCODE = 2

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define COLOR_RESET "\x1b[0m"

void *handle_client(void *arg);

struct Server {
  int port;
  char *address;
  char *root_dir;
  int socket;

  int (*create_socket)(struct Server *);
  void (*start_listening)(struct Server *);
};

struct Server new_server(int port, char *address, char *root_dir) {
  struct Server server;

  server.port = port;
  server.address = address;
  server.root_dir = root_dir;

  return server;
}

// Creates a socket connection
// Returns 0 on success, -1 on failure
int create_socket(struct Server *server) {
  printf(YELLOW "Creating socket on port %d..." COLOR_RESET "\n", server->port);

  server->socket = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_address;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(server->port);

  if (inet_pton(AF_INET, server->address, &server_address.sin_addr) <= 0) {
    printf(RED "The specified address is invalid." COLOR_RESET "\n");
    return -1;
  }

  if (bind(server->socket, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) {
    perror("bind");
    return -1;
  }

  printf(GREEN "Socket created successfully." COLOR_RESET "\n");

  return 0;
}

void start_listening(struct Server *server) {
  printf(YELLOW "Listening on port %d..." COLOR_RESET "\n", server->port);

  listen(server->socket, 0);

  while (1) {
    int client_socket = accept(server->socket, NULL, NULL);

    pthread_t thread;

    if (pthread_create(&thread, NULL, handle_client, &client_socket) != 0) {
      perror("pthread_create");
      continue;
    }

    pthread_detach(thread);
  }
}

// Function that handles a write request from the client.
// It receives the where the file should be created.
// Should be able to listen to the file being sent by the client
// Returns 0 on success, -1 on failure
int handle_write_request(int socket, char *path) {
  // Check if the file already exists
  // If it does, return an error
  if (fs_exists(path)) {
    send(socket, "FILE EXISTS", 11, 0);
    return -1;
  }

  // Create the file

  int file = open(path, O_CREAT | O_WRONLY, 0644);

  if (file == -1) {
    perror("open");
    send(socket, "ERROR", 5, 0);
    return -1;
  }

  send(socket, "OK", 2, 0);

  // Receive the file from the client
}

void *handle_client(void *arg) {
  int socket = *(int *)arg;

  char buffer[1024];

  read(socket, buffer, 1024);

  printf("Received: %s\n", buffer);

  close(socket);
}

int main(int argc, char *argv[]) {

  char *address = NULL;
  int port;
  char *root_dir = NULL;

  int opt;

  while ((opt = getopt(argc, argv, "a:p:d:")) != -1) {
    switch (opt) {
    case 'a':
      address = strdup(optarg);
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'd':
      struct stat st = {0};
      if (stat(optarg, &st) == -1) {
        if (mkdir_p(optarg) == -1) {
          perror("Error creating directory");
          return 1;
        }
      }

      root_dir = realpath(optarg, NULL);

      if (root_dir == NULL) {
        perror("realpath");
        return -1;
      }

      break;

    default:
      printf("Unknown argument: %c\n", opt);
      return 0;
    }
  }

  if (!address || !port || !root_dir) {
    printf("Usage: ./server.out -a <address> -p <port> -d <root_dir>\n");
    return 0;
  }

  struct Server server = new_server(port, address, root_dir);
  server.create_socket = create_socket;
  server.start_listening = start_listening;

  int error = server.create_socket(&server);

  if (error == -1) {
    printf("Error creating socket.\n");
    return error;
  }

  server.start_listening(&server);
}