#include <assert.h>
#include <stdio.h>
#include "../fs/fat32.h"
int main(void) {
    Fat32BootSector boot = {0}; Fat32Volume volume = {0};
    boot.bytes_per_sector = 512; boot.sectors_per_cluster = 8; boot.reserved_sectors = 32; boot.fats = 2; boot.sectors_per_fat = 100; boot.root_cluster = 2;
    assert(fat32_init(&volume, &boot) == 0); assert(volume.first_data_sector == 232); assert(fat32_cluster_to_lba(&volume, 2) == 232);
    puts("fat32 tests: ok"); return 0;
}
