#include "utils.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

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
  ssize_t bytes_received = recv(socket_fd, buffer, size, 0);
  if (bytes_received == -1) {
    perror("recv failed");
    return -1; // Return an error indicator
  }
  return bytes_received; // Return the number of bytes received
}

int main(int argc, char *argv[]) {
  // Create the necessary flags and variables to store the arguments.
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

  if (write_flag) {
    // Check if the input path exists and is a file.
    struct stat st;
    if (stat(f_path, &st) == -1) {
      printf("The specified path does not exist: %s\n", f_path);
      return 1;
    }

    if (!S_ISREG(st.st_mode)) {
      printf("The specified path is not a file.\n");
      return 1;
    }
  }

  // Create a socket and connect to the server.
  int socket_fd = create_socket(address, port, 0);

  if (write_flag) {
    // If the output path is not specified, use the same as the input file path.
    if (!o_path) {
      o_path = strdup(f_path);
    }

    // Send the operation code to the server.
    int request = send_data(socket_fd, "1", 1);

    if (request == -1) {
      return 1;
    }

    char handshake[50];
    int bytes_received = receive_data(socket_fd, handshake, 50);

    if (strcmp(handshake, "OK") != 0) {
      printf("The server responded with an error: %s\n", handshake);
      return 1;
    }

    // Send the file path to the server.
    // +1 to include the null terminator
    send_data(socket_fd, o_path, strlen(o_path) + 1);

    char creation_confirm[50];
    bytes_received = receive_data(socket_fd, creation_confirm, 50);

    if (strcmp(creation_confirm, "OK") != 0) {
      printf("The server responded with an error: %s\n", creation_confirm);
      return 1;
    }

    // Send the file contents to the server.
    FILE *file = fopen(f_path, "r");

    if (file == NULL) {
      perror("fopen");
      return 1;
    }

    char file_buffer[BUFFER_SIZE];

    while (1) {
      size_t bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file);

      if (bytes_read == 0) {
        break;
      }

      send_data(socket_fd, file_buffer, bytes_read);
    }

    printf("File sent successfully.\n");

    fclose(file);
  } else if (read_flag) {
    // If the output path is not specified, use the same as the input file path.
    if (!o_path) {
      o_path = strdup(f_path);
    }

    // Send the operation code to the server.
    int request = send_data(socket_fd, "2", 1);

    if (request == -1) {
      return 1;
    }

    char handshake[50];
    int bytes_received = receive_data(socket_fd, handshake, 50);

    if (strcmp(handshake, "OK") != 0) {
      printf("The server responded with an error: %s\n", handshake);
      return 1;
    }

    // Send the file that the client wants to read.
    send_data(socket_fd, f_path, strlen(f_path) + 1);

    // Listen for the server's response.
    // This is when the server could send an error if the file does not exist.
    char read_confirm[50];
    bytes_received = receive_data(socket_fd, read_confirm, 50);

    if (strcmp(read_confirm, "OK") != 0) {
      printf("The server responded with an error: %s\n", read_confirm);
      return 1;
    }

    FILE *file = fopen(o_path, "w");

    if (file == NULL) {
      perror("fopen");
      printf("AAAA");
      return 1;
    }

    // Send indication that the client is ready to receive the file.
    send_data(socket_fd, "READY\0", 6);

    // Receive the file data from the server.
    char file_buffer[BUFFER_SIZE];

    while (1) {
      int bytes_received = receive_data(socket_fd, file_buffer, BUFFER_SIZE);

      if (bytes_received == -1 || bytes_received == 0) {
        break;
      }

      fwrite(file_buffer, 1, bytes_received, file);
    }

    printf("File received successfully.\n");

    fclose(file);
  } else if (list_flag) {
    // Send the operation code to the server.
    int request = send_data(socket_fd, "3", 1);

    if (request == -1) {
      return 1;
    }

    // Receive the server's response.
    // Handshake
    char handshake[50];
    int bytes_received = receive_data(socket_fd, handshake, 50);

    if (strcmp(handshake, "OK") != 0) {
      printf("The server responded with an error: %s\n", handshake);
      return 1;
    }

    // Send the folder path to the server.
    send_data(socket_fd, f_path, strlen(f_path) + 1);

    // Receive the server's response.
    char list_confirm[50];
    bytes_received = receive_data(socket_fd, list_confirm, 50);

    if (strcmp(list_confirm, "OK") != 0) {
      printf("The server responded with an error: %s\n", list_confirm);
      return 1;
    }

    // Send a READY signal to the server.
    send_data(socket_fd, "READY\0", 6);

    // Receive the list of files from the server.
    char file_list[BUFFER_SIZE];

    while (1) {
      char length_str[10];

      int bytes_received = receive_data(socket_fd, length_str, 10);
      if (bytes_received <= 0 || strcmp(length_str, "EOF") == 0) {
        break; // Server closed connection or error occurred
      }

      length_str[bytes_received] = '\0'; // Null-terminate the received string

      int size = atoi(length_str);

      // Send OK signal back to the server
      send_data(socket_fd, "OK\0", 3);

      // Receive the file name
      char file_name[size + 1];
      bytes_received = receive_data(socket_fd, file_name, size);

      file_name[bytes_received] = '\0'; // Null-terminate the received string

      if (bytes_received <= 0) {
        break; // Server closed connection or error occurred
      }

      printf("%s\n", file_name);
    }
  }

  // Close the socket.
  close(socket_fd);

  return 0;
}
