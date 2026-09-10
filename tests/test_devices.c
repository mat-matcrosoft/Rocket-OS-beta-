#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../drivers/device.h"
#include "../drivers/driver.h"

static unsigned char storage[512];
static int mock_read(void *context, uint32_t lba, uint32_t count, void *buffer) {
    (void)context; (void)lba; (void)count; memcpy(buffer, storage, sizeof(storage)); return 0;
}
static int mock_probe(void) { return 1; }

int main(void) {
    Device device = {0};
    Driver driver = {0};
    unsigned char output[512] = {0};
    strcpy(device.name, "mock0"); device.type = DEVICE_TYPE_BLOCK; device.sector_size = 512;
    device.sector_count = 1; device.read = mock_read;
    device_manager_init(); driver_manager_init();
    assert(device_register(&device) == 0);
    assert(device_manager_count() == 1);
    assert(device_find("mock0") != 0);
    assert(device_read(device_find("mock0"), 0, 1, output) == 0);
    strcpy(driver.name, "mock"); driver.probe = mock_probe;
    assert(driver_register(&driver) == 0);
    assert(driver_manager_probe_all() == 1);
    puts("device/driver tests: ok");
    return 0;
}
