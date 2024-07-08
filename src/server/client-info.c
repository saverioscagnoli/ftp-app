
#include "client-info.h"

void add(struct ClientInfoList *list, client_info info) {
  list->length++;
  list->client_info =
      realloc(list->client_info, list->length * sizeof(client_info));
  list->client_info[list->length - 1] = info;
}

void remove(struct ClientInfoList *list, int index) {
  for (int i = index; i < list->length - 1; i++) {
    list->client_info[i] = list->client_info[i + 1];
  }

  list->length--;
  list->client_info =
      realloc(list->client_info, list->length * sizeof(client_info));
}

struct ClientInfoList new_list() {
  struct ClientInfoList list;

  list.client_info = malloc(sizeof(client_info));
  list.length = 0;

  list.add = add;
  list.remove = remove;

  return list;
}
