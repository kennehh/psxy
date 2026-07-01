#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include <string.h>
#include "common.h"

typedef struct Bus {
    uint8_t* read_pages[BUS_PAGE_COUNT];
    uint8_t* write_pages[BUS_PAGE_COUNT];

    uint8_t* ram;
    uint8_t* exp1;
    uint8_t* scratchpad;
    uint8_t* bios;
} Bus;

void bus_init(Bus *bus);
void bus_destroy(Bus *bus);
void bus_reset(Bus *bus);

#ifdef SINGLE_STEP_TEST_MODE
#include "single_step_bus.h"
#endif // SINGLE_STEP_TEST_MODE

#endif // BUS_H
