#include "cli_io.h"
#include "window_manager.h"

// Select window with the cursor and return its name
bool select_window(string *name) {
  printf("Open the app and press enter to continue...\n");
  getchar();
  printf("Click on the app...\n");

  // Get PID
  string pid = STR_INIT;
  int ret = console("xdotool selectwindow getwindowpid", &pid);
  if (ret) {
    str_free(&pid);
    ERROR("Error while getting the PID of the window. The app is not compatible...");
    return false;
  }

  // Get name
  string cmd = STR_INIT;
  str_append(&cmd, "ps -p ");
  str_append(&cmd, pid.str);
  str_append(&cmd, " -o comm=");
  ret = console(cmd.str, name);
  str_free(&pid);
  str_free(&cmd);
  if (ret) {
    ERROR("Error while getting the name of the window.");
    str_free(name);
    return false;
  }

  return true;
}
