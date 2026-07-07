#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "bus.h"
#include "gpu.h"
#include "irq.h"
#include "timers.h"

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

uint8_t io_read8(PSX *psx, uint32_t addr) {
    if (addr >= IO_GPU_START && addr <= IO_GPU_END) {
        return gpu_read8(psx, addr);
    }
    if (addr >= IO_IRQ_START && addr <= IO_IRQ_END) {
        return irq_read8(psx, addr);
    }
    if (addr >= IO_TIMERS_START && addr <= IO_TIMERS_END) {
        return timers_read8(psx, addr);
    }
    printf("I/O read (8) from unimplemented address: 0x%08X\n", addr);
    return 0; // Placeholder for I/O read implementation
}

uint16_t io_read16(PSX *psx, uint32_t addr) {
    if (addr >= IO_GPU_START && addr <= IO_GPU_END) {
        return gpu_read16(psx, addr);
    }
    if (addr >= IO_IRQ_START && addr <= IO_IRQ_END) {
        return irq_read16(psx, addr);
    }
    if (addr >= IO_TIMERS_START && addr <= IO_TIMERS_END) {
        return timers_read16(psx, addr);
    }
    printf("I/O read (16) from unimplemented address: 0x%08X\n", addr);
    return 0; // Placeholder for I/O read implementation
}

uint32_t io_read32(PSX *psx, uint32_t addr) {
    if (addr >= IO_GPU_START && addr <= IO_GPU_END) {
        return gpu_read32(psx, addr);
    }
    if (addr >= IO_IRQ_START && addr <= IO_IRQ_END) {
        return irq_read32(psx, addr);
    }
    if (addr >= IO_TIMERS_START && addr <= IO_TIMERS_END) {
        return timers_read32(psx, addr);
    }
    printf("I/O read (32) from unimplemented address: 0x%08X\n", addr);
    return 0; // Placeholder for I/O read implementation
}

void io_write8(PSX *psx, uint32_t addr, uint8_t value) {
    if (addr >= IO_GPU_START && addr <= IO_GPU_END) {
        gpu_write8(psx, addr, value);
        return;
    }
    if (addr >= IO_IRQ_START && addr <= IO_IRQ_END) {
        irq_write8(psx, addr, value);
        return;
    }
    if (addr >= IO_TIMERS_START && addr <= IO_TIMERS_END) {
        timers_write8(psx, addr, value);
        return;
    }
    printf("I/O write (8) to unimplemented address: 0x%08X, value: 0x%02X\n", addr, value);
}

void io_write16(PSX *psx, uint32_t addr, uint16_t value) {
    if (addr >= IO_GPU_START && addr <= IO_GPU_END) {
        gpu_write16(psx, addr, value);
        return;
    }
    if (addr >= IO_IRQ_START && addr <= IO_IRQ_END) {
        irq_write16(psx, addr, value);
        return;
    }
    if (addr >= IO_TIMERS_START && addr <= IO_TIMERS_END) {
        timers_write16(psx, addr, value);
        return;
    }
    printf("I/O write (16) to unimplemented address: 0x%08X, value: 0x%04X\n", addr, value);
}

void io_write32(PSX *psx, uint32_t addr, uint32_t value) {
    if (addr >= IO_GPU_START && addr <= IO_GPU_END) {
        gpu_write32(psx, addr, value);
        return;
    }
    if (addr >= IO_IRQ_START && addr <= IO_IRQ_END) {
        irq_write32(psx, addr, value);
        return;
    }
    if (addr >= IO_TIMERS_START && addr <= IO_TIMERS_END) {
        timers_write32(psx, addr, value);
        return;
    }
    printf("I/O write (32) to unimplemented address: 0x%08X, value: 0x%08X\n", addr, value);
}
