#include "memory/memctrl.h"
#include "core/psx.h"

#define MEMCTRL_REGS(X) \
    X(0x1F801000, exp1_base) \
    X(0x1F801004, exp2_base) \
    X(0x1F801008, exp1_size) \
    X(0x1F80100C, exp3_size) \
    X(0x1F801010, bios_size) \
    X(0x1F801014, spu_delay) \
    X(0x1F801018, cdrom_delay) \
    X(0x1F80101C, exp2_size) \
    X(0x1F801020, common_delay) \
    X(0x1F801060, ram_size) \
    X(0xFFFE0130, cache_ctrl)

void memctrl_reset(MemCtrl *memctrl) {
#define X(ADDR, FIELD) memctrl->FIELD = 0;
    MEMCTRL_REGS(X)
#undef X
}

uint8_t memctrl_read8(PSX *psx, uint32_t addr) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_READ8(ADDR, psx->memctrl.FIELD)
        MEMCTRL_REGS(X)
#undef X
        default:
            printf("MemCtrl read8 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

uint16_t memctrl_read16(PSX *psx, uint32_t addr) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_READ16(ADDR, psx->memctrl.FIELD)
        MEMCTRL_REGS(X)
#undef X
        default:
            printf("MemCtrl read16 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

uint32_t memctrl_read32(PSX *psx, uint32_t addr) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_READ32(ADDR, psx->memctrl.FIELD)
        MEMCTRL_REGS(X)
#undef X
        default:
            printf("MemCtrl read32 from unimplemented address: 0x%08X\n", addr);
            return 0;
    }
}

void memctrl_write8(PSX *psx, uint32_t addr, uint8_t value) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_WRITE8(ADDR, psx->memctrl.FIELD)
        MEMCTRL_REGS(X)
#undef X
        default:
            printf("MemCtrl write8 to unimplemented address: 0x%08X, value: 0x%02X\n", addr, value);
            return;
    }
}

void memctrl_write16(PSX *psx, uint32_t addr, uint16_t value) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_WRITE16(ADDR, psx->memctrl.FIELD)
        MEMCTRL_REGS(X)
#undef X
        default:
            printf("MemCtrl write16 to unimplemented address: 0x%08X, value: 0x%04X\n", addr, value);
            return;
    }
}

void memctrl_write32(PSX *psx, uint32_t addr, uint32_t value) {
    switch (addr) {
#define X(ADDR, FIELD) CASE_REG16_WRITE32(ADDR, psx->memctrl.FIELD)
        MEMCTRL_REGS(X)
#undef X
        default:
            printf("MemCtrl write32 to unimplemented address: 0x%08X, value: 0x%08X\n", addr, value);
            return;
    }
}
