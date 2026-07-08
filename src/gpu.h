#ifndef GPU_H
#define GPU_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "common.h"

typedef struct GpuDisplay {
    uint16_t vram_x, vram_y;
    uint16_t h_start, h_end;
    uint16_t v_start, v_end;

    uint8_t disabled;

    uint8_t h_res, v_res;
    uint8_t h_res_368;
    uint8_t video_mode;
    uint8_t color_depth;
    uint8_t interlaced;
    uint8_t reverse_flag;
} GpuDisplay;

typedef struct GpuRenderAttributes {
    uint32_t draw_mode;
    uint32_t texture_window;
    uint32_t drawing_area_top_left;
    uint32_t drawing_area_bottom_right;
    uint32_t drawing_offset;
    uint32_t mask_bit_setting;
} GpuRenderAttributes;

typedef struct Gpu {
    uint16_t *vram;

    uint32_t gpu_stat;
    uint32_t gpu_read;

    // GP0 has a 16-word FIFO buffer
    uint32_t gp0_buffer[16];
    uint32_t gp0_count;
    uint32_t gp0_expected;

    GpuDisplay display;

    uint8_t display_disabled;
    uint8_t dma_direction;
    uint8_t vram_2MB;

    GpuRenderAttributes render_attr;

    uint16_t output_w;
    uint16_t output_h;
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
