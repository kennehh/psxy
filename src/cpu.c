#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "exceptions.h"

#define OPCODE(cpu) ((cpu->inst >> 26) & 0x3F)
#define FUNCT(cpu) (cpu->inst & 0x3F)
#define RS(cpu) ((cpu->inst >> 21) & 0x1F)
#define RT(cpu) ((cpu->inst >> 16) & 0x1F)
#define RD(cpu) ((cpu->inst >> 11) & 0x1F)
#define SHAMT(cpu) ((cpu->inst >> 6) & 0x1F)
#define IMM(cpu) (cpu->inst & 0xFFFF)
#define SIMM(cpu) ((int16_t)(cpu->inst & 0xFFFF))
#define SIMM_EXT(cpu) ((uint32_t)(int16_t)(cpu->inst & 0xFFFF))
#define TARGET(cpu) (cpu->inst & 0x3FFFFFF)

#define ADD_OVERFLOW_CHECK(a, b, result) (((a ^ result) & (b ^ result) & 0x80000000) != 0)
#define SUB_OVERFLOW_CHECK(a, b, result) (((a ^ b) & (a ^ result) & 0x80000000) != 0)

#define OPCODE_TABLE \
    X(0x01, bcond) \
    X(0x02, j) \
    X(0x03, jal) \
    X(0x04, beq) \
    X(0x05, bne) \
    X(0x06, blez) \
    X(0x07, bgtz) \
    X(0x08, addi) \
    X(0x09, addiu) \
    X(0x0A, slti) \
    X(0x0B, sltiu) \
    X(0x0C, andi) \
    X(0x0D, ori) \
    X(0x0E, xori) \
    X(0x0F, lui) \
    X(0x10, cop0) \
    X(0x12, cop2) \
    X(0x20, lb) \
    X(0x21, lh) \
    X(0x22, lwl) \
    X(0x23, lw) \
    X(0x24, lbu) \
    X(0x25, lhu) \
    X(0x26, lwr) \
    X(0x28, sb) \
    X(0x29, sh) \
    X(0x2A, swl) \
    X(0x2B, sw) \
    X(0x2E, swr) \

#define FUNCT_TABLE \
    X(0x00, sll) \
    X(0x02, srl) \
    X(0x03, sra) \
    X(0x04, sllv) \
    X(0x06, srlv) \
    X(0x07, srav) \
    X(0x08, jr) \
    X(0x09, jalr) \
    X(0x0C, sys) \
    X(0x0D, brk) \
    X(0x10, mfhi) \
    X(0x11, mthi) \
    X(0x12, mflo) \
    X(0x13, mtlo) \
    X(0x18, mult) \
    X(0x19, multu) \
    X(0x1A, div) \
    X(0x1B, divu) \
    X(0x20, add) \
    X(0x21, addu) \
    X(0x22, sub) \
    X(0x23, subu) \
    X(0x24, and) \
    X(0x25, or) \
    X(0x26, xor) \
    X(0x27, nor) \
    X(0x2A, slt) \
    X(0x2B, sltu)


#define OP_R(name, expr) \
static inline void execute_##name(Cpu *cpu, Bus *bus) { \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    uint32_t rt = reg_read(cpu, RT(cpu)); \
    uint32_t result = expr; \
    reg_write(cpu, RD(cpu), result); \
}

#define OP_R_SHAMT(name, expr) \
static inline void execute_##name(Cpu *cpu, Bus *bus) { \
    uint32_t value = reg_read(cpu, RT(cpu)); \
    uint32_t result = expr; \
    reg_write(cpu, RD(cpu), result); \
}

#define OP_EXC(name, exc_code) \
static inline void execute_##name(Cpu *cpu, Bus *bus) { \
    cpu->next_exc_code = exc_code; \
}

#define OP_I(name, expr) \
static inline void execute_##name(Cpu *cpu, Bus *bus) { \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    uint16_t imm = IMM(cpu); \
    uint32_t result = expr; \
    reg_write(cpu, RT(cpu), result); \
}

#define OP_I_BRANCH_RT(name, condition) \
static inline void execute_##name(Cpu *cpu, Bus *bus) { \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    uint32_t rt = reg_read(cpu, RT(cpu)); \
    cpu->next_branch_state = ((condition) << 1) | BRANCH_STATE_IN_DELAY_SLOT; \
    cpu->next_branch_target = get_next_pc(cpu) + (SIMM_EXT(cpu) << 2); \
}

