// This is the file where the tests are defined.
// It will test single and multiple connections at once.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

// Number of threads to test concurrent write and read requests
#define NUM_THREADS 10

void write_tests(char *address, int port);
void read_tests(char *address, int port);
void write_tests_concurrent(char *address, int port);
void read_tests_concurrent(char *address, int port);
int file_exists(char *path);
void cleanup();

int main() {
  char *address = "127.0.0.0";
  int port = 8080;

  write_tests(address, port);
  read_tests(address, port);
  write_tests_concurrent(address, port);
  read_tests_concurrent(address, port);

  cleanup();

  return EXIT_SUCCESS;
}

// Function to test write requests
void write_tests(char *address, int port) {
  // Test 1: Write request on a single connection
  char *path_to_file = "testfiles/prova-w.txt";

  char command[200];
  sprintf(command,
          "./bin/client.out -w -a %s -p %d -f %s -o prova-w-output.txt",
          address, port, path_to_file);

  system(command);

  memset(command, 0, 200);

  if (file_exists("root/prova-w-output.txt")) {
    printf(GREEN
           "Test 1: Write request on a single connection - PASSED\n" RESET);
  } else {
    printf(RED "Test 1: Write request on a single connection - FAILED\n" RESET);
  }

  // Test 2: Write request with an image file
  path_to_file = "testfiles/raccoon.jpg";
  command[200];
  sprintf(command,
          "./bin/client.out -w -a %s -p %d -f %s -o raccoon-output.jpg",
          address, port, path_to_file);

  system(command);

  memset(command, 0, 200);

  if (file_exists("root/raccoon-output.jpg")) {
    printf(GREEN "Test 2: Write request with an image file - PASSED\n" RESET);
  } else {
    printf(RED "Test 2: Write request with an image file - FAILED\n" RESET);
  }

  // Test 3: Invalid write request, send a file that does not exist
  path_to_file = "testfiles/does-not-exist.txt";
  command[200];

  sprintf(command,
          "./bin/client.out -w -a %s -p %d -f %s -o prova-w-invalid.txt",
          address, port, path_to_file);

  system(command);
  memset(command, 0, 200);

  if (!file_exists("root/prova-w-invalid.txt")) {
    printf(GREEN
           "Test 3: Invalid write request, send a file that does not exist - "
           "PASSED\n" RESET);
  } else {
    printf(RED
           "Test 3: Invalid write request, send a file that does not exist - "
           "FAILED\n" RESET);
  }
}

// Function to test read requests
void read_tests(char *address, int port) {
  // Test 1: Request a text file
  char *path_to_request = "prova-r.txt";
  char command[200];

  sprintf(
      command,
      "./bin/client.out -r -a %s -p %d -f %s -o testfiles/prova-r-output.txt",
      address, port, path_to_request);

  system(command);
  memset(command, 0, 200);

  if (file_exists("testfiles/prova-r-output.txt")) {
    printf(GREEN "Test 1: Request a text file - PASSED\n" RESET);
  } else {
    printf(RED "Test 1: Request a text file - FAILED\n" RESET);
  }

  // Test 2: Request an image file
  path_to_request = "raccoon.jpg";

  sprintf(
      command,
      "./bin/client.out -r -a %s -p %d -f %s -o testfiles/raccoon-output.jpg",
      address, port, path_to_request);

  system(command);
  memset(command, 0, 200);

  if (file_exists("testfiles/raccoon-output.jpg")) {
    printf(GREEN "Test 2: Request an image file - PASSED\n" RESET);
  } else {
    printf(RED "Test 2: Request an image file - FAILED\n" RESET);
  }

  // Test 3: Invalid read request, request a file that does not exist
  path_to_request = "does-not-exist.txt";

  sprintf(
      command,
      "./bin/client.out -r -a %s -p %d -f %s -o testfiles/does-not-exist.txt",
      address, port, path_to_request);

  system(command);
  memset(command, 0, 200);

  if (!file_exists("testfiles/does-not-exist.txt")) {
    printf(GREEN
           "Test 3: Invalid read request, request a file that does not exist - "
           "PASSED\n" RESET);
  } else {
    printf(RED
           "Test 3: Invalid read request, request a file that does not exist - "
           "FAILED\n" RESET);
  }
}

void *write_concurrent_fn(void *args) {
  char *address = (char *)args;
  int port = 8080;

  // Test 1: Write text file concurrently
  char *path_to_file = "testfiles/prova-w.txt";
  char command[200];

  sprintf(
      command,
      "./bin/client.out -w -a %s -p %d -f %s -o prova-w-output-concurrent.txt",
      address, port, path_to_file);

  system(command);
  memset(command, 0, 200);

  // Text 2: Write image file concurrently
  path_to_file = "testfiles/raccoon.jpg";
  sprintf(
      command,
      "./bin/client.out -w -a %s -p %d -f %s -o raccoon-output-concurrent.jpg",
      address, port, path_to_file);

  system(command);
  memset(command, 0, 200);

  return NULL;
}

void write_tests_concurrent(char *address, int port) {
  pthread_t threads[NUM_THREADS];

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, write_concurrent_fn, address);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
}

typedef struct {
  int index;
  char *address;
  int port;
} read_thread_payload;

void *read_concurrent_fn(void *args) {
  read_thread_payload *payload = (read_thread_payload *)args;

  char *address = payload->address;
  int port = payload->port;
  int index = payload->index;

  // Test 1: Read text file concurrently
  char *path_to_request = "prova-r.txt";
  char command[200];

  sprintf(command,
          "./bin/client.out -r -a %s -p %d -f %s -o "
          "testfiles/prova-r-output-concurrent-%d.txt",
          address, port, path_to_request, index);

  system(command);
  memset(command, 0, 200);

  // Test 2: Read image file concurrently
  path_to_request = "raccoon.jpg";
  sprintf(command,
          "./bin/client.out -r -a %s -p %d -f %s -o "
          "testfiles/raccoon-output-concurrent-%d.jpg",
          address, port, path_to_request, index);

  system(command);
  memset(command, 0, 200);

  return NULL;
}

void read_tests_concurrent(char *address, int port) {
  pthread_t threads[NUM_THREADS];

  for (int i = 0; i < NUM_THREADS; i++) {

    read_thread_payload *payload = malloc(sizeof(read_thread_payload));
    payload->index = i;
    payload->address = address;
    payload->port = port;

    pthread_create(&threads[i], NULL, read_concurrent_fn, payload);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
}

// Function to test if a file exists
int file_exists(char *path) {
  FILE *file = fopen(path, "r");
  if (file) {
    fclose(file);
    return 1;
  }
  return 0;
}

// Function to delete all the output files created by the tests
void cleanup() {
  system("rm -f root/prova-w-output.txt");
  system("rm -f root/raccoon-output.jpg");
  system("rm -f testfiles/prova-r-output.txt");
  system("rm -f testfiles/raccoon-output.jpg");
}
