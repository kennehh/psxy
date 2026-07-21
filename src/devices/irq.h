#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include "core/common.h"

typedef struct Irq {
    uint16_t stat;   // Interrupt status
    uint16_t mask;   // Interrupt mask
} Irq;

void irq_reset(Irq *irq);

uint8_t irq_read8(PSX *psx, uint32_t addr);
uint16_t irq_read16(PSX *psx, uint32_t addr);
uint32_t irq_read32(PSX *psx, uint32_t addr);
void irq_write8(PSX *psx, uint32_t addr, uint8_t value);
void irq_write16(PSX *psx, uint32_t addr, uint16_t value);
void irq_write32(PSX *psx, uint32_t addr, uint32_t value);

void irq_raise(PSX *psx, uint16_t value);

#endif
