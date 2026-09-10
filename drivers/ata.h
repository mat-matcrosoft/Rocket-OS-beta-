#ifndef ROCKET_ATA_H
#define ROCKET_ATA_H

#include <stdint.h>

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6

typedef struct {
    uint16_t io_base;
    uint16_t control_base;
    uint8_t slave;
    uint8_t present;
    uint32_t sectors;
} AtaDevice;

int ata_driver_register(void);
int ata_probe(void);
uint32_t ata_device_count(void);
AtaDevice *ata_device_get(uint32_t index);

#endif
