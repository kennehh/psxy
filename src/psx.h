#ifndef PSX_H
#define PSX_H

#include "common.h"
#include "bus.h"
#include "cpu.h"
#include "tty.h"

typedef struct PSX {
    Cpu cpu;
    Bus bus;
    TTY tty;
} PSX;

PSX *psx_create(void);
void psx_destroy(PSX *psx);
void psx_reset(PSX *psx);
void psx_run(PSX *psx, uint32_t cycles);

#endif // PSX_H
