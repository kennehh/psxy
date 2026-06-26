#include "bus.h"
#include <stdio.h>
#include <assert.h>

#define KSEG0_VIRT_START 0x80000000
#define KSEG1_VIRT_START 0xA0000000

int test_read(Bus* bus, uint32_t addr, uint32_t value) {
    uint32_t orig_addr = addr;

    uint32_t read_value_32 = bus_read32(bus, addr);
    assert(read_value_32 == value && "bus_read32 failed");

    uint16_t read_value_16 = bus_read16(bus, addr);
    assert(read_value_16 == (uint16_t)(value & 0xFFFF) && "bus_read16 failed");

    addr += 2; // Move to the next 16-bit boundary
    read_value_16 = bus_read16(bus, addr);
    assert(read_value_16 == (uint16_t)((value >> 16) & 0xFFFF) && "bus_read16 failed");

    addr = orig_addr; // Reset to original address
    uint8_t read_value_8 = bus_read8(bus, addr);
    assert(read_value_8 == (uint8_t)(value & 0xFF) && "bus_read8 failed");

    addr += 1; // Move to the next 8-bit boundary
    read_value_8 = bus_read8(bus, addr);
    assert(read_value_8 == (uint8_t)((value >> 8) & 0xFF) && "bus_read8 failed");

    addr += 1; // Move to the next 8-bit boundary
    read_value_8 = bus_read8(bus, addr);
    assert(read_value_8 == (uint8_t)((value >> 16) & 0xFF) && "bus_read8 failed");

    addr += 1; // Move to the next 8-bit boundary
    read_value_8 = bus_read8(bus, addr);
    assert(read_value_8 == (uint8_t)((value >> 24) & 0xFF) && "bus_read8 failed");

    return 0;
}

int test_read_all_segments(Bus* bus, uint32_t addr, uint32_t value) {
    // Test KUSEG
    printf("Testing KUSEG at address 0x%08X with value 0x%08X\n", addr, value);
    test_read(bus, addr, value);

    // Test KSEG0
    printf("Testing KSEG0 at address 0x%08X with value 0x%08X\n", addr | KSEG0_VIRT_START, value);
    uint32_t kseg0_addr = addr | KSEG0_VIRT_START;
    test_read(bus, kseg0_addr, value);

    // Test KSEG1
    printf("Testing KSEG1 at address 0x%08X with value 0x%08X\n", addr | KSEG1_VIRT_START, value);
    uint32_t kseg1_addr = addr | KSEG1_VIRT_START;
    test_read(bus, kseg1_addr, value);

    return 0;
}

int test_ram(Bus* bus) {
    uint32_t addr1 = 0x00000000;
    uint32_t value1 = 0x12345678;

    uint32_t addr2 = 0x00001000;
    uint32_t value2 = 0xCAFEBABE;

    uint32_t addr3 = 0x00002000;
    uint32_t value3 = 0xDEADBEEF;

    bus_write32(bus, addr1, value1);
    bus_write32(bus, addr2, value2);
    bus_write32(bus, addr3, value3);

    test_read_all_segments(bus, addr1, value1);
    test_read_all_segments(bus, addr2, value2);
    test_read_all_segments(bus, addr3, value3);

    return 0;
}

int test_exp1(Bus* bus) {
    uint32_t addr1 = 0x1F000000;
    uint32_t value1 = 0x0BADF00D;

    uint32_t addr2 = 0x1F000100;
    uint32_t value2 = 0xFEEDFACE;

    bus_write32(bus, addr1, value1);
    bus_write32(bus, addr2, value2);

    test_read_all_segments(bus, addr1, value1);
    test_read_all_segments(bus, addr2, value2);

    return 0;
}

int test_scratchpad(Bus* bus) {
    uint32_t addr1 = 0x1F800000;
    uint32_t value1 = 0xDEADC0DE;

    uint32_t addr2 = 0x1F800100;
    uint32_t value2 = 0xBAADF00D;

    bus_write32(bus, addr1, value1);
    bus_write32(bus, addr2, value2);

    test_read_all_segments(bus, addr1, value1);
    test_read_all_segments(bus, addr2, value2);

    return 0;
}

int test_bios(Bus* bus) {
    uint32_t addr1 = 0x1FC00000;
    uint32_t value1 = 0xFEEDBEEF;

    uint32_t addr2 = 0x1FC00010;
    uint32_t value2 = 0xCAFEBABE;

    bus_write32(bus, addr1, value1);
    bus_write32(bus, addr2, value2);

    test_read_all_segments(bus, addr1, value1);
    test_read_all_segments(bus, addr2, value2);

    return 0;
}

int main() {
    Bus* bus = create_bus();

    test_ram(bus);
    test_exp1(bus);
    test_scratchpad(bus);
    test_bios(bus);

    destroy_bus(bus);

    return 0;
}