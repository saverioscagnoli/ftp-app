#define BUFFER_SIZE 1024

// Different opcodes for each different type of request, used to identify them
#define WRITE_OPCODE 1
#define READ_OPCODE 2
#define LIST_OPCODE 3

int create_socket(char *address, int port, int is_server);

int mkdir_p(const char *path);

char *path_join(char *path_1, char *path_2);

int file_exists(const char *path);

int is_dir(const char *path);
