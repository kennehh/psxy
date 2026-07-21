#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include <string.h>
#include "core/common.h"

#define RAM_SIZE       1024 * 1024 * 2 // 2MB of RAM
#define RAM_PHYS_START 0x00000000
#define RAM_PHYS_END   (RAM_PHYS_START + RAM_SIZE - 1)

#define EXP1_SIZE       0x800000 // 8MB of expansion 1 memory
#define EXP1_PHYS_START 0x1F000000
#define EXP1_PHYS_END   (EXP1_PHYS_START + EXP1_SIZE - 1)

#define SCRATCHPAD_SIZE 1024 // 1KB of scratchpad memory
#define SCRATCHPAD_PHYS_START 0x1F800000
#define SCRATCHPAD_PHYS_END   0x1F800FFF

#define IO_SIZE       0x2000 // 8KB of I/O memory
#define IO_PHYS_START 0x1F801000
#define IO_PHYS_END   (IO_PHYS_START + IO_SIZE - 1)

#define BIOS_SIZE       0x80000 // 512KB of BIOS
#define BIOS_PHYS_START 0x1FC00000
#define BIOS_PHYS_END   (BIOS_PHYS_START + BIOS_SIZE - 1)

#define IO_GPU_START 0x1F801810
#define IO_GPU_END   0x1F80181F

#define IO_IRQ_START 0x1F801070
#define IO_IRQ_END   0x1F80107F

#define IO_TIMERS_START 0x1F801100
#define IO_TIMERS_END   0x1F80112F

#define IO_MEMCTRL_1_START 0x1F801000
#define IO_MEMCTRL_1_END   0x1F80102F

#define IO_MEMCTRL_2_START 0x1F801060
#define IO_MEMCTRL_2_END   0x1F80106F

#define IO_SPU_START 0x1F801C00
#define IO_SPU_END   0x1F801DFF

#define IO_MEMCTRL_3_START 0xFFFE0000

#define IO_EXP2_START 0x1F802000
#define IO_EXP2_END   0x1F8020FF

typedef struct Bus {
    uint8_t* read_pages[BUS_PAGE_COUNT];
    uint8_t* write_pages[BUS_PAGE_COUNT];

    uint8_t ram[RAM_SIZE];
    uint8_t exp1[EXP1_SIZE];
    uint8_t scratchpad[SCRATCHPAD_SIZE];
    uint8_t bios[BIOS_SIZE];
} Bus;

void bus_init(Bus *bus);
void bus_reset(Bus *bus);

#ifndef PSXY_SINGLE_STEP_TEST_MODE

uint8_t io_read8(PSX *psx, uint32_t addr);
uint16_t io_read16(PSX *psx, uint32_t addr);
uint32_t io_read32(PSX *psx, uint32_t addr);

void io_write8(PSX *psx, uint32_t addr, uint8_t value);
void io_write16(PSX *psx, uint32_t addr, uint16_t value);
void io_write32(PSX *psx, uint32_t addr, uint32_t value);

// The bus read/write functions are inlined for performance reasons.
// They first check if the address is mapped to a memory page, and if so, they perform the read/write directly.
// If not, they delegate to the I/O handlers.

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
    uint32_t page = get_page_index(addr);
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
