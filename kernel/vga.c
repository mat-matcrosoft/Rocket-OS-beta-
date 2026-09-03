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

int vga_getchar_nonblocking(void) {
    static const char map[] = "?1234567890-=\\bqwertyuiop[]\\nasdfghjkl;'\\zxcvbnm,./";
    if (!(port_in8(0x64) & 1)) return -1;
    uint8_t scan = port_in8(0x60);
    if (scan & 0x80) return -1;
    if (scan == 0x1C) return '\n';
    if (scan == 0x0E) return '\b';
    if (scan < sizeof(map) - 1) return map[scan];
    return -1;
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
