#define BUFFER_SIZE 1024

#define OPCODE_WRITE 1
#define OPCODE_READ 2
#define OPCODE_LIST 3

int create_socket(char *address, int port, int is_server);
int mkdir_p(const char *path);
char *path_join(char *path_1, char *path_2);
