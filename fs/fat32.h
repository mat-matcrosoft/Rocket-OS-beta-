#ifndef ROCKET_FAT32_H
#define ROCKET_FAT32_H

#include <stdint.h>
#include "../drivers/device.h"

typedef struct __attribute__((packed)) {
    uint8_t jump[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fats;
    uint16_t root_entries;
    uint16_t total_sectors16;
    uint8_t media;
    uint16_t sectors_per_fat16;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors32;
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fs_type[8];
} Fat32BootSector;

typedef struct {
    Device *device;
    uint32_t total_sectors;
    uint32_t first_data_sector;
    uint32_t fat_begin_sector;
    uint32_t sectors_per_fat;
    uint32_t cluster_count;
    uint32_t max_cluster;
    uint32_t root_cluster;
    uint16_t bytes_per_sector;
    uint8_t fats;
    uint8_t sectors_per_cluster;
    uint8_t mounted;
} Fat32Volume;

typedef struct {
    char name[13];
    uint8_t attributes;
    uint32_t first_cluster;
    uint32_t size;
} Fat32DirEntry;

typedef int (*Fat32ListCallback)(const Fat32DirEntry *entry, void *context);

int fat32_init(Fat32Volume *volume, const Fat32BootSector *boot);
int fat32_mount(Fat32Volume *volume, Device *device);
uint32_t fat32_cluster_to_lba(const Fat32Volume *volume, uint32_t cluster);
uint32_t fat32_next_cluster(uint32_t entry);
int fat32_is_end(uint32_t cluster);
int fat32_list(Fat32Volume *volume, const char *path, Fat32ListCallback callback, void *context);
int fat32_read_file_on(Fat32Volume *volume, const char *path, void *buffer, uint32_t capacity);
int fat32_write_file_on(Fat32Volume *volume, const char *path, const void *data, uint32_t length);
void fat32_set_default(Fat32Volume *volume);
Fat32Volume *fat32_default(void);
int fat32_read_file(const char *path, void *buffer, uint32_t capacity);
int fat32_write_file(const char *path, const void *data, uint32_t length);

#endif
