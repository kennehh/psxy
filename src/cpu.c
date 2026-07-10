#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "psx.h"
#include "cop0.h"
#include "cpu.h"
#include "tty.h"
#include "bus.h"

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
static inline void execute_##name(PSX *psx) { \
    Cpu *cpu = &psx->cpu; \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    uint32_t rt = reg_read(cpu, RT(cpu)); \
    uint32_t result = expr; \
    reg_write(cpu, RD(cpu), result); \
}

#define OP_R_SHAMT(name, expr) \
static inline void execute_##name(PSX *psx) { \
    Cpu *cpu = &psx->cpu; \
    uint32_t value = reg_read(cpu, RT(cpu)); \
    uint32_t result = expr; \
    reg_write(cpu, RD(cpu), result); \
}

#define OP_EXC(name, exc_code) \
static inline void execute_##name(PSX *psx) { \
    Cpu *cpu = &psx->cpu; \
    cpu->next_exc_code = exc_code; \
}

#define OP_I_RS(name, expr) \
static inline void execute_##name(PSX *psx) { \
    Cpu *cpu = &psx->cpu; \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    uint16_t imm = IMM(cpu); \
    uint32_t result = expr; \
    reg_write(cpu, RT(cpu), result); \
}

#define OP_I(name, expr) \
static inline void execute_##name(PSX *psx) { \
    Cpu *cpu = &psx->cpu; \
    uint16_t imm = IMM(cpu); \
    uint32_t result = expr; \
    reg_write(cpu, RT(cpu), result); \
}

#define OP_I_BRANCH_RT(name, condition) \
static inline void execute_##name(PSX *psx) { \
    Cpu *cpu = &psx->cpu; \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    uint32_t rt = reg_read(cpu, RT(cpu)); \
    cpu->next_branch_state = ((condition) << 1) | BRANCH_STATE_IN_DELAY_SLOT; \
    cpu->next_branch_target = get_next_pc(cpu) + (SIMM_EXT(cpu) << 2); \
}

#define OP_I_BRANCH_Z(name, condition) \
static inline void execute_##name(PSX *psx) { \
    Cpu *cpu = &psx->cpu; \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    cpu->next_branch_target = get_next_pc(cpu) + (SIMM_EXT(cpu) << 2); \
    cpu->next_branch_state = ((condition) << 1) | BRANCH_STATE_IN_DELAY_SLOT; \
}

#define OP_I_BRANCH_Z_LINK(name, condition) \
static inline void execute_##name(PSX *psx) { \
    Cpu *cpu = &psx->cpu; \
    uint32_t rs = reg_read(cpu, RS(cpu)); \
    uint32_t pc = get_next_pc(cpu); \
    cpu->next_branch_target = pc + (SIMM_EXT(cpu) << 2); \
    cpu->next_branch_state = ((condition) << 1) | BRANCH_STATE_IN_DELAY_SLOT; \
    reg_write(cpu, 31, pc + 4); \
}

