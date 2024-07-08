struct Client {
  int socket;
  int writing;
  int reading;
  int listing;

  char *address;
  int port;
  char *f_path;
  char *o_path;
};


int main(int argc, char *argv[])