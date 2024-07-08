#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Check if a file exists
// Returns 1 if the file exists, 0 otherwise
int fs_exists(char *path) { return access(path, F_OK) != -1 ? 1 : 0; }

// Function that generates a directory tree
// And then creates the directories
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