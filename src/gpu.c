#include <stdlib.h>
#include <string.h>
#include "gpu.h"

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
