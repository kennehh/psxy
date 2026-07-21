#include "gpu_internal.h"

static inline uint16_t gp0_read_vram_pixel(Gpu *gpu) {
    GpuVramRead *read = &gpu->vram_read;
    uint32_t x = (read->x + read->cur_x) & 0x3FF;
    uint32_t y = (read->y + read->cur_y) & 0x1FF;
    uint16_t pixel = gpu->vram[y * 1024 + x];

    read->cur_x++;
    if (read->cur_x >= read->w) {
        read->cur_x = 0;
        read->cur_y++;
    }
    if (read->pixels_left > 0) read->pixels_left--;
    return pixel;
}

static inline uint32_t gp0_read_vram_data(Gpu *gpu) {
    GpuVramRead *read = &gpu->vram_read;
    uint16_t lo = 0;
    uint16_t hi = 0;

    if (read->pixels_left > 0) {
        lo = gp0_read_vram_pixel(gpu);
        if (read->pixels_left > 0) hi = gp0_read_vram_pixel(gpu);
    }
    read->words_left--;
    if (read->words_left == 0) {
        read->active = 0;
        gpu->gpu_stat &= ~GPUSTAT_READ_READY_BIT;
    }
    gpu->gpu_read = (uint32_t)(hi << 16) | lo;
    gpu_update_drq(gpu);
    return gpu->gpu_read;
}

uint32_t gp0_read(Gpu *gpu) {
    return gpu->vram_read.active ? gp0_read_vram_data(gpu) : gpu->gpu_read;
}

static inline void gp0_draw_mode(Gpu *gpu, uint32_t param) {
    gpu->render_attr.draw_mode_raw = param & 0xFFFFFF;
    gpu->gpu_stat = (gpu->gpu_stat & ~GPUSTAT_DRAW_MODE_MASK) | (param & GPUSTAT_DRAW_MODE_MASK);
}

static inline void gp0_start_vram_write(Gpu *gpu) {
    uint32_t coords = gpu->gp0_buffer[1];
    uint32_t size = gpu->gp0_buffer[2];
    GpuVramWrite *write = &gpu->vram_write;
    write->active = 1;
    write->x = (coords >> 16) & 0x3FF;
    write->y = coords & 0x1FF;
    write->cur_x = 0;
    write->cur_y = 0;
    write->w = (size >> 16) & 0x3FF;
    write->h = size & 0x1FF;
    write->words_left = (write->w * write->h + 1) / 2;
    write->pixels_left = write->w * write->h;
}

static inline void gp0_write_vram_pixel(Gpu *gpu, uint16_t pixel) {
    GpuVramWrite *write = &gpu->vram_write;
    if (!write->pixels_left) return;
    write->pixels_left--;
    uint32_t x = (write->x + write->cur_x) & 0x3FF;
    uint32_t y = (write->y + write->cur_y) & 0x1FF;
    gpu->vram[y * 1024 + x] = pixel;
    write->cur_x++;
    if (write->cur_x >= write->w) {
        write->cur_x = 0;
        write->cur_y++;
    }
    if (write->words_left > 0) write->words_left--;
}

static inline void gp0_write_vram_data(Gpu *gpu, uint32_t value) {
    GpuVramWrite *write = &gpu->vram_write;
    gp0_write_vram_pixel(gpu, value & 0xFFFF);
    if (write->pixels_left > 0) gp0_write_vram_pixel(gpu, value >> 16);
    if (write->words_left == 0) {
        write->active = 0;
        gpu->gpu_stat |= GPUSTAT_WRITE_FIFO_EMPTY_BIT;
    }
}

static inline void gp0_start_vram_read(Gpu *gpu) {
    uint32_t coords = gpu->gp0_buffer[1];
    uint32_t size = gpu->gp0_buffer[2];
    GpuVramRead *read = &gpu->vram_read;
    read->active = 1;
    read->x = (coords >> 16) & 0x3FF;
    read->y = coords & 0x1FF;
    read->cur_x = 0;
    read->cur_y = 0;
    read->w = (size >> 16) & 0x3FF;
    read->h = size & 0x1FF;
    read->pixels_left = read->w * read->h;
    read->words_left = (read->pixels_left + 1) / 2;
    gpu->gpu_stat |= GPUSTAT_READ_READY_BIT;
}

static inline void gp0_execute(Gpu *gpu) {
    uint32_t word0 = gpu->gp0_buffer[0];
    switch (word0 >> 24) {
        case 0x00: case 0x01: case 0x02: case 0x68:
            break;
        case 0xA0:
            printf("Starting VRAM write transfer\n");
            gp0_start_vram_write(gpu);
            break;
        case 0xC0:
            printf("Starting VRAM read transfer\n");
            gp0_start_vram_read(gpu);
            break;
        case 0xE1: gp0_draw_mode(gpu, word0); break;
        case 0xE2: gpu->render_attr.tex_window_raw = word0 & 0xFFFFFF; break;
        case 0xE3: gpu->render_attr.draw_tl_raw = word0 & 0xFFFFFF; break;
        case 0xE4: gpu->render_attr.draw_br_raw = word0 & 0xFFFFFF; break;
        case 0xE5: gpu->render_attr.draw_offset_raw = word0 & 0xFFFFFF; break;
        case 0xE6: gpu->render_attr.mask_setting_raw = word0 & 0xFFFFFF; break;
        default:
            printf("Unimplemented GP0 command: 0x%02X\n", (uint8_t)(word0 >> 24));
            break;
    }
}

static uint32_t gp0_word_count(uint8_t cmd) {
    switch (cmd) {
        case 0x02: case 0xA0: case 0xC0: return 3;
        default: return 1;
    }
}

void gp0_write(Gpu *gpu, uint32_t value) {
    if (gpu->vram_write.active) {
        gp0_write_vram_data(gpu, value);
        return;
    }
    if (gpu->gp0_count == 0) gpu->gp0_expected = gp0_word_count(value >> 24);
    gpu->gp0_buffer[gpu->gp0_count++] = value;
    if (gpu->gp0_count >= gpu->gp0_expected) {
        gp0_execute(gpu);
        gpu->gp0_count = 0;
    }
}

void gp0_clear_fifo(Gpu *gpu) {
    gpu->gp0_count = 0;
    gpu->gp0_expected = 0;
}
