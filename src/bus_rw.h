#ifndef BUS_RW_H
#define BUS_RW_H

#ifdef SINGLE_STEP_TEST_MODE
#include "single_step_bus.h"
#else

#include <string.h>
#include <stdint.h>
#include "psx.h"
#include "gpu.h"
#include "irq.h"

static inline uint8_t io_read8(PSX *psx, uint32_t addr) {
    switch (addr) {
        case 0x1f801070: return irq_read_stat(psx);
        case 0x1f801074: return irq_read_mask(psx);
        case 0x1f801810: return gpu_read_gp0(&psx->gpu);
        case 0x1f801814: return gpu_read_gp1(&psx->gpu);
    }
    printf("I/O read (8) from unimplemented address: 0x%08X\n", addr);
    return 0; // Placeholder for I/O read implementation
}

static inline uint16_t io_read16(PSX *psx, uint32_t addr) {
    switch (addr) {
        case 0x1f801070: return irq_read_stat(psx);
        case 0x1f801074: return irq_read_mask(psx);
        case 0x1f801810: return gpu_read_gp0(&psx->gpu);
        case 0x1f801814: return gpu_read_gp1(&psx->gpu);
    }
    printf("I/O read (16) from unimplemented address: 0x%08X\n", addr);
    return 0; // Placeholder for I/O read implementation
}

static inline uint32_t io_read32(PSX *psx, uint32_t addr) {
    switch (addr) {
        case 0x1f801070: return irq_read_stat(psx);
        case 0x1f801074: return irq_read_mask(psx);
        case 0x1f801810: return gpu_read_gp0(&psx->gpu);
        case 0x1f801814: return gpu_read_gp1(&psx->gpu);
    }
    printf("I/O read (32) from unimplemented address: 0x%08X\n", addr);
    return 0; // Placeholder for I/O read implementation
}

static inline void io_write8(PSX *psx, uint32_t addr, uint8_t value) {
    printf("I/O write (8) to unimplemented address: 0x%08X, value: 0x%02X\n", addr, value);
}

static inline void io_write16(PSX *psx, uint32_t addr, uint16_t value) {
    switch (addr) {
        case 0x1f801070: irq_write_stat(psx, value); break;
        case 0x1f801074: irq_write_mask(psx, value); break;
    }
    printf("I/O write (16) to unimplemented address: 0x%08X, value: 0x%04X\n", addr, value);
}

static inline void io_write32(PSX *psx, uint32_t addr, uint32_t value) {
    switch (addr) {
        case 0x1f801070: irq_write_stat(psx, value); break;
        case 0x1f801074: irq_write_mask(psx, value); break;
        case 0x1f801810: gpu_write_gp0(&psx->gpu, value); break;
        case 0x1f801814: gpu_write_gp1(&psx->gpu, value); break;
        default:
            printf("I/O write (32) to unimplemented address: 0x%08X, value: 0x%08X\n", addr, value);
            break;
    }
}

static inline uint8_t bus_read8(PSX *psx, uint32_t addr) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = psx->bus.read_pages[page];

    if (page_ptr == NULL) {
        return io_read8(psx, addr);
    }

    uint32_t offset = addr & BUS_PAGE_MASK;
    return page_ptr[offset];
}

static inline uint16_t bus_read16(PSX *psx, uint32_t addr) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = psx->bus.read_pages[page];

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
    uint8_t *page_ptr = psx->bus.read_pages[page];

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
    uint8_t *page_ptr = psx->bus.write_pages[page];

    if (page_ptr == NULL) {
        io_write8(psx, addr, value);
        return;
    }
    uint32_t offset = addr & BUS_PAGE_MASK;
    page_ptr[offset] = value;
}

static inline void bus_write16(PSX *psx, uint32_t addr, uint16_t value) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = psx->bus.write_pages[page];

    if (page_ptr == NULL) {
        io_write16(psx, addr, value);
        return;
    }
    uint32_t offset = addr & BUS_PAGE_MASK;
    memcpy(page_ptr + offset, &value, sizeof(uint16_t));

}

static inline void bus_write32(PSX *psx, uint32_t addr, uint32_t value) {
    uint32_t page = get_page_index(addr);
    uint8_t *page_ptr = psx->bus.write_pages[page];

    if (page_ptr == NULL) {
        io_write32(psx, addr, value);
        return;
    }
    uint32_t offset = addr & BUS_PAGE_MASK;
    memcpy(page_ptr + offset, &value, sizeof(uint32_t));
}

#endif // SINGLE_STEP_TEST_MODE
#endif // BUS_RW_H
