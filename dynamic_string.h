#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  char *str;
  size_t str_len;
  size_t alloc_size;
} String;

String str_initialize();
void str_free(String *str);
bool str_append(String *str, char *append);
bool str_replace(String *str, char *new_str);
bool str_remove(String *str, const int from, const int to);
bool str_insert_at(String *string, const int pos, const char *insert);
bool str_search_and_replace(String *str, const char *search, const char *replace);

// DEBUG
void str_inspect(String str);

#endif // STRING_H
