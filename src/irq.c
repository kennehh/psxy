#include "irq.h"
#include "psx.h"
#include "exceptions.h"

#define CAUSE_IP_BIT 0x00008000 // Bit in the Cause register indicating an interrupt is pending

static inline void irq_update_cause(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint16_t pending = psx->irq.stat & psx->irq.mask;

    if (pending) {
        cpu->cop0.cause |= CAUSE_IP_BIT; // Set the interrupt pending bit
    } else {
        cpu->cop0.cause &= ~CAUSE_IP_BIT; // Clear the interrupt pending bit
    }
}

void irq_reset(Irq *irq) {
    irq->stat = 0;
    irq->mask = 0;
}

uint16_t irq_read_stat(PSX *psx) {
    return psx->irq.stat;
}

uint16_t irq_read_mask(PSX *psx) {
    return psx->irq.mask;
}

void irq_write_stat(PSX *psx, uint16_t value) {
    // Writing 0 clears the corresponding bits in the status register
    psx->irq.stat &= value;
    irq_update_cause(psx);
}

void irq_write_mask(PSX *psx, uint16_t value) {
    psx->irq.mask = value;
    irq_update_cause(psx);
}

void irq_raise(PSX *psx, uint16_t value) {
    psx->irq.stat |= value;
    irq_update_cause(psx);
}
