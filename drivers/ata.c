#include "ata.h"
#include "device.h"
#include "driver.h"

#define ATA_STATUS_ERROR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_DF 0x20
#define ATA_STATUS_READY 0x40
#define ATA_STATUS_BUSY 0x80
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_MAX_LBA28 0x10000000u
#define ATA_POLL_LIMIT 100000u

static AtaDevice ata_devices[2];
static uint32_t ata_count;

static inline uint8_t port_in8(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void port_out8(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void port_in16_string(uint16_t port, uint16_t *buffer, uint32_t words) {
    __asm__ volatile ("rep insw" : "+D"(buffer), "+c"(words) : "d"(port) : "memory");
}

static inline void port_out16_string(uint16_t port, const uint16_t *buffer, uint32_t words) {
    __asm__ volatile ("rep outsw" : "+S"(buffer), "+c"(words) : "d"(port));
}

static void ata_delay(AtaDevice *device) {
    (void)port_in8(device->control_base);
    (void)port_in8(device->control_base);
    (void)port_in8(device->control_base);
    (void)port_in8(device->control_base);
}

static int ata_wait(AtaDevice *device, uint8_t want_drq) {
    uint32_t i;
    uint8_t status = 0;
    for (i = 0; i < ATA_POLL_LIMIT; ++i) {
        status = port_in8(device->io_base + 7);
        if (!(status & ATA_STATUS_BUSY)) break;
    }
    if (status & ATA_STATUS_BUSY) return -1;
    if (!status || status == 0xFF) return -2;
    if (status & (ATA_STATUS_ERROR | ATA_STATUS_DF)) return -3;
    if (want_drq && !(status & ATA_STATUS_DRQ)) return -4;
    return 0;
}

static void ata_select(AtaDevice *device, uint32_t lba) {
    port_out8(device->io_base + 6, (uint8_t)(0xE0 | (device->slave << 4) | ((lba >> 24) & 0x0F)));
    ata_delay(device);
}

static int ata_identify(AtaDevice *device) {
    uint16_t identify[256];
    uint8_t status;
    ata_select(device, 0);
    port_out8(device->io_base + 2, 0);
    port_out8(device->io_base + 3, 0);
    port_out8(device->io_base + 4, 0);
    port_out8(device->io_base + 5, 0);
    port_out8(device->io_base + 7, ATA_CMD_IDENTIFY);
    status = port_in8(device->io_base + 7);
    if (!status || status == 0xFF) return -1;
    if (ata_wait(device, 1) < 0) return -2;
    port_in16_string(device->io_base, identify, 256);
    device->sectors = ((uint32_t)identify[61] << 16) | identify[60];
    if (!device->sectors || device->sectors > ATA_MAX_LBA28) return -3;
    device->present = 1;
    return 0;
}

static int ata_rw28(AtaDevice *device, uint32_t lba, uint32_t count, void *buffer, int write) {
    uint32_t sector;
    uint8_t *bytes = (uint8_t *)buffer;
    if (!device || !device->present || !buffer || !count) return -1;
    if (lba >= ATA_MAX_LBA28 || count > ATA_MAX_LBA28 - lba || lba >= device->sectors || count > device->sectors - lba)
        return -2;
    while (count) {
        uint32_t batch = count > 255 ? 255 : count;
        ata_select(device, lba);
        port_out8(device->io_base + 2, (uint8_t)batch);
        port_out8(device->io_base + 3, (uint8_t)lba);
        port_out8(device->io_base + 4, (uint8_t)(lba >> 8));
        port_out8(device->io_base + 5, (uint8_t)(lba >> 16));
        port_out8(device->io_base + 7, write ? ATA_CMD_WRITE_PIO : ATA_CMD_READ_PIO);
        for (sector = 0; sector < batch; ++sector) {
            if (ata_wait(device, 1) < 0) return -3;
            if (write) port_out16_string(device->io_base, (const uint16_t *)(bytes + sector * 512), 256);
            else port_in16_string(device->io_base, (uint16_t *)(bytes + sector * 512), 256);
        }
        if (write) {
            port_out8(device->io_base + 7, ATA_CMD_CACHE_FLUSH);
            if (ata_wait(device, 0) < 0) return -4;
        }
        lba += batch;
        count -= batch;
        bytes += batch * 512;
    }
    return 0;
}

static int ata_read_block(void *context, uint32_t lba, uint32_t count, void *buffer) {
    return ata_rw28((AtaDevice *)context, lba, count, buffer, 0);
}

static int ata_write_block(void *context, uint32_t lba, uint32_t count, const void *buffer) {
    return ata_rw28((AtaDevice *)context, lba, count, (void *)buffer, 1);
}

int ata_probe(void) {
    uint32_t i;
    ata_count = 0;
    for (i = 0; i < 2; ++i) {
        Device block;
        AtaDevice *device = &ata_devices[i];
        device->io_base = ATA_PRIMARY_IO;
        device->control_base = ATA_PRIMARY_CONTROL;
        device->slave = (uint8_t)i;
        device->present = 0;
        device->sectors = 0;
        if (ata_identify(device) < 0) continue;
        block.id = 0;
        block.name[0] = 'a'; block.name[1] = 't'; block.name[2] = 'a'; block.name[3] = (char)('0' + i); block.name[4] = 0;
        block.type = DEVICE_TYPE_BLOCK;
        block.sector_size = 512;
        block.sector_count = device->sectors;
        block.context = device;
        block.read = ata_read_block;
        block.write = ata_write_block;
        block.active = 1;
        if (device_register(&block) >= 0) { ++ata_count; }
    }
    return (int)ata_count;
}

int ata_driver_register(void) {
    Driver driver;
    driver.name[0] = 'a'; driver.name[1] = 't'; driver.name[2] = 'a'; driver.name[3] = 0;
    driver.device_type = DEVICE_TYPE_BLOCK;
    driver.probe = ata_probe;
    driver.active = 1;
    return driver_register(&driver);
}

uint32_t ata_device_count(void) { return ata_count; }
AtaDevice *ata_device_get(uint32_t index) { return index < ata_count ? &ata_devices[index] : 0; }
