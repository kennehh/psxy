#ifndef BUS_H
#define BUS_H

#include <stdint.h>

#define BUS_PAGE_SIZE 4096 // 4KB page size
#define BUS_PAGE_COUNT (0x20000000 / BUS_PAGE_SIZE) // 2^29 / 2^12 = 2^17 pages
#define BUS_PAGE_SHIFT 12 // 2^12 = 4096
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

Bus* bus_create();
void bus_destroy(Bus *bus);

extern uint8_t (*bus_read8)(Bus *bus, uint32_t addr);
extern uint16_t (*bus_read16)(Bus *bus, uint32_t addr);
extern uint32_t (*bus_read32)(Bus *bus, uint32_t addr);
extern uint32_t (*bus_fetch32)(Bus *bus, uint32_t addr);
extern void (*bus_write8)(Bus *bus, uint32_t addr, uint8_t value);
extern void (*bus_write16)(Bus *bus, uint32_t addr, uint16_t value);
extern void (*bus_write32)(Bus *bus, uint32_t addr, uint32_t value);

#endif // BUS_H
