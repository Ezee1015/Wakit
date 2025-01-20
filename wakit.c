#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wakit.h"
#include "cli_io.h"
#include "dynamic_string.h"
#include "window_manager.h"

#define GTABLET "Wacom One by Wacom S Pen stylus"

void print_help(const char *app_path) {
  printf("Wakit is a command manager for xsetwacom that allows per-application configuration.\n");
  printf("Usage: %s [mode] [arguments]\n", app_path);
  printf("\nModes:\n");
  printf("\t-h, --help ........................ Show this message\n");
  printf("\t-a [name] [command] [type] ........ Add a new command. Arguments:\n");
  printf("\t                                    - name: Name of the command\n");
  printf("\t                                    - command: Command to execute\n");
  printf("\t                                    - type: 'action' or 'profile'\n");
  printf("\t-l ................................ List all commands\n");
  printf("\t     --filter-by-app .............. Show the commands that are related to an app\n");
  printf("\t     --profiles ................... Show only profiles\n");
  printf("\t     --actions .................... Show only actions\n");
  printf("\t     --show-cmd ................... Show the console command to be executed for each command\n");
  printf("\t-r [name] ......................... Remove a command\n");
  printf("\t-e ................................ Edit a command\n");
  printf("\t-m ................................ Run menu\n");
  printf("\t-s ................................ Run service\n");
}

bool add_command(cmd_node **list, cmd c) {
  if (!list) {
    ERROR("No list was given");
    return false;
  }

  cmd_node *new_node = malloc(sizeof(cmd_node));
  if (!new_node) {
    ERROR("No free space");
    return false;
  }
  new_node->info = c;
  new_node->next = NULL;

  // Empty list
  if ( !(*list) ) {
    *list = new_node;
    return true;
  }

  cmd_node *aux = *list;
  while (aux->next) aux = aux->next;
  aux->next = new_node;

  return true;
}

// Return values:
//   1  --> EOF
//   -1 --> Error
//   0  --> OK
int read_cmd_from_file(FILE *f, cmd *c) {
  // EOF
  if (!str_read_from_bfile(&c->name, f)) return 1;

  // Read
  if ( !str_read_from_bfile(&c->cmd, f)
       || !fread(&c->type, sizeof(cmd_type), 1, f)
       || !str_read_from_bfile(&c->app, f)
       || !fread(&c->default_for_app, sizeof(bool), 1, f)
  ) {
    ERROR("Save file is corrupted :´(");
    return -1;
  }

  return 0;
}

bool get_config_path(string *path) {
  const char *home = getenv("HOME");
  if (!home) return false;

  str_append(path, home);
  str_append(path, "/.config/wakit");
  return true;
}

// Return values:
//   1  --> Error while opening
//   -1 --> Format error
//   0  --> OK
int load_cmd_list(cmd_node **list) {
  string path = STR_INIT;
  if (!get_config_path(&path)) {
    ERROR("Get yourself a home");
    return 1;
  }

  FILE *f = fopen(path.str, "rb");
  if (!f) {
    str_insert_at(&path, 0, "Can't open file ");
    str_append(&path, ", continuing without loading it...");
    DEBUG(path.str);
    str_free(&path);
    return 0;
  }
  str_free(&path);

  cmd c;
  INIT_CMD(c);
  int ret;
  while ((ret = read_cmd_from_file(f, &c)) == 0) {
    add_command(list, c);
    INIT_CMD(c);
  }

  return (ret == 1) ? 0 : -1;
}

bool write_cmd_to_file(FILE *f, cmd c) {
  return str_write_to_file(c.name, f)
         && str_write_to_file(c.cmd, f)
         && fwrite(&c.type, sizeof(cmd_type), 1, f)
         && str_write_to_file(c.app, f)
         && fwrite(&c.default_for_app, sizeof(bool), 1, f);
}

bool save_cmd_list(cmd_node *list) {
  if (!list) {
    ERROR("No list was given");
    return false;
  }

  string path = STR_INIT;
  if (!get_config_path(&path)) {
    ERROR("Get yourself a home");
    return 1;
  }

  FILE *f = fopen(path.str, "wb");
  if (!f) {
    str_insert_at(&path, 0, "Can't open file ");
    ERROR(path.str);
    str_free(&path);
    return false;
  }

  while (list) {
    if (!write_cmd_to_file(f, list->info)) {
      str_insert_at(&path, 0, "Couldn't write file ");
      ERROR(path.str);
      str_free(&path);
      return false;
    }

    list = list->next;
  }

  fclose(f);
  str_free(&path);
  return true;
}

void free_cmd(cmd_node *cmd) {
  str_free( &(cmd->info.app) );
  str_free( &(cmd->info.name) );
  str_free( &(cmd->info.cmd) );
  free(cmd);
}

void free_cmd_list(cmd_node **list) {
  while (*list) {
    cmd_node *aux = *list;
    (*list) = (*list)->next;
    free_cmd(aux);
  }
}

cmd_node *default_app_profile(cmd_node *list, string app_name) {
  if (!list) return NULL;

  // TODO
  return NULL;
}

