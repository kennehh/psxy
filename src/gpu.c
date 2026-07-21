#include <stdlib.h>
#include <string.h>
#include "gpu.h"
#include "psx.h"

enum GpuStat {
    GPUSTAT_TEX_PAGE_X_MASK = 3 << 0,
    GPUSTAT_TEX_PAGE_Y_BIT = 1 << 4,
    GPUSTAT_SEMI_TRANSPARENT_BITS = 2 << 5,
    GPUSTAT_TEX_PAGE_COLOR_BITS = 2 << 7,
    GPUSTAT_DITHERING_BIT = 1 << 9,
    GPUSTAT_INTERLACE_BIT = 1 << 10,
    GPUSTAT_TEX_PAGE_Y_MODE_BIT = 1 << 11,
    GPUSTAT_DRAW_MODE_MASK = 0xF << 12,

    GPUSTAT_REVERSE_BIT = 1 << 14,
    GPUSTAT_H_RES_368_BIT = 1 << 16,
    GPUSTAT_H_RES_BITS = 3 << 17,
    GPUSTAT_V_RES_BIT = 1 << 19,
    GPUSTAT_VIDEO_MODE_BIT = 3 << 20,
    GPUSTAT_COLOR_DEPTH_BIT = 1 << 21,
    GPUSTAT_INTERLACED_BIT = 1 << 22,
    GPUSTAT_DISPLAY_DISABLED_BIT = 1 << 23,
    GPUSTAT_INTERRUPT_BIT = 1 << 24,
    GPUSTAT_DISPLAY_MODE_MASK = (GPUSTAT_REVERSE_BIT | GPUSTAT_H_RES_368_BIT | GPUSTAT_H_RES_BITS | GPUSTAT_V_RES_BIT | GPUSTAT_VIDEO_MODE_BIT | GPUSTAT_COLOR_DEPTH_BIT | GPUSTAT_INTERLACED_BIT | GPUSTAT_DISPLAY_DISABLED_BIT),

    GPUSTAT_DRQ_BIT = 1 << 25,
    GPUSTAT_CMD_READY_BIT = 1 << 26,
    GPUSTAT_READ_READY_BIT = 1 << 27,
    GPUSTAT_WRITE_FIFO_EMPTY_BIT = 1 << 28,

    GPUSTAT_DMA_DIRECTION_SHIFT = 29,
    GPUSTAT_DMA_DIRECTION_MASK = 3 << GPUSTAT_DMA_DIRECTION_SHIFT,
};

enum GpuDmaDirection {
    GPU_DMA_NONE = 0,
    GPU_DMA_CPU_TO_GPU = 1,
    GPU_DMA_GPU_TO_CPU = 2,
};

static inline void gpu_update_display_size(Gpu *gpu) {
    gpu->output_w = gpu->display.h_end - gpu->display.h_start;
    gpu->output_h = gpu->display.v_end - gpu->display.v_start;
}

static inline uint16_t gp0_read_vram_pixel(Gpu *gpu) {
    GpuVramRead *read = &gpu->vram_read;
    uint32_t x = (read->x + read->cur_x) & 0x3FF; // Wrap around at 1024
    uint32_t y = (read->y + read->cur_y) & 0x1FF; // Wrap around at 512
    uint16_t pixel = gpu->vram[y * 1024 + x];

    // Update current position
    read->cur_x++;
    if (read->cur_x >= read->w) {
        read->cur_x = 0;
        read->cur_y++;
    }
    if (read->pixels_left > 0) {
        read->pixels_left--;
    }
    return pixel;
}

static inline void gpu_update_drq(Gpu *gpu) {
    gpu->gpu_stat &= ~GPUSTAT_DRQ_BIT; // Clear DRQ bit

    switch (gpu->dma_direction) {
        case GPU_DMA_NONE:
            break;
        case GPU_DMA_CPU_TO_GPU:
            if (gpu->gp0_count > 0) {
                gpu->gpu_stat |= GPUSTAT_DRQ_BIT; // Set DRQ bit
            }
            break;
        case GPU_DMA_GPU_TO_CPU:
            if (gpu->vram_read.active && gpu->vram_read.words_left > 0) {
                gpu->gpu_stat |= GPUSTAT_DRQ_BIT; // Set DRQ bit
            }
            break;
        default:
            break;
    }
}

