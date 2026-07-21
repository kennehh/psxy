#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include "core/common.h"

typedef struct Timer {
    uint16_t counter;
    uint16_t mode;
    uint16_t target;
} Timer;

typedef struct Timers {
    Timer timer0;
    Timer timer1;
    Timer timer2;
} Timers;

void timers_reset(Timers *timers);

uint8_t timers_read8(PSX *psx, uint32_t addr);
uint16_t timers_read16(PSX *psx, uint32_t addr);
uint32_t timers_read32(PSX *psx, uint32_t addr);
void timers_write8(PSX *psx, uint32_t addr, uint8_t value);
void timers_write16(PSX *psx, uint32_t addr, uint16_t value);
void timers_write32(PSX *psx, uint32_t addr, uint32_t value);

#endif // TIMERS_H
