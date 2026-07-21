#ifndef GPU_H
#define GPU_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "core/common.h"

typedef struct GpuVramWrite {
    uint8_t active;
    uint16_t x, y;
    uint16_t w, h;
    uint16_t cur_x, cur_y;
    uint32_t pixels_left;
    uint32_t words_left;
} GpuVramWrite;

typedef struct GpuVramRead {
    uint8_t active;
    uint16_t x, y;
    uint16_t w, h;
    uint16_t cur_x, cur_y;
    uint32_t pixels_left;
    uint32_t words_left;
} GpuVramRead;

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

typedef struct GpuRenderRawAttributes {
    uint32_t draw_mode_raw;
    uint32_t tex_window_raw;
    uint32_t draw_tl_raw;
    uint32_t draw_br_raw;
    uint32_t draw_offset_raw;
    uint32_t mask_setting_raw;
} GpuRenderRawAttributes;


typedef struct Gpu {
    uint16_t vram[1024 * 512];

    uint32_t gpu_stat;
    uint32_t gpu_read;

    // GP0 has a 16-word FIFO buffer
    uint32_t gp0_buffer[16];
    uint32_t gp0_count;
    uint32_t gp0_expected;

    GpuVramRead vram_read;
    GpuVramWrite vram_write;

    GpuDisplay display;

    uint8_t display_disabled;
    uint8_t dma_direction;
    uint8_t vram_2MB;

    GpuRenderRawAttributes render_attr;

    uint16_t output_w;
    uint16_t output_h;
} Gpu;

void gpu_reset(Gpu *gpu);

uint8_t gpu_read8(PSX *psx, uint32_t addr);
uint16_t gpu_read16(PSX *psx, uint32_t addr);
uint32_t gpu_read32(PSX *psx, uint32_t addr);
void gpu_write8(PSX *psx, uint32_t addr, uint8_t value);
void gpu_write16(PSX *psx, uint32_t addr, uint16_t value);
void gpu_write32(PSX *psx, uint32_t addr, uint32_t value);

void gpu_vram_dump_ppm(Gpu *gpu, const char *filename);

#endif // GPU_H