static inline uint32_t gp0_read_vram_data(Gpu *gpu) {
    GpuVramRead *read = &gpu->vram_read;
    uint16_t lo = 0;
    uint16_t hi = 0;

    if (read->pixels_left > 0) {
        // read vram pixel
        lo = gp0_read_vram_pixel(gpu);
        if (read->pixels_left > 0) {
            hi = gp0_read_vram_pixel(gpu);
        }
    }

    read->words_left--;
    if (read->words_left == 0) {
        read->active = 0;
        gpu->gpu_stat &= ~GPUSTAT_READ_READY_BIT;
    }

    gpu->gpu_read = (uint32_t)(hi << 16) | (uint32_t)lo;
    gpu_update_drq(gpu);

    return gpu->gpu_read;
}


static inline uint32_t gpu_gp0_read32(Gpu *gpu) {
    if (gpu->vram_read.active) {
        return gp0_read_vram_data(gpu);
    }
    return gpu->gpu_read;
}

static inline uint32_t gpu_gp1_read32(Gpu *gpu) {
    return gpu->gpu_stat;
}

static inline void gp0_draw_mode(Gpu *gpu, uint32_t param) {
    gpu->render_attr.draw_mode_raw = param & 0xFFFFFF;
    gpu->gpu_stat = (gpu->gpu_stat & ~GPUSTAT_DRAW_MODE_MASK) | (param & GPUSTAT_DRAW_MODE_MASK);
}

static inline void gp0_start_vram_write(Gpu *gpu) {
    uint32_t coords = gpu->gp0_buffer[1];
    uint16_t x = (coords >> 16) & 0x3FF;
    uint16_t y = coords & 0x1FF;

    uint32_t size = gpu->gp0_buffer[2];
    uint16_t w = (size >> 16) & 0x3FF;
    uint16_t h = size & 0x1FF;

    GpuVramWrite *write = &gpu->vram_write;
    write->active = 1;
    write->x = x;
    write->y = y;
    write->cur_x = 0;
    write->cur_y = 0;
    write->w = w;
    write->h = h;
    write->words_left = (w * h + 1) / 2; // Each word contains two pixels
    write->pixels_left = w * h;
}

static inline void gp0_write_vram_pixel(Gpu *gpu, uint16_t pixel) {
    GpuVramWrite *write = &gpu->vram_write;
    if (!write->pixels_left) {
        return;
    }
    write->pixels_left--;

    uint32_t x = (write->x + write->cur_x) & 0x3FF; // Wrap around at 1024
    uint32_t y = (write->y + write->cur_y) & 0x1FF; // Wrap around at 512
    gpu->vram[y * 1024 + x] = pixel;

    // Update current position
    write->cur_x++;
    if (write->cur_x >= write->w) {
        write->cur_x = 0;
        write->cur_y++;
    }
    if (write->words_left > 0) {
        write->words_left--;
    }
}

static inline void gp0_write_vram_data(Gpu *gpu, uint32_t value) {
    GpuVramWrite *write = &gpu->vram_write;
    uint16_t lo = value & 0xFFFF;
    uint16_t hi = (value >> 16) & 0xFFFF;

    gp0_write_vram_pixel(gpu, lo);
    if (write->pixels_left > 0) {
        gp0_write_vram_pixel(gpu, hi);
    }

    if (write->words_left == 0) {
        write->active = 0;
        gpu->gpu_stat |= GPUSTAT_WRITE_FIFO_EMPTY_BIT;
    }
}

static inline void gp0_start_vram_read(Gpu *gpu) {
    uint32_t coords = gpu->gp0_buffer[1];
    uint16_t x = (coords >> 16) & 0x3FF;
    uint16_t y = coords & 0x1FF;

    uint32_t size = gpu->gp0_buffer[2];
    uint16_t w = (size >> 16) & 0x3FF;
    uint16_t h = size & 0x1FF;

    GpuVramRead *read = &gpu->vram_read;
    read->active = 1;
    read->x = x;
    read->y = y;
    read->cur_x = 0;
    read->cur_y = 0;
    read->w = w;
    read->h = h;
    read->pixels_left = w * h;
    read->words_left = (read->pixels_left + 1) / 2;

    gpu->gpu_stat |= GPUSTAT_READ_READY_BIT;
}

