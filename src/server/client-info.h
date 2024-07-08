typedef struct {
  char *path_to_access;
  int writing;
} client_info;

struct ClientInfoList {
  client_info *client_info;
  int length;
  void (*add)(struct ClientInfoList *, client_info);
  void (*remove)(struct ClientInfoList *, int);
};