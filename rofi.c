#include "gui_io.h"
#include "dynamic_string.h"
#include "wakit.h"
#include "cli_io.h"

cmd_node *ask_for_cmd(cmd_node *list) {
  if (!list) return NULL;

  string input = {0}, name = {0};
  cmd_node *aux = list;
  while (aux) {
    str_replace(&name, aux->info.name.str);
    str_search_and_replace(&name, "\n", "\\n"); // Escape new lines in order to not break rofi's syntax

    str_append(&input, name.str);
    if (aux->next) str_append_char(&input, '\n');

    aux = aux->next;
  }
  str_free(&name);

  if (console_input("rofi -dmenu > /tmp/rofi_temp.wakit", input.str)) {
    str_free(&input);
    return NULL;
  }
  str_free(&input);

  string str_select = {0};
  char s[256] = {0};
  FILE *file_select = fopen("/tmp/rofi_temp.wakit", "r");
  if (!file_select) return NULL;
  while (fgets(s, 256, file_select)) str_append(&str_select, s);
  fclose(file_select);
  if (remove("/tmp/rofi_temp.wakit")) ERROR("Unable to remove the rofi temp file...");
  str_select.str[str_select.str_len-1] = '\0'; // remove last '\n'

  cmd_node *selected = search_cmd(list, str_select.str);
  str_free(&str_select);
  return selected;
}
