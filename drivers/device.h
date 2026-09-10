#ifndef ROCKET_DEVICE_H
#define ROCKET_DEVICE_H

#include <stdint.h>

#define DEVICE_NAME_LENGTH 16

typedef enum {
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_BLOCK = 1,
    DEVICE_TYPE_CHAR = 2,
    DEVICE_TYPE_CONSOLE = 3
} DeviceType;

typedef int (*DeviceReadFn)(void *context, uint32_t lba, uint32_t count, void *buffer);
typedef int (*DeviceWriteFn)(void *context, uint32_t lba, uint32_t count, const void *buffer);

typedef struct {
    uint32_t id;
    char name[DEVICE_NAME_LENGTH];
    DeviceType type;
    uint32_t sector_size;
    uint32_t sector_count;
    void *context;
    DeviceReadFn read;
    DeviceWriteFn write;
    uint8_t active;
} Device;

void device_manager_init(void);
int device_register(const Device *device);
int device_unregister(const char *name);
Device *device_get(uint32_t index);
Device *device_find(const char *name);
uint32_t device_manager_count(void);
int device_read(Device *device, uint32_t lba, uint32_t count, void *buffer);
int device_write(Device *device, uint32_t lba, uint32_t count, const void *buffer);

#endif
