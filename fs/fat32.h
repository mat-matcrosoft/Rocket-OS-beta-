#ifndef ROCKET_FAT32_H
#define ROCKET_FAT32_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t jump[3]; uint8_t oem[8]; uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster; uint16_t reserved_sectors; uint8_t fats;
    uint16_t root_entries; uint16_t total_sectors16; uint8_t media;
    uint16_t sectors_per_fat16; uint16_t sectors_per_track; uint16_t heads;
    uint32_t hidden_sectors; uint32_t total_sectors32; uint32_t sectors_per_fat;
    uint16_t flags; uint16_t version; uint32_t root_cluster;
} Fat32BootSector;

typedef struct { uint32_t first_data_sector; uint32_t fat_begin_sector; uint32_t root_cluster; uint8_t sectors_per_cluster; } Fat32Volume;

int fat32_init(Fat32Volume *volume, const Fat32BootSector *boot);
uint32_t fat32_cluster_to_lba(const Fat32Volume *volume, uint32_t cluster);
int fat32_read_file(const char *path, void *buffer, uint32_t capacity);
int fat32_write_file(const char *path, const void *data, uint32_t length);

#endif
