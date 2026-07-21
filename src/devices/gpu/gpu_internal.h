#ifndef GPU_INTERNAL_H
#define GPU_INTERNAL_H

#include "devices/gpu/gpu.h"

enum GpuStat {
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
    GPUSTAT_DISPLAY_MODE_MASK = GPUSTAT_REVERSE_BIT | GPUSTAT_H_RES_368_BIT | GPUSTAT_H_RES_BITS | GPUSTAT_V_RES_BIT | GPUSTAT_VIDEO_MODE_BIT | GPUSTAT_COLOR_DEPTH_BIT | GPUSTAT_INTERLACED_BIT | GPUSTAT_DISPLAY_DISABLED_BIT,
    GPUSTAT_DRQ_BIT = 1 << 25,
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

void gpu_update_drq(Gpu *gpu);
uint32_t gp0_read(Gpu *gpu);
void gp0_write(Gpu *gpu, uint32_t value);
void gp0_clear_fifo(Gpu *gpu);
uint32_t gp1_read(Gpu *gpu);
void gp1_write(Gpu *gpu, uint32_t value);
void gp1_reset(Gpu *gpu);

#endif // GPU_INTERNAL_H