static inline void gp0_execute(Gpu *gpu) {
    uint32_t word0 = gpu->gp0_buffer[0];
    uint8_t cmd = (uint8_t)(word0 >> 24);

    switch (cmd) {
        case 0x00: // NOP
            break;
        case 0x01: // Clear cache
            break;
        case 0x02: // Fill rectangle
            break;
        case 0xA0: // CPU to VRAM transfer
            printf("Starting VRAM write transfer\n");
            gp0_start_vram_write(gpu);
            break;
        case 0xC0: // VRAM to CPU transfer
            printf("Starting VRAM read transfer\n");
            gp0_start_vram_read(gpu);
            break;
        case 0x68:
            break;
        case 0xE1: // Draw Mode setting
            gp0_draw_mode(gpu, word0);
            break;
        case 0xE2: // Texture Window setting
            gpu->render_attr.tex_window_raw = word0 & 0xFFFFFF;
            break;
        case 0xE3:
            gpu->render_attr.draw_tl_raw = word0 & 0xFFFFFF;
            break;
        case 0xE4:
            gpu->render_attr.draw_br_raw = word0 & 0xFFFFFF;
            break;
        case 0xE5:
            gpu->render_attr.draw_offset_raw = word0 & 0xFFFFFF;
            break;
        case 0xE6:
            gpu->render_attr.mask_setting_raw = word0 & 0xFFFFFF;
            break;
        default:
            printf("Unimplemented GP0 command: 0x%02X\n", cmd);
            break;
    }
}

static uint32_t gp0_word_count(uint8_t cmd) {
    switch (cmd) {
        case 0x02: return 3; // Fill rectangle
        case 0xA0: return 3; // CPU to VRAM transfer
        case 0xC0: return 3; // VRAM to CPU transfer

        default: return 1; // 1 word for other commands
    }
}

static inline void gp0_write(Gpu *gpu, uint32_t value) {
    if (gpu->vram_write.active) {
        gp0_write_vram_data(gpu, value);
        return;
    }

    if (gpu->gp0_count == 0) {
        // First word of a new command
        uint8_t cmd = (value >> 24) & 0xFF;
        gpu->gp0_expected = gp0_word_count(cmd);
    }

    gpu->gp0_buffer[gpu->gp0_count++] = value;

    if (gpu->gp0_count >= gpu->gp0_expected) {
        // Execute the command
        gp0_execute(gpu);
        gpu->gp0_count = 0; // Reset for the next command
    }
}

static inline void gp1_clear_fifo(Gpu *gpu) {
    gpu->gp0_count = 0;
    gpu->gp0_expected = 0;
}

static inline void gp1_stat_reset(Gpu *gpu) {
    gpu->gpu_stat = 0x14802000; // Reset to default status
}

static inline void gp1_ack_irq(Gpu *gpu) {
    // Clear interrupt flag
    gpu->gpu_stat &= ~GPUSTAT_INTERRUPT_BIT;
}

static inline void gp1_toggle_display(Gpu *gpu, uint32_t param) {
    bool disable = param & 0x01;
    gpu->display_disabled = disable;

    if (disable) {
        gpu->gpu_stat &= ~GPUSTAT_DISPLAY_DISABLED_BIT;
    } else {
        gpu->gpu_stat |= GPUSTAT_DISPLAY_DISABLED_BIT;
    }
}

static inline void gp1_dma_direction(Gpu *gpu, uint32_t param) {
    uint8_t direction = param & 0x03;
    gpu->dma_direction = direction;
    gpu->gpu_stat = (gpu->gpu_stat & ~GPUSTAT_DMA_DIRECTION_MASK) | (direction << GPUSTAT_DMA_DIRECTION_SHIFT);
}

static inline void gp1_display_start(Gpu *gpu, uint32_t param) {
    GpuDisplay *display = &gpu->display;
    display->vram_x = (param >> 16) & 0x3FF; // Bits 16-25 for X
    display->vram_y = param & 0x1FF;         // Bits 0-8 for Y
}

static inline void gp1_display_h(Gpu *gpu, uint32_t param) {
    GpuDisplay *display = &gpu->display;
    display->h_start = (uint16_t)(param & 0xFFF); // Bits 0-11 for H start
    display->h_end = (uint16_t)((param >> 12) & 0xFFF); // Bits 12-23 for H end
    gpu_update_display_size(gpu);
}

static inline void gp1_display_v(Gpu *gpu, uint32_t param) {
    GpuDisplay *display = &gpu->display;
    display->v_start = (uint16_t)(param & 0x3FF); // Bits 0-9 for V start
    display->v_end = (uint16_t)((param >> 10) & 0x3FF); // Bits 10-19 for V end
    gpu_update_display_size(gpu);
}

static inline void gp1_display_mode(Gpu *gpu, uint32_t param) {
    GpuDisplay *display = &gpu->display;
    display->h_res = (param >> 0) & 0x03;        // Bits 0-1 for horizontal resolution
    display->v_res = (param >> 2) & 0x03;        // Bits 2-3 for vertical resolution
    display->h_res_368 = (param >> 4) & 0x01;    // Bit 4 for 368-pixel mode
    display->video_mode = (param >> 5) & 0x03;   // Bits 5-6 for video mode
    display->color_depth = (param >> 7) & 0x01;  // Bit 7 for color depth
    display->interlaced = (param >> 8) & 0x01;   // Bit 8 for interlaced mode
    display->reverse_flag = (param >> 9) & 0x01; // Bit 9 for reverse flag

    // update status register which sets 14, 16-22 bits
    uint32_t status = gpu->gpu_stat & ~GPUSTAT_DISPLAY_MODE_MASK;
    gpu->gpu_stat = status
        | (display->reverse_flag << 14)
        | (display->h_res_368 << 16)
        | (display->h_res << 17)
        | (display->v_res << 19)
        | (display->video_mode << 20)
        | (display->color_depth << 21)
        | (display->interlaced << 22);
}

