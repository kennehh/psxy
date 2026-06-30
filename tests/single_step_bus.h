
#ifndef SINGLE_STEP_BUS_H
#define SINGLE_STEP_BUS_H

#include <stdio.h>
#include <stdint.h>
#include "common.h"

typedef struct {
    uint8_t actions;
    uint8_t size;
    uint32_t addr;
    uint32_t val;
} Cycle;

extern Cycle *cycles;
extern uint8_t cycle_count;
extern uint8_t cycle_index;

#define ACTION_READ 0x01
#define ACTION_WRITE 0x02
#define ACTION_FETCH 0x04

static inline uint32_t consume_read_cycle(uint8_t action, uint8_t size, uint32_t addr) {
    if (cycle_index >= cycle_count) {
        fprintf(stderr, "No more cycles to consume\n");
        return 0;
    }

    Cycle *current_cycle = &cycles[cycle_index];

    if (current_cycle->actions != action || current_cycle->size != size || current_cycle->addr != addr) {
        fprintf(stderr, "Cycle mismatch at index %d\n", cycle_index);
        return 0;
    }

    cycle_index++;
    return current_cycle->val;
}

static inline void consume_write_cycle(uint8_t action, uint8_t size, uint32_t addr, uint32_t val) {
    if (cycle_index >= cycle_count) {
        fprintf(stderr, "No more cycles to consume\n");
        return;
    }

    Cycle *current_cycle = &cycles[cycle_index];

    if (current_cycle->actions != action || current_cycle->size != size || current_cycle->addr != addr || current_cycle->val != val) {
        fprintf(stderr, "Cycle mismatch at index %d\n", cycle_index);
        return;
    }

    cycle_index++;
}

static inline uint8_t bus_read8(PSX *psx, uint32_t addr) {
    return consume_read_cycle(ACTION_READ, 1, addr);
}
static inline uint16_t bus_read16(PSX *psx, uint32_t addr) {
    return consume_read_cycle(ACTION_READ, 2, addr);
}
static inline uint32_t bus_read32(PSX *psx, uint32_t addr) {
    return consume_read_cycle(ACTION_READ, 4, addr);
}
static inline uint32_t bus_fetch32(PSX *psx, uint32_t addr) {
    return consume_read_cycle(ACTION_FETCH, 4, addr);
}
static inline void bus_write8(PSX *psx, uint32_t addr, uint8_t value) {
    consume_write_cycle(ACTION_WRITE, 1, addr, value);
}
static inline void bus_write16(PSX *psx, uint32_t addr, uint16_t value) {
    consume_write_cycle(ACTION_WRITE, 2, addr, value);
}
static inline void bus_write32(PSX *psx, uint32_t addr, uint32_t value) {
    consume_write_cycle(ACTION_WRITE, 4, addr, value);
}

#endif // SINGLE_STEP_BUS_H
