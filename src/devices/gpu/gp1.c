#include "gpu_internal.h"

static inline void gp1_update_display_size(Gpu *gpu) {
    gpu->output_w = gpu->display.h_end - gpu->display.h_start;
    gpu->output_h = gpu->display.v_end - gpu->display.v_start;
}

uint32_t gp1_read(Gpu *gpu) { return gpu->gpu_stat; }

void gp1_reset(Gpu *gpu) { gpu->gpu_stat = 0x14802000; }

static inline void gp1_ack_irq(Gpu *gpu) { gpu->gpu_stat &= ~GPUSTAT_INTERRUPT_BIT; }

static inline void gp1_toggle_display(Gpu *gpu, uint32_t param) {
    gpu->display_disabled = param & 1;
    if (gpu->display_disabled) gpu->gpu_stat &= ~GPUSTAT_DISPLAY_DISABLED_BIT;
    else gpu->gpu_stat |= GPUSTAT_DISPLAY_DISABLED_BIT;
}

static inline void gp1_dma_direction(Gpu *gpu, uint32_t param) {
    uint8_t direction = param & 3;
    gpu->dma_direction = direction;
    gpu->gpu_stat = (gpu->gpu_stat & ~GPUSTAT_DMA_DIRECTION_MASK) | (direction << GPUSTAT_DMA_DIRECTION_SHIFT);
}

static inline void gp1_display_start(Gpu *gpu, uint32_t param) {
    gpu->display.vram_x = (param >> 16) & 0x3FF;
    gpu->display.vram_y = param & 0x1FF;
}

static inline void gp1_display_h(Gpu *gpu, uint32_t param) {
    gpu->display.h_start = param & 0xFFF;
    gpu->display.h_end = (param >> 12) & 0xFFF;
    gp1_update_display_size(gpu);
}

static inline void gp1_display_v(Gpu *gpu, uint32_t param) {
    gpu->display.v_start = param & 0x3FF;
    gpu->display.v_end = (param >> 10) & 0x3FF;
    gp1_update_display_size(gpu);
}

static inline void gp1_display_mode(Gpu *gpu, uint32_t param) {
    GpuDisplay *display = &gpu->display;
    display->h_res = param & 3;
    display->v_res = (param >> 2) & 3;
    display->h_res_368 = (param >> 4) & 1;
    display->video_mode = (param >> 5) & 3;
    display->color_depth = (param >> 7) & 1;
    display->interlaced = (param >> 8) & 1;
    display->reverse_flag = (param >> 9) & 1;
    gpu->gpu_stat = (gpu->gpu_stat & ~GPUSTAT_DISPLAY_MODE_MASK)
        | (display->reverse_flag << 14) | (display->h_res_368 << 16)
        | (display->h_res << 17) | (display->v_res << 19)
        | (display->video_mode << 20) | (display->color_depth << 21)
        | (display->interlaced << 22);
}

static inline void gp1_set_gpu_read(Gpu *gpu, uint32_t param) {
    GpuRenderRawAttributes *attr = &gpu->render_attr;
    switch (param & 0x0F) {
        case 0x02: gpu->gpu_read = attr->tex_window_raw; break;
        case 0x03: gpu->gpu_read = attr->draw_tl_raw; break;
        case 0x04: gpu->gpu_read = attr->draw_br_raw; break;
        case 0x05: gpu->gpu_read = attr->draw_offset_raw; break;
        case 0x07: gpu->gpu_read = 2; break;
        case 0x08: gpu->gpu_read = 0; break;
        default: break;
    }
}

void gp1_write(Gpu *gpu, uint32_t value) {
    uint8_t cmd = value >> 24;
    uint32_t param = value & 0x00FFFFFF;
    switch (cmd) {
        case 0x00: gp1_reset(gpu); break;
        case 0x01: gp0_clear_fifo(gpu); break;
        case 0x02: gp1_ack_irq(gpu); break;
        case 0x03: gp1_toggle_display(gpu, param); break;
        case 0x04: gp1_dma_direction(gpu, param); break;
        case 0x05: gp1_display_start(gpu, param); break;
        case 0x06: gp1_display_h(gpu, param); break;
        case 0x07: gp1_display_v(gpu, param); break;
        case 0x08: gp1_display_mode(gpu, param); break;
        case 0x09: gpu->vram_2MB = param & 1; break;
        case 0x10: gp1_set_gpu_read(gpu, param); break;
    }
}
