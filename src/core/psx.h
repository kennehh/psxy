#ifndef PSX_H
#define PSX_H

#include "core/common.h"
#include "memory/bus.h"
#include "cpu/cpu.h"
#include "support/tty.h"
#include "devices/gpu/gpu.h"
#include "devices/irq.h"
#include "cpu/cop0.h"
#include "devices/timers.h"
#include "memory/memctrl.h"

typedef struct PSX {
    Cpu cpu;
    Cop0 cop0;
    Bus bus;
    Gpu gpu;
    TTY tty;
    Irq irq;
    Timers timers;
    MemCtrl memctrl;
} PSX;

PSX *psx_create(void);
void psx_destroy(PSX *psx);
void psx_reset(PSX *psx);
void psx_run(PSX *psx, uint32_t cycles);

#endif // PSX_H
