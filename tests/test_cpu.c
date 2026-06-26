#include "cpu.h"
#include "bus.h"
#include <stdio.h>
#include <assert.h>

#define ITYPE(opcode, rs, rt, imm) (((opcode & 0x3F) << 26) | ((rs & 0x1F) << 21) | ((rt & 0x1F) << 16) | (imm & 0xFFFF))
#define LW(rs, rt, imm) ITYPE(0x23, rs, rt, imm)
#define ADDIU(rs, rt, imm) ITYPE(0x09, rs, rt, imm)

int main() {
    Bus* bus = create_flat_bus();
    Cpu* cpu = create_cpu();

    // should delay LW result by one instruction
    cpu->pc = 0x00000000;
    cpu->next_pc = cpu->pc + 4;
    cpu->r[1] = 0x00002000; // base address for LW
    cpu->r[2] = 0xDEADBEEF; // old value

    bus_write32(bus, 0x00002000, 0xCAFEBABE); // write value to memory

    // Write the instructions
    bus_write32(bus, 0x00000000, LW(1, 2, 0)); // schedules a load of 0xCAFEBABE into $2
    bus_write32(bus, 0x00000004, ADDIU(2, 3, 1)); // ADDIU $3, $2, 1 (should use old value of $2)
    bus_write32(bus, 0x00000008, ADDIU(2, 4, 1)); // ADDIU $4, $2, 1 (should use new value of $2)

    // Step through the instructions
    cpu_step(cpu, bus); // Execute LW
    cpu_step(cpu, bus); // Execute first ADDIU
    cpu_step(cpu, bus); // Execute second ADDIU

    // Check the results
    assert(cpu->r[2] == 0xCAFEBABE && "$2 should now have the loaded value");
    assert(cpu->r[3] == 0xDEADBEEF + 1 && "$3 should use old value of $2");
    assert(cpu->r[4] == 0xCAFEBABE + 1 && "$4 should use new value of $2");

    destroy_cpu(cpu);
    destroy_flat_bus(bus);
}
