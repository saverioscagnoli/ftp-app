#include "../headers/utils.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <libgen.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  int client_socket;
  char *root_dir;
} client_info;

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg);

int main(int argc, char *argv[]) {

  char *address = NULL;
  char *root_dir = NULL;
  int port = 0;

  // Using getopt, parse the command line arguments to get the address, port,
  // and the root directory, where the files will be stored.
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

      // Create directory if it does not exist
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

  int socket_fd = create_socket(address, port, 1);

  if (socket_fd == -1) {
    return 1;
  }

  // Listen for incoming connections.
  listen(socket_fd, 10);

  // While loop to listen for incoming connections,
  // and crete a new thread to handle each connection.
  // The handle_client function is responsible for handling the client
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
  // Parse the client_info structure from the argument

  client_info *info = (client_info *)arg;

  char buffer[BUFFER_SIZE];

  recv(info->client_socket, buffer, BUFFER_SIZE, 0);

  int opcode = 0;
  char *dest_path = NULL;

  // Split the request, the separator is '-'

  char *token = strtok(buffer, ";");
  opcode = atoi(token);

  switch (opcode) {
  case WRITE_OPCODE: {
    // Write request

    token = strtok(NULL, ";");
    dest_path = strdup(token);

    // Get the full path, with the root path of the server
    char *full_path = path_join(info->root_dir, dest_path);

    // Get the file, and send it to the client,
    // if it does not exist, create it.

    int file_fd = open(full_path, O_CREAT | O_WRONLY, 0666);

    if (file_fd == -1) {
      perror("open");
      send(info->client_socket, "ERROR CREATING FILE\0", 20, 0);
      return NULL;
    }

    // The is created. Send the OK message to the client
    // Handshake
    send(info->client_socket, "OK\0", 3, 0);

    // Receive the file

    char file_buffer[BUFFER_SIZE];
    int bytes_read = 0;

    while ((bytes_read =
                recv(info->client_socket, file_buffer, BUFFER_SIZE, 0)) > 0) {
      write(file_fd, file_buffer, bytes_read);
    }

    printf("File received: %s\n", full_path);
    break;
  }

  case READ_OPCODE: {
    token = strtok(NULL, ";");
    char *path_to_read = strdup(token);

    char *full_path = path_join(info->root_dir, path_to_read);

    // Check if the file exists
    struct stat st;
    if (stat(full_path, &st) != 0) {
      send(info->client_socket, "FILE NOT FOUND\0", 15, 0);
      return NULL;
    }

    send(info->client_socket, "OK\0", 3, 0);

    char confirm[BUFFER_SIZE];
    recv(info->client_socket, confirm, BUFFER_SIZE, 0);

    if (strcmp(confirm, "OK") != 0) {
      free(full_path);
      close(info->client_socket);
      return NULL;
    }

    pthread_mutex_lock(&file_mutex);

    // Open the file to send it to the client
    int file_fd = open(full_path, O_RDONLY);

    if (file_fd == -1) {
      perror("open");
      send(info->client_socket, "ERROR OPENING FILE\0", 20, 0);
      return NULL;
    }

    char file_buffer[BUFFER_SIZE];
    int bytes_read = 0;

    // Send the file to the client
    while ((bytes_read = read(file_fd, file_buffer, BUFFER_SIZE)) > 0) {
      if (send(info->client_socket, file_buffer, bytes_read, 0) == -1) {
        perror("send");
        return NULL;
      }
    }

    pthread_mutex_unlock(&file_mutex);

    // Close the file and the client socket
    close(file_fd);
    close(info->client_socket);
    printf("File sent: %s\n", full_path);
    break;
  }

  case LIST_OPCODE: {
    // List request
    token = strtok(NULL, ";");
    dest_path = strdup(token);

    // Get the full path
    char *full_path = path_join(info->root_dir, dest_path);

    printf("Listing directory: %s\n", full_path);

    // Check if the path is a directory
    if (!is_dir(full_path)) {
      send(info->client_socket, "NOT A DIRECTORY\0", 17, 0);
      return NULL;
    }

    // Send the OK message to the client
    send(info->client_socket, "OK\0", 3, 0);

    char confirm[BUFFER_SIZE];
    recv(info->client_socket, confirm, BUFFER_SIZE, 0);

    if (strcmp(confirm, "OK") != 0) {
      free(full_path);
      close(info->client_socket);
      return NULL;
    }

    // Open the directory
    DIR *dir = opendir(full_path);

    if (dir == NULL) {
      perror("opendir");
      send(info->client_socket, "ERROR OPENING DIRECTORY\0", 24, 0);
      return NULL;
    }

    struct dirent *entry;
    struct stat file_stat;
    char buffer[BUFFER_SIZE];

    while ((entry = readdir(dir)) != NULL) {
      // Get the full path of the file
      snprintf(buffer, sizeof(buffer), "%s/%s", full_path, entry->d_name);

      // Get file stats
      if (stat(buffer, &file_stat) < 0) {
        perror("stat");
        continue;
      }

      // Format and send file details
      char file_details[2048];
      strftime(buffer, sizeof(buffer), "%b %d %H:%M",
               localtime(&file_stat.st_mtime));

      struct passwd *pwd;
      struct group *grp;

      pwd = getpwuid(file_stat.st_uid);
      grp = getgrgid(file_stat.st_gid);


      snprintf(file_details, sizeof(file_details),
               "%c%c%c%c%c%c%c%c%c%c %ld %.900s %.256s %ld %.50s %.50s\n",
               S_ISDIR(file_stat.st_mode) ? 'd' : '-',
               file_stat.st_mode & S_IRUSR ? 'r' : '-',
               file_stat.st_mode & S_IWUSR ? 'w' : '-',
               file_stat.st_mode & S_IXUSR ? 'x' : '-',
               file_stat.st_mode & S_IRGRP ? 'r' : '-',
               file_stat.st_mode & S_IWGRP ? 'w' : '-',
               file_stat.st_mode & S_IXGRP ? 'x' : '-',
               file_stat.st_mode & S_IROTH ? 'r' : '-',
               file_stat.st_mode & S_IWOTH ? 'w' : '-',
               file_stat.st_mode & S_IXOTH ? 'x' : '-',
               (long)file_stat.st_nlink, pwd ? pwd->pw_name : "unknown",
               grp ? grp->gr_name : "unknown", (long)file_stat.st_size, buffer,
               entry->d_name);

      send(info->client_socket, file_details, strlen(file_details), 0);
    }

    // Close the directory and the client socket
    free(full_path);
    free(dest_path);
    free(info);
    closedir(dir);
    close(info->client_socket);
    break;
  }

  default: {
    printf("Unknown opcode\n");
    break;
  }
  }
}