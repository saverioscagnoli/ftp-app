#include "utils.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
  char *root_dir;
  int client_socket;
} client_info;

// Function to send data
int send_data(int socket_fd, const void *data, size_t size) {
  ssize_t bytes_sent = send(socket_fd, data, size, 0);
  if (bytes_sent == -1) {
    perror("send failed");
    return -1; // Return an error indicator
  }

  return 0; // Success
}

// Optional: Function to receive data
int receive_data(int socket_fd, void *buffer, size_t size) {
  int bytes_received = recv(socket_fd, buffer, size, 0);
  if (bytes_received == -1) {
    perror("recv failed");
    return -1; // Return an error indicator
  }

  return bytes_received; // Return the number of bytes received
}

void *handle_client(void *arg);

int main(int argc, char *argv[]) {
  // Store the address, port, and root directory.
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
    printf("There was an error creating the socket server.\n");
    return 1;
  }

  // Listen for incoming connections.
  listen(socket_fd, 0);

  printf("Server is running on %s:%d\n", address, port);

  while (1) {
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    // Accept incoming connections.
    int client_socket = accept(socket_fd, (struct sockaddr *)&client_address,
                               &client_address_len);

    if (client_socket < 0) {
      perror("accept");
      return 1;
    }

    // Create a new thread to handle the client connection.
    client_info *info = (client_info *)malloc(sizeof(client_info));
    info->root_dir = root_dir;
    info->client_socket = client_socket;

    pthread_t thread;
    pthread_create(&thread, NULL, handle_client, (void *)info);
  }

  return 0;
}

