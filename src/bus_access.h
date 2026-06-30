#ifndef BUS_ACCESS_H
#define BUS_ACCESS_H

#include "psx.h"

static inline uint8_t io_read8(PSX *psx, uint32_t addr) {
    return 0; // Placeholder for I/O read implementation
}

static inline uint16_t io_read16(PSX *psx, uint32_t addr) {
    return 0; // Placeholder for I/O read implementation
}

static inline uint32_t io_read32(PSX *psx, uint32_t addr) {
    switch (addr) {
        case 0x1f801814: return 0x1c802000; // GP1 Status Register
    }
    return 0; // Placeholder for I/O read implementation
}

static inline void io_write8(PSX *psx, uint32_t addr, uint8_t value) {
    // Placeholder for I/O write implementation
}

static inline void io_write16(PSX *psx, uint32_t addr, uint16_t value) {
    // Placeholder for I/O write implementation
}

static inline void io_write32(PSX *psx, uint32_t addr, uint32_t value) {
    // Placeholder for I/O write implementation
}

static inline uint8_t bus_read8(PSX *psx, uint32_t addr) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = psx->bus->read_pages[page];

    if (page_ptr == NULL) {
        return io_read8(psx, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    return page_ptr[offset];
}

static inline uint16_t bus_read16(PSX *psx, uint32_t addr) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = psx->bus->read_pages[page];

    if (page_ptr == NULL) {
        return io_read16(psx, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    uint16_t value;
    memcpy(&value, page_ptr + offset, sizeof(uint16_t));
    return value;
}

static inline uint32_t bus_read32_internal(PSX *psx, uint32_t addr) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint8_t *page_ptr = psx->bus->read_pages[page];

    if (page_ptr == NULL) {
        return io_read32(psx, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    uint32_t value;
    memcpy(&value, page_ptr + offset, sizeof(uint32_t));
    return value;
}


static inline uint32_t bus_read32(PSX *psx, uint32_t addr) {
    return bus_read32_internal(psx, addr);
}

static inline uint32_t bus_fetch32(PSX *psx, uint32_t addr) {
    return bus_read32_internal(psx, addr);
}

static inline void bus_write8(PSX *psx, uint32_t addr, uint8_t value) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = psx->bus->write_pages[page];

    if (page_ptr == NULL) {
        io_write8(psx, addr, value);
        return;
    }
    // bcache_invalidate_page(psx->bcache, page); // Invalidate the block cache for this page
    uint32_t offset = addr & BUS_PAGE_MASK;
    page_ptr[offset] = value;
}

static inline void bus_write16(PSX *psx, uint32_t addr, uint16_t value) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = psx->bus->write_pages[page];

    if (page_ptr == NULL) {
        io_write16(psx, addr, value);
        return;
    }
    // bcache_invalidate_page(psx->bcache, page); // Invalidate the block cache for this page
    uint32_t offset = addr & BUS_PAGE_MASK;
    memcpy(page_ptr + offset, &value, sizeof(uint16_t));

}

static inline void bus_write32(PSX *psx, uint32_t addr, uint32_t value) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = psx->bus->write_pages[page];

    if (page_ptr == NULL) {
        io_write32(psx, addr, value);
        return;
    }
    // bcache_invalidate_page(psx->bcache, page); // Invalidate the block cache for this page
    uint32_t offset = addr & BUS_PAGE_MASK;
    memcpy(page_ptr + offset, &value, sizeof(uint32_t));
}

#endif // BUS_ACCESS_H
