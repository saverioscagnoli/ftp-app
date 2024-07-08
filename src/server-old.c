#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
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

char *path_join(const char *path1, const char *path2) {
  // Calculate the length of the paths
  size_t path1_len = strlen(path1);
  size_t path2_len = strlen(path2);

  // Allocate memory for the full path
  char *full_path = malloc(path1_len + path2_len +
                           2); // +1 for the slash, +1 for the null terminator

  // Check if malloc was successful
  if (full_path == NULL) {
    perror("malloc");
    return NULL;
  }

  // Create the full file path
  snprintf(full_path, path1_len + path2_len + 2, "%s/%s", path1, path2);

  return full_path;
}

void *handle_client(void *arg);

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
      } else {
        printf("Directory exists.\n");
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

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_address;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  if (inet_pton(AF_INET, address, &server_address.sin_addr) <= 0) {
    printf("The specified address is invalid.\n");
    return -1;
  }

  if (bind(socket_fd, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) {
    perror("bind");
    return -1;
  }

  listen(socket_fd, 0);

  while (1) {
    client_info *info = malloc(sizeof(client_info));
    info->client_socket = accept(socket_fd, NULL, NULL);
    info->root_dir = root_dir;

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

void *handle_client(void *arg) {
  client_info *info = (client_info *)arg;

  int socket = info->client_socket;
  char *root_dir = info->root_dir;

  char buffer[BUFFER_SIZE];
  int bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);

  // Check if message starts with "WRITE TO "

  if (bytes_received < 0) {
    perror("recv");
    close(socket);
    free(info);
    return NULL;
  }

  buffer[bytes_received] = '\0';

  if (strncmp(buffer, "WRITE TO ", 9) == 0) {
    // Write new file
    char *file_path = path_join(root_dir, buffer + 9);

    if (file_path == NULL) {
      close(socket);
      free(info);
      return NULL;
    }

    int file_fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (file_fd < 0) {
      perror("open");
      close(socket);
      free(info);
      free(file_path);
      return NULL;
    }

    send(socket, "OK\0", 3, 0);

    char file_buffer[BUFFER_SIZE];

    while (1) {
      int bytes_received = recv(socket, file_buffer, BUFFER_SIZE, 0);

      if (bytes_received < 0) {
        perror("recv");
        close(socket);
        free(info);
        free(file_path);
        close(file_fd);
        return NULL;
      }

      if (bytes_received == 0) {
        break;
      }

      write(file_fd, file_buffer, bytes_received);
    }

    printf("Received file and written to %s\n", file_path);

  } else if (strncmp(buffer, "READ FROM ", 10) == 0) {
    char *file_path = path_join(root_dir, buffer + 10);

    printf("Reading file from %s\n", file_path);

    if (file_path == NULL) {
      close(socket);
      free(info);
      return NULL;
    }

    int file_fd = open(file_path, O_RDONLY);

    if (file_fd < 0) {
      perror("open");
      close(socket);
      free(info);
      free(file_path);
      return NULL;
    }

    send(socket, "OK\0", 3, 0);

    char file_buffer[BUFFER_SIZE];

    printf("Reading file from %s\n", file_path);

    while (1) {
      int bytes_read = read(file_fd, file_buffer, BUFFER_SIZE);

      if (bytes_read < 0) {
        perror("read");
        close(socket);
        free(info);
        free(file_path);
        close(file_fd);
        return NULL;
      }

      if (bytes_read == 0) {
        break;
      }

      send(socket, file_buffer, bytes_read, 0);
    }

    close(file_fd);
    close(socket);
    printf("Sent file %s\n", file_path);
  } else if (strncmp(buffer, "LIST ", 5) == 0) {
    char *file_path = path_join(root_dir, buffer + 5);

    printf("Listing files in %s\n", file_path);

    struct stat path_stat;
    stat(file_path, &path_stat);

    if (!S_ISDIR(path_stat.st_mode)) {
      send(socket, "NOT A DIRECTORY\0", 16, 0);
      printf("Not a directory\n");
      close(socket);
      free(info);
      free(file_path);
      return NULL;
    } else {
      send(socket, "OK\0", 3, 0);
    }

    char confirmation_buffer[BUFFER_SIZE];
    recv(socket, confirmation_buffer, BUFFER_SIZE, 0);

    if (strcmp(confirmation_buffer, "OK") != 0) {
      close(socket);
      free(info);
      free(file_path);
      return NULL;
    }

    DIR *dir = opendir(file_path);

    if (!dir) {
      perror("opendir");
      close(socket);
      free(info);
      free(file_path);
      return NULL;
    }

    struct dirent *dp;
    char buffer[1024];

    while ((dp = readdir(dir)) != NULL) {
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        continue;

      snprintf(buffer, sizeof(buffer), "%s\n", dp->d_name);
      send(socket, buffer, strlen(buffer), 0);
    }

    closedir(dir);
    close(socket);
  }

  free(info);

  return NULL;
}