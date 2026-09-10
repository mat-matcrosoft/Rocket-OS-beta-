#include "fat32.h"

#define FAT32_EOC 0x0FFFFFF8u
#define FAT32_BAD 0x0FFFFFF7u
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_VOLUME_ID 0x08
#define FAT32_ATTR_LONG_NAME 0x0F

typedef struct __attribute__((packed)) {
    uint8_t name[11];
    uint8_t attributes;
    uint8_t nt_reserved;
    uint8_t create_time_tenths;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} Fat32RawEntry;

typedef struct {
    Fat32DirEntry entry;
    Fat32RawEntry raw;
    uint32_t sector;
    uint32_t offset;
    uint8_t valid;
} Fat32FoundEntry;

static Fat32Volume *default_volume;

static uint32_t min_u32(uint32_t a, uint32_t b) { return a < b ? a : b; }

static void copy_bytes(void *dst, const void *src, uint32_t count) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    uint32_t i;
    for (i = 0; i < count; ++i) d[i] = s[i];
}

static void zero_bytes(void *dst, uint32_t count) {
    uint8_t *d = (uint8_t *)dst;
    uint32_t i;
    for (i = 0; i < count; ++i) d[i] = 0;
}

static int text_equal(const char *a, const char *b) {
    uint32_t i = 0;
    while (a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == 0 && b[i] == 0;
}

static char upper_ascii(char c) {
    return c >= 'a' && c <= 'z' ? (char)(c - 'a' + 'A') : c;
}

static int build_short_name(const char *source, uint8_t result[11]) {
    uint32_t i = 0;
    uint32_t base = 0;
    uint32_t ext = 0;
    int in_extension = 0;
    if (!source || !source[0] || text_equal(source, ".") || text_equal(source, "..")) return -1;
    for (i = 0; i < 11; ++i) result[i] = ' ';
    for (i = 0; source[i]; ++i) {
        char c = upper_ascii(source[i]);
        if (c == '.') {
            if (in_extension) return -1;
            in_extension = 1;
            continue;
        }
        if (c == '/' || c == ' ' || c == '\\' || c < 33) return -1;
        if (!in_extension) {
            if (base >= 8) return -1;
            result[base++] = (uint8_t)c;
        } else {
            if (ext >= 3) return -1;
            result[8 + ext++] = (uint8_t)c;
        }
    }
    return base ? 0 : -1;
}

static void public_name(const uint8_t short_name[11], char result[13]) {
    uint32_t i = 0;
    uint32_t out = 0;
    while (i < 8 && short_name[i] != ' ') result[out++] = (char)short_name[i++];
    if (short_name[8] != ' ') {
        result[out++] = '.';
        for (i = 8; i < 11 && short_name[i] != ' '; ++i) result[out++] = (char)short_name[i];
    }
    result[out] = 0;
}

static int read_sector(Fat32Volume *volume, uint32_t sector, void *buffer) {
    if (!volume || !volume->device || volume->bytes_per_sector != 512) return -1;
    return device_read(volume->device, sector, 1, buffer);
}

int fat32_init(Fat32Volume *volume, const Fat32BootSector *boot) {
    uint32_t total;
    uint64_t fat_end;
    uint32_t data_start;
    if (!volume || !boot || boot->bytes_per_sector != 512 || !boot->sectors_per_cluster ||
        (boot->sectors_per_cluster & (boot->sectors_per_cluster - 1)) != 0 ||
        !boot->reserved_sectors || !boot->fats || !boot->sectors_per_fat) return -1;
    total = boot->total_sectors16 ? boot->total_sectors16 : boot->total_sectors32;
    if (!total) return -1;
    fat_end = (uint64_t)boot->reserved_sectors + (uint64_t)boot->fats * boot->sectors_per_fat;
    if (fat_end >= total || fat_end > 0xFFFFFFFFu) return -1;
    data_start = (uint32_t)fat_end;
    volume->device = 0;
    volume->total_sectors = total;
    volume->fat_begin_sector = boot->reserved_sectors;
    volume->first_data_sector = data_start;
    volume->sectors_per_fat = boot->sectors_per_fat;
    volume->cluster_count = (total - data_start) / boot->sectors_per_cluster;
    if (!volume->cluster_count || volume->cluster_count > 0x0FFFFFFDu) return -1;
    volume->max_cluster = volume->cluster_count + 1;
    volume->root_cluster = boot->root_cluster;
    volume->bytes_per_sector = boot->bytes_per_sector;
    volume->fats = boot->fats;
    volume->sectors_per_cluster = boot->sectors_per_cluster;
    volume->mounted = 0;
    if (volume->root_cluster < 2 || volume->root_cluster > volume->max_cluster) return -1;
    return 0;
}

int fat32_mount(Fat32Volume *volume, Device *device) {
    uint8_t sector[512];
    Fat32BootSector *boot = (Fat32BootSector *)sector;
    if (!volume || !device || device->sector_size != 512 || device->sector_count < 1) return -1;
    if (read_sector(&(Fat32Volume){ .device = device, .bytes_per_sector = 512 }, 0, sector) < 0) return -1;
    if (sector[510] != 0x55 || sector[511] != 0xAA) return -2;
    if (fat32_init(volume, boot) < 0) return -3;
    if (volume->total_sectors > device->sector_count) return -4;
    volume->device = device;
    volume->mounted = 1;
    return 0;
}

uint32_t fat32_cluster_to_lba(const Fat32Volume *volume, uint32_t cluster) {
    if (!volume || cluster < 2) return 0;
    return volume->first_data_sector + (cluster - 2) * volume->sectors_per_cluster;
}

static int valid_cluster(const Fat32Volume *volume, uint32_t cluster) {
    return volume && cluster >= 2 && cluster <= volume->max_cluster;
}

static int read_fat_entry(Fat32Volume *volume, uint32_t cluster, uint32_t *value) {
    uint8_t sector[512];
    uint32_t lba;
    uint32_t offset;
    if (!valid_cluster(volume, cluster) || !value) return -1;
    lba = volume->fat_begin_sector + (cluster * 4) / 512;
    offset = (cluster * 4) % 512;
    if (read_sector(volume, lba, sector) < 0) return -2;
    *value = (uint32_t)sector[offset] | ((uint32_t)sector[offset + 1] << 8) |
             ((uint32_t)sector[offset + 2] << 16) | ((uint32_t)sector[offset + 3] << 24);
    *value &= 0x0FFFFFFFu;
    return 0;
}

static int raw_to_public(const Fat32RawEntry *raw, Fat32DirEntry *entry) {
    if (!raw || !entry || raw->name[0] == 0xE5 || raw->name[0] == 0x00 ||
        raw->attributes == FAT32_ATTR_LONG_NAME || (raw->attributes & FAT32_ATTR_VOLUME_ID)) return 0;
    public_name(raw->name, entry->name);
    entry->attributes = raw->attributes;
    entry->first_cluster = ((uint32_t)raw->first_cluster_high << 16) | raw->first_cluster_low;
    entry->size = raw->file_size;
    return 1;
}

static int find_in_directory(Fat32Volume *volume, uint32_t directory_cluster, const uint8_t wanted[11], Fat32FoundEntry *found) {
    uint8_t sector[512];
    uint32_t cluster = directory_cluster;
    uint32_t visited = 0;
    if (!valid_cluster(volume, directory_cluster) || !wanted || !found) return -1;
    found->valid = 0;
    while (valid_cluster(volume, cluster) && visited++ <= volume->cluster_count) {
        uint32_t s;
        for (s = 0; s < volume->sectors_per_cluster; ++s) {
            uint32_t offset;
            if (read_sector(volume, fat32_cluster_to_lba(volume, cluster) + s, sector) < 0) return -2;
            for (offset = 0; offset + sizeof(Fat32RawEntry) <= 512; offset += sizeof(Fat32RawEntry)) {
                Fat32RawEntry *raw = (Fat32RawEntry *)(sector + offset);
                Fat32DirEntry public_entry;
                if (raw->name[0] == 0x00) return 0;
                if (!raw_to_public(raw, &public_entry)) continue;
                {
                    uint32_t i;
                    int same = 1;
                    for (i = 0; i < 11; ++i) if (raw->name[i] != wanted[i]) same = 0;
                    if (same) {
                        found->entry = public_entry;
                        found->raw = *raw;
                        found->sector = fat32_cluster_to_lba(volume, cluster) + s;
                        found->offset = offset;
                        found->valid = 1;
                        return 0;
                    }
                }
            }
        }
        {
            uint32_t next;
            if (read_fat_entry(volume, cluster, &next) < 0) return -3;
            if (fat32_is_end(next)) return 0;
            if (next == FAT32_BAD || next < 2 || next > volume->max_cluster) return -4;
            cluster = next;
        }
    }
    return -5;
}

static int list_directory(Fat32Volume *volume, uint32_t directory_cluster, Fat32ListCallback callback, void *context) {
    uint8_t sector[512];
    uint32_t cluster = directory_cluster;
    uint32_t visited = 0;
    int count = 0;
    if (!valid_cluster(volume, directory_cluster) || !callback) return -1;
    while (valid_cluster(volume, cluster) && visited++ <= volume->cluster_count) {
        uint32_t s;
        for (s = 0; s < volume->sectors_per_cluster; ++s) {
            uint32_t offset;
            if (read_sector(volume, fat32_cluster_to_lba(volume, cluster) + s, sector) < 0) return -2;
            for (offset = 0; offset + sizeof(Fat32RawEntry) <= 512; offset += sizeof(Fat32RawEntry)) {
                Fat32RawEntry *raw = (Fat32RawEntry *)(sector + offset);
                Fat32DirEntry entry;
                int result;
                if (raw->name[0] == 0x00) return count;
                if (!raw_to_public(raw, &entry)) continue;
                result = callback(&entry, context);
                if (result < 0) return result;
                ++count;
            }
        }
        {
            uint32_t next;
            if (read_fat_entry(volume, cluster, &next) < 0) return -3;
            if (fat32_is_end(next)) return count;
            if (next == FAT32_BAD || next < 2 || next > volume->max_cluster) return -4;
            cluster = next;
        }
    }
    return -5;
}

static int copy_path(char *destination, const char *source) {
    uint32_t i;
    if (!destination || !source) return -1;
    for (i = 0; i < 255 && source[i]; ++i) destination[i] = source[i];
    if (source[i]) return -1;
    destination[i] = 0;
    return 0;
}

static int resolve_path(Fat32Volume *volume, const char *path, Fat32FoundEntry *found) {
    char path_copy[256];
    char *cursor;
    uint32_t directory = volume->root_cluster;
    if (!volume || !path || !found || copy_path(path_copy, path) < 0) return -1;
    cursor = path_copy;
    while (*cursor == '/') ++cursor;
    if (!*cursor) return -2;
    for (;;) {
        char *component = cursor;
        char *next;
        uint8_t wanted[11];
        while (*cursor && *cursor != '/') ++cursor;
        next = cursor;
        while (*next == '/') ++next;
        if (*cursor) *cursor = 0;
        if (build_short_name(component, wanted) < 0) return -3;
        if (find_in_directory(volume, directory, wanted, found) < 0 || !found->valid) return -4;
        if (!*next) return 0;
        if (!(found->entry.attributes & FAT32_ATTR_DIRECTORY)) return -5;
        directory = found->entry.first_cluster;
        cursor = next;
    }
}

int fat32_list(Fat32Volume *volume, const char *path, Fat32ListCallback callback, void *context) {
    Fat32FoundEntry found;
    uint32_t directory;
    if (!volume || !volume->mounted || !callback) return -1;
    if (!path || !path[0] || text_equal(path, "/")) directory = volume->root_cluster;
    else {
        if (resolve_path(volume, path, &found) < 0 || !found.valid) return -2;
        if (!(found.entry.attributes & FAT32_ATTR_DIRECTORY)) return -3;
        directory = found.entry.first_cluster;
    }
    return list_directory(volume, directory, callback, context);
}

int fat32_read_file_on(Fat32Volume *volume, const char *path, void *buffer, uint32_t capacity) {
    Fat32FoundEntry found;
    uint8_t sector[512];
    uint8_t *destination = (uint8_t *)buffer;
    uint32_t cluster;
    uint32_t remaining;
    if (!volume || !volume->mounted || !buffer) return -1;
    if (resolve_path(volume, path, &found) < 0 || !found.valid) return -2;
    if (found.entry.attributes & FAT32_ATTR_DIRECTORY) return -3;
    if (found.entry.size > capacity) return -4;
    if (!found.entry.size) return 0;
    cluster = found.entry.first_cluster;
    remaining = found.entry.size;
    while (remaining && valid_cluster(volume, cluster)) {
        uint32_t s;
        for (s = 0; s < volume->sectors_per_cluster && remaining; ++s) {
            uint32_t amount;
            if (read_sector(volume, fat32_cluster_to_lba(volume, cluster) + s, sector) < 0) return -5;
            amount = min_u32(remaining, 512);
            copy_bytes(destination, sector, amount);
            destination += amount;
            remaining -= amount;
        }
        if (!remaining) return (int)found.entry.size;
        if (read_fat_entry(volume, cluster, &cluster) < 0 || fat32_is_end(cluster)) return -6;
        if (cluster == FAT32_BAD || !valid_cluster(volume, cluster)) return -7;
    }
    return -8;
}

static int chain_length(Fat32Volume *volume, uint32_t first, uint32_t *last) {
    uint32_t cluster = first;
    uint32_t count = 0;
    if (!last) return -1;
    if (!first) { *last = 0; return 0; }
    while (valid_cluster(volume, cluster) && count <= volume->cluster_count) {
        uint32_t next;
        *last = cluster;
        ++count;
        if (read_fat_entry(volume, cluster, &next) < 0) return -2;
        if (fat32_is_end(next)) return (int)count;
        if (next == FAT32_BAD || !valid_cluster(volume, next)) return -3;
        cluster = next;
    }
    return -4;
}

int fat32_write_file_on(Fat32Volume *volume, const char *path, const void *data, uint32_t length) {
    Fat32FoundEntry found;
    const uint8_t *source = (const uint8_t *)data;
    uint8_t sector[512];
    uint32_t cluster;
    uint32_t last;
    uint32_t required;
    uint32_t available;
    if (!volume || !volume->mounted || !data || !volume->device->write) return -1;
    if (resolve_path(volume, path, &found) < 0 || !found.valid) return -2;
    if (found.entry.attributes & FAT32_ATTR_DIRECTORY) return -3;
    required = (length + volume->sectors_per_cluster * 512 - 1) / (volume->sectors_per_cluster * 512);
    available = (uint32_t)chain_length(volume, found.entry.first_cluster, &last);
    if ((int)available < 0 || required > available) return -4;
    cluster = found.entry.first_cluster;
    while (cluster && length) {
        uint32_t s;
        for (s = 0; s < volume->sectors_per_cluster; ++s) {
            uint32_t amount = min_u32(length, 512);
            zero_bytes(sector, 512);
            copy_bytes(sector, source, amount);
            if (device_write(volume->device, fat32_cluster_to_lba(volume, cluster) + s, 1, sector) < 0) return -5;
            source += amount;
            length -= amount;
            if (!length) break;
        }
        if (length && read_fat_entry(volume, cluster, &cluster) < 0) return -6;
    }
    if (read_sector(volume, found.sector, sector) < 0) return -7;
    ((Fat32RawEntry *)(sector + found.offset))->file_size = (uint32_t)(source - (const uint8_t *)data);
    if (device_write(volume->device, found.sector, 1, sector) < 0) return -8;
    return 0;
}

void fat32_set_default(Fat32Volume *volume) { default_volume = volume; }
Fat32Volume *fat32_default(void) { return default_volume; }
int fat32_read_file(const char *path, void *buffer, uint32_t capacity) {
    return default_volume ? fat32_read_file_on(default_volume, path, buffer, capacity) : -1;
}
int fat32_write_file(const char *path, const void *data, uint32_t length) {
    return default_volume ? fat32_write_file_on(default_volume, path, data, length) : -1;
}
