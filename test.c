#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_THREADS 5

void *test_request(void *threadid) {
  long tid;
  tid = (long)threadid;
  printf("Thread #%ld!\n", tid);

  system("./bin/client.out -w -a 127.0.0.1 -p 8080 -f ./prova.txt -o "
         "./prova-write-same-file");

  char command[1024] = "./bin/client.out -w -a 127.0.0.1 -p 8080 -f "
                       "./prova.txt -o prova-write-different-file-";

  char tid_str[10];
  sprintf(tid_str, "%ld", tid);

  strcat(command, tid_str);

  system(command);

  system("./bin/client.out -a 127.0.0.1 -p 8080 -r -f ./a.txt -o "
         "./prova-read-same-file");

  char command2[1024] = "./bin/client.out -a 127.0.0.1 -p 8080 -r -f "
                        "./a.txt -o ./prova-read-different-file-";

  strcat(command2, strdup(tid_str));
  printf("Command: %s\n", command2);
  system(command2);

  printf("Thread #%ld done.\n", tid);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  pthread_t threads[NUM_THREADS];
  int rc;
  long t;
  for (t = 0; t < NUM_THREADS; t++) {
    printf("Creating thread %ld\n", t);
    rc = pthread_create(&threads[t], NULL, test_request, (void *)t);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  /* Last thing that main() should do */
  pthread_exit(NULL);
}