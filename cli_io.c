#include "cli_io.h"

void error(const char *msg, char *filename, int line) {
  printf("%s:%d: [ERROR] %s\n", filename, line, msg);
}

void debug(const char *msg, char *filename, int line) {
  printf("%s:%d: [DEBUG] %s\n", filename, line, msg);
}

bool question_yn(char *msg) {
  char opt;
  do {
    printf("%s [y/n]: ", msg);
    scanf("%c", &opt);
  } while (opt != 'y' && opt != 'Y' && opt != 'n' && opt != 'N');
  getchar();

  return (opt == 'Y' || opt == 'y')
         ? true
         : false;
}

int console(char *cmd, string *output) {
  if (output && output->str)
    str_free(output);

  FILE* pipe = popen(cmd, "r");

  if (!pipe) {
    ERROR("popen() failed!");
    return 1;
  }

  if (output) {
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), pipe) != NULL)
      str_append(output, buffer);

    // Removes the Line break at the end of the line if it's not empty
    if (output->str_len != 0)
      str_remove(output, output->str_len-1, output->str_len-1);
  }

  return WEXITSTATUS(pclose(pipe));
}