static inline void gp1_set_vram_size(Gpu *gpu, uint32_t param) {
    // Bit 0 indicates VRAM size: 0 = 1MB, 1 = 2MB (upper MB is an open bus)
    // Only relevant for newer PS1 models
    gpu->vram_2MB = param & 0x01;
}

static inline void gp1_set_gpu_read(Gpu *gpu, uint32_t param) {
    // TODO: handle old GPU behaviour

    GpuRenderRawAttributes *attr = &gpu->render_attr;
    switch (param & 0x0F) {
        case 0x02: gpu->gpu_read = attr->tex_window_raw; break;
        case 0x03: gpu->gpu_read = attr->draw_tl_raw; break;
        case 0x04: gpu->gpu_read = attr->draw_br_raw; break;
        case 0x05: gpu->gpu_read = attr->draw_offset_raw; break;
        case 0x06: /* gpu_read unchanged */ break;
        case 0x07: gpu->gpu_read = 2; break; // GPU version
        case 0x08: gpu->gpu_read = 0; break; // unknown. returns 0 on newer PS1 models
        default: /* gpu_read unchanged */ break;
    }
}

static inline void gp1_write(Gpu *gpu, uint32_t value) {
    uint8_t cmd = (value >> 24) & 0xFF;
    uint32_t param = value & 0x00FFFFFF;

    switch (cmd) {
        case 0x00: gp1_stat_reset(gpu); break;
        case 0x01: gp1_clear_fifo(gpu); break;
        case 0x02: gp1_ack_irq(gpu); break;
        case 0x03: gp1_toggle_display(gpu, param); break;
        case 0x04: gp1_dma_direction(gpu, param); break;
        case 0x05: gp1_display_start(gpu, param); break;
        case 0x06: gp1_display_h(gpu, param); break;
        case 0x07: gp1_display_v(gpu, param); break;
        case 0x08: gp1_display_mode(gpu, param); break;
        case 0x09: gp1_set_vram_size(gpu, param); break;
        case 0x10: gp1_set_gpu_read(gpu, param); break;
    }
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
            gp0_write(&psx->gpu, value);
            break;
        CASE_REG32_8(0x1f801814):
            value = reg32_write8(addr, 0, value);
            gp1_write(&psx->gpu, value);
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
            gp0_write(&psx->gpu, value);
            break;
        CASE_REG32_16(0x1f801814):
            value = reg32_write16(addr, 0, value);
            gp1_write(&psx->gpu, value);
            break;
        default:
            printf("GPU write16 to unimplemented address: 0x%08X, value: 0x%04X\n", addr, value);
            return;
    }
}

void gpu_write32(PSX *psx, uint32_t addr, uint32_t value) {
    switch (addr) {
        case 0x1f801810:
            gp0_write(&psx->gpu, value);
            break;
        case 0x1f801814:
            gp1_write(&psx->gpu, value);
            break;
        default:
            printf("GPU write32 to unimplemented address: 0x%08X, value: 0x%08X\n", addr, value);
            return;
    }
}

void gpu_reset(Gpu *gpu) {
    if (!gpu) return;
    memset(gpu->vram, 0, 1024 * 512 * sizeof(uint16_t));
    gp1_stat_reset(gpu);
}

void gpu_vram_dump_ppm(Gpu *gpu, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;

    fprintf(f, "P6\n%d %d\n255\n", 1024, 512);

    for (uint32_t y = 0; y < 512; y++) {
        for (uint32_t x = 0; x < 1024; x++) {
            uint16_t p = gpu->vram[y * 1024 + x];

            uint8_t r5 = (p >> 0) & 0x1F;
            uint8_t g5 = (p >> 5) & 0x1F;
            uint8_t b5 = (p >> 10) & 0x1F;

            uint8_t rgb[3] = {
                (uint8_t)((r5 << 3) | (r5 >> 2)),
                (uint8_t)((g5 << 3) | (g5 >> 2)),
                (uint8_t)((b5 << 3) | (b5 >> 2)),
            };

            fwrite(rgb, 1, 3, f);
        }
    }

    fclose(f);
}
