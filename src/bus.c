#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bus.h"

static void map_buffer(Bus *bus, uint8_t *buffer, uint32_t start_phys_addr, uint32_t end_phys_addr) {
    for (uint32_t addr = start_phys_addr; addr < end_phys_addr; addr += BUS_PAGE_SIZE) {
        uint32_t page = addr >> BUS_PAGE_SHIFT;
        bus->page_table[page] = buffer + (addr - start_phys_addr);
    }
}

static inline uint32_t physical_address(uint32_t addr) {
    return addr & 0x1FFFFFFF; // Mask to 29 bits
}

void init_buffer(uint8_t **buffer, size_t size) {
    *buffer = (uint8_t *)malloc(size);
    if (!*buffer) {
        fprintf(stderr, "Failed to allocate buffer of size %zu\n", size);
        exit(EXIT_FAILURE);
    }
}

Bus* bus_create() {
    Bus* bus = (Bus*)malloc(sizeof(Bus));
    if (!bus) {
        fprintf(stderr, "Failed to allocate Bus structure\n");
        exit(EXIT_FAILURE);
    }

    init_buffer(&bus->ram, RAM_SIZE);
    map_buffer(bus, bus->ram, RAM_PHYS_START, RAM_PHYS_END);

    init_buffer(&bus->exp1, EXP1_SIZE);
    map_buffer(bus, bus->exp1, EXP1_PHYS_START, EXP1_PHYS_END);

    init_buffer(&bus->scratchpad, SCRATCHPAD_SIZE);
    map_buffer(bus, bus->scratchpad, SCRATCHPAD_PHYS_START, SCRATCHPAD_PHYS_END);

    init_buffer(&bus->bios, BIOS_SIZE);
    map_buffer(bus, bus->bios, BIOS_PHYS_START, BIOS_PHYS_END);

    return bus;
}

void bus_destroy(Bus *bus) {
    if (!bus) return;

    free(bus->ram);
    free(bus->exp1);
    free(bus->scratchpad);
    free(bus->bios);
    free(bus);
}

static inline uint8_t io_read8(Bus *bus, uint32_t addr) {
    return 0; // Placeholder for I/O read implementation
}

static inline uint16_t io_read16(Bus *bus, uint32_t addr) {
    return 0; // Placeholder for I/O read implementation
}

static inline uint32_t io_read32(Bus *bus, uint32_t addr) {
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

uint8_t bus_read8_actual(Bus *bus, uint32_t addr) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr) {
        return page_ptr[offset];
    }
    return io_read8(bus, addr);
}

uint16_t bus_read16_actual(Bus *bus, uint32_t addr) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr) {
        uint16_t value;
        memcpy(&value, page_ptr + offset, sizeof(uint16_t));
        return value;
    }
    return io_read16(bus, addr);
}

uint32_t bus_read32_actual(Bus *bus, uint32_t addr) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr) {
        uint32_t value;
        memcpy(&value, page_ptr + offset, sizeof(uint32_t));
        return value;
    }
    return io_read32(bus, addr);
}

void bus_write8_actual(Bus *bus, uint32_t addr, uint8_t value) {
    addr = physical_address(addr); uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr) {
        page_ptr[offset] = value;
        return;
    }
    io_write8(bus, addr, value);
}

void bus_write16_actual(Bus *bus, uint32_t addr, uint16_t value) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr) {
        memcpy(page_ptr + offset, &value, sizeof(uint16_t));
        return;
    }
    io_write16(bus, addr, value);
}

void bus_write32_actual(Bus *bus, uint32_t addr, uint32_t value) {
    addr = physical_address(addr);
    uint32_t page = addr >> BUS_PAGE_SHIFT;
    uint32_t offset = addr & BUS_PAGE_MASK;
    uint8_t *page_ptr = bus->page_table[page];

    if (page_ptr) {
        memcpy(page_ptr + offset, &value, sizeof(uint32_t));
        return;
    }
    io_write32(bus, addr, value);
}

uint8_t (*bus_read8)(Bus *bus, uint32_t addr) = bus_read8_actual;
uint16_t (*bus_read16)(Bus *bus, uint32_t addr) = bus_read16_actual;
uint32_t (*bus_read32)(Bus *bus, uint32_t addr) = bus_read32_actual;
uint32_t (*bus_fetch32)(Bus *bus, uint32_t addr) = bus_read32_actual;
void (*bus_write8)(Bus *bus, uint32_t addr, uint8_t value) = bus_write8_actual;
void (*bus_write16)(Bus *bus, uint32_t addr, uint16_t value) = bus_write16_actual;
void (*bus_write32)(Bus *bus, uint32_t addr, uint32_t value) = bus_write32_actual;
