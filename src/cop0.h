#ifndef COP0_H
#define COP0_H

#include <stdint.h>
#include "common.h"

#define EXC_NONE 0xFF // No exception
#define EXC_INT  0x00 // Interrupt
#define EXC_MOD  0x01 // TLB modification
#define EXC_TLBL 0x02 // TLB load/fetch
#define EXC_TLBS 0x03 // TLB store
#define EXC_ADEL 0x04 // Address error load/fetch
#define EXC_ADES 0x05 // Address error store
#define EXC_IBE  0x06 // Bus error instruction fetch
#define EXC_DBE  0x07 // Bus error data load/store
#define EXC_SYS  0x08 // Syscall
#define EXC_BP   0x09 // Breakpoint
#define EXC_RI   0x0A // Reserved instruction
#define EXC_CPU  0x0B // Coprocessor unusable
#define EXC_OV   0x0C // Arithmetic overflow
#define EXC_TR   0x0D // Trap

typedef struct Cop0 {
    uint32_t bpc; // Breakpoint Program Counter
    uint32_t bda; // Breakpoint Data Address
    uint32_t tar; // Target Address
    uint32_t dcic; // Debug and Cache Invalidate Control
    uint32_t badAddr; // Bad Address
    uint32_t bdam; // Breakpoint Data Address Mask
    uint32_t status; // Status Register
    uint32_t cause; // Cause of last exception
    uint32_t epc; // Exception Program Counter
    uint32_t prid; // Processor Revision ID

    uint8_t cache_isolated; // Cache isolation flag
} Cop0;

void cop0_reset(Cop0 *cop0);
void cop0_update_interrupts(PSX *psx);
void cop0_raise_exception(PSX *psx, uint8_t exc_code);
void cop0_write(Cop0 *cop0, uint8_t rd, uint32_t value);
uint32_t cop0_read(Cop0 *cop0, uint8_t rd);
void cop0_rfe(Cop0 *cop0);

static inline bool cop0_interrupts_pending(Cop0 *cop0) {
    uint32_t status = cop0->status;
    if (status & 0x1) { // Check if interrupts are enabled
        return false;
    }

    uint32_t pending = cop0->cause & status & 0xFF00; // Check for pending interrupts
    return pending;
}

#endif // COP0_H
