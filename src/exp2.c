#include <stdio.h>
#include "exp2.h"

uint8_t exp2_read8(PSX *psx, uint32_t addr) {
    return 0;
}
uint16_t exp2_read16(PSX *psx, uint32_t addr) {
    return 0;
}
uint32_t exp2_read32(PSX *psx, uint32_t addr) {
    return 0;
}
void exp2_write8(PSX *psx, uint32_t addr, uint8_t value) {
    if (addr == 0x1F802041) {
        // POST status to be displayed on external 7-segment display
        // printf("POST: %X\n", value);
        return;
    }
}
void exp2_write16(PSX *psx, uint32_t addr, uint16_t value) {
}
void exp2_write32(PSX *psx, uint32_t addr, uint32_t value) {
}