void *handle_client(void *arg) {
  client_info *info = (client_info *)arg;

  // Read the client's request.
  char buffer[1];
  int bytes_read = receive_data(info->client_socket, buffer, 1);

  if (bytes_read == -1) {
    printf("Error receiving data.\n");
    close(info->client_socket);
    free(info);
    return NULL;
  }

  int opcode = atoi(buffer);

  switch (opcode) {
  case OPCODE_WRITE: {
    // Write request
    // The client wants to write a file to the server.
    // Send OK response, to indicate that the server received and understood the
    // request
    send_data(info->client_socket, "OK\0", 3);

    char file_path[BUFFER_SIZE];
    int bytes_received =
        receive_data(info->client_socket, file_path, BUFFER_SIZE);

    if (bytes_received == -1) {
      printf("Error receiving data.\n");
      break;
    }

    char *full_path = path_join(info->root_dir, file_path);

    // Create the file if it does not exist
    FILE *file = fopen(full_path, "wb");

    if (file == NULL) {
      perror("fopen");
      break;
    }

    // Send OK response
    send_data(info->client_socket, "OK\0", 3);

    char file_buffer[BUFFER_SIZE];

    while (1) {
      int bytes_received =
          receive_data(info->client_socket, file_buffer, BUFFER_SIZE);

      if (bytes_received == -1) {
        printf("Error receiving data.\n");
        break;
      }

      if (bytes_received == 0) {
        break;
      }

      fwrite(file_buffer, 1, bytes_received, file);
    }

    printf("File received successfully. Path: %s\n", full_path);

    fclose(file);
    free(full_path);
    break;
  }

  case OPCODE_READ: {
    // Read request.
    // The client wants to read a file from the server.
    // Send OK response, to indicate that the server received and understood the
    // request
    send_data(info->client_socket, "OK\0", 3);

    char file_path[BUFFER_SIZE];
    int bytes_received =
        receive_data(info->client_socket, file_path, BUFFER_SIZE);

    if (bytes_received == -1) {
      printf("Error receiving data.\n");
      break;
    }

    // Join the root directory with the file path
    char *full_path = path_join(info->root_dir, file_path);

    // Check if the entry exists and is a file
    struct stat st;
    if (stat(full_path, &st) == -1) {
      printf("The specified path does not exist: %s\n", full_path);
      send_data(info->client_socket, "ENTRY NOT FOUND\0", 15);
      break;
    }

    // Send OK response, The file exists and is ready to be sent
    send_data(info->client_socket, "OK\0", 3);

    // Wait for the client to confirm that it is ready to receive the file
    // This will be a READY signal
    char confirmation[6];
    bytes_received = receive_data(info->client_socket, confirmation, 6);

    if (bytes_received == -1) {
      printf("Error receiving data.\n");
      break;
    }

    if (strcmp(confirmation, "READY") != 0) {
      printf("The client did not confirm the request.\n");
      break;
    }

    // Open the file for reading

    FILE *file = fopen(full_path, "rb");

    if (file == NULL) {
      perror("fopen");
      break;
    }

    // Send the file data to the client
    char file_buffer[BUFFER_SIZE];
    while (1) {
      size_t bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file);

      if (bytes_read == 0) {
        break;
      }

      send_data(info->client_socket, file_buffer, bytes_read);
    }

    printf("File sent successfully. Path: %s\n", full_path);

    fclose(file);
    free(full_path);
    break;
  }

  case OPCODE_LIST: {
    // List request.
    // The client wants to list the files in the server.
    // Send OK response, to indicate that the server received and understood the
    // request
    send_data(info->client_socket, "OK\0", 3);

    // Receive the path to list
    char path[BUFFER_SIZE];
    int bytes_received = receive_data(info->client_socket, path, BUFFER_SIZE);

    if (bytes_received == -1) {
      printf("Error receiving data.\n");
      break;
    }

    // Join the root directory with the file path
    char *full_path = path_join(info->root_dir, path);

    // Check if the path exista and is a directory
    struct stat st;

    if (stat(full_path, &st) == -1) {
      printf("The path to list does not exist: %s\n", full_path);
      send_data(info->client_socket, "ENTRY NOT FOUND\0", 15);
      break;
    } else if (!S_ISDIR(st.st_mode)) {
      printf("The path is not a directory: %s\n", full_path);
      send_data(info->client_socket, "NOT A DIRECTORY\0", 16);
      break;
    }

    // Open the directory
    DIR *dir = opendir(full_path);

    if (dir == NULL) {
      perror("opendir");
      break;
    }

    // Send OK response, The path exists and is ready to be listed
    send_data(info->client_socket, "OK\0", 3);

    // Expect READY signal from the client
    char confirmation[6];
    bytes_received = receive_data(info->client_socket, confirmation, 6);

    if (bytes_received == -1) {
      printf("Error receiving data.\n");
      break;
    }

    if (strcmp(confirmation, "READY") != 0) {
      printf("The client did not confirm the request.\n");
      break;
    }

    // Send the list of files to the client
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      char *path = path_join(full_path, entry->d_name);

      struct stat file_stat;
      if (stat(path, &file_stat) == -1) {
        perror("stat");
        free(path);
        continue;
      }

      char file_details[BUFFER_SIZE * 2];
      char time_str[20];
      struct tm *tm_info = localtime(&file_stat.st_mtime);
      strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);

      char perm[11];
      snprintf(perm, sizeof(perm), "%c%c%c%c%c%c%c%c%c%c",
               (S_ISDIR(file_stat.st_mode)) ? 'd' : '-',
               (file_stat.st_mode & S_IRUSR) ? 'r' : '-',
               (file_stat.st_mode & S_IWUSR) ? 'w' : '-',
               (file_stat.st_mode & S_IXUSR) ? 'x' : '-',
               (file_stat.st_mode & S_IRGRP) ? 'r' : '-',
               (file_stat.st_mode & S_IWGRP) ? 'w' : '-',
               (file_stat.st_mode & S_IXGRP) ? 'x' : '-',
               (file_stat.st_mode & S_IROTH) ? 'r' : '-',
               (file_stat.st_mode & S_IWOTH) ? 'w' : '-',
               (file_stat.st_mode & S_IXOTH) ? 'x' : '-');

      struct passwd *pw = getpwuid(file_stat.st_uid);
      struct group *gr = getgrgid(file_stat.st_gid);

      snprintf(file_details, sizeof(file_details), "%s %ld %s %s %ld %s %s",
               perm, (long)file_stat.st_nlink, pw ? pw->pw_name : "unknown",
               gr ? gr->gr_name : "unknown", (long)file_stat.st_size, time_str,
               entry->d_name);

      int length = strlen(file_details);
      char length_str[10];
      snprintf(length_str, sizeof(length_str), "%d", length);
      send_data(info->client_socket, length_str,
                strlen(length_str) + 1); // +1 for null terminator

      char ok[3];
      int bytes_received = receive_data(info->client_socket, ok, 3);
      if (bytes_received == -1 || strcmp(ok, "OK") != 0) {
        printf("Error or client did not confirm.\n");
        free(path);
        break;
      }

      send_data(info->client_socket, file_details, length);

      free(path);
    }
    // Send an empty string to indicate the end of the list
    send_data(info->client_socket, "EOF", 1);
  }

  default:
    break;
  }

  // Close the client socket and free the memory.
  close(info->client_socket);
  free(info);
  return NULL;
}
