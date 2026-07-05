#include "irq.h"
#include "psx.h"
#include "cop0.h"

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
    cop0_update_interrupts(psx);
}

void irq_write_mask(PSX *psx, uint16_t value) {
    psx->irq.mask = value;
    cop0_update_interrupts(psx);
}

void irq_raise(PSX *psx, uint16_t value) {
    psx->irq.stat |= value;
    cop0_update_interrupts(psx);
}
