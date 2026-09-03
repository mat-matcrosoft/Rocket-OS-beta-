#include "fat32.h"

int fat32_init(Fat32Volume *volume, const Fat32BootSector *boot) {
    if (!volume || !boot || boot->bytes_per_sector != 512 || boot->sectors_per_cluster == 0 || boot->sectors_per_fat == 0) return -1;
    volume->fat_begin_sector = boot->reserved_sectors;
    volume->first_data_sector = boot->reserved_sectors + boot->fats * boot->sectors_per_fat;
    volume->root_cluster = boot->root_cluster;
    volume->sectors_per_cluster = boot->sectors_per_cluster;
    return 0;
}

uint32_t fat32_cluster_to_lba(const Fat32Volume *volume, uint32_t cluster) {
    return volume->first_data_sector + (cluster - 2) * volume->sectors_per_cluster;
}

int fat32_read_file(const char *path, void *buffer, uint32_t capacity) { (void)path; (void)buffer; (void)capacity; return -2; }
int fat32_write_file(const char *path, const void *data, uint32_t length) { (void)path; (void)data; (void)length; return -2; }
