#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "string.h"

#define MEMORY_SIZE 0x20000000 // 512MB of addressable memory

#define BUS_PAGE_SHIFT 12 // 2^12 = 4096
#define BUS_PAGE_SIZE (1 << BUS_PAGE_SHIFT) // 4096 bytes per page
#define BUS_PAGE_COUNT (MEMORY_SIZE / BUS_PAGE_SIZE) // 2^29 / 2^12 = 2^17 pages
#define BUS_PAGE_MASK (BUS_PAGE_SIZE - 1) // 0xFFF

#define RAM_SIZE       1024 * 1024 * 2 // 2MB of RAM
#define RAM_PHYS_START 0x00000000
#define RAM_PHYS_END   (RAM_PHYS_START + RAM_SIZE)

#define EXP1_SIZE 1024 * 1024 * 8 // 8MB of expansion 1 memory
#define EXP1_PHYS_START 0x1F000000
#define EXP1_PHYS_END   (EXP1_PHYS_START + EXP1_SIZE)

#define SCRATCHPAD_SIZE 4096 // 1KB of scratchpad memory, but we will allocate 4KB for alignment
#define SCRATCHPAD_PHYS_START 0x1F800000
#define SCRATCHPAD_PHYS_END   (SCRATCHPAD_PHYS_START + SCRATCHPAD_SIZE)

#define IO_SIZE 8192 // 8KB of I/O memory
#define IO_PHYS_START 0x1F801000
#define IO_PHYS_END   (IO_PHYS_START + IO_SIZE)

#define BIOS_SIZE 1024 * 512 // 512KB of BIOS
#define BIOS_PHYS_START 0x1FC00000
#define BIOS_PHYS_END   (BIOS_PHYS_START + BIOS_SIZE)

typedef struct {
    uint8_t* page_table[BUS_PAGE_COUNT];

    uint8_t* ram;
    uint8_t* exp1;
    uint8_t* scratchpad;
    uint8_t* bios;
} Bus;

Bus* bus_create(void);
void bus_destroy(Bus *bus);
void bus_reset(Bus *bus);

#ifdef USE_SINGLE_STEP_BUS
#include "single_step_bus.h"
#else

static inline uint8_t io_read8(Bus *bus, uint32_t addr) {
    return 0; // Placeholder for I/O read implementation
}

static inline uint16_t io_read16(Bus *bus, uint32_t addr) {
    return 0; // Placeholder for I/O read implementation
}

static inline uint32_t io_read32(Bus *bus, uint32_t addr) {
    switch (addr) {
        case 0x1f801814: return 0x1c802000; // GP1 Status Register
    }
    return 0; // Placeholder for I/O read implementation
}

static inline void io_write8(Bus *bus, uint32_t addr, uint8_t value) {
    // Placeholder for I/O write implementation
}

static inline void io_write16(Bus *bus, uint32_t addr, uint16_t value) {
    // Placeholder for I/O write implementation
}

static inline void io_write32(Bus *bus, uint32_t addr, uint32_t value) {
    // Placeholder for I/O write implementation
}

static inline uint32_t physical_address(uint32_t addr) {
    return addr & 0x1FFFFFFF; // Mask to 29 bits
}

static inline uint8_t bus_read8(Bus *bus, uint32_t addr) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr == NULL) {
        return io_read8(bus, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    return page_ptr[offset];
}

static inline uint16_t bus_read16(Bus *bus, uint32_t addr) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr == NULL) {
        return io_read16(bus, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    uint16_t value;
    memcpy(&value, page_ptr + offset, sizeof(uint16_t));
    return value;
}

static inline uint32_t bus_read32_internal(Bus *bus, uint32_t addr) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr == NULL) {
        return io_read32(bus, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    uint32_t value;
    memcpy(&value, page_ptr + offset, sizeof(uint32_t));
    return value;
}


static inline uint32_t bus_read32(Bus *bus, uint32_t addr) {
    return bus_read32_internal(bus, addr);
}

static inline uint32_t bus_fetch32(Bus *bus, uint32_t addr) {
    return bus_read32_internal(bus, addr);
}

static inline void bus_write8(Bus *bus, uint32_t addr, uint8_t value) {
    addr = physical_address(addr); uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr == NULL) {
        io_write8(bus, addr, value);
        return;
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    page_ptr[offset] = value;
}

static inline void bus_write16(Bus *bus, uint32_t addr, uint16_t value) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr == NULL) {
        io_write16(bus, addr, value);
        return;
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    memcpy(page_ptr + offset, &value, sizeof(uint16_t));

}

static inline void bus_write32(Bus *bus, uint32_t addr, uint32_t value) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr == NULL) {
        io_write32(bus, addr, value);
        return;
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    memcpy(page_ptr + offset, &value, sizeof(uint32_t));
}

#endif // USE_SINGLE_STEP_BUS
#endif // BUS_H
