#include "fat32.h"

uint32_t fat32_next_cluster(uint32_t entry) { return entry & 0x0FFFFFFFu; }
int fat32_is_end(uint32_t cluster) { return cluster >= 0x0FFFFFF8u; }
