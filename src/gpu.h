#ifndef GPU_H
#define GPU_H

#include <stdint.h>
#include <stdio.h>
#include "common.h"

typedef struct Gpu {
    uint16_t *vram;

    uint32_t status;

    // GP0 has a 16-word FIFO buffer
    uint32_t gp0_buffer[16];
    uint32_t gp0_count;
    uint32_t gp0_expected;

    uint16_t display_x;
    uint16_t display_y;
    uint16_t display_w;
    uint16_t display_h;

    uint16_t draw_x_start;
    uint16_t draw_y_start;
    uint16_t draw_x_end;
    uint16_t draw_y_end;

    int16_t draw_x_offset;
    int16_t draw_y_offset;
} Gpu;

void gpu_init(Gpu *gpu);
void gpu_reset(Gpu *gpu);
void gpu_destroy(Gpu *gpu);

uint8_t gpu_read8(PSX *psx, uint32_t addr);
uint16_t gpu_read16(PSX *psx, uint32_t addr);
uint32_t gpu_read32(PSX *psx, uint32_t addr);
void gpu_write8(PSX *psx, uint32_t addr, uint8_t value);
void gpu_write16(PSX *psx, uint32_t addr, uint16_t value);
void gpu_write32(PSX *psx, uint32_t addr, uint32_t value);

#endif // GPU_H
