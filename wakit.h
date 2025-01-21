#ifndef WAKIT_H
#define WAKIT_H

#include "dynamic_string.h"

typedef enum {
  Profile,
  Action
} cmd_type;

typedef struct {
  string name;
  string cmd;
  cmd_type type;

  // Only for profiles. Specifies that the profile is custom to the app.
  // It should be 'generic' or be the name of the asociated app. It should be
  // empty for Actions
  string app;
  bool default_for_app;
} cmd;

// Init cmd strigs
#define INIT_CMD(c) c.cmd = STR_INIT;   \
                    c.name = STR_INIT;  \
                    c.app = STR_INIT;   \

typedef struct command_node {
  cmd info;
  struct command_node *next;
} cmd_node;

// cmd list operations
bool add_command(cmd_node **list, cmd c);
int load_cmd_list(cmd_node **list);
bool save_cmd_list(cmd_node *list);
void free_cmd_list(cmd_node **list);
cmd_node *search_cmd(cmd_node *list, char *cmd_name);

// cmd operations
void print_cmd(cmd c, bool show_cmd);

// User interaction
void print_help(const char *app_path);
int create_command(char *name, char *command, char *type);
int menu();

cmd_node *default_profile_for_app(string app_name);

#endif // WAKIT_H
