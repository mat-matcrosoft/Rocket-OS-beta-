#include <assert.h>
#include <stdio.h>
#include "../kernel/shell.h"
int main(void) { assert(shell_is_command("echo hello", "echo")); assert(!shell_is_command("echoes", "echo")); puts("shell tests: ok"); return 0; }
