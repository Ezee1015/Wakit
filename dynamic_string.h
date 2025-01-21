#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
  char *str;
  size_t str_len;
  size_t alloc_size;
} string;

#define STR_INIT (string) {   \
    .str = NULL,              \
    .alloc_size = 0,          \
    .str_len = 0              \
  }

void str_free(string *s);
bool str_append(string *s, const char *append);
bool str_append_int(string *s, const int append);
bool str_replace(string *s, char *new_str);
bool str_remove(string *s, const int from, const int to);
bool str_insert_at(string *s, const int pos, const char *insert);
bool str_search_and_replace(string *s, const char *search, const char *replace);

bool str_write_to_file(string s, FILE *f );
bool str_read_from_bfile(string *s, FILE *f);

// DEBUG
void str_inspect(string s);

#endif // STRING_H
