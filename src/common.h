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

#define SCRATCHPAD_SIZE 0x1000 // 1KB of scratchpad memory, but we will allocate 4KB for alignment
#define SCRATCHPAD_PHYS_START 0x1F800000
#define SCRATCHPAD_PHYS_END   (SCRATCHPAD_PHYS_START + SCRATCHPAD_SIZE - 1)

#define IO_SIZE       0x2000 // 8KB of I/O memory
#define IO_PHYS_START 0x1F801000
#define IO_PHYS_END   (IO_PHYS_START + IO_SIZE - 1)

#define BIOS_SIZE       0x80000 // 512KB of BIOS
#define BIOS_PHYS_START 0x1FC00000
#define BIOS_PHYS_END   (BIOS_PHYS_START + BIOS_SIZE - 1)

// Shared declarations

typedef struct PSX PSX;
typedef struct Cpu Cpu;
typedef struct Cop0 Cop0;
typedef struct Bus Bus;
typedef struct TTY TTY;

// Shared utility functions

static inline uint32_t physical_address(uint32_t addr) {
    return addr & 0x1FFFFFFF; // Mask to 29 bits
}

static inline uint32_t get_page_index(uint32_t virt_addr) {
    return physical_address(virt_addr) >> BUS_PAGE_SHIFT;
}

#endif // COMMON_H