#define OP_I_BRANCH_Z(name, condition) \
static inline void execute_##name(Cpu *cpu, Bus *bus) { \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    cpu->next_branch_target = get_next_pc(cpu) + (SIMM_EXT(cpu) << 2); \
    cpu->next_branch_state = ((condition) << 1) | BRANCH_STATE_IN_DELAY_SLOT; \
}

#define OP_I_BRANCH_Z_LINK(name, condition) \
static inline void execute_##name(Cpu *cpu, Bus *bus) { \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    uint32_t pc = get_next_pc(cpu); \
    cpu->next_branch_target = pc + (SIMM_EXT(cpu) << 2); \
    cpu->next_branch_state = ((condition) << 1) | BRANCH_STATE_IN_DELAY_SLOT; \
    reg_write(cpu, 31, pc + 4); \
}

static inline uint32_t get_next_pc(Cpu *cpu) {
    if (IS_BRANCH_TAKEN(cpu->branch_state)) {
        return cpu->branch_target;
    }
    return cpu->next_pc;
}

static inline uint32_t reg_read(Cpu *cpu, uint8_t reg) {
    if (reg == 0) {
        return 0; // Register $zero always returns 0
    }
    return cpu->r[reg];
}

static inline uint32_t reg_read_with_load(Cpu *cpu, uint8_t reg) {
    if (reg == 0) {
        return 0; // Register $zero always returns 0
    }
    if (cpu->load_reg == reg) {
        return cpu->load_value; // Return the value to be loaded after the delay slot
    }
    return cpu->r[reg];
}

static inline void reg_write(Cpu *cpu, uint8_t reg, uint32_t value) {
    if (reg == 0) {
        return; // Register $zero is read-only
    }
    if (cpu->load_reg == reg) {
        cpu->load_reg = 0; // Clear the load register to prevent double loading
    }
    cpu->r[reg] = value;
}

static inline bool is_cache_isolated(uint32_t status) {
    return (status & 0x00010000) != 0; // Check the KSU bits for cache isolation
}

static inline void cpu_write8(Cpu *cpu, Bus *bus, uint32_t addr, uint8_t value) {
    if (is_cache_isolated(cpu->cop0.status)) {
        return;
    }
    bus_write8(bus, addr, value);
}

static inline void cpu_write16(Cpu *cpu, Bus *bus, uint32_t addr, uint16_t value) {
    if (is_cache_isolated(cpu->cop0.status)) {
        return;
    }
    bus_write16(bus, addr, value);
}

static inline void cpu_write32(Cpu *cpu, Bus *bus, uint32_t addr, uint32_t value) {
    if (is_cache_isolated(cpu->cop0.status)) {
        return;
    }
    bus_write32(bus, addr, value);
}

static inline void schedule_load(Cpu *cpu, uint8_t reg, uint32_t value) {
    if (cpu->load_reg == reg) {
        cpu->load_reg = 0; // Clear the load register to prevent double loading
    }
    cpu->next_load_reg = reg;
    cpu->next_load_value = value;
}

static inline void commit_load(Cpu *cpu) {
    if (cpu->load_reg > 0) {
        cpu->r[cpu->load_reg] = cpu->load_value;
    }
    cpu->load_reg = cpu->next_load_reg;
    cpu->load_value = cpu->next_load_value;
    cpu->next_load_reg = 0;
}

static inline uint32_t get_addr_from_imm(Cpu *cpu) {
    uint32_t base = reg_read(cpu, RS(cpu));
    int16_t offset = SIMM(cpu);
    return base + offset;
}

OP_R_SHAMT(sll, value << SHAMT(cpu))
OP_R_SHAMT(srl, value >> SHAMT(cpu))
OP_R_SHAMT(sra, (int32_t)value >> SHAMT(cpu)) // Arithmetic right shift
OP_R(sllv, rt << (rs & 0x1F))
OP_R(srlv, rt >> (rs & 0x1F))
OP_R(srav, (int32_t)rt >> (rs & 0x1F)) // Arithmetic right shift
OP_EXC(sys, EXC_SYS)
OP_EXC(brk, EXC_BP)
OP_R(addu, rs + rt)
OP_R(subu, rs - rt)
OP_R(and, rs & rt)
OP_R(or, rs | rt)
OP_R(xor, rs ^ rt)
OP_R(nor, ~(rs | rt))
OP_R(slt, (int32_t)rs < (int32_t)rt ? 1 : 0)
OP_R(sltu, rs < rt ? 1 : 0)

