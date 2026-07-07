#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

// Shared constants and macros

#if defined(__GNUC__) || defined(__clang__)
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x)   (x)
    #define unlikely(x) (x)
#endif

#define MEMORY_SIZE     0x20000000 // 512MB of addressable memory

#define BUS_PAGE_SHIFT  12 // 2^12 = 4096
#define BUS_PAGE_SIZE   (1 << BUS_PAGE_SHIFT) // 4096 bytes per page
#define BUS_PAGE_COUNT  (MEMORY_SIZE / BUS_PAGE_SIZE) // 2^29 / 2^12 = 2^17 pages
#define BUS_PAGE_MASK   (BUS_PAGE_SIZE - 1) // 0xFFF

#define RAM_SIZE       1024 * 1024 * 2 // 2MB of RAM
#define RAM_PHYS_START 0x00000000
#define RAM_PHYS_END   (RAM_PHYS_START + RAM_SIZE - 1)

#define EXP1_SIZE       0x800000 // 8MB of expansion 1 memory
#define EXP1_PHYS_START 0x1F000000
#define EXP1_PHYS_END   (EXP1_PHYS_START + EXP1_SIZE - 1)

#define SCRATCHPAD_SIZE 1024 // 1KB of scratchpad memory
#define SCRATCHPAD_PHYS_START 0x1F800000
#define SCRATCHPAD_PHYS_END   0x1F800FFF

#define IO_SIZE       0x2000 // 8KB of I/O memory
#define IO_PHYS_START 0x1F801000
#define IO_PHYS_END   (IO_PHYS_START + IO_SIZE - 1)

#define BIOS_SIZE       0x80000 // 512KB of BIOS
#define BIOS_PHYS_START 0x1FC00000
#define BIOS_PHYS_END   (BIOS_PHYS_START + BIOS_SIZE - 1)

#define IO_GPU_START 0x1F801810
#define IO_GPU_END   0x1F80181F

#define IO_IRQ_START 0x1F801070
#define IO_IRQ_END   0x1F80107F

#define IO_TIMERS_START 0x1F801100
#define IO_TIMERS_END   0x1F80112F

#define IO_MEMCTRL_1_START 0x1F801000
#define IO_MEMCTRL_1_END   0x1F80101F

#define IO_MEMCTRL_2_START 0x1F801060
#define IO_MEMCTRL_2_END   0x1F80106F

#define IO_MEMCTRL_3_START 0xFFFE0000

#define CASE_REG32_8(base) \
    case (base): case (base + 1): case (base + 2): case (base + 3)

#define CASE_REG32_16(base) \
    case (base): case (base + 2)

#define CASE_REG16_8(base) \
    case (base): case (base + 1)

#define CASE_REG32_READ8(base, field) \
    CASE_REG32_8(base): \
        return reg32_read8(field, addr);

#define CASE_REG32_READ16(base, field) \
    CASE_REG32_16(base): \
        return reg32_read16(field, addr);

#define CASE_REG32_READ32(base, field) \
    case (base): \
        return field;

#define CASE_REG32_WRITE8(base, field) \
    CASE_REG32_8(base): \
        field = reg32_write8(addr, field, value); \
        return;

#define CASE_REG32_WRITE16(base, field) \
    CASE_REG32_16(base): \
        field = reg32_write16(addr, field, value); \
        return;

#define CASE_REG32_WRITE32(base, field) \
    case (base): \
        field = value; \
        return;

#define CASE_REG16_READ8(base, field) \
    CASE_REG16_8(base): \
        return reg16_read8(field, addr);

#define CASE_REG16_READ16(base, field) \
    case (base): \
        return field;

#define CASE_REG16_READ32(base, field) \
    case (base): \
        return (uint32_t)field;

#define CASE_REG16_WRITE8(base, field) \
    CASE_REG16_8(base): \
        field = reg16_write8(addr, field, value); \
        return;

#define CASE_REG16_WRITE16(base, field) \
    case (base): \
        field = value; \
        return;

#define CASE_REG16_WRITE32(base, field) \
    case (base): \
        field = (uint16_t)value; \
        return;

#define CASE_REG8_READ8(base, field) \
    case (base): \
        return field;

#define CASE_REG8_READ16(base, field) \
    case (base): \
        return (uint16_t)field;

#define CASE_REG8_READ32(base, field) \
    case (base): \
        return (uint32_t)field;

#define CASE_REG8_WRITE8(base, field) \
    case (base): \
        field = value; \
        return;

#define CASE_REG8_WRITE16(base, field) \
    case (base): \
        field = (uint8_t)value; \
        return;

#define CASE_REG8_WRITE32(base, field) \
    case (base): \
        field = (uint8_t)value; \
        return;

// Shared declarations

typedef struct PSX PSX;
typedef struct Cpu Cpu;
typedef struct Cop0 Cop0;
typedef struct Bus Bus;
typedef struct TTY TTY;
typedef struct Gpu Gpu;
typedef struct Irq Irq;
typedef struct Timers Timers;

// Shared utility functions

static inline uint32_t physical_address(uint32_t addr) {
    return addr & 0x1FFFFFFF; // Mask to 29 bits
}

static inline uint32_t get_page_index(uint32_t virt_addr) {
    return physical_address(virt_addr) >> BUS_PAGE_SHIFT;
}

static inline uint8_t reg32_read8(uint32_t value, uint32_t addr) {
    return (uint8_t)(value >> ((addr & 3u) * 8u));
}

static inline uint16_t reg32_read16(uint32_t reg, uint32_t addr) {
    return (uint16_t)(reg >> ((addr & 2u) * 8u));
}

static inline uint32_t reg32_write8(uint32_t addr, uint32_t old_value, uint8_t new_value) {
    uint32_t shift = (addr & 3u) * 8u;
    uint32_t mask = 0xFFu << shift;
    return (old_value & ~mask) | ((uint32_t)new_value << shift);
}

static inline uint32_t reg32_write16(uint32_t addr, uint32_t old_value, uint16_t new_value) {
    uint32_t shift = (addr & 2u) * 8u;
    uint32_t mask = 0xFFFFu << shift;
    return (old_value & ~mask) | ((uint32_t)new_value << shift);
}

static inline uint8_t reg16_read8(uint16_t reg, uint32_t addr) {
    return (uint8_t)(reg >> ((addr & 1u) * 8u));
}

static inline uint16_t reg16_write8(uint32_t addr, uint16_t old_value, uint8_t new_value) {
    uint32_t shift = (addr & 1u) * 8u;
    uint16_t mask = 0xFFu << shift;
    return (old_value & ~mask) | ((uint16_t)new_value << shift);
}

#endif // COMMON_H
