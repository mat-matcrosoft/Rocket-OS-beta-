#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../fs/fat32.h"

typedef struct { unsigned char bytes[16 * 512]; } MemoryDisk;

static int memory_read(void *context, uint32_t lba, uint32_t count, void *buffer) {
    MemoryDisk *disk = (MemoryDisk *)context;
    memcpy(buffer, disk->bytes + lba * 512, count * 512);
    return 0;
}

static int memory_write(void *context, uint32_t lba, uint32_t count, const void *buffer) {
    MemoryDisk *disk = (MemoryDisk *)context;
    memcpy(disk->bytes + lba * 512, buffer, count * 512);
    return 0;
}

static void put16(unsigned char *p, unsigned int value) { p[0] = value; p[1] = value >> 8; }
static void put32(unsigned char *p, unsigned int value) { p[0] = value; p[1] = value >> 8; p[2] = value >> 16; p[3] = value >> 24; }

static int count_entries(const Fat32DirEntry *entry, void *context) {
    int *count = (int *)context;
    assert(entry->name[0] != 0);
    ++*count;
    return 0;
}

int main(void) {
    MemoryDisk disk = {{0}};
    Device device = {0};
    Fat32Volume volume = {0};
    char contents[16] = {0};
    int entries = 0;
    unsigned char *boot = disk.bytes;
    unsigned char *fat = disk.bytes + 512;
    unsigned char *root = disk.bytes + 2 * 512;

    boot[0] = 0xEB; boot[2] = 0x90; boot[510] = 0x55; boot[511] = 0xAA;
    put16(boot + 11, 512); boot[13] = 1; put16(boot + 14, 1); boot[16] = 1;
    put32(boot + 32, 16); put32(boot + 36, 1); put32(boot + 44, 2);
    put16(fat + 0, 0xFFF8); put16(fat + 2, 0xFFFF); put32(fat + 8, 0x0FFFFFFF); put32(fat + 12, 0x0FFFFFFF);
    memcpy(root, "HELLO   TXT", 11); root[11] = 0x20; put16(root + 26, 3); put32(root + 28, 5);
    memcpy(disk.bytes + 3 * 512, "hello", 5);

    strcpy(device.name, "mem0"); device.type = DEVICE_TYPE_BLOCK; device.sector_size = 512;
    device.sector_count = 16; device.active = 1; device.context = &disk; device.read = memory_read; device.write = memory_write;
    assert(fat32_mount(&volume, &device) == 0);
    assert(volume.first_data_sector == 2);
    assert(fat32_cluster_to_lba(&volume, 2) == 2);
    assert(fat32_read_file_on(&volume, "/HELLO.TXT", contents, sizeof(contents)) == 5);
    assert(strcmp(contents, "hello") == 0);
    assert(fat32_list(&volume, "/", count_entries, &entries) == 1);
    assert(fat32_write_file_on(&volume, "/HELLO.TXT", "world", 5) == 0);
    memset(contents, 0, sizeof(contents));
    assert(fat32_read_file_on(&volume, "/HELLO.TXT", contents, sizeof(contents)) == 5);
    assert(strcmp(contents, "world") == 0);
    puts("fat32 tests: ok");
    return 0;
}
