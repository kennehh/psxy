#ifndef GPU_H
#define GPU_H

#include <stdint.h>
#include <stdio.h>

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

static inline void gpu_write_gp0(Gpu *gpu, uint32_t value) {
    // printf("GPU GP0 write: 0x%08X\n", value);
}

static inline void gpu_write_gp1(Gpu *gpu, uint32_t value) {
    // printf("GPU GP1 write: 0x%08X\n", value);
}

static inline uint32_t gpu_read_gp0(Gpu *gpu) {
    // printf("GPU GP0 read\n");
    return 0;
}

static inline uint32_t gpu_read_gp1(Gpu *gpu) {
    // printf("GPU GP1 read\n");
    // return gpu->status;
    return 0x1c802000; // gpu ready
}



#endif // GPU_H
