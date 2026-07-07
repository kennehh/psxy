#ifndef MEMCTRL_H
#define MEMCTRL_H

#include <stdint.h>
#include "common.h"

typedef struct MemCtrl {
    uint32_t exp1_base; // Expansion 1 base address
    uint32_t exp2_base; // Expansion 2 base address

    uint32_t exp1_size; // Expansion 1 size
    uint32_t exp2_size; // Expansion 2 size
    uint32_t exp3_size; // Expansion 3 size
    uint32_t bios_size; // BIOS size
    uint32_t ram_size;  // RAM size

    uint32_t spu_delay; // SPU delay
    uint32_t cdrom_delay; // CD-ROM delay
    uint32_t common_delay; // Common delay

    uint32_t cache_ctrl; // Cache control register
} MemCtrl;

void memctrl_reset(MemCtrl *memctrl);
void memctrl_write8(PSX *psx, uint32_t addr, uint8_t value);
void memctrl_write16(PSX *psx, uint32_t addr, uint16_t value);
void memctrl_write32(PSX *psx, uint32_t addr, uint32_t value);
uint8_t memctrl_read8(PSX *psx, uint32_t addr);
uint16_t memctrl_read16(PSX *psx, uint32_t addr);
uint32_t memctrl_read32(PSX *psx, uint32_t addr);

#endif // MEMCTRL_H
