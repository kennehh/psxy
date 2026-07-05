#include "irq.h"
#include "psx.h"
#include "cop0.h"

void irq_reset(Irq *irq) {
    irq->stat = 0;
    irq->mask = 0;
}

uint8_t irq_read8(PSX *psx, uint32_t addr) {
    switch (addr) {
        CASE_REG16_READ8(0x1f801070, psx->irq.stat)
        CASE_REG16_READ8(0x1f801074, psx->irq.mask)
        default:
            printf("IRQ read8 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

uint16_t irq_read16(PSX *psx, uint32_t addr) {
    switch (addr) {
        CASE_REG16_READ16(0x1f801070, psx->irq.stat)
        CASE_REG16_READ16(0x1f801074, psx->irq.mask)
        default:
            printf("IRQ read16 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

uint32_t irq_read32(PSX *psx, uint32_t addr) {
    switch (addr) {
        CASE_REG16_READ32(0x1f801070, psx->irq.stat)
        CASE_REG16_READ32(0x1f801074, psx->irq.mask)
        default:
            printf("IRQ read32 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

void irq_write8(PSX *psx, uint32_t addr, uint8_t value) {
    switch (addr) {
        CASE_REG16_WRITE8(0x1f801070, psx->irq.stat)
        CASE_REG16_WRITE8(0x1f801074, psx->irq.mask)
        default:
            printf("IRQ write8 to unimplemented address: 0x%08X, value: 0x%02X\n", addr, value);
            return;
    }
}

void irq_write16(PSX *psx, uint32_t addr, uint16_t value) {
    switch (addr) {
        CASE_REG16_WRITE16(0x1f801070, psx->irq.stat)
        CASE_REG16_WRITE16(0x1f801074, psx->irq.mask)
        default:
            printf("IRQ write16 to unimplemented address: 0x%08X, value: 0x%04X\n", addr, value);
            return;
    }
}

void irq_write32(PSX *psx, uint32_t addr, uint32_t value) {
    switch (addr) {
        CASE_REG16_WRITE32(0x1f801070, psx->irq.stat)
        CASE_REG16_WRITE32(0x1f801074, psx->irq.mask)
        default:
            printf("IRQ write32 to unimplemented address: 0x%08X, value: 0x%08X\n", addr, value);
            return;
    }
}

void irq_raise(PSX *psx, uint16_t value) {
    psx->irq.stat |= value;
    cop0_update_interrupts(psx);
}
