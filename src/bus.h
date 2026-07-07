#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include <string.h>
#include "common.h"

typedef struct Bus {
    uint8_t* read_pages[BUS_PAGE_COUNT];
    uint8_t* write_pages[BUS_PAGE_COUNT];

    uint8_t* ram;
    uint8_t* exp1;
    uint8_t scratchpad[SCRATCHPAD_SIZE];
    uint8_t* bios;
} Bus;

void bus_init(Bus *bus);
void bus_destroy(Bus *bus);
void bus_reset(Bus *bus);


#ifndef PSXY_SINGLE_STEP_TEST_MODE

uint8_t io_read8(PSX *psx, uint32_t addr);
uint16_t io_read16(PSX *psx, uint32_t addr);
uint32_t io_read32(PSX *psx, uint32_t addr);

void io_write8(PSX *psx, uint32_t addr, uint8_t value);
void io_write16(PSX *psx, uint32_t addr, uint16_t value);
void io_write32(PSX *psx, uint32_t addr, uint32_t value);

static inline uint8_t bus_read8(PSX *psx, Bus* bus, uint32_t addr) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = bus->read_pages[page];

    if (unlikely(page_ptr == NULL)) {
        return io_read8(psx, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    return page_ptr[offset];
}

static inline uint16_t bus_read16(PSX *psx, Bus* bus, uint32_t addr) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = bus->read_pages[page];

    if (unlikely(page_ptr == NULL)) {
        return io_read16(psx, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    uint16_t value;
    memcpy(&value, page_ptr + offset, sizeof(uint16_t));
    return value;
}

static inline uint32_t bus_read32_internal(PSX *psx, Bus* bus, uint32_t addr) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint8_t *page_ptr = bus->read_pages[page];

    if (unlikely(page_ptr == NULL)) {
        return io_read32(psx, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    uint32_t value;
    memcpy(&value, page_ptr + offset, sizeof(uint32_t));
    return value;
}

static inline uint32_t bus_read32(PSX *psx, Bus* bus, uint32_t addr) {
    return bus_read32_internal(psx, bus, addr);
}

static inline uint32_t bus_fetch32(PSX *psx, Bus* bus, uint32_t addr) {
    return bus_read32_internal(psx, bus, addr);
}

static inline void bus_write8(PSX *psx, Bus* bus, uint32_t addr, uint8_t value) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = bus->write_pages[page];

    if (unlikely(page_ptr == NULL)) {
        io_write8(psx, addr, value);
        return;
    }
    uint32_t offset = addr & BUS_PAGE_MASK;
    page_ptr[offset] = value;
}

static inline void bus_write16(PSX *psx, Bus* bus, uint32_t addr, uint16_t value) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = bus->write_pages[page];

    if (unlikely(page_ptr == NULL)) {
        io_write16(psx, addr, value);
        return;
    }
    uint32_t offset = addr & BUS_PAGE_MASK;
    memcpy(page_ptr + offset, &value, sizeof(uint16_t));

}

static inline void bus_write32(PSX *psx, Bus* bus, uint32_t addr, uint32_t value) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = bus->write_pages[page];

    if (unlikely(page_ptr == NULL)) {
        io_write32(psx, addr, value);
        return;
    }
    uint32_t offset = addr & BUS_PAGE_MASK;
    memcpy(page_ptr + offset, &value, sizeof(uint32_t));
}

#else
#include "single_step_bus.h"
#endif // PSXY_SINGLE_STEP_TEST_MODE
#endif // BUS_H