OP_I(addiu, rs + (int16_t)imm)
OP_I(andi, rs & imm)
OP_I(ori, rs | imm)
OP_I(xori, rs ^ imm)
OP_I(slti, (int32_t)rs < (int16_t)imm ? 1 : 0)
OP_I(sltiu, rs < (uint32_t)(int16_t)imm ? 1 : 0)
OP_I(lui, imm << 16)
OP_I_BRANCH_RT(beq, rs == rt)
OP_I_BRANCH_RT(bne, rs != rt)
OP_I_BRANCH_Z(bgez, (int32_t)rs >= 0)
OP_I_BRANCH_Z(bgtz, (int32_t)rs > 0)
OP_I_BRANCH_Z(blez, (int32_t)rs <= 0)
OP_I_BRANCH_Z(bltz, (int32_t)rs < 0)
OP_I_BRANCH_Z_LINK(bgezal, (int32_t)rs >= 0)
OP_I_BRANCH_Z_LINK(bltzal, (int32_t)rs < 0)

static inline void execute_jr(Cpu *cpu, Bus *bus) {
    uint32_t target = reg_read(cpu, RS(cpu));
    cpu->next_branch_target = target;
    cpu->next_branch_state = BRANCH_STATE_TAKEN;
}

static inline void execute_jalr(Cpu *cpu, Bus *bus) {
    execute_jr(cpu, bus); // Jump to target address
    reg_write(cpu, RD(cpu), get_next_pc(cpu) + 4); // Save return address
}

static inline void execute_mfhi(Cpu *cpu, Bus *bus) {
    reg_write(cpu, RD(cpu), cpu->hi);
}

static inline void execute_mthi(Cpu *cpu, Bus *bus) {
    cpu->hi = reg_read(cpu, RS(cpu));
}

static inline void execute_mflo(Cpu *cpu, Bus *bus) {
    reg_write(cpu, RD(cpu), cpu->lo);
}

static inline void execute_mtlo(Cpu *cpu, Bus *bus) {
    cpu->lo = reg_read(cpu, RS(cpu));
}

static inline void execute_mult(Cpu *cpu, Bus *bus) {
    int64_t product = (int64_t)(int32_t)reg_read(cpu, RS(cpu)) * (int64_t)(int32_t)reg_read(cpu, RT(cpu));
    cpu->hi = (uint32_t)(product >> 32);
    cpu->lo = (uint32_t)(product & 0xFFFFFFFF);
}

static inline void execute_multu(Cpu *cpu, Bus *bus) {
    uint64_t product = (uint64_t)reg_read(cpu, RS(cpu)) * (uint64_t)reg_read(cpu, RT(cpu));
    cpu->hi = (uint32_t)(product >> 32);
    cpu->lo = (uint32_t)(product & 0xFFFFFFFF);
}

static inline void execute_div(Cpu *cpu, Bus *bus) {
    int32_t rs = (int32_t)reg_read(cpu, RS(cpu));
    int32_t rt = (int32_t)reg_read(cpu, RT(cpu));
    if (rt == 0) {
        cpu->hi = rs; // Division by zero: set HI to dividend
        cpu->lo = (rs >= 0) ? -1 : 1; // Set LO to -1 or 1 based on the sign of the dividend
    } else if (rs == INT32_MIN && rt == -1) {
        cpu->hi = 0; // Overflow case: set HI to 0
        cpu->lo = INT32_MIN; // Set LO to INT32_MIN
    } else {
        cpu->lo = rs / rt; // Quotient
        cpu->hi = rs % rt; // Remainder
    }
}

static inline void execute_divu(Cpu *cpu, Bus *bus) {
    uint32_t rs = reg_read(cpu, RS(cpu));
    uint32_t rt = reg_read(cpu, RT(cpu));
    if (rt == 0) {
        cpu->hi = rs; // Division by zero: set HI to dividend
        cpu->lo = UINT32_MAX; // Set LO to maximum unsigned value
    } else {
        cpu->lo = rs / rt; // Quotient
        cpu->hi = rs % rt; // Remainder
    }
}

static inline void execute_add(Cpu *cpu, Bus *bus) {
    uint32_t rs = reg_read(cpu, RS(cpu));
    uint32_t rt = reg_read(cpu, RT(cpu));
    uint32_t result = rs + rt;
    // Check for signed overflow
    if (ADD_OVERFLOW_CHECK(rs, rt, result)) {
        cpu->next_exc_code = EXC_OV; // Set exception code for overflow
    } else {
        reg_write(cpu, RD(cpu), result);
    }
}

