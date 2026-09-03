#include "rbe.h"
#include "../config/config.h"

static uint32_t sum_bytes(const uint8_t *data, uint32_t length) { uint32_t sum = 0; for (uint32_t i = 0; i < length; ++i) sum += data[i]; return sum; }

int rbe_validate(const void *image, uint32_t length) {
    if (!image || length < sizeof(RbeHeader)) return -1;
    const RbeHeader *header = (const RbeHeader *)image;
    if (header->magic != RBE_MAGIC || header->version != RBE_VERSION) return -2;
    uint32_t table_end = sizeof(RbeHeader) + header->section_count * sizeof(RbeSection);
    if (table_end > length) return -3;
    const RbeSection *sections = (const RbeSection *)((const uint8_t *)image + sizeof(RbeHeader));
    for (uint16_t i = 0; i < header->section_count; ++i)
        if (sections[i].offset > length || sections[i].size > length - sections[i].offset) return -4;
    return 0;
}

const RbeSection *rbe_sections(const void *image) { return (const RbeSection *)((const uint8_t *)image + sizeof(RbeHeader)); }
const void *rbe_entry(const void *image) { const RbeHeader *h = (const RbeHeader *)image; return (const uint8_t *)image + h->entry_point; }
