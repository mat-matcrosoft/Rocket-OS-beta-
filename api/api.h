#ifndef ROCKET_API_H
#define ROCKET_API_H

#include <stdint.h>
void rocket_print(const char *text);
void rocket_println(const char *text);
int rocket_open(const char *path, uint32_t flags);
int rocket_read(int fd, void *buffer, uint32_t length);
int rocket_write(int fd, const void *buffer, uint32_t length);
void *rocket_malloc(uint32_t size);
void rocket_free(void *pointer);

#endif