static inline void execute_sub(Cpu *cpu, Bus *bus) {
    uint32_t rs = reg_read(cpu, RS(cpu));
    uint32_t rt = reg_read(cpu, RT(cpu));
    uint32_t result = rs - rt;
    // Check for signed overflow
    if (SUB_OVERFLOW_CHECK(rs, rt, result)) {
        cpu->next_exc_code = EXC_OV; // Set exception code for overflow
    } else {
        reg_write(cpu, RD(cpu), result);
    }
}

static inline void execute_j(Cpu *cpu, Bus *bus) {
    uint32_t pc = get_next_pc(cpu);
    uint32_t target = (pc & 0xF0000000) | (TARGET(cpu) << 2);
    cpu->next_branch_target = target;
    cpu->next_branch_state = BRANCH_STATE_TAKEN;
}

static inline void execute_jal(Cpu *cpu, Bus *bus) {
    uint32_t pc = get_next_pc(cpu);
    uint32_t target = (pc & 0xF0000000) | (TARGET(cpu) << 2);
    reg_write(cpu, 31, pc + 4); // Save return address in $ra
    cpu->next_branch_target = target;
    cpu->next_branch_state = BRANCH_STATE_TAKEN;
}


static inline void execute_addi(Cpu *cpu, Bus *bus) {
    uint32_t rs = reg_read(cpu, RS(cpu));
    int16_t simm = SIMM(cpu);
    uint32_t result = rs + simm;
    // Check for signed overflow
    if (ADD_OVERFLOW_CHECK(rs, simm, result)) {
        cpu->next_exc_code = EXC_OV; // Set exception code for overflow
    } else {
        reg_write(cpu, RT(cpu), result);
    }
}

