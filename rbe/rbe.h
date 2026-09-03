#ifndef ROCKET_RBE_H
#define ROCKET_RBE_H

#include <stdint.h>
#define RBE_CODE 1
#define RBE_DATA 2
#define RBE_RDATA 3
#define RBE_BSS 4

typedef struct __attribute__((packed)) { uint32_t magic; uint16_t version; uint16_t section_count; uint32_t entry_point; uint32_t checksum; } RbeHeader;
typedef struct __attribute__((packed)) { uint8_t type; uint32_t offset; uint32_t size; char name[8]; } RbeSection;
int rbe_validate(const void *image, uint32_t length);
const RbeSection *rbe_sections(const void *image);
const void *rbe_entry(const void *image);

#endif
