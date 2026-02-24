#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wakit.h"
#include "cli_io.h"
#include "gui_io.h"
#include "dynamic_string.h"
#include "window_manager.h"

#define TABLET_MODEL "Wacom One by Wacom S Pen stylus"
#define MODEL_PLACEHOLDER "%TabletID%"
#define STOP_DAEMON_PATH "/tmp/stop.wakit"
#define RUNNING_DAEMON_PATH "/tmp/running.wakit"
#define DAEMON_DELAY 1

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
  printf("\t--remove [name] ................... Remove a command\n");
  printf("\t-r, --run [name] .................. Run a command\n");
  printf("\t-e [name] [variable] [new_value] .. Edit a command. Variables:\n");
  printf("\t                                    - name: Name of the command\n");
  printf("\t                                    - command: Command to execute\n");
  printf("\t                                    - type: 'action' or 'profile'\n");
  printf("\t                                    - app: if it's a profile, change the app that it's assign to. You should not input a new_value\n");
  printf("\t                                    - default: if it's a profile, change if it's the default profile for the app. Values are: yes/no\n");
  printf("\t-m ................................ Run menu\n");
  printf("\t-d ................................ Start/Stop daemon\n");
  printf("\t--export .......................... Print all the commands as wakit instructions\n");
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
  str_append(path, "/.local/share/wakit");
  return true;
}

