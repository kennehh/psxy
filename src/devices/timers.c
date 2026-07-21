#include <stdlib.h>
#include "core/common.h"
#include "devices/timers.h"
#include "core/psx.h"

#define TIMERS_REGS(X) \
    X(0x1F801100, timer0.counter) \
    X(0x1F801104, timer0.mode) \
    X(0x1F801108, timer0.target) \
    X(0x1F801110, timer1.counter) \
    X(0x1F801114, timer1.mode) \
    X(0x1F801118, timer1.target) \
    X(0x1F801120, timer2.counter) \
    X(0x1F801124, timer2.mode) \
    X(0x1F801128, timer2.target)

void timers_reset(Timers *timers) {
#define X(ADDR, FIELD) timers->FIELD = 0;
    TIMERS_REGS(X)
#undef X
}

uint8_t timers_read8(PSX *psx, uint32_t addr) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_READ8(ADDR, psx->timers.FIELD)
        TIMERS_REGS(X)
#undef X
        default:
            printf("Timers read8 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

uint16_t timers_read16(PSX *psx, uint32_t addr) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_READ16(ADDR, psx->timers.FIELD)
        TIMERS_REGS(X)
#undef X
        default:
            printf("Timers read16 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

uint32_t timers_read32(PSX *psx, uint32_t addr) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_READ32(ADDR, psx->timers.FIELD)
        TIMERS_REGS(X)
#undef X
        default:
            printf("Timers read32 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

void timers_write8(PSX *psx, uint32_t addr, uint8_t value) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_WRITE8(ADDR, psx->timers.FIELD)
        TIMERS_REGS(X)
#undef X
        default:
            printf("Timers write8 to unimplemented address: 0x%08X, value: 0x%02X\n", addr, value);
            return;
    }
}

void timers_write16(PSX *psx, uint32_t addr, uint16_t value) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_WRITE16(ADDR, psx->timers.FIELD)
        TIMERS_REGS(X)
#undef X
        default:
            printf("Timers write16 to unimplemented address: 0x%08X, value: 0x%04X\n", addr, value);
            return;
    }
}

void timers_write32(PSX *psx, uint32_t addr, uint32_t value) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_WRITE32(ADDR, psx->timers.FIELD)
        TIMERS_REGS(X)
#undef X
        default:
            printf("Timers write32 to unimplemented address: 0x%08X, value: 0x%08X\n", addr, value);
            return;
    }
}