static inline uint32_t get_next_pc(Cpu *cpu) {
    uint32_t taken = (uint32_t)IS_BRANCH_TAKEN(cpu->branch_state);
    uint32_t mask = 0u - taken; // 0x00000000 or 0xFFFFFFFF
    return cpu->next_pc ^ ((cpu->next_pc ^ cpu->branch_target) & mask);
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

static inline void schedule_load(Cpu *cpu, uint8_t reg, uint32_t value) {
    if (cpu->load_reg == reg) {
        cpu->load_reg = 0; // Clear the load register to prevent double loading
    }
    cpu->next_load_reg = reg;
    cpu->next_load_value = value;
}

static inline void commit_load(Cpu *cpu) {
    uint8_t reg = cpu->load_reg;
    if (reg > 0) {
        cpu->r[reg] = cpu->load_value;
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

static inline bool add_overflow_check(uint32_t a, uint32_t b, uint32_t result) {
    return ((a ^ result) & (b ^ result) & 0x80000000) != 0;
}

static inline bool sub_overflow_check(uint32_t a, uint32_t b, uint32_t result) {
    return ((a ^ b) & (a ^ result) & 0x80000000) != 0;
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

OP_I_RS(addiu, rs + (int16_t)imm)
OP_I_RS(andi, rs & imm)
OP_I_RS(ori, rs | imm)
OP_I_RS(xori, rs ^ imm)
OP_I_RS(slti, (int32_t)rs < (int16_t)imm ? 1 : 0)
OP_I_RS(sltiu, rs < (uint32_t)(int16_t)imm ? 1 : 0)
OP_I(lui, imm << 16)
OP_I_BRANCH_RT(beq, rs == rt)
OP_I_BRANCH_RT(bne, rs != rt)
OP_I_BRANCH_Z(bgez, (int32_t)rs >= 0)
OP_I_BRANCH_Z(bgtz, (int32_t)rs > 0)
OP_I_BRANCH_Z(blez, (int32_t)rs <= 0)
OP_I_BRANCH_Z(bltz, (int32_t)rs < 0)
OP_I_BRANCH_Z_LINK(bgezal, (int32_t)rs >= 0)
OP_I_BRANCH_Z_LINK(bltzal, (int32_t)rs < 0)

static inline void execute_jr(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t target = reg_read(cpu, RS(cpu));
    cpu->next_branch_target = target;
    cpu->next_branch_state = BRANCH_STATE_TAKEN;
}

static inline void execute_jalr(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    execute_jr(psx); // Jump to target address
    reg_write(cpu, RD(cpu), get_next_pc(cpu) + 4); // Save return address
}

static inline void execute_mfhi(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    reg_write(cpu, RD(cpu), cpu->hi);
}

static inline void execute_mthi(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    cpu->hi = reg_read(cpu, RS(cpu));
}

static inline void execute_mflo(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    reg_write(cpu, RD(cpu), cpu->lo);
}

static inline void execute_mtlo(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    cpu->lo = reg_read(cpu, RS(cpu));
}

static inline void execute_mult(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    int64_t product = (int64_t)(int32_t)reg_read(cpu, RS(cpu)) * (int64_t)(int32_t)reg_read(cpu, RT(cpu));
    cpu->hi = (uint32_t)(product >> 32);
    cpu->lo = (uint32_t)(product & 0xFFFFFFFF);
}

static inline void execute_multu(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint64_t product = (uint64_t)reg_read(cpu, RS(cpu)) * (uint64_t)reg_read(cpu, RT(cpu));
    cpu->hi = (uint32_t)(product >> 32);
    cpu->lo = (uint32_t)(product & 0xFFFFFFFF);
}

static inline void execute_div(PSX *psx) {
    Cpu *cpu = &psx->cpu;
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

static inline void execute_divu(PSX *psx) {
    Cpu *cpu = &psx->cpu;
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

static inline void execute_add(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t rs = reg_read(cpu, RS(cpu));
    uint32_t rt = reg_read(cpu, RT(cpu));
    uint32_t result = rs + rt;
    // Check for signed overflow
    if (add_overflow_check(rs, rt, result)) {
        cpu->next_exc_code = EXC_OV; // Set exception code for overflow
    } else {
        reg_write(cpu, RD(cpu), result);
    }
}

static inline void execute_sub(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t rs = reg_read(cpu, RS(cpu));
    uint32_t rt = reg_read(cpu, RT(cpu));
    uint32_t result = rs - rt;
    // Check for signed overflow
    if (sub_overflow_check(rs, rt, result)) {
        cpu->next_exc_code = EXC_OV; // Set exception code for overflow
    } else {
        reg_write(cpu, RD(cpu), result);
    }
}

static inline void execute_j(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t pc = get_next_pc(cpu);
    uint32_t target = (pc & 0xF0000000) | (TARGET(cpu) << 2);
    cpu->next_branch_target = target;
    cpu->next_branch_state = BRANCH_STATE_TAKEN;
}

static inline void execute_jal(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t pc = get_next_pc(cpu);
    uint32_t target = (pc & 0xF0000000) | (TARGET(cpu) << 2);
    reg_write(cpu, 31, pc + 4); // Save return address in $ra
    cpu->next_branch_target = target;
    cpu->next_branch_state = BRANCH_STATE_TAKEN;
}


static inline void execute_addi(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t rs = reg_read(cpu, RS(cpu));
    int16_t simm = SIMM(cpu);
    uint32_t result = rs + simm;
    // Check for signed overflow
    if (add_overflow_check(rs, simm, result)) {
        cpu->next_exc_code = EXC_OV; // Set exception code for overflow
    } else {
        reg_write(cpu, RT(cpu), result);
    }
}

static inline void execute_lb(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t base = reg_read(cpu, RS(cpu));
    int16_t offset = SIMM(cpu);
    uint32_t addr = base + offset;
    int32_t value = (int32_t)(int8_t)bus_read8(psx, &psx->bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lbu(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t base = reg_read(cpu, RS(cpu));
    int16_t offset = SIMM(cpu);
    uint32_t addr = base + offset;
    uint32_t value = bus_read8(psx, &psx->bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lh(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t base = reg_read(cpu, RS(cpu));
    int16_t offset = SIMM(cpu);
    uint32_t addr = base + offset;
    if (unlikely(addr & 1)) {
        cpu->next_exc_code = EXC_ADEL; // Address error load/fetch
        return;
    }
    int32_t value = (int32_t)(int16_t)bus_read16(psx, &psx->bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lhu(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t addr = get_addr_from_imm(cpu);
    if (unlikely(addr & 1)) {
        cpu->next_exc_code = EXC_ADEL; // Address error load/fetch
        return;
    }
    uint32_t value = bus_read16(psx, &psx->bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lw(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t addr = get_addr_from_imm(cpu);
    if (unlikely(addr & 3)) {
        cpu->next_exc_code = EXC_ADEL; // Address error load/fetch
        return;
    }
    uint32_t value = bus_read32(psx, &psx->bus, addr);
    schedule_load(cpu, RT(cpu), value);
}

static inline void execute_lwl(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t addr = get_addr_from_imm(cpu);
    uint32_t aligned_addr = addr & ~3; // Align address to 4 bytes
    uint32_t result;

    switch (addr & 3) {
        case 0: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint8_t mem_value = bus_read8(psx, &psx->bus, aligned_addr);
            result = (reg_value & 0x00FFFFFF) | (mem_value << 24);
            break;
        }
        case 1: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint16_t mem_value = bus_read16(psx, &psx->bus, aligned_addr);
            result = (reg_value & 0x0000FFFF) | (mem_value << 16);
            break;
        }
        case 2: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint16_t mem_value_16 = bus_read16(psx, &psx->bus, aligned_addr);
            uint8_t mem_value_8 = bus_read8(psx, &psx->bus, addr);
            result = (reg_value & 0x000000FF) | (mem_value_16 << 8) | (mem_value_8 << 24);
            break;
        }
        case 3: {
            result = bus_read32(psx, &psx->bus, aligned_addr);
            break;
        }
        default:
            result = 0; // This case should never happen
            break;
    }

    schedule_load(cpu, RT(cpu), result);
}

static inline void execute_lwr(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t addr = get_addr_from_imm(cpu);
    uint32_t result;

    switch (addr & 3) {
        case 0:
            result = bus_read32(psx, &psx->bus, addr);
            break;
        case 1: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint8_t mem_value_8 = bus_read8(psx, &psx->bus, addr);
            uint16_t mem_value_16 = bus_read16(psx, &psx->bus, addr + 1);
            result = (reg_value & 0xFF000000) | (mem_value_16 << 8) | mem_value_8;
            break;
        }
        case 2: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint16_t mem_value_16 = bus_read16(psx, &psx->bus, addr);
            result = (reg_value & 0xFFFF0000) | mem_value_16;
            break;
        }
        case 3: {
            uint32_t reg_value = reg_read_with_load(cpu, RT(cpu));
            uint8_t mem_value_8 = bus_read8(psx, &psx->bus, addr);
            result = (reg_value & 0xFFFFFF00) | mem_value_8;
            break;
        }
        default:
            result = 0; // This case should never happen
            break;
    }

    schedule_load(cpu, RT(cpu), result);
}

static inline void execute_sb(PSX *psx) {
    if (unlikely(psx->cop0.cache_isolated)) {
        return;
    }

    Cpu *cpu = &psx->cpu;
    uint32_t addr = get_addr_from_imm(cpu);
    uint8_t value = (uint8_t)reg_read(cpu, RT(cpu));
    bus_write8(psx, &psx->bus, addr, value);
}

static inline void execute_sh(PSX *psx) {
    if (unlikely(psx->cop0.cache_isolated)) {
        return;
    }

    Cpu *cpu = &psx->cpu;
    uint32_t addr = get_addr_from_imm(cpu);
    if (unlikely(addr & 1)) {
        cpu->next_exc_code = EXC_ADES; // Address error store
        return;
    }

    uint16_t value = (uint16_t)reg_read(cpu, RT(cpu));
    bus_write16(psx, &psx->bus, addr, value);
}

static inline void execute_sw(PSX *psx) {
    if (unlikely(psx->cop0.cache_isolated)) {
        return;
    }

    Cpu *cpu = &psx->cpu;
    uint32_t addr = get_addr_from_imm(cpu);
    if (unlikely(addr & 3)) {
        cpu->next_exc_code = EXC_ADES; // Address error store
        return;
    }

    uint32_t value = reg_read(cpu, RT(cpu));
    bus_write32(psx, &psx->bus, addr, value);
}

static inline void execute_swl(PSX *psx) {
    if (unlikely(psx->cop0.cache_isolated)) {
        return;
    }

    Cpu *cpu = &psx->cpu;
    uint32_t addr = get_addr_from_imm(cpu);
    uint32_t aligned_addr = addr & ~3; // Align address to 4 bytes
    uint32_t value = reg_read(cpu, RT(cpu));

    switch (addr & 3) {
        case 0:
            bus_write8(psx, &psx->bus, aligned_addr, value >> 24);
            break;
        case 1:
            bus_write16(psx, &psx->bus, aligned_addr, value >> 16);
            break;
        case 2:
            bus_write16(psx, &psx->bus, aligned_addr, value >> 8);
            bus_write8(psx, &psx->bus, addr, (value >> 24) & 0xFF);
            break;
        case 3:
            bus_write32(psx, &psx->bus, aligned_addr, value);
            break;
    }
}

static inline void execute_swr(PSX *psx) {
    if (unlikely(psx->cop0.cache_isolated)) {
        return;
    }

    Cpu *cpu = &psx->cpu;
    uint32_t addr = get_addr_from_imm(cpu);
    uint32_t value = reg_read(cpu, RT(cpu));

    switch (addr & 3) {
        case 0:
            bus_write32(psx, &psx->bus, addr, value);
            break;
        case 1:
            bus_write8(psx, &psx->bus, addr, value);
            bus_write16(psx, &psx->bus, addr + 1, value >> 8);
            break;
        case 2:
            bus_write16(psx, &psx->bus, addr, value);
            break;
        case 3:
            bus_write8(psx, &psx->bus, addr, value);
            break;
        default:
            // This case should never happen
            break;
    }
}

static inline void execute_cop0(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    switch (RS(cpu)) {
        case 0x00: { // MFC0
            uint32_t value = cop0_read(&psx->cop0, RD(cpu));
            schedule_load(cpu, RT(cpu), value);
            break;
        }
        case 0x04: { // MTC0
            uint32_t value = reg_read(cpu, RT(cpu));
            cop0_write(&psx->cop0, RD(cpu), value);
            break;
        }
        case 0x10: // RFE
            cop0_rfe(&psx->cop0);
            break;
        default:
            cpu->next_exc_code = EXC_RI; // Reserved instruction exception
            break;
    }
}

static inline void execute_cop2(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    // Placeholder for COP2 instructions (e.g., GTE)
    cpu->next_exc_code = EXC_RI; // Reserved instruction exception
}

static inline void execute_bcond(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint8_t rt = RT(cpu);
    if (rt == 0x10) {
        execute_bltzal(psx);
    } else if (rt == 0x11) {
        execute_bgezal(psx);
    } else if (rt & 0x1) {
        execute_bgez(psx);
    } else {
        execute_bltz(psx);
    }
}

static inline uint32_t fetch_instruction(PSX *psx, uint32_t *fetch_page, uint8_t **fetch_page_ptr) {
    #ifdef PSXY_SINGLE_STEP_TEST_MODE
    return bus_fetch32(psx, &psx->bus, psx->cpu.pc);
    #endif

    uint32_t pc = psx->cpu.pc;
    uint32_t page = get_page_index(pc);

    if (page != *fetch_page) {
        *fetch_page_ptr = psx->bus.read_pages[page];
        *fetch_page = page;
    }

    if (unlikely(*fetch_page_ptr == NULL)) {
        // Fallback to bus fetch if page pointer is NULL, unlikely to happen in normal operation
        return bus_fetch32(psx, &psx->bus, pc);
    }

    uint32_t inst;
    uint32_t offset = pc & BUS_PAGE_MASK;
    memcpy(&inst, *fetch_page_ptr + offset, sizeof(uint32_t));
    return inst;
}

static inline void begin_step(Cpu *cpu) {
    cpu->next_branch_state = BRANCH_STATE_NO_DELAY;
    cpu->next_load_reg = 0;

#ifdef PSXY_SINGLE_STEP_TEST_MODE
    // only next_branch_state and next_load_reg are required to be cleared,
    // but single step tests expect next_branch_target and next_load_value to be cleared as well
    cpu->next_branch_target = 0;
    cpu->next_load_value = 0;
#endif
}

static inline void check_interrupts(PSX *psx) {
    bool pending = cop0_interrupts_pending(&psx->cop0);
    if (pending) {
        psx->cpu.next_exc_code = EXC_INT; // Set exception code for interrupt
    }
}

static inline void finish_step(PSX *psx) {
    Cpu *cpu = &psx->cpu;
    commit_load(cpu); // Commit any scheduled load after executing the instruction

    if (unlikely(cpu->next_exc_code != EXC_NONE)) {
        cop0_raise_exception(psx, cpu->next_exc_code);
        return;
    }

    cpu->pc = get_next_pc(cpu);
    cpu->next_pc = cpu->pc + 4;
    cpu->branch_state = cpu->next_branch_state;

#ifdef PSXY_SINGLE_STEP_TEST_MODE
    // no need to set branch_target if not in delay slot, but single step tests expect it to be set
    cpu->branch_target = cpu->next_branch_target;
#else
    if (IS_IN_DELAY_SLOT(cpu->branch_state)) {
        cpu->branch_target = cpu->next_branch_target;
    }
#endif

    check_interrupts(psx); // Check for any pending interrupts after instruction execution
}

uint32_t cpu_step(PSX *psx) {
    return cpu_run(psx, 1);
}

uint32_t cpu_run(PSX *psx, uint32_t cycles) {
    Cpu *cpu = &psx->cpu;
    uint32_t fetch_page = cpu->fetch_page;
    uint8_t *fetch_page_ptr = cpu->fetch_page_ptr;

    // Use the computed goto technique for faster instruction dispatch
    static void *opcode_table[64] = {
        [1 ... 63] = &&label_invalid,
        [0] = &&label_special,
        #define X(opcode, name) [opcode] = &&label_##name,
        OPCODE_TABLE
        #undef X
    };

    static void *funct_table[64] = {
        [0 ... 63] = &&label_invalid,
        #define X(funct, name) [funct] = &&label_##name,
        FUNCT_TABLE
        #undef X
    };

label_fetch:
    if (cycles-- == 0) {
        return cpu->pc;
    }

    // tty_maybe_putchar(&psx->tty, cpu);
    begin_step(cpu);

    if (unlikely(cpu->pc & 3)) {
        cpu->next_exc_code = EXC_ADEL; // Address error load/fetch
        goto label_finish;
    }

    cpu->inst = fetch_instruction(psx, &fetch_page, &fetch_page_ptr);

    if (cpu->inst == 0) {
        goto label_finish; // Skip execution for NOP
    }

    // Dispatch to the appropriate instruction handler using computed goto
    goto *opcode_table[OPCODE(cpu)];

label_special:
    goto *funct_table[FUNCT(cpu)];

#define X(opcode, name) \
label_##name: \
    execute_##name(psx); \
    goto label_finish;
OPCODE_TABLE
#undef X

#define X(funct, name) \
label_##name: \
    execute_##name(psx); \
    goto label_finish;
FUNCT_TABLE
#undef X

label_invalid:
    cpu->next_exc_code = EXC_RI; // Reserved instruction exception
    goto label_finish;

label_finish:
    finish_step(psx);
    goto label_fetch;
}

void cpu_reset(Cpu *cpu) {
    if (!cpu) return;
    memset(cpu, 0, sizeof(Cpu)); // Clear CPU state
    memset(cpu->r, 0, sizeof(cpu->r)); // Clear general-purpose registers
    cpu->pc = 0xBFC00000; // Reset vector
    cpu->next_pc = cpu->pc + 4;
    cpu->next_exc_code = EXC_NONE;
    cpu->fetch_page = BUS_PAGE_COUNT; // Invalidate fetch page
    cpu->fetch_page_ptr = NULL;
}
