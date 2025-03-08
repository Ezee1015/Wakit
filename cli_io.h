#ifndef CLI_IO_H
#define CLI_IO_H

#include <stdlib.h>
#include "dynamic_string.h"

#define ERROR(msg) error(msg, __FILE__, __LINE__)
#define DEBUG(msg) debug(msg, __FILE__, __LINE__)

void error(const char *msg, char *filename, int line);
void debug(const char *msg, char *filename, int line);
bool question_yn(char *msg);

int console_input(char *cmd, const char *input);
int console_output(char *cmd, string *output);
int console_silent(char *cmd);

#endif // CLI_IO_H
