#include "cpu.h"
#include "psx.h"
#include "bus_access.h"
#include <stdio.h>

#define ITYPE(opcode, rs, rt, imm) (((opcode & 0x3F) << 26) | ((rs & 0x1F) << 21) | ((rt & 0x1F) << 16) | (imm & 0xFFFF))
#define LW(rs, rt, imm) ITYPE(0x23, rs, rt, imm)
#define ADDIU(rs, rt, imm) ITYPE(0x09, rs, rt, imm)

int main() {
    PSX *psx = psx_create();
    Cpu *cpu = psx->cpu;

    // should delay LW result by one instruction
    cpu->pc = 0x00000000;
    cpu->next_pc = cpu->pc + 4;
    cpu->r[1] = 0x00002000; // base address for LW
    cpu->r[2] = 0xDEADBEEF; // old value

    bus_write32(psx, 0x00002000, 0xCAFEBABE); // write value to memory

    // Write the instructions
    bus_write32(psx, 0x00000000, LW(1, 2, 0)); // schedules a load of 0xCAFEBABE into $2
    bus_write32(psx, 0x00000004, ADDIU(2, 3, 1)); // ADDIU $3, $2, 1 (should use old value of $2)
    bus_write32(psx, 0x00000008, ADDIU(2, 4, 1)); // ADDIU $4, $2, 1 (should use new value of $2)

    // Step through the instructions
    cpu_step(psx); // Execute LW
    cpu_step(psx); // Execute first ADDIU
    cpu_step(psx); // Execute second ADDIU

    int result = 0;

    // Check the results
    if (cpu->r[2] != 0xCAFEBABE) {
        fprintf(stderr, "$2 should now have the loaded value, but got 0x%08X\n", cpu->r[2]);
        result = 1;
    }
    if (cpu->r[3] != 0xDEADBEEF + 1) {
        fprintf(stderr, "$3 should use old value of $2, but got 0x%08X\n", cpu->r[3]);
        result = 1;
    }
    if (cpu->r[4] != 0xCAFEBABE + 1) {
        fprintf(stderr, "$4 should use new value of $2, but got 0x%08X\n", cpu->r[4]);
        result = 1;
    }

    psx_destroy(psx);
    return result;
}