// Return values:
//   1  --> Error while opening
//   -1 --> Format error
//   0  --> OK
int load_cmd_list(cmd_node **list) {
  string path = {0};
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

  fclose(f);
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

  string path = {0};
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

cmd_node *search_cmd(cmd_node *list, char *cmd_name) {
  if (!cmd_name) return NULL;

  while (list && strcmp(list->info.name.str, cmd_name))
    list = list->next;

  return list;
}

cmd_node *default_app_profile(cmd_node *list, char *app_name) {
  while (list) {
    if (list->info.type==Profile && !strcmp(list->info.app.str, app_name) && list->info.default_for_app)
      return list;

    list = list->next;
  }

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
    string err = {0};
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
  cmd_node *def_profile = NULL;
  if (new_cmd.default_for_app && (def_profile = default_app_profile(list, new_cmd.app.str))) {
    DEBUG("There's already a default profile for the app. Disabling it...");
    def_profile->info.default_for_app = false;
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
      string err = {0};
      str_append(&err, "Unrecognized option for 'list' mode: ");
      str_append(&err, argv[i]);
      ERROR(err.str);
      str_free(&err);
      free_cmd_list(&list);
      return 1;
    }
  }

  string app_name = {0};
  if (mode == Filter && !select_window(&app_name)) {
    ERROR("Unable to filter for this app...");
    str_free(&app_name);
    free_cmd_list(&list);
    return 1;
  }

  while (list) {
    if ( (mode == Default)
         || (mode == Profiles && list->info.type == Profile)
         || (mode == Actions && list->info.type == Action)
         || (mode == Filter && !strcmp(list->info.app.str, app_name.str))
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

int print_instructions(cmd_node *list, char *wakit_path) {
  if (!list) return 1;

  while (list) {
    cmd info = list->info;
    if (!str_search_and_replace(&info.cmd, "\"", "\\\"")) return 1;
    printf("%s -a \"%s\" \"%s\" ", wakit_path, info.name.str, info.cmd.str);

    switch (info.type) {
      case Profile:
        printf("profile");

        if (!info.app.str) return 1;
        if (!strcmp(info.app.str, "generic")) {
          printf(" # Generic\n");
          break;
        }
        printf(" # Custom to %s", info.app.str);
        if (info.default_for_app) printf(" (Default)\n");
        else putc('\n', stdout);
        break;
      case Action:
        printf("action\n");
        break;
    }

    list = list->next;
  }

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

int run_cmd(cmd cmd, string *output) {
  string command = {0}, model = {0};
  str_append(&command, cmd.cmd.str);
  str_append(&model, "'");
  str_append(&model, TABLET_MODEL);
  str_append(&model, "'");
  str_search_and_replace(&command, MODEL_PLACEHOLDER, model.str);

  int ret = console_output(command.str, output);
  str_free(&command);
  str_free(&model);
  return ret;
}

int menu() {
  cmd_node *list = NULL;
  if (load_cmd_list(&list) != 0) return 1;

  if (!list) {
    DEBUG("Empty list.");
    return 0;
  }

  cmd_node *selected = ask_for_cmd(list);

  if (!selected) {
    DEBUG("Didn't select anything");
    free_cmd_list(&list);
    return 0;
  }

  string output = {0};
  int ret = run_cmd(selected->info, &output);

  if (ret) {
    string error_msg = {0};
    str_append(&error_msg, "Command failed with exit code ");
    str_append_int(&error_msg, ret);
    ERROR(error_msg.str);
    str_free(&error_msg);
  }

  if (output.str) {
    str_insert_at(&output, 0, "Command output: ");
    DEBUG(output.str);
  }

  str_free(&output);
  free_cmd_list(&list);
  return 0;
}

bool run(cmd_node *list, char *cmd_name) {
  cmd_node *selected = search_cmd(list, cmd_name);
  if (!selected) {
    string error_msg = {0};
    str_append(&error_msg, "Unable to find the command '");
    str_append(&error_msg, cmd_name);
    str_append(&error_msg, "'");
    ERROR(error_msg.str);
    str_free(&error_msg);
    return false;
  }

  string output = {0};
  int ret = run_cmd(selected->info, &output);

  if (ret) {
    string error_msg = {0};
    str_append(&error_msg, "Command failed with exit code ");
    str_append_int(&error_msg, ret);
    ERROR(error_msg.str);
    str_free(&error_msg);
  }

  if (output.str) {
    str_insert_at(&output, 0, "Command output: ");
    DEBUG(output.str);
    str_free(&output);
  }

  return (ret == 0);
}

cmd duplicate_cmd(cmd info) {
  cmd new;
  INIT_CMD(new);

  str_append(&(new.name), info.name.str);
  str_append(&(new.app), info.app.str);
  str_append(&(new.cmd), info.cmd.str);
  new.type = info.type;
  new.default_for_app = info.default_for_app;

  return new;
}

// Creates a separated list with the available profiles for the given app
cmd_node *search_profiles_app(cmd_node *list, char *app_name) {
  cmd_node *availables = NULL, *aux = NULL;
  if ( (aux = default_app_profile(list, app_name)) ) {
    add_command(&availables, duplicate_cmd(aux->info));
    return availables;
  }

  // Add custom profiles
  if (strcmp(app_name,"generic")) {
    aux = list;
    while (aux) {
      if (aux->info.type == Profile && !strcmp(aux->info.app.str, app_name))
        add_command(&availables, duplicate_cmd(aux->info));

      aux = aux->next;
    }
  }

  // Add generic profiles
  aux = list;
  while (aux) {
    if (aux->info.type == Profile && !strcmp(aux->info.app.str, "generic"))
      add_command(&availables, duplicate_cmd(aux->info));

    aux = aux->next;
  }

  return availables;
}

struct generic_selection_list {
  string app;
  cmd_node *profile; // (should be generic)

  struct generic_selection_list *next;
};
typedef struct generic_selection_list generic_selection_list;

void free_generic_selection(generic_selection_list **list) {
  while (*list) {
    generic_selection_list *aux = *list;
    *list = aux->next;
    str_free(&(aux->app));
    free_cmd_list(&(aux->profile));
    free(aux);
  }
}

cmd_node *search_generic_selection(generic_selection_list *list, const char *app_name) {
  while (list && strcmp(list->app.str, app_name)) list = list->next;
  if (!list) return NULL;

  cmd_node *n = malloc(sizeof(cmd_node));
  n->next = NULL;
  n->info = duplicate_cmd(list->profile->info);
  return n;
}

int start_daemon() {
  cmd_node *list = NULL;
  if (load_cmd_list(&list) != 0) return 1;

  if (!list) {
    DEBUG("Empty list.");
    return 0;
  }

  // To remember the selection of a generic profile (custom profiles are
  // remembered by changing to 'true' the default_for_app variable)
  generic_selection_list *generic_selection = NULL;

  DEBUG("Daemon running...");
  string last_app = {0}, app = {0};
  while (access(STOP_DAEMON_PATH, F_OK)) {
    // If it's unable to get the active window's app name, default to generic...
    if (!get_active_window(&app)) str_replace(&app, "generic");

    if (!last_app.str || strcmp(app.str, last_app.str)) {
      cmd_node *profile = NULL;
      cmd_node *available_profiles = NULL;
      if ( !(available_profiles = search_generic_selection(generic_selection, app.str)) )
        available_profiles = search_profiles_app(list, app.str);

      if (available_profiles && available_profiles->next) { // More than one profile
        while (!profile) profile = ask_for_cmd(available_profiles);

        // Remember the selection for this session.
        //
        // We can activate default_for_app as the changes to the list are not
        // been saved to disk and it will make the custom profile selected
        // enable automatically when focusing in the app again.
        //
        // If it's a generic profile, it's added to the list "generic_selection"
        // for future reference.
        if (strcmp(app.str, "generic")) {
          if (!strcmp(profile->info.app.str, "generic")) {
            generic_selection_list *new_element = malloc(sizeof(generic_selection_list));
            new_element->app = (string) {NULL, 0, 0};
            str_append(&(new_element->app), app.str);
            new_element->profile = malloc(sizeof(cmd_node));
            new_element->profile->next = NULL;
            new_element->profile->info = duplicate_cmd(profile->info);
            new_element->next = generic_selection;
            generic_selection = new_element;
          } else {
            search_cmd(list, profile->info.name.str)->info.default_for_app = true;
          }
        }

      } else {
        profile = available_profiles;
      }

      // Debug information
      DEBUG("----------------------------------------");
      if (!strcmp(app.str, "generic")) DEBUG("Unable to get the active window's app name. Defaulting to generic...");
      string debug_msg = {0};
      str_append(&debug_msg, "The window focused has changed: ");
      if (last_app.str) str_append(&debug_msg, last_app.str);
      else str_append(&debug_msg, "[empty]");
      str_append(&debug_msg, " --> ");
      str_append(&debug_msg, app.str);
      DEBUG(debug_msg.str);
      cmd_node *aux = available_profiles;
      if (aux && aux->info.default_for_app) { // Found a default profile
        str_replace(&debug_msg, "Found a default profile: ");
        str_append(&debug_msg, aux->info.name.str);
      } else {
        str_replace(&debug_msg, "Available profiles for the app: ");
        while (aux) {
          str_append(&debug_msg, aux->info.name.str);
          if (aux->next) str_append(&debug_msg, ", ");
          aux = aux->next;
        }
      }
      DEBUG(debug_msg.str);
      str_replace(&debug_msg, "Profile selected: ");
      if (profile) str_append(&debug_msg, profile->info.name.str);
      else str_append(&debug_msg, "No profile was found");
      DEBUG(debug_msg.str);

      // Update running file
      FILE *f = fopen(RUNNING_DAEMON_PATH, "w");
      string text = {0};
      str_append(&text, app.str);
      str_append(&text, " | ");
      if (profile) str_append(&text, profile->info.name.str);
      else str_append(&text, "-no profile-");
      fwrite(text.str, text.str_len, 1, f);
      str_free(&text);
      fclose(f);

      if (profile) {
        // Apply profile
        int ret = run_cmd(profile->info, &debug_msg);

        if (debug_msg.str) {
          str_insert_at(&debug_msg, 0, "Command output: ");
          DEBUG(debug_msg.str);
        }

        if (ret) {
          str_replace(&debug_msg, "The command failed with exit code ");
          str_append_int(&debug_msg, ret);
        } else {
          str_replace(&debug_msg, "The command was executed successfully");
        }
        DEBUG(debug_msg.str);
      }

      str_free(&debug_msg);
      free_cmd_list(&available_profiles);
      str_replace(&last_app, app.str);
    }

    sleep(DAEMON_DELAY);
  }
  DEBUG("Daemon closed...");

  if (remove(STOP_DAEMON_PATH))    ERROR("Unable to remove the stop file...");
  if (remove(RUNNING_DAEMON_PATH)) ERROR("Unable to remove the running file...");

  str_free(&app);
  str_free(&last_app);
  free_cmd_list(&list);
  free_generic_selection(&generic_selection);
  return 0;
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

  } else if (!strcmp(argv[1], "--remove")) {
    if (argc != 3) {
      ERROR("Expected the command name.");
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
    if (argc == 2) {
      ERROR("The name of the command and the variable to edit were expected, but none was given");
      return 1;
    }
    if (argc == 3) {
      ERROR("The variable to edit was expected, but it was not given");
      return 1;
    }

    enum cmd_variable {
      cmd_name,
      cmd_command,
      cmd_type,
      cmd_app,
      cmd_default
    } var;

    if (!strcmp(argv[3], "name")) {
      if (argc != 5) {
        ERROR("The name was expected, but more than one argument was given");
        return 1;
      }
      var = cmd_name;

    } else if (!strcmp(argv[3], "command")) {
      if (argc != 5) {
        ERROR("The command was expected, but more than one argument was given");
        return 1;
      }
      var = cmd_command;

    } else if (!strcmp(argv[3], "type")) {
      if (argc != 5) {
        ERROR("The command was expected, but more than one argument was given");
        return 1;
      }
      if (strcmp(argv[4], "profile") && strcmp(argv[4], "action")) {
        ERROR("'profile' or 'action' was expected for the new value of the type");
        return 1;
      }
      var = cmd_type;

    } else if (!strcmp(argv[3], "app")) {
      if (argc != 4) {
        ERROR("No argument was expected");
        return 1;
      }
      var = cmd_app;

    } else if (!strcmp(argv[3], "default")) {
      if (argc != 5 || (strcmp(argv[4], "yes") && strcmp(argv[4], "no")) ) {
        ERROR("'yes' or 'no' was expected");
        return 1;
      }
      var = cmd_default;

    } else {
      ERROR("Variable not recognized...");
      return 1;
    }

    cmd_node *list = NULL, *node = NULL;
    if (load_cmd_list(&list) != 0) return 1;

    if ( !(node = search_cmd(list, argv[2])) ) {
      ERROR("Can't find the command...");
      free_cmd_list(&list);
      return 1;
    }

    string app_name = {0};
    switch (var) {
      case cmd_name:
        str_replace(&(node->info.name), argv[4]);
        break;

      case cmd_command:
        str_replace(&(node->info.cmd), argv[4]);
        break;

      case cmd_type:
        str_replace(&(node->info.app), "generic");
        node->info.default_for_app = false;
        if (!strcmp(argv[4], "profile"))
          node->info.type = Profile;
        else
          node->info.type = Action;
        break;

      case cmd_app:
        if (node->info.type != Profile) {
          ERROR("The command should be a profile in order to edit this...");
          free_cmd_list(&list);
          return 1;
        }

        if (question_yn("Do you want it to be a generic profile?")) {
          str_replace(&(node->info.app), "generic");
        } else {
          if (!select_window(&app_name)) {
            ERROR("App not compatible...");
            free_cmd_list(&list);
            str_free(&app_name);
            return 1;
          }
          str_free(&(node->info.app));
          node->info.app = app_name;
        }
        break;

      case cmd_default:
        if (node->info.type != Profile) {
          ERROR("The command should be a profile in order to edit this...");
          free_cmd_list(&list);
          return 1;
        }

        if (!strcmp(node->info.app.str, "generic")) {
          ERROR("The command should not be a generic profile in order to edit this...");
          free_cmd_list(&list);
          return 1;
        }

        bool default_for_app = (!strcmp(argv[4], "yes"));
        cmd_node *def_profile = NULL;
        if ( default_for_app && (def_profile = default_app_profile(list, node->info.app.str)) ) {
          DEBUG("There's already a default profile for the app. Disabling it...");
          def_profile->info.default_for_app = false;
        }
        node->info.default_for_app = default_for_app;
        break;
    }

    if (!save_cmd_list(list)) ret = 1;
    free_cmd_list(&list);

  } else if (!strcmp(argv[1], "-m")) {
    ret = menu();

  } else if (!strcmp(argv[1], "--export")) {
    cmd_node *list = NULL;
    if (load_cmd_list(&list) == -1) {
      ERROR("Can't load the save file");
      return 1;
    }
    ret = print_instructions(list, argv[0]);
    free_cmd_list(&list);

  } else if (!strcmp(argv[1], "-d")) {
    if (!access(RUNNING_DAEMON_PATH, F_OK)) {
      FILE *f = fopen(STOP_DAEMON_PATH, "w");
      fclose(f);
      DEBUG("Closing daemon...");
    } else {
      ret = start_daemon();
    }

  } else if (!strcmp(argv[1], "--run") || !strcmp(argv[1], "-r")) {
    if (argc != 3) {
      ERROR("Expected the command name.");
      return 1;
    }
    cmd_node *list = NULL;
    if (load_cmd_list(&list) == -1) {
      ERROR("Can't load the save file");
      return 1;
    }
    ret = (run(list, argv[2])) ? 0 : 1;
    free_cmd_list(&list);

  } else {
    string error_msg = {0};
    str_append(&error_msg, "Mode not recognized: ");
    str_append(&error_msg, argv[1]);
    ERROR(error_msg.str);
    str_free(&error_msg);

    ret = 1;
  }

  return ret;
}
