#include "stdio.h"
#include "stdlib.h"
#include "bus.h"

#define BUS_PAGE_SIZE 4096 // 4KB page size
#define BUS_PAGE_SHIFT 12 // Shift for page size (2^12 = 4096)
#define BUS_PAGE_MASK (BUS_PAGE_SIZE - 1) // Mask for page offset
#define BUS_PAGE_COUNT (1 << (32 - BUS_PAGE_SHIFT)) // Number of pages in 32-bit address space

#define KUSEG_VIRT_START 0x00000000
#define KUSEG_VIRT_END   0x1FFFFFFF
#define KSEG0_VIRT_START 0x80000000
#define KSEG0_VIRT_END   0x9FFFFFFF
#define KSEG1_VIRT_START 0xA0000000
#define KSEG1_VIRT_END   0xBFFFFFFF

#define KSEG0_PAGE_START (KSEG0_VIRT_START >> BUS_PAGE_SHIFT)
#define KSEG0_PAGE_END   (KSEG0_VIRT_END >> BUS_PAGE_SHIFT)
#define KSEG1_PAGE_START (KSEG1_VIRT_START >> BUS_PAGE_SHIFT)
#define KSEG1_PAGE_END   (KSEG1_VIRT_END >> BUS_PAGE_SHIFT)

#define RAM_SIZE       1024 * 1024 * 2 // 2MB of RAM
#define RAM_PHYS_START 0x00000000
#define RAM_PHYS_END   (RAM_PHYS_START + RAM_SIZE - 1)

#define EXP1_SIZE 1024 * 1024 * 8 // 8MB of expansion 1 memory
#define EXP1_PHYS_START 0x1F000000
#define EXP1_PHYS_END   (EXP1_PHYS_START + EXP1_SIZE - 1)

#define SCRATCHPAD_SIZE 1024 // 1KB of scratchpad memory
#define SCRATCHPAD_PHYS_START 0x1F800000
#define SCRATCHPAD_PHYS_END   (SCRATCHPAD_PHYS_START + SCRATCHPAD_SIZE - 1)

#define IO_SIZE 1024 * 8 // 8KB of I/O space
#define IO_PHYS_START 0x1F000000
#define IO_PHYS_END   (IO_PHYS_START + IO_SIZE - 1)

#define BIOS_SIZE 1024 * 512 // 512KB of BIOS
#define BIOS_PHYS_START 0x1FC00000
#define BIOS_PHYS_END   (BIOS_PHYS_START + BIOS_SIZE - 1)


uint8_t* page_table[BUS_PAGE_COUNT]; // Page table for 32-bit address space

static void bus_map_ram(uint8_t *ram) {
    uint32_t start_page = RAM_PHYS_START >> BUS_PAGE_SHIFT;
    uint32_t end_page = RAM_PHYS_END >> BUS_PAGE_SHIFT;
    // map to KUSEG, KSEG0, and KSEG1
    for (uint32_t page = start_page; page <= end_page; page++) {
        page_table[page] = ram + (page << BUS_PAGE_SHIFT);
        page_table[page + KSEG0_PAGE_START] = ram + (page << BUS_PAGE_SHIFT);
        page_table[page + KSEG1_PAGE_START] = ram + (page << BUS_PAGE_SHIFT);
    }
}

void bus_init() {
    uint8_t *ram = (uint8_t *)malloc(RAM_SIZE);
    if (!ram) {
        fprintf(stderr, "Failed to allocate RAM\n");
        exit(EXIT_FAILURE);
    }
    bus_map_ram(ram);
}

uint8_t bus_read8(uint32_t addr) {
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = page_table[page];

    if (page >= BUS_PAGE_COUNT || !page_ptr) {
        fprintf(stderr, "Invalid read8 at address 0x%08X\n", addr);
        exit(EXIT_FAILURE);
    }

    return page_ptr[offset];
}

uint16_t bus_read16(uint32_t addr) {
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = page_table[page];

    if (page >= BUS_PAGE_COUNT || !page_ptr) {
        fprintf(stderr, "Invalid read16 at address 0x%08X\n", addr);
        exit(EXIT_FAILURE);
    }

    uint16_t value;
    memcpy(&value, page_ptr + offset, sizeof(uint16_t));
    return value;
}

uint32_t bus_read32(uint32_t addr) {
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = page_table[page];

    if (page >= BUS_PAGE_COUNT || !page_ptr) {
        fprintf(stderr, "Invalid read32 at address 0x%08X\n", addr);
        exit(EXIT_FAILURE);
    }

    uint32_t value;
    memcpy(&value, page_ptr + offset, sizeof(uint32_t));
    return value;
}

void bus_write8(uint32_t addr, uint8_t value) {
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = page_table[page];

    if (page >= BUS_PAGE_COUNT || !page_ptr) {
        fprintf(stderr, "Invalid write8 at address 0x%08X\n", addr);
        exit(EXIT_FAILURE);
    }

    page_ptr[offset] = value;
}

void bus_write16(uint32_t addr, uint16_t value) {
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = page_table[page];

    if (page >= BUS_PAGE_COUNT || !page_ptr) {
        fprintf(stderr, "Invalid write16 at address 0x%08X\n", addr);
        exit(EXIT_FAILURE);
    }

    memcpy(page_ptr + offset, &value, sizeof(uint16_t));
}

void bus_write32(uint32_t addr, uint32_t value) {
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = page_table[page];

    if (page >= BUS_PAGE_COUNT || !page_ptr) {
        fprintf(stderr, "Invalid write32 at address 0x%08X\n", addr);
        exit(EXIT_FAILURE);
    }

    memcpy(page_ptr + offset, &value, sizeof(uint32_t));
}
