#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include "common.h"

typedef struct Irq {
    uint16_t stat;   // Interrupt status
    uint16_t mask;   // Interrupt mask
} Irq;

void irq_reset(Irq *irq);

uint16_t irq_read_stat(PSX *psx);
uint16_t irq_read_mask(PSX *psx);

void irq_write_stat(PSX *psx, uint16_t value);
void irq_write_mask(PSX *psx, uint16_t value);

void irq_raise(PSX *psx, uint16_t value);

#endif