int create_command(char *name, char *command, char *type) {
  cmd new_cmd;
  INIT_CMD(new_cmd);

  if (!strcmp(type, "action")) {
    new_cmd.type = Action;
  } else if (!strcmp(type, "profile")) {
    new_cmd.type = Profile;
  } else {
    string err = STR_INIT;
    str_append(&err, "Command type not recognized: ");
    str_append(&err, type);
    ERROR(err.str);
    str_free(&err);
    return 1;
  }
  str_append(&new_cmd.name, name);
  str_append(&new_cmd.cmd, command);

  if (new_cmd.type == Profile) {
    if (question_yn("Is custom to an app?")) {
      if (!select_window(&new_cmd.app)) {
        ERROR("Sorry. App not compatible...");
        str_free(&new_cmd.name);
        str_free(&new_cmd.cmd);
        str_free(&new_cmd.app);
        return 1;
      }

      new_cmd.default_for_app = question_yn("Do you want to make it default for the app?");
    } else {
      str_append(&new_cmd.app, "generic");
    }
  }

  cmd_node *list = NULL;
  if (load_cmd_list(&list) != 0) return 1;

  // Search if the name is unique
  cmd_node *aux = list;
  while (aux && strcmp(aux->info.name.str, name)) aux = aux->next;
  if (aux) {
    ERROR("Command name is already registered");
    free_cmd_list(&list);
    return 1;
  }

  // Make it the default profile if there's already one
  if (new_cmd.default_for_app && default_app_profile(list, new_cmd.app)) {
    string msg = STR_INIT;
    str_append(&msg, "There's already a default app for ");
    str_append(&msg, new_cmd.app.str);
    DEBUG(msg.str);
    str_free(&msg);

    ERROR("NOT IMPLEMENTED");
    // TODO Search the default profile for the app and make it not the default
  }

  if (!add_command(&list, new_cmd)) {
    free_cmd_list(&list);
    return 1;
  }
  if (!save_cmd_list(list)) {
    free_cmd_list(&list);
    return 1;
  }
  free_cmd_list(&list);
  return 0;

  free_cmd_list(&list);
}

int list_commands(int argc, char *argv[]) {
  cmd_node *list = NULL;
  if (load_cmd_list(&list) != 0) return 1;

  bool show_cmd = false;
  enum Mode {
    Default,
    Filter,
    Profiles,
    Actions
  } mode = Default;

  for (int i=2; i<argc; i++) {
    if (!strcmp(argv[i], "--show-cmd")) {
      show_cmd = true;

    } else if (!strcmp(argv[i], "--filter-by-app")) {
      if (mode != Default) {
        ERROR("List mode already selected...");
        free_cmd_list(&list);
        return 1;
      }
      mode = Filter;
      ERROR("Not implemented"); // TODO
      free_cmd_list(&list);
      return 1;

    } else if (!strcmp(argv[i], "--profiles")) {
      if (mode != Default) {
        ERROR("List mode already selected...");
        free_cmd_list(&list);
        return 1;
      }
      mode = Profiles;

    } else if (!strcmp(argv[i], "--actions")) {
      if (mode != Default) {
        ERROR("List mode already selected...");
        free_cmd_list(&list);
        return 1;
      }
      mode = Actions;

    } else {
      string err = STR_INIT;
      str_append(&err, "Unrecognized option for 'list' mode: ");
      str_append(&err, argv[i]);
      ERROR(err.str);
      str_free(&err);
      free_cmd_list(&list);
      return 1;
    }
  }

  while (list) {
    if ( (mode == Default)
         ||(mode == Profiles && list->info.type == Profile)
         || (mode == Actions && list->info.type == Action)
    ) {
      if (list->info.type == Action)
        printf("--> %s (Action)", list->info.name.str);
      else {
        printf("--> %s ", list->info.name.str);
        if (!strcmp(list->info.app.str, "generic")) {
          printf("(Generic Profile)");
        } else {
          printf("(");
          if (list->info.default_for_app) printf("Default ");
          printf("Profile for %s)", list->info.app.str);
        }
      }
      if (show_cmd) printf("\n\tCommand: %s\n\n", list->info.cmd.str);
      else printf("\n");
    }

    list = list->next;
  }

  free_cmd_list(&list);
  return 0;
}

bool remove_command(cmd_node **list, char *name) {
  if (!list || !(*list)) return false;

  // Check the first element
  if ( !strcmp((*list)->info.name.str, name) ) {
    cmd_node *aux = *list;
    *list = (*list)->next;
    free_cmd(aux);
    return true;
  }

  // Another element
  cmd_node *prev = *list, *current = (*list)->next;
  while ( current && strcmp(current->info.name.str, name) ) {
    prev = current;
    current = current->next;
  }

  if (!current) return false;
  prev->next = current->next;
  free_cmd(current);
  return true;
}

int main(int argc, char *argv[]) {
  int ret = 0;

  if (argc < 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
    print_help(argv[0]);

  } else if (!strcmp(argv[1], "-a")) {
    if (argc != 5) {
      ERROR("Expected 3 arguments for 'add' mode");
      return 1;
    }
    ret = create_command(argv[2], argv[3], argv[4]);

  } else if (!strcmp(argv[1], "-l")) {
    ret = list_commands(argc, argv);

  } else if (!strcmp(argv[1], "-r")) {
    if (argc != 3) {
      ERROR("Expected the command id.");
      return 1;
    }

    cmd_node *list = NULL;
    if (load_cmd_list(&list) == -1) {
      ERROR("Can't load the save file");
      return 1;
    }
    if (!remove_command(&list, argv[2])) {
      ERROR("Couldn't delete the command...");
      free_cmd_list(&list);
      return 1;
    }
    if (!save_cmd_list(list)) {
      ERROR("Can't save the file");
      free_cmd_list(&list);
      return 1;
    }

  } else if (!strcmp(argv[1], "-e")) {
    ERROR("Not implemented"); // TODO
    return 1;

  } else if (!strcmp(argv[1], "-m")) {
    ERROR("Not implemented"); // TODO
    return 1;

  } else if (!strcmp(argv[1], "-s")) {
    ERROR("Not implemented"); // TODO
    return 1;

  } else {
    string error_msg = STR_INIT;
    str_append(&error_msg, "Mode not recognized: ");
    str_append(&error_msg, argv[1]);
    ERROR(error_msg.str);
    str_free(&error_msg);

    ret = 1;
  }

  return ret;
}
