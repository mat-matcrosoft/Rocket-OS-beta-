#include "kernel.h"
#include "vga.h"
#include "shell.h"
#include "../config/config.h"

static int text_equal(const char *a, const char *b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a == 0 && *b == 0;
}

static void print_help(void) {
    vga_println("Commands: help clear about ls cat mkdir rmdir touch rm cp mv echo load run");
    vga_println("This alpha image provides the console shell and modular kernel skeleton.");
}

void shell_execute(const char *line) {
    if (!*line) return;
    if (text_equal(line, "help")) print_help();
    else if (text_equal(line, "clear")) vga_clear();
    else if (text_equal(line, "about")) { vga_print(ROCKET_OS_NAME " " ROCKET_OS_VERSION); vga_putc('\n'); }
    else if (shell_is_command(line, "echo")) { vga_println(line + 5); }
    else if (text_equal(line, "ls")) vga_println("[FAT32 mount point is ready for the next implementation step]");
    else if (shell_is_command(line, "cat")) vga_println("cat: FAT32 file reads are not wired in this alpha");
    else if (shell_is_command(line, "mkdir") || shell_is_command(line, "rmdir") || shell_is_command(line, "touch") || text_equal(line, "rm"))
        vga_println("filesystem command: FAT32 write support is scaffolded, not enabled yet");
    else if (shell_is_command(line, "cp") || shell_is_command(line, "mv"))
        vga_println("filesystem command: copy/move will use the FAT32 API");
    else if (shell_is_command(line, "load") || shell_is_command(line, "run"))
        vga_println("rbe: parser and verifier are scaffolded; loader is the next milestone");
    else vga_println("Unknown command. Type help.");
}

void kernel_main(void) {
    char line[128];
    vga_set_color(0x1F);
    vga_clear();
    vga_println("ROCKET-OS kernel online");
    vga_println("Protected mode active | VGA text console ready");
    vga_println("Type help for commands.");
    for (;;) {
        vga_print("rocket> ");
        vga_readline(line, sizeof(line));
        shell_execute(line);
    }
}
