#include <string.h>
#include <stdio.h>

#include "dynamic_string.h"

#define BLOCK_SIZE 64

bool str_resize(string *s, size_t str_len) {
  const size_t alloc_size = BLOCK_SIZE * (str_len / BLOCK_SIZE + 1);
  s->str = realloc(s->str, alloc_size);
  s->alloc_size = alloc_size;
  return (s->str);
}

bool str_append(string *s, const char *append) {
  if (!s) return false;
  if (!append) return true;

  const size_t new_len = s->str_len + strlen(append);

  // Reallocate new space if necessary
  if (!s->alloc_size || new_len > s->alloc_size-1) {
    if (!str_resize(s, new_len)) return false;
  }

  strcpy(s->str+s->str_len, append);
  s->str_len = new_len;
  return true;
}

bool str_append_char(string *s, const char c) {
  if (!s) return false;

  const size_t new_len = s->str_len + 1;

  // Reallocate new space if necessary
  if (!s->alloc_size || new_len > s->alloc_size-1) {
    if (!str_resize(s, new_len)) return false;
  }

  s->str[s->str_len] = c;
  s->str[s->str_len+1] = '\0';
  s->str_len = new_len;
  return true;
}

bool str_append_int(string *s, int append) {
  if (!s) return false;

  int aux = append;
  int digits = (append == 0) ? 1 : 0;
  while (aux != 0) {
    aux /= 10;
    digits++;
  }

  const size_t new_len = s->str_len + digits;

  // Reallocate new space if necessary
  if (!s->alloc_size || new_len > s->alloc_size-1) {
    if (!str_resize(s, new_len)) return false;
  }

  for (int i=1; i<=digits; i++) {
    s->str[s->str_len+digits-i] = append % 10 + '0';
    append /= 10;
  }

  s->str[new_len] = '\0';
  s->str_len = new_len;
  return true;
}

void str_inspect(string s) {
  printf("[String inspect]: alloc_size: %ld ; str_len: %ld ; string: %s\n", s.alloc_size, s.str_len, s.str);

  for (int i=0; i<s.alloc_size; i++) {
    printf("\t[%d]: %c\tCodigo: %d\n", i, s.str[i], s.str[i]);
  }
}

void str_free(string *s) {
  if (!s) return;

  s->alloc_size = 0;
  s->str_len = 0;
  free(s->str);
  s->str = NULL;
}

// Check if there's too much free space reserved.
void str_trim_excess(string *s) {
  if (s->alloc_size - s->str_len > BLOCK_SIZE)
    str_resize(s, s->str_len); // Reallocation shouldn't fail since we're only reducing the size...
}

bool str_replace(string *s, char *new_str) {
  if (!s) return false;
  if (!new_str) {
    str_free(s);
    return true;
  }

  const size_t new_len = strlen(new_str);

  // Reallocate new space if necessary
  if (s->alloc_size == 0 || (new_len > s->alloc_size-1)) {
    if (!str_resize(s, new_len)) return false;
  }

  strcpy(s->str, new_str);
  s->str_len = new_len;

  str_trim_excess(s);
  return true;
}

bool str_remove(string *s, const int from, const int to) {
  if (!s) return false;

  if (from >= s->str_len) return false;
  if (to >= s->str_len) return false;
  // to >= from >= 0
  if (from < 0) return false;
  if (to < from) return false;

  int from_cursor = from;
  int to_cursor = to + 1;
  while (to_cursor < s->str_len) {
    s->str[from_cursor] = s->str[to_cursor];
    from_cursor++;
    to_cursor++;
  }
  s->str[from_cursor] = '\0';

  const size_t new_str_len = s->str_len-(to-from+1);
  s->str_len = new_str_len;
  if (!str_resize(s, new_str_len)) return false;

  return true;
}

bool str_insert_at(string *s, const int pos, const char *insert) {
  if (pos < 0 || pos > s->str_len) return false;
  if (insert == NULL) return false;

  const char insert_len = strlen(insert);
  if (!insert_len) return false;

  const size_t new_str_len = s->str_len + insert_len;
  if (!str_resize(s, new_str_len)) return false;

  // shift array
  for (int i=new_str_len; i>=pos+insert_len; i--)
    s->str[i] = s->str[i-insert_len];

  // Insert string
  for (int i=0; i<insert_len; i++)
    s->str[pos+i] = insert[i];

  s->str_len = new_str_len;

  return true;
}

bool str_search_and_replace(string *s, const char *search, const char *replace) {
  if (!s || !search || !replace) return false;

  const size_t search_len = strlen(search);
  const size_t replace_len = strlen(replace);

  for (int i=0; i<s->str_len; i++) {
    int match = 0;
    while (match < search_len
           && i+match < s->str_len
           && s->str[i+match] == search[match])
    {
      match++;
    }

    if (match == search_len) {
      if (!str_remove(s, i, i+search_len-1)) return false;
      if (!str_insert_at(s, i, replace)) return false;
      i += replace_len;
    }
  }

  return true;
}

bool str_write_to_file(const string s, FILE *f) {
  if (!f) return false;

  if (!fwrite(&s.str_len, sizeof(size_t), 1, f)) return false;
  if (s.str_len>0 &&!fwrite(s.str, sizeof(char), s.str_len+1, f)) return false;

  return true;
}

bool str_read_from_bfile(string *s, FILE *f) {
  if (!f) return false;

  size_t str_len;
  if (!fread(&str_len, sizeof(size_t), 1, f)) return false;

  // If it's empty
  if (str_len == 0){
    str_free(s);
    return true;
  }

  if (!str_resize(s, str_len)
      || !fread(s->str, sizeof(char), str_len+1, f)
  ) {
    return false;
  }

  s->str_len = str_len;
  return true;
}
