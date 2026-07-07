#include <stdlib.h>
#include <string.h>
#include "gpu.h"
#include "psx.h"

void gpu_init(Gpu *gpu) {
    gpu->vram = (uint16_t *)malloc(1024 * 512 * sizeof(uint16_t));
    gpu_reset(gpu);
}

void gpu_reset(Gpu *gpu) {
    if (!gpu) return;
    memset(gpu->vram, 0, 1024 * 512 * sizeof(uint16_t));
    gpu->status = 0x14802000; // Default status value
    gpu->gp0_count = 0;
    gpu->gp0_expected = 0;
    gpu->display_x = 0;
    gpu->display_y = 0;
    gpu->display_w = 320; // Default width
    gpu->display_h = 240; // Default height
    gpu->draw_x_start = 0;
    gpu->draw_y_start = 0;
    gpu->draw_x_end = 319; // Default end x
    gpu->draw_y_end = 239; // Default end y
    gpu->draw_x_offset = 0;
    gpu->draw_y_offset = 0;
}

void gpu_destroy(Gpu *gpu) {
    if (gpu->vram) {
        free(gpu->vram);
        gpu->vram = NULL;
    }
}

static inline uint32_t gpu_gp0_read32(Gpu *gpu) {
    // Implement GP0 read logic here
    return 0; // Placeholder
}

static inline uint32_t gpu_gp1_read32(Gpu *gpu) {
    // Implement GP1 read logic here
    // return gpu->status; // Return status for GP1 reads
    return 0x1c802000;
}

static inline void gpu_gp0_write32(Gpu *gpu, uint32_t value) {
    // Implement GP0 write logic here
}

static inline void gpu_gp1_write32(Gpu *gpu, uint32_t value) {
    // Implement GP1 write logic here
    gpu->status = value; // Example: Update status on GP1 write
}

uint8_t gpu_read8(PSX *psx, uint32_t addr) {
    switch (addr) {
        CASE_REG32_READ8(0x1f801810, gpu_gp0_read32(&psx->gpu))
        CASE_REG32_READ8(0x1f801814, gpu_gp1_read32(&psx->gpu))
        default:
            printf("GPU read8 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

uint16_t gpu_read16(PSX *psx, uint32_t addr) {
    switch (addr) {
        CASE_REG32_READ16(0x1f801810, gpu_gp0_read32(&psx->gpu))
        CASE_REG32_READ16(0x1f801814, gpu_gp1_read32(&psx->gpu))
        default:
            printf("GPU read16 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

uint32_t gpu_read32(PSX *psx, uint32_t addr) {
    switch (addr) {
        CASE_REG32_READ32(0x1f801810, gpu_gp0_read32(&psx->gpu))
        CASE_REG32_READ32(0x1f801814, gpu_gp1_read32(&psx->gpu))
        default:
            printf("GPU read32 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

void gpu_write8(PSX *psx, uint32_t addr, uint8_t value) {
    switch (addr) {
        CASE_REG32_8(0x1f801810):
            value = reg32_write8(addr, 0, value);
            gpu_gp0_write32(&psx->gpu, value);
            break;
        CASE_REG32_8(0x1f801814):
            value = reg32_write8(addr, 0, value);
            gpu_gp1_write32(&psx->gpu, value);
            break;
        default:
            printf("GPU write8 to unimplemented address: 0x%08X, value: 0x%02X\n", addr, value);
            return;
    }
}

void gpu_write16(PSX *psx, uint32_t addr, uint16_t value) {
    switch (addr) {
        CASE_REG32_16(0x1f801810):
            value = reg32_write16(addr, 0, value);
            gpu_gp0_write32(&psx->gpu, value);
            break;
        CASE_REG32_16(0x1f801814):
            value = reg32_write16(addr, 0, value);
            gpu_gp1_write32(&psx->gpu, value);
            break;
        default:
            printf("GPU write16 to unimplemented address: 0x%08X, value: 0x%04X\n", addr, value);
            return;
    }
}

void gpu_write32(PSX *psx, uint32_t addr, uint32_t value) {
    switch (addr) {
        case 0x1f801810:
            gpu_gp0_write32(&psx->gpu, value);
            break;
        case 0x1f801814:
            gpu_gp1_write32(&psx->gpu, value);
            break;
        default:
            printf("GPU write32 to unimplemented address: 0x%08X, value: 0x%08X\n", addr, value);
            return;
    }
}
