#include "device.h"

#define DEVICE_LIMIT 8

static Device devices[DEVICE_LIMIT];
static uint32_t device_count;

static void copy_name(char *dst, const char *src) {
    uint32_t i = 0;
    if (!src) { dst[0] = 0; return; }
    while (src[i] && i + 1 < DEVICE_NAME_LENGTH) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

static int text_equal(const char *a, const char *b) {
    uint32_t i = 0;
    if (!a || !b) return 0;
    while (a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == 0 && b[i] == 0;
}

void device_manager_init(void) {
    uint32_t i;
    device_count = 0;
    for (i = 0; i < DEVICE_LIMIT; ++i) devices[i].active = 0;
}

int device_register(const Device *device) {
    if (!device || !device->name[0] || device->type == DEVICE_TYPE_UNKNOWN || !device->read)
        return -1;
    if (device_find(device->name)) return -2;
    if (device_count >= DEVICE_LIMIT) return -3;
    devices[device_count] = *device;
    devices[device_count].id = device_count;
    devices[device_count].active = 1;
    copy_name(devices[device_count].name, device->name);
    ++device_count;
    return (int)(device_count - 1);
}

int device_unregister(const char *name) {
    Device *device = device_find(name);
    if (!device) return -1;
    device->active = 0;
    return 0;
}

Device *device_get(uint32_t index) {
    if (index >= device_count || !devices[index].active) return 0;
    return &devices[index];
}

Device *device_find(const char *name) {
    uint32_t i;
    for (i = 0; i < device_count; ++i)
        if (devices[i].active && text_equal(devices[i].name, name)) return &devices[i];
    return 0;
}

uint32_t device_manager_count(void) { return device_count; }

int device_read(Device *device, uint32_t lba, uint32_t count, void *buffer) {
    if (!device || !device->active || !device->read || !buffer || !count) return -1;
    if (lba >= device->sector_count || count > device->sector_count - lba) return -2;
    return device->read(device->context, lba, count, buffer);
}

int device_write(Device *device, uint32_t lba, uint32_t count, const void *buffer) {
    if (!device || !device->active || !device->write || !buffer || !count) return -1;
    if (lba >= device->sector_count || count > device->sector_count - lba) return -2;
    return device->write(device->context, lba, count, buffer);
}
