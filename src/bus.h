#ifndef BUS_H
#define BUS_H

#include <stdint.h>

void bus_init();

uint8_t bus_read8(uint32_t addr);
uint16_t bus_read16(uint32_t addr);
uint32_t bus_read32(uint32_t addr);

void bus_write8(uint32_t addr, uint8_t value);
void bus_write16(uint32_t addr, uint16_t value);
void bus_write32(uint32_t addr, uint32_t value);

#endif // BUS_H
