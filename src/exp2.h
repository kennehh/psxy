#ifndef EXP2_H
#define EXP2_H

#include <stdint.h>
#include "common.h"

uint8_t exp2_read8(PSX *psx, uint32_t addr);
uint16_t exp2_read16(PSX *psx, uint32_t addr);
uint32_t exp2_read32(PSX *psx, uint32_t addr);
void exp2_write8(PSX *psx, uint32_t addr, uint8_t value);
void exp2_write16(PSX *psx, uint32_t addr, uint16_t value);
void exp2_write32(PSX *psx, uint32_t addr, uint32_t value);

#endif // EXP2_H
