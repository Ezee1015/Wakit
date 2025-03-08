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

int console_input(char *cmd, const char *input) {
  if (!cmd || !input) return 1;

  FILE *pipe = popen(cmd, "w");
  if (!pipe) {
    ERROR("popen() failed!");
    return 1;
  }

  fprintf(pipe, "%s", input);
  return WEXITSTATUS(pclose(pipe));
}

int console_output(char *cmd, string *output) {
  if (!cmd || !output) return 1;
  if (output->str) str_free(output);

  FILE* pipe = popen(cmd, "r");
  if (!pipe) {
    ERROR("popen() failed!");
    return 1;
  }

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != NULL)
    str_append(output, buffer);

  // Removes the line break at the end of the line
  if (output->str_len != 0)
    output->str[output->str_len-1] = '\0';

  return WEXITSTATUS(pclose(pipe));
}

int console_silent(char *cmd) {
  if (!cmd) return 1;

  return system(cmd);
}
