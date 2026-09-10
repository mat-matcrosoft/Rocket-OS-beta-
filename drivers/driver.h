#ifndef ROCKET_DRIVER_H
#define ROCKET_DRIVER_H

#include <stdint.h>

#define DRIVER_NAME_LENGTH 16

typedef int (*DriverProbeFn)(void);

typedef struct {
    char name[DRIVER_NAME_LENGTH];
    uint32_t device_type;
    DriverProbeFn probe;
    uint8_t active;
} Driver;

void driver_manager_init(void);
int driver_register(const Driver *driver);
Driver *driver_find(const char *name);
uint32_t driver_manager_count(void);
int driver_manager_probe_all(void);

#endif
