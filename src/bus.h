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
    uint8_t scratchpad[SCRATCHPAD_SIZE];
    uint8_t* bios;
} Bus;

void bus_init(Bus *bus);
void bus_destroy(Bus *bus);
void bus_reset(Bus *bus);

#endif // BUS_H
