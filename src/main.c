#include <stdint.h>


// r3000a state
typedef struct {
    uint32_t pc; // program counter
    uint32_t r[32]; // general purpose registers
    uint32_t hi; // high register
    uint32_t lo; // low register
} Cpu;

typedef struct {
    uint8_t *ram;
} Bus;

uint32_t bus_read32(Bus *bus, uint32_t addr) {
    uint32_t physical_addr = addr & 0x1FFFFFFF; // Mask to 29 bits
    uint32_t value;
    memcpy(&value, bus->ram + physical_addr, sizeof(uint32_t));
#
}

int main() {
    // Your code here
    return 0;
}
