#include "driver.h"

#define DRIVER_LIMIT 8

static Driver drivers[DRIVER_LIMIT];
static uint32_t driver_count;

static void copy_name(char *dst, const char *src) {
    uint32_t i = 0;
    if (!src) { dst[0] = 0; return; }
    while (src[i] && i + 1 < DRIVER_NAME_LENGTH) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

static int text_equal(const char *a, const char *b) {
    uint32_t i = 0;
    if (!a || !b) return 0;
    while (a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == 0 && b[i] == 0;
}

void driver_manager_init(void) {
    uint32_t i;
    driver_count = 0;
    for (i = 0; i < DRIVER_LIMIT; ++i) drivers[i].active = 0;
}

int driver_register(const Driver *driver) {
    if (!driver || !driver->name[0] || !driver->probe) return -1;
    if (driver_find(driver->name)) return -2;
    if (driver_count >= DRIVER_LIMIT) return -3;
    drivers[driver_count] = *driver;
    copy_name(drivers[driver_count].name, driver->name);
    drivers[driver_count].active = 1;
    ++driver_count;
    return (int)(driver_count - 1);
}

Driver *driver_find(const char *name) {
    uint32_t i;
    for (i = 0; i < driver_count; ++i)
        if (drivers[i].active && text_equal(drivers[i].name, name)) return &drivers[i];
    return 0;
}

uint32_t driver_manager_count(void) { return driver_count; }

int driver_manager_probe_all(void) {
    uint32_t i;
    int total = 0;
    for (i = 0; i < driver_count; ++i)
        if (drivers[i].active && drivers[i].probe) {
            int result = drivers[i].probe();
            if (result > 0) total += result;
        }
    return total;
}
