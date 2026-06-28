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

static void init_buffer(uint8_t **buffer, size_t size) {
    *buffer = (uint8_t *)malloc(size);
    if (!*buffer) {
        fprintf(stderr, "Failed to allocate buffer of size %zu\n", size);
        exit(EXIT_FAILURE);
    }
}

Bus* bus_create(void) {
    Bus* bus = (Bus*)malloc(sizeof(Bus));
    if (!bus) {
        fprintf(stderr, "Failed to allocate Bus structure\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < BUS_PAGE_COUNT; i++) {
        bus->page_table[i] = NULL;
    }

    init_buffer(&bus->ram, RAM_SIZE);
    map_buffer(bus, bus->ram, RAM_PHYS_START, RAM_PHYS_END);

    init_buffer(&bus->exp1, EXP1_SIZE);
    map_buffer(bus, bus->exp1, EXP1_PHYS_START, EXP1_PHYS_END);

    init_buffer(&bus->scratchpad, SCRATCHPAD_SIZE);
    map_buffer(bus, bus->scratchpad, SCRATCHPAD_PHYS_START, SCRATCHPAD_PHYS_END);

    init_buffer(&bus->bios, BIOS_SIZE);
    map_buffer(bus, bus->bios, BIOS_PHYS_START, BIOS_PHYS_END);

    bus_reset(bus);
    return bus;
}

void bus_install_bios_trampolines(Bus *bus) {
    bus_write32(bus, 0xA0, 0x03E00008); // jr $ra
    bus_write32(bus, 0xA4, 0x00000000);
    bus_write32(bus, 0xB0, 0x03E00008); // jr $ra
    bus_write32(bus, 0xB4, 0x00000000);
    bus_write32(bus, 0xC0, 0x03E00008); // jr $ra
    bus_write32(bus, 0xC4, 0x00000000);
}

void bus_clear_bios_trampolines(Bus *bus) {
    bus_write32(bus, 0xA0, 0x00000000);
    bus_write32(bus, 0xA4, 0x00000000);
    bus_write32(bus, 0xB0, 0x00000000);
    bus_write32(bus, 0xB4, 0x00000000);
    bus_write32(bus, 0xC0, 0x00000000);
    bus_write32(bus, 0xC4, 0x00000000);
}

void bus_reset(Bus *bus) {
    if (!bus) return;

    memset(bus->ram, 0, RAM_SIZE);
    memset(bus->exp1, 0, EXP1_SIZE);
    memset(bus->scratchpad, 0, SCRATCHPAD_SIZE);
    memset(bus->bios, 0, BIOS_SIZE);
    bus_install_bios_trampolines(bus);
}

void bus_destroy(Bus *bus) {
    if (!bus) return;

    free(bus->ram);
    free(bus->exp1);
    free(bus->scratchpad);
    free(bus->bios);
    free(bus);
}
