#include <stdlib.h>
#include <string.h>
#include "gpu.h"
#include "psx.h"

#define GPU_STATUS_REVERSE_BIT (1 << 14)
#define GPU_STATUS_H_RES_368_BIT (1 << 16)
#define GPU_STATUS_H_RES_BIT (3 << 17)
#define GPU_STATUS_V_RES_BIT (1 << 19)
#define GPU_STATUS_VIDEO_MODE_BIT (3 << 20)
#define GPU_STATUS_COLOR_DEPTH_BIT (1 << 21)
#define GPU_STATUS_INTERLACED_BIT (1 << 22)
#define GPU_STATUS_DISPLAY_DISABLED_BIT (1 << 23)
#define GPU_STATUS_INTERRUPT_BIT (1 << 24)

#define GPU_STATUS_DMA_DIRECTION_SHIFT 29
#define GPU_STATUS_DMA_DIRECTION_MASK (3 << GPU_STATUS_DMA_DIRECTION_SHIFT)

#define GPU_STATUS_DISPLAY_MODE_MASK (GPU_STATUS_REVERSE_BIT | GPU_STATUS_H_RES_368_BIT | GPU_STATUS_H_RES_BIT | GPU_STATUS_V_RES_BIT | GPU_STATUS_VIDEO_MODE_BIT | GPU_STATUS_COLOR_DEPTH_BIT | GPU_STATUS_INTERLACED_BIT | GPU_STATUS_DISPLAY_DISABLED_BIT)

static inline void gpu_update_display_size(Gpu *gpu) {
    gpu->output_w = gpu->display.h_end - gpu->display.h_start;
    gpu->output_h = gpu->display.v_end - gpu->display.v_start;
}

static inline uint32_t gpu_gp0_read32(Gpu *gpu) {
    return gpu->gpu_read;
}

static inline uint32_t gpu_gp1_read32(Gpu *gpu) {
    // return gpu->gpu_stat;
    return 0x1c802000; // Default status for now
}

static inline void gp0_write(Gpu *gpu, uint32_t value) {

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
    gpu->gpu_stat &= ~GPU_STATUS_INTERRUPT_BIT;
}

static inline void gp1_toggle_display(Gpu *gpu, uint32_t param) {
    bool disable = param & 0x01;
    gpu->display_disabled = disable;

    if (disable) {
        gpu->gpu_stat &= ~GPU_STATUS_DISPLAY_DISABLED_BIT;
    } else {
        gpu->gpu_stat |= GPU_STATUS_DISPLAY_DISABLED_BIT;
    }
}

static inline void gp1_dma_direction(Gpu *gpu, uint32_t param) {
    uint8_t direction = param & 0x03;
    gpu->dma_direction = direction;
    gpu->gpu_stat = (gpu->gpu_stat & ~GPU_STATUS_DMA_DIRECTION_MASK) | (direction << GPU_STATUS_DMA_DIRECTION_SHIFT);
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
    uint32_t status = gpu->gpu_stat & ~GPU_STATUS_DISPLAY_MODE_MASK;
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

    GpuRenderAttributes *attr = &gpu->render_attr;
    switch (param & 0x0F) {
        case 0x02: gpu->gpu_read = attr->texture_window; break;
        case 0x03: gpu->gpu_read = attr->drawing_area_top_left; break;
        case 0x04: gpu->gpu_read = attr->drawing_area_bottom_right; break;
        case 0x05: gpu->gpu_read = attr->drawing_offset; break;
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
