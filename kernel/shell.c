#include "shell.h"

static int same_prefix(const char *line, const char *command) {
    while (*command && *line == *command) { ++line; ++command; }
    return *command == 0 && (*line == 0 || *line == ' ');
}

int shell_is_command(const char *line, const char *command) { return same_prefix(line, command); }