static inline void execute_lb(Cpu *cpu, Bus *bus) {
    uint32_t base = reg_read(cpu, RS(cpu));
    int16_t offset = SIMM(cpu);
    uint32_t addr = base + offset;
    int32_t value = (int32_t)(int8_t)bus_read8(bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lbu(Cpu *cpu, Bus *bus) {
    uint32_t base = reg_read(cpu, RS(cpu));
    int16_t offset = SIMM(cpu);
    uint32_t addr = base + offset;
    uint32_t value = bus_read8(bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lh(Cpu *cpu, Bus *bus) {
    uint32_t base = reg_read(cpu, RS(cpu));
    int16_t offset = SIMM(cpu);
    uint32_t addr = base + offset;
    if (addr & 1) {
        cpu->next_exc_code = EXC_ADEL; // Address error load/fetch
        return;
    }
    int32_t value = (int32_t)(int16_t)bus_read16(bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lhu(Cpu *cpu, Bus *bus) {
    uint32_t addr = get_addr_from_imm(cpu);
    if (addr & 1) {
        cpu->next_exc_code = EXC_ADEL; // Address error load/fetch
        return;
    }
    uint32_t value = bus_read16(bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lw(Cpu *cpu, Bus *bus) {
    uint32_t addr = get_addr_from_imm(cpu);
    if (addr & 3) {
        cpu->next_exc_code = EXC_ADEL; // Address error load/fetch
        return;
    }
    uint32_t value = bus_read32(bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lwl(Cpu *cpu, Bus *bus) {
    uint32_t addr = get_addr_from_imm(cpu);
    uint32_t aligned_addr = addr & ~3; // Align address to 4 bytes
    uint32_t result;

    switch (addr & 3) {
        case 0: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint8_t mem_value = bus_read8(bus, aligned_addr);
            result = (reg_value & 0x00FFFFFF) | (mem_value << 24);
            break;
        }
        case 1: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint16_t mem_value = bus_read16(bus, aligned_addr);
            result = (reg_value & 0x0000FFFF) | (mem_value << 16);
            break;
        }
        case 2: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint16_t mem_value_16 = bus_read16(bus, aligned_addr);
            uint8_t mem_value_8 = bus_read8(bus, addr);
            result = (reg_value & 0x000000FF) | (mem_value_16 << 8) | (mem_value_8 << 24);
            break;
        }
        case 3: {
            result = bus_read32(bus, aligned_addr);
            break;
        }
    }

    schedule_load(cpu, RT(cpu), result);
}

static inline void execute_lwr(Cpu *cpu, Bus *bus) {
    uint32_t addr = get_addr_from_imm(cpu);
    uint32_t result;

    switch (addr & 3) {
        case 0:
            result = bus_read32(bus, addr);
            break;
        case 1: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint8_t mem_value_8 = bus_read8(bus, addr);
            uint16_t mem_value_16 = bus_read16(bus, addr + 1);
            result = (reg_value & 0xFF000000) | (mem_value_16 << 8) | mem_value_8;
            break;
        }
        case 2: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint16_t mem_value_16 = bus_read16(bus, addr);
            result = (reg_value & 0xFFFF0000) | mem_value_16;
            break;
        }
        case 3: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint8_t mem_value_8 = bus_read8(bus, addr);
            result = (reg_value & 0xFFFFFF00) | mem_value_8;
            break;
        }
    }

    schedule_load(cpu, RT(cpu), result);
}

static inline void execute_sb(Cpu *cpu, Bus *bus) {
    uint32_t addr = get_addr_from_imm(cpu);
    uint8_t value = (uint8_t)reg_read(cpu, RT(cpu));
    cpu_write8(cpu, bus, addr, value);
}

static inline void execute_sh(Cpu *cpu, Bus *bus) {
    uint32_t addr = get_addr_from_imm(cpu);
    if (addr & 1) {
        cpu->next_exc_code = EXC_ADES; // Address error store
        return;
    }
    uint16_t value = (uint16_t)reg_read(cpu, RT(cpu));
    cpu_write16(cpu, bus, addr, value);
}

static inline void execute_sw(Cpu *cpu, Bus *bus) {
    uint32_t addr = get_addr_from_imm(cpu);
    if (addr & 3) {
        cpu->next_exc_code = EXC_ADES; // Address error store
        return;
    }
    uint32_t value = reg_read(cpu, RT(cpu));
    cpu_write32(cpu, bus, addr, value);
}

static inline void execute_swl(Cpu *cpu, Bus *bus) {
    uint32_t addr = get_addr_from_imm(cpu);
    uint32_t aligned_addr = addr & ~3; // Align address to 4 bytes
    uint32_t value = reg_read(cpu, RT(cpu));

    switch (addr & 3) {
        case 0:
            cpu_write8(cpu, bus, aligned_addr, value >> 24);
            break;
        case 1:
            cpu_write16(cpu, bus, aligned_addr, value >> 16);
            break;
        case 2:
            cpu_write16(cpu, bus, aligned_addr, value >> 8);
            cpu_write8(cpu, bus, addr, (value >> 24) & 0xFF);
            break;
        case 3:
            cpu_write32(cpu, bus, aligned_addr, value);
            break;
    }
}

static inline void execute_swr(Cpu *cpu, Bus *bus) {
    uint32_t addr = get_addr_from_imm(cpu);
    uint32_t value = reg_read(cpu, RT(cpu));

    switch (addr & 3) {
        case 0:
            cpu_write32(cpu, bus, addr, value);
            break;
        case 1:
            cpu_write8(cpu, bus, addr, value);
            cpu_write16(cpu, bus, addr + 1, value >> 8);
            break;
        case 2:
            cpu_write16(cpu, bus, addr, value);
            break;
        case 3:
            cpu_write8(cpu, bus, addr, value);
            break;
    }
}

static inline void cop0_write(Cpu *cpu, uint8_t rd, uint32_t value) {
    switch (rd) {
        case 3:  cpu->cop0.bpc = value;       break;
        case 5:  cpu->cop0.bda = value;       break;
        case 6:  cpu->cop0.tar = value;       break;
        case 7:  cpu->cop0.dcic = value;      break;
        case 9:  cpu->cop0.bdam = value;      break;
        case 12: cpu->cop0.status = value;    break;
    }
}

static inline void cop0_read(Cpu *cpu, Bus *bus) {
    uint32_t value = 0;
    switch (RD(cpu)) {
        case 3:  value = cpu->cop0.bpc;       break;
        case 5:  value = cpu->cop0.bda;       break;
        case 6:  value = cpu->cop0.tar;       break;
        case 7:  value = cpu->cop0.dcic;      break;
        case 8:  value = cpu->cop0.badAddr;   break;
        case 12: value = cpu->cop0.status;    break;
        case 13: value = cpu->cop0.cause;     break;
        case 14: value = cpu->cop0.epc;       break;
        case 15: value = cpu->cop0.prid;      break;
    }
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_cop0(Cpu *cpu, Bus *bus) {
    switch (RS(cpu)) {
        case 0x00: // MFC0
            cop0_read(cpu, bus);
            break;
        case 0x04: // MTC0
            cop0_write(cpu, RD(cpu), reg_read(cpu, RT(cpu)));
            break;
        case 0x10: {// RFE
            uint32_t stat = cpu->cop0.status;
            cpu->cop0.status = (stat & ~0x0F) | ((stat >> 2) & 0x0F);
            break;
        }
        default:
            cpu->next_exc_code = EXC_RI; // Reserved instruction exception
            break;
    }
}

static inline void execute_cop2(Cpu *cpu, Bus *bus) {
    // Placeholder for COP2 instructions (e.g., GTE)
    cpu->next_exc_code = EXC_RI; // Reserved instruction exception
}

static inline void execute_bcond(Cpu *cpu, Bus *bus) {
    uint8_t rt = RT(cpu);
    if (rt == 0x10) {
        execute_bltzal(cpu, bus);
    } else if (rt == 0x11) {
        execute_bgezal(cpu, bus);
    } else if (rt & 0x1) {
        execute_bgez(cpu, bus);
    } else {
        execute_bltz(cpu, bus);
    }
}

static inline void execute_instruction(Cpu *cpu, Bus *bus) {
    if (cpu->inst == 0) {
        return;
    }

    switch (OPCODE(cpu)) {
        case 0x00: // SPECIAL
            switch (FUNCT(cpu)) {
                #define X(funct, name) case funct: execute_##name(cpu, bus); break;
                FUNCT_TABLE
                #undef X
                default:
                    cpu->next_exc_code = EXC_RI; // Reserved instruction exception
                    break;
            }
            break;
        #define X(opcode, name) case opcode: execute_##name(cpu, bus); break;
        OPCODE_TABLE
        #undef X
        default:
            cpu->next_exc_code = EXC_RI; // Reserved instruction exception
            break;
    }
}

static inline void cpu_begin_step(Cpu *cpu) {
    cpu->next_branch_state = BRANCH_STATE_NO_DELAY;
    cpu->next_load_reg = 0;

#ifdef SINGLE_STEP_TEST_MODE
    // only next_branch_state and next_load_reg are required to be cleared,
    // but single step tests expect next_branch_target and next_load_value to be cleared as well
    cpu->next_branch_target = 0;
    cpu->next_load_value = 0;
#endif
}

static inline bool cpu_fetch(Cpu *cpu, Bus *bus) {
    if (cpu->pc & 3) {
        cpu->next_exc_code = EXC_ADEL; // Address error load/fetch
        return false;
    }
    cpu->inst = bus_fetch32(bus, cpu->pc);
    return true;
}

static inline void cpu_execute(Cpu *cpu, Bus *bus) {
    execute_instruction(cpu, bus);
}

static inline void cpu_finish_step(Cpu *cpu) {
    commit_load(cpu); // Commit any scheduled load after executing the instruction

    if (cpu->next_exc_code != EXC_NONE) {
        raise_exception(cpu, cpu->next_exc_code);
        return;
    }

    cpu->pc = get_next_pc(cpu);
    cpu->next_pc = cpu->pc + 4;
    cpu->branch_state = cpu->next_branch_state;

#ifdef SINGLE_STEP_TEST_MODE
    // no need to clear if we've already cleared branch_state, but single step tests expect branch_target to be cleared as well
    cpu->branch_target = cpu->next_branch_target;
#else
    if (IS_IN_DELAY_SLOT(cpu->branch_state)) {
        cpu->branch_target = cpu->next_branch_target;
    }
#endif
}

uint32_t cpu_step(Cpu *cpu, Bus *bus) {
    cpu_begin_step(cpu);
    if (!cpu_fetch(cpu, bus)) {
        cpu_finish_step(cpu);
        return cpu->pc;
    }
    cpu_execute(cpu, bus);
    cpu_finish_step(cpu);
    return cpu->pc;
}

Cpu *cpu_create(void) {
    Cpu *cpu = (Cpu *)malloc(sizeof(Cpu));
    if (!cpu) {
        exit(EXIT_FAILURE); // Handle memory allocation failure
    }
    cpu_reset(cpu);
    return cpu;
}

void cpu_reset(Cpu *cpu) {
    if (!cpu) return;
    memset(cpu, 0, sizeof(Cpu)); // Reset all fields to zero
    cpu->pc = 0xBFC00000; // Reset the program counter to the reset vector
    cpu->next_pc = cpu->pc + 4;
    cpu->next_exc_code = EXC_NONE;
}

void cpu_destroy(Cpu *cpu) {
    if (!cpu) return;
    free(cpu);
}
