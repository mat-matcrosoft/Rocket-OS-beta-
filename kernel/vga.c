#include "vga.h"
#include "../config/config.h"

static volatile uint16_t *const video = (uint16_t *)VGA_MEMORY;
static uint8_t color = 0x1F;
static uint32_t row;
static uint32_t column;

static inline uint8_t port_in8(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void cursor(void) {
    uint16_t position = (uint16_t)(row * VGA_WIDTH + column);
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(position & 0xFF)), "Nd"((uint16_t)0x3D5));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(position >> 8)), "Nd"((uint16_t)0x3D5));
}

void vga_set_color(uint8_t new_color) { color = new_color; }

void vga_clear(void) {
    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) video[i] = ((uint16_t)color << 8) | ' ';
    row = 0;
    column = 0;
    cursor();
}

void vga_putc(char c) {
    if (c == '\n') { column = 0; ++row; }
    else if (c == '\r') column = 0;
    else if (c == '\b') {
        if (column) { --column; video[row * VGA_WIDTH + column] = ((uint16_t)color << 8) | ' '; }
    } else {
        video[row * VGA_WIDTH + column] = ((uint16_t)color << 8) | (uint8_t)c;
        if (++column == VGA_WIDTH) { column = 0; ++row; }
    }
    if (row >= VGA_HEIGHT) {
        for (uint32_t r = 1; r < VGA_HEIGHT; ++r)
            for (uint32_t c = 0; c < VGA_WIDTH; ++c) video[(r - 1) * VGA_WIDTH + c] = video[r * VGA_WIDTH + c];
        --row;
    }
    cursor();
}

void vga_print(const char *text) { while (*text) vga_putc(*text++); }
void vga_println(const char *text) { vga_print(text); vga_putc('\n'); }

static int scancode_to_ascii(uint8_t scan) {
    switch (scan) {
        case 0x02: return '1'; case 0x03: return '2'; case 0x04: return '3'; case 0x05: return '4';
        case 0x06: return '5'; case 0x07: return '6'; case 0x08: return '7'; case 0x09: return '8';
        case 0x0A: return '9'; case 0x0B: return '0'; case 0x0C: return '-'; case 0x0D: return '=';
        case 0x10: return 'q'; case 0x11: return 'w'; case 0x12: return 'e'; case 0x13: return 'r';
        case 0x14: return 't'; case 0x15: return 'y'; case 0x16: return 'u'; case 0x17: return 'i';
        case 0x18: return 'o'; case 0x19: return 'p'; case 0x1A: return '['; case 0x1B: return ']';
        case 0x1E: return 'a'; case 0x1F: return 's'; case 0x20: return 'd'; case 0x21: return 'f';
        case 0x22: return 'g'; case 0x23: return 'h'; case 0x24: return 'j'; case 0x25: return 'k';
        case 0x26: return 'l'; case 0x27: return ';'; case 0x28: return '\'';
        case 0x2C: return 'z'; case 0x2D: return 'x'; case 0x2E: return 'c'; case 0x2F: return 'v';
        case 0x30: return 'b'; case 0x31: return 'n'; case 0x32: return 'm'; case 0x33: return ',';
        case 0x34: return '.'; case 0x35: return '/'; default: return -1;
    }
}

int vga_getchar_nonblocking(void) {
    if (!(port_in8(0x64) & 1)) return -1;
    uint8_t scan = port_in8(0x60);
    if (scan & 0x80) return -1;
    if (scan == 0x1C) return '\n';
    if (scan == 0x0E) return '\b';
    return scancode_to_ascii(scan);
}

char *vga_readline(char *buffer, uint32_t capacity) {
    uint32_t length = 0;
    for (;;) {
        int ch = vga_getchar_nonblocking();
        if (ch < 0) { __asm__ volatile ("hlt"); continue; }
        if (ch == '\n') { buffer[length] = 0; vga_putc('\n'); return buffer; }
        if (ch == '\b' && length) { --length; vga_putc('\b'); continue; }
        if (ch >= 32 && ch < 127 && length + 1 < capacity) { buffer[length++] = (char)ch; vga_putc((char)ch); }
    }
}
