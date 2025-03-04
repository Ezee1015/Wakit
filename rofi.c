#include "gui_io.h"
#include "dynamic_string.h"
#include "wakit.h"
#include "cli_io.h"

cmd_node *ask_for_cmd(cmd_node *list) {
  if (!list) return NULL;

  string cmd = {0};
  str_append(&cmd, "echo \"");

  cmd_node *aux = list;
  string name = {0};
  while (aux) {
    str_replace(&name, aux->info.name.str);
    str_search_and_replace(&name, "\"", "\\\"");

    str_append(&cmd, name.str);
    if (aux->next) str_append(&cmd, "\n");

    aux = aux->next;
  }
  str_free(&name);

  str_append(&cmd, "\" | rofi -dmenu");

  string output = {0};
  int ret = console(cmd.str, &output);
  str_free(&cmd);

  cmd_node *selected = (ret)
                       ? NULL
                       : search_cmd(list, output.str);
  str_free(&output);
  return selected;
}
