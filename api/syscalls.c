#include "api.h"

/* The syscall numbers are intentionally kept in one place for ABI stability. */
enum { SYS_PRINT = 1, SYS_OPEN = 2, SYS_READ = 3, SYS_WRITE = 4, SYS_MALLOC = 5, SYS_FREE = 6 };

static int syscall3(int number, uintptr_t a, uintptr_t b, uintptr_t c) {
    int result; __asm__ volatile ("int $0x80" : "=a"(result) : "a"(number), "b"(a), "c"(b), "d"(c)); return result;
}
void rocket_print(const char *text) { (void)syscall3(SYS_PRINT, (uintptr_t)text, 0, 0); }
void rocket_println(const char *text) { rocket_print(text); rocket_print("\\n"); }
int rocket_open(const char *path, uint32_t flags) { return syscall3(SYS_OPEN, (uintptr_t)path, flags, 0); }
int rocket_read(int fd, void *buffer, uint32_t length) { return syscall3(SYS_READ, fd, (uintptr_t)buffer, length); }
int rocket_write(int fd, const void *buffer, uint32_t length) { return syscall3(SYS_WRITE, fd, (uintptr_t)buffer, length); }
void *rocket_malloc(uint32_t size) { return (void *)(uintptr_t)syscall3(SYS_MALLOC, size, 0, 0); }
void rocket_free(void *pointer) { (void)syscall3(SYS_FREE, (uintptr_t)pointer, 0, 0); }
