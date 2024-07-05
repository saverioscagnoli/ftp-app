#include <arpa/inet.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h> //structure for storing address information
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> //for socket APIs
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

typedef struct {
  int client_socket;
  char *root_dir;
} client_info;

int mkdir_p(const char *path) {
  char opath[256];
  char *p;
  size_t len;

  strncpy(opath, path, sizeof(opath));
  opath[sizeof(opath) - 1] = '\0';
  len = strlen(opath);
  if (len == 0)
    return -1;
  else if (opath[len - 1] == '/')
    opath[len - 1] = '\0';
  for (p = opath; *p; p++)
    if (*p == '/') {
      *p = '\0';
      if (access(opath, F_OK))
        if (mkdir(opath, S_IRWXU))
          return -1;
      *p = '/';
    }
  if (access(opath, F_OK)) /* if path is not terminated with / */
    if (mkdir(opath, S_IRWXU))
      return -1;
  return 0;
}

void *handle_client(void *arg) {
  client_info *info = (client_info *)arg;

  int socket = info->client_socket;
  char *root_dir = info->root_dir;
  char buffer[BUFFER_SIZE];
  ssize_t bytes_received;

  // Receive the file content
  FILE *received_file;
  char *file_path = malloc(BUFFER_SIZE);
  if (!file_path) {
    fprintf(stderr, "Failed to allocate memory.\n");
    return NULL;
  }

  // Receive the file path
  if ((bytes_received = recv(socket, file_path, BUFFER_SIZE, 0)) <= 0) {
    perror("Failed to receive file path from client");
    free(file_path);
    free(arg);
    return NULL;
  }

  // Concatenate the root directory path with the received file path
  char *full_path = malloc(strlen(root_dir) + strlen(file_path) + 2);
  sprintf(full_path, "%s/%s", root_dir, file_path);

  // Open the file for writing
  received_file = fopen(full_path, "w");
  if (received_file == NULL) {
    fprintf(stderr, "Failed to open file to write.\n");
    free(file_path);
    free(full_path);
    return NULL;
  }

  while ((bytes_received = recv(socket, buffer, BUFFER_SIZE, 0)) > 0) {
    fwrite(buffer, sizeof(char), bytes_received, received_file);
  }

  if (bytes_received < 0) {
    perror("Failed to receive file from client");
  }

  printf("File received and saved to %s\n", full_path);

  fclose(received_file);
  free(file_path);
  free(full_path);
  close(socket);

  return NULL;
}

int main(int argc, char const *argv[]) {

  char *address = NULL;
  int port = 0;
  char *root_dir = NULL;

  for (int i = 1; i < argc; i += 2) {
    if (i + 1 < argc) {
      if (strcmp(argv[i], "-a") == 0) {
        address = strdup(argv[i + 1]);
      } else if (strcmp(argv[i], "-p") == 0) {
        port = atoi(argv[i + 1]);
      } else if (strcmp(argv[i], "-d") == 0) {
        root_dir = strdup(argv[i + 1]);
      } else {
        printf("Unknown argument: %s\n", argv[i]);
        return 0;
      }
    }
  }

  if (!address || !port || !root_dir) {
    printf("Usage: ./server.out -a <address> -p <port> -d <root_dir>\n");
    return 0;
  }

  // Check if the root directory specified with -d exists, otherwise create it.
  struct stat st = {0};

  if (stat(root_dir, &st) == -1) {
    if (mkdir_p(root_dir) == -1) {
      perror("Error creating directory");
      return 1;
    }
  } else {
    printf("Directory exists.\n");
  }

  int server_socket_d = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_address;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  if (inet_pton(AF_INET, address, &(server_address.sin_addr)) <= 0) {
    printf("Invalid address / Address not supported \n");
    return -1;
  }

  bind(server_socket_d, (struct sockaddr *)&server_address,
       sizeof(server_address));

  listen(server_socket_d, 0);

  while (1) {
    client_info *info = malloc(sizeof(client_info));
    info->client_socket = accept(server_socket_d, NULL, NULL);
    info->root_dir = root_dir; // assuming root_dir is defined earlier

    if (info->client_socket < 0) {
      perror("accept");
      free(info);
      continue;
    }

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, info) != 0) {
      perror("pthread_create");
      free(info);
      continue;
    }

    pthread_detach(thread_id);
  }

  return 0;
}

// // create server socket similar to what was done in
// // client program
// int servSockD = socket(AF_INET, SOCK_STREAM, 0);

// // string store data to send to client
// char serMsg[255] = "Message from the server to the "
//                    "client \'Hello Client\' ";

// // define server address
// struct sockaddr_in servAddr;

// servAddr.sin_family = AF_INET;
// servAddr.sin_port = htons(9001);
// servAddr.sin_addr.s_addr = INADDR_ANY;

// // bind socket to the specified IP and port
// bind(servSockD, (struct sockaddr *)&servAddr, sizeof(servAddr));

// // listen for connections
// listen(servSockD, 1);

// // integer to hold client socket.
// int clientSocket = accept(servSockD, NULL, NULL);

// // send's messages to client socket
// send(clientSocket, serMsg, sizeof(serMsg), 0);

// return 0;
// }
