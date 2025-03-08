#include "cli_io.h"
#include "window_manager.h"

// Select window with the cursor and return its name
bool select_window(string *name) {
  printf("Open the app and press enter to continue...\n");
  getchar();
  printf("Click on the app...\n");

  // Get PID
  string pid = {0};
  int ret = console_output("xdotool selectwindow getwindowpid", &pid);
  if (ret) {
    str_free(&pid);
    DEBUG("Error while getting the PID of the window. The app is not compatible...");
    return false;
  }

  // Get name
  string cmd = {0};
  str_append(&cmd, "ps -p ");
  str_append(&cmd, pid.str);
  str_append(&cmd, " -o comm=");
  ret = console_output(cmd.str, name);
  str_free(&pid);
  str_free(&cmd);
  if (ret) {
    DEBUG("Error while getting the name of the window.");
    str_free(name);
    return false;
  }

  return true;
}

// get_active_window should not print anything as it's being executed constantly
bool get_active_window(string *name) {
  // Get PID
  string pid = {0};
  int ret = console_output("xdotool getactivewindow getwindowpid 2> /dev/null", &pid);
  if (ret) {
    str_free(&pid);
    return false;
  }

  // Get name
  string cmd = {0};
  str_append(&cmd, "ps -p ");
  str_append(&cmd, pid.str);
  str_append(&cmd, " -o comm=");
  ret = console_output(cmd.str, name);
  str_free(&pid);
  str_free(&cmd);
  if (ret) {
    str_free(name);
    return false;
  }

  return true;
}
