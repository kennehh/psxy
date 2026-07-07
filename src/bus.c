#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "bus.h"
#include "gpu.h"
#include "irq.h"
#include "timers.h"
#include "memctrl.h"

static void map_buffer(Bus *bus, uint8_t *buffer, size_t size, uint32_t start_phys_addr, uint32_t end_phys_addr, bool read_only) {
    for (uint32_t addr = start_phys_addr; addr < end_phys_addr; addr += BUS_PAGE_SIZE) {
        uint32_t page = get_page_index(addr);
        if (page >= BUS_PAGE_COUNT) {
            fprintf(stderr, "Address 0x%08X is out of bus page range\n", addr);
            exit(EXIT_FAILURE);
        }

        uint32_t buffer_offset = (addr - start_phys_addr) % size;
        bus->read_pages[page] = buffer + buffer_offset;
        if (!read_only) {
            bus->write_pages[page] = buffer + buffer_offset;
        }
    }
}

static uint8_t *init_buffer(size_t size) {
    uint8_t *buffer = (uint8_t *)malloc(size);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate buffer of size %zu\n", size);
        exit(EXIT_FAILURE);
    }
    return buffer;
}

void bus_init(Bus *bus) {
    for (int i = 0; i < BUS_PAGE_COUNT; i++) {
        bus->read_pages[i] = NULL;
        bus->write_pages[i] = NULL;
    }

    bus->ram = init_buffer(RAM_SIZE);
    map_buffer(bus, bus->ram, RAM_SIZE, RAM_PHYS_START, RAM_PHYS_END, false);

    bus->exp1 = init_buffer(EXP1_SIZE);
    map_buffer(bus, bus->exp1, EXP1_SIZE, EXP1_PHYS_START, EXP1_PHYS_END, false);

    map_buffer(bus, bus->scratchpad, SCRATCHPAD_SIZE, SCRATCHPAD_PHYS_START, SCRATCHPAD_PHYS_END, false);

    bus->bios = init_buffer(BIOS_SIZE);
    map_buffer(bus, bus->bios, BIOS_SIZE, BIOS_PHYS_START, BIOS_PHYS_END, true);

    bus_reset(bus);
}

void bus_reset(Bus *bus) {
    if (!bus) return;

    memset(bus->ram, 0, RAM_SIZE);
    memset(bus->exp1, 0, EXP1_SIZE);
    memset(bus->scratchpad, 0, SCRATCHPAD_SIZE);
    memset(bus->bios, 0, BIOS_SIZE);
}

void bus_destroy(Bus *bus) {
    if (!bus) return;

    free(bus->ram);
    free(bus->exp1);
    free(bus->bios);
}

#define IO_READ(size) \
uint##size##_t io_read##size(PSX *psx, uint32_t addr) { \
    if (addr >= IO_MEMCTRL_3_START) { \
        return memctrl_read##size(psx, addr); \
    } \
    addr = physical_address(addr); \
    if (addr >= IO_GPU_START && addr <= IO_GPU_END) { \
        return gpu_read##size(psx, addr); \
    } \
    if (addr >= IO_IRQ_START && addr <= IO_IRQ_END) { \
        return irq_read##size(psx, addr); \
    } \
    if (addr >= IO_TIMERS_START && addr <= IO_TIMERS_END) { \
        return timers_read##size(psx, addr); \
    } \
    if (addr >= IO_MEMCTRL_1_START && addr <= IO_MEMCTRL_1_END) { \
        return memctrl_read##size(psx, addr); \
    } \
    if (addr >= IO_MEMCTRL_2_START && addr <= IO_MEMCTRL_2_END) { \
        return memctrl_read##size(psx, addr); \
    } \
    printf("I/O read (%d) from unimplemented address: 0x%08X\n", size, addr); \
    return 0; \
}

#define IO_WRITE(size) \
void io_write##size(PSX *psx, uint32_t addr, uint##size##_t value) { \
    if (addr >= IO_MEMCTRL_3_START) { \
        memctrl_write##size(psx, addr, value); \
        return; \
    } \
    addr = physical_address(addr); \
    if (addr >= IO_GPU_START && addr <= IO_GPU_END) { \
        gpu_write##size(psx, addr, value); \
        return; \
    } \
    if (addr >= IO_IRQ_START && addr <= IO_IRQ_END) { \
        irq_write##size(psx, addr, value); \
        return; \
    } \
    if (addr >= IO_TIMERS_START && addr <= IO_TIMERS_END) { \
        timers_write##size(psx, addr, value); \
        return; \
    } \
    if (addr >= IO_MEMCTRL_1_START && addr <= IO_MEMCTRL_1_END) { \
        memctrl_write##size(psx, addr, value); \
        return; \
    } \
    if (addr >= IO_MEMCTRL_2_START && addr <= IO_MEMCTRL_2_END) { \
        memctrl_write##size(psx, addr, value); \
        return; \
    } \
    printf("I/O write (%d) to unimplemented address: 0x%08X, value: 0x%0*X\n", size, addr, size / 8 * 2, value); \
}

IO_READ(8)
IO_READ(16)
IO_READ(32)

IO_WRITE(8)
IO_WRITE(16)
IO_WRITE(32)
