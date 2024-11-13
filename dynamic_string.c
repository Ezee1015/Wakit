#include <string.h>
#include <stdio.h>

#include "dynamic_string.h"

#define BLOCK_SIZE 50

String str_initialize() {
  return (String) {
    .str = NULL,
    .alloc_size = 0,
    .str_len = 0
  };
}

bool str_resize(String *string, size_t str_len) {
  const size_t alloc_size = BLOCK_SIZE * (str_len / BLOCK_SIZE + 1);
  string->str = realloc(string->str, alloc_size);
  string->alloc_size = alloc_size;
  return (string->str);
}

bool str_append(String *string, char *append) {
  if (!string) return false;
  if (!append) return true;

  const size_t new_size = string->str_len + strlen(append);

  // Reallocate new space if necessary
  if (!string->alloc_size || new_size > string->alloc_size-1) {
    if (!str_resize(string, new_size)) return false;
  }

  string->str_len = new_size;
  strcat(string->str, append);
  return true;
}

void str_inspect(String string) {
  printf("[String inspect]: alloc_size: %ld ; str_len: %ld ; string: %s\n", string.alloc_size, string.str_len, string.str);

  for (int i=0; i<string.alloc_size; i++) {
    printf("\t[%d]: %c\tCodigo: %d\n", i, string.str[i], string.str[i]);
  }
}

void str_free(String *string) {
  if (!string) return;

  string->alloc_size = 0;
  string->str_len = 0;
  free(string->str);
  string->str = NULL;
}

// Check if there's too much free space reserved.
void str_trim_excess(String *string) {
  if (string->alloc_size - string->str_len > BLOCK_SIZE)
    str_resize(string, string->str_len); // Reallocation shouldn't fail since we're only reducing the size...
}

bool str_replace(String *string, char *new_str) {
  if (!string) return false;
  if (!new_str) {
    str_free(string);
    return true;
  }

  const size_t new_len = strlen(new_str);

  // Reallocate new space if necessary
  if (new_len > string->alloc_size-1) {
    if (!str_resize(string, new_len)) return false;
  }

  strcpy(string->str, new_str);

  str_trim_excess(string);
  return true;
}

bool str_remove(String *string, const int from, const int to) {
  if (!string) return false;

  if (from >= string->str_len) return false;
  if (to >= string->str_len) return false;
  // to >= from >= 0
  if (from < 0) return false;
  if (to < from) return false;

  int from_cursor = from;
  int to_cursor = to + 1;
  while (to_cursor < string->str_len) {
    string->str[from_cursor] = string->str[to_cursor];
    from_cursor++;
    to_cursor++;
  }
  string->str[from_cursor] = '\0';

  const size_t new_str_len = string->str_len-(to-from+1);
  string->str_len = new_str_len;
  if (!str_resize(string, new_str_len)) return false;

  return true;
}

bool str_insert_at(String *string, const int pos, const char *insert) {
  if (pos < 0 || pos > string->str_len) return false;
  if (insert == NULL) return false;

  const char insert_len = strlen(insert);
  if (!insert_len) return false;

  const size_t new_str_len = string->str_len + insert_len;
  if (!str_resize(string, new_str_len)) return false;

  // shift array
  for (int i=new_str_len; i>=pos+insert_len; i--)
    string->str[i] = string->str[i-insert_len];

  // Insert string
  for (int i=0; i<insert_len; i++)
    string->str[pos+i] = insert[i];

  string->str_len = new_str_len;

  return true;
}

bool str_search_and_replace(String *string, const char *search, const char *replace) {
  if (!string || !search || !replace) return false;

  const size_t search_len = strlen(search);

  for (int i=0; i<string->str_len; i++) {
    int match = 0;
    while (match < search_len
           && match+i < string->str_len
           && string->str[i+match] == search[match])
    {
      match++;
    }

    if (match == search_len) {
      if (!str_remove(string, i, i+search_len-1)) return false;
      if (!str_insert_at(string, i, replace)) return false;
    }
  }

  return true;
}
