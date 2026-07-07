#ifndef SPU_H
#define SPU_H

#include <stdint.h>
#include "common.h"

uint8_t spu_read8(PSX *psx, uint32_t addr);
uint16_t spu_read16(PSX *psx, uint32_t addr);
uint32_t spu_read32(PSX *psx, uint32_t addr);
void spu_write8(PSX *psx, uint32_t addr, uint8_t value);
void spu_write16(PSX *psx, uint32_t addr, uint16_t value);
void spu_write32(PSX *psx, uint32_t addr, uint32_t value);

#endif // SPU_H
