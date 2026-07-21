#include <stdlib.h>
#include <string.h>
#include "gpu_internal.h"
#include "core/psx.h"

void gpu_update_drq(Gpu *gpu) {
    gpu->gpu_stat &= ~GPUSTAT_DRQ_BIT;
    switch (gpu->dma_direction) {
        case GPU_DMA_CPU_TO_GPU:
            if (gpu->gp0_count > 0) gpu->gpu_stat |= GPUSTAT_DRQ_BIT;
            break;
        case GPU_DMA_GPU_TO_CPU:
            if (gpu->vram_read.active && gpu->vram_read.words_left > 0) gpu->gpu_stat |= GPUSTAT_DRQ_BIT;
            break;
        case GPU_DMA_NONE:
        default:
            break;
    }
}

uint8_t gpu_read8(PSX *psx, uint32_t addr) {
    switch (addr) {
        CASE_REG32_READ8(0x1f801810, gp0_read(&psx->gpu))
        CASE_REG32_READ8(0x1f801814, gp1_read(&psx->gpu))
        default: printf("GPU read8 from unimplemented address: 0x%08X\n", addr); return 0;
    }
}

uint16_t gpu_read16(PSX *psx, uint32_t addr) {
    switch (addr) {
        CASE_REG32_READ16(0x1f801810, gp0_read(&psx->gpu))
        CASE_REG32_READ16(0x1f801814, gp1_read(&psx->gpu))
        default: printf("GPU read16 from unimplemented address: 0x%08X\n", addr); return 0;
    }
}

uint32_t gpu_read32(PSX *psx, uint32_t addr) {
    switch (addr) {
        CASE_REG32_READ32(0x1f801810, gp0_read(&psx->gpu))
        CASE_REG32_READ32(0x1f801814, gp1_read(&psx->gpu))
        default: printf("GPU read32 from unimplemented address: 0x%08X\n", addr); return 0;
    }
}

void gpu_write8(PSX *psx, uint32_t addr, uint8_t value) {
    switch (addr) {
        CASE_REG32_8(0x1f801810): gp0_write(&psx->gpu, reg32_write8(addr, 0, value)); break;
        CASE_REG32_8(0x1f801814): gp1_write(&psx->gpu, reg32_write8(addr, 0, value)); break;
        default: printf("GPU write8 to unimplemented address: 0x%08X, value: 0x%02X\n", addr, value); return;
    }
}

void gpu_write16(PSX *psx, uint32_t addr, uint16_t value) {
    switch (addr) {
        CASE_REG32_16(0x1f801810): gp0_write(&psx->gpu, reg32_write16(addr, 0, value)); break;
        CASE_REG32_16(0x1f801814): gp1_write(&psx->gpu, reg32_write16(addr, 0, value)); break;
        default: printf("GPU write16 to unimplemented address: 0x%08X, value: 0x%04X\n", addr, value); return;
    }
}

void gpu_write32(PSX *psx, uint32_t addr, uint32_t value) {
    switch (addr) {
        case 0x1f801810: gp0_write(&psx->gpu, value); break;
        case 0x1f801814: gp1_write(&psx->gpu, value); break;
        default: printf("GPU write32 to unimplemented address: 0x%08X, value: 0x%08X\n", addr, value); return;
    }
}

void gpu_reset(Gpu *gpu) {
    if (!gpu) return;
    memset(gpu->vram, 0, sizeof(gpu->vram));
    gp1_reset(gpu);
}

void gpu_vram_dump_ppm(Gpu *gpu, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", 1024, 512);
    for (uint32_t y = 0; y < 512; y++) {
        for (uint32_t x = 0; x < 1024; x++) {
            uint16_t p = gpu->vram[y * 1024 + x];
            uint8_t r5 = p & 0x1F;
            uint8_t g5 = (p >> 5) & 0x1F;
            uint8_t b5 = (p >> 10) & 0x1F;
            uint8_t rgb[3] = {(uint8_t)((r5 << 3) | (r5 >> 2)), (uint8_t)((g5 << 3) | (g5 >> 2)), (uint8_t)((b5 << 3) | (b5 >> 2))};
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
}
