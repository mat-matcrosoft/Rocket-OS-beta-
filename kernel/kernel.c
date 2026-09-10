#include "kernel.h"
#include "vga.h"
#include "shell.h"
#include "../config/config.h"
#include "../drivers/device.h"
#include "../drivers/driver.h"
#include "../drivers/ata.h"
#include "../fs/fat32.h"

static Fat32Volume root_volume;
static int root_mounted;

static int text_equal(const char *a, const char *b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a == 0 && *b == 0;
}

static const char *command_arg(const char *line, const char *command) {
    const char *p = line;
    while (*command && *p == *command) { ++p; ++command; }
    if (*command || (*p && *p != ' ')) return 0;
    while (*p == ' ') ++p;
    return p;
}

static void print_help(void) {
    vga_println("Commands: help clear about devices ls cat write");
    vga_println("FAT32 supports short 8.3 paths and existing-file writes.");
}

static int print_entry(const Fat32DirEntry *entry, void *context) {
    (void)context;
    vga_print(entry->name);
    if (entry->attributes & 0x10) vga_putc('/');
    vga_putc('\n');
    return 0;
}

static void storage_init(void) {
    Device *disk;
    device_manager_init();
    driver_manager_init();
    if (ata_driver_register() < 0 || driver_manager_probe_all() <= 0) {
        vga_println("ATA: no disk detected");
        return;
    }
    disk = device_find("ata0");
    if (!disk || fat32_mount(&root_volume, disk) < 0) {
        vga_println("FAT32: no valid volume on ata0");
        return;
    }
    root_mounted = 1;
    fat32_set_default(&root_volume);
    vga_println("ATA: ata0 online | FAT32 mounted");
}

void shell_execute(const char *line) {
    const char *argument;
    if (!*line) return;
    if (text_equal(line, "help")) print_help();
    else if (text_equal(line, "clear")) vga_clear();
    else if (text_equal(line, "about")) { vga_print(ROCKET_OS_NAME " " ROCKET_OS_VERSION); vga_putc('\n'); }
    else if (text_equal(line, "devices")) {
        vga_println(device_manager_count() ? "ata0: block device" : "no devices");
    } else if (shell_is_command(line, "ls")) {
        argument = command_arg(line, "ls");
        if (!root_mounted) vga_println("ls: FAT32 is not mounted");
        else if (fat32_list(&root_volume, argument && *argument ? argument : "/", print_entry, 0) < 0) vga_println("ls: unable to read directory");
    } else if (shell_is_command(line, "cat")) {
        char buffer[4096];
        argument = command_arg(line, "cat");
        if (!root_mounted || !argument || !*argument) vga_println("cat: FAT32 path required");
        else {
            int size = fat32_read_file_on(&root_volume, argument, buffer, sizeof(buffer) - 1);
            if (size < 0) vga_println("cat: read failed or file is too large");
            else { buffer[size] = 0; vga_print(buffer); vga_putc('\n'); }
        }
    } else if (shell_is_command(line, "write")) {
        vga_println("write: only existing short-name files can be replaced");
        vga_println("write support is available through the FAT32 kernel API");
    } else vga_println("Unknown command. Type help.");
}

void kernel_main(void) {
    char line[128];
    vga_set_color(0x1F);
    vga_clear();
    vga_println("ROCKET-OS kernel online");
    vga_println("Protected mode active | VGA text console ready");
    storage_init();
    vga_println("Type help for commands.");
    for (;;) {
        vga_print("rocket> ");
        vga_readline(line, sizeof(line));
        shell_execute(line);
    }
}
