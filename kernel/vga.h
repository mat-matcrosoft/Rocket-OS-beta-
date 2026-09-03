#ifndef ROCKET_VGA_H
#define ROCKET_VGA_H

#include <stdint.h>

void vga_clear(void);
void vga_set_color(uint8_t color);
void vga_putc(char c);
void vga_print(const char *text);
void vga_println(const char *text);
int vga_getchar_nonblocking(void);
char *vga_readline(char *buffer, uint32_t capacity);

#endif
