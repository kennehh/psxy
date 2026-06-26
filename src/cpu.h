#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "bus.h"

typedef union {
    uint32_t raw;

    struct {
        uint8_t unused1 : 2;   // 0-1: Unused bits
        uint8_t exc_code : 5;  // 2-6: Exception code
        uint8_t unused2 : 1;   // 7: Unused bit
        uint8_t ip : 8;        // 8-15: Interrupt pending bits
        uint16_t unused3 : 12; // 16-27: Unused bits
        uint8_t ce : 2;        // 28-29: Coprocessor error
        uint8_t unused4 : 1;   // 30: Unused bit
        uint8_t bd : 1;        // 31: Branch delay bit
    } fields;
} CauseRegister;

typedef struct {
    uint32_t bpc; // Breakpoint Program Counter
    uint32_t bda; // Breakpoint Data Address
    uint32_t tar; // Target Address
    uint32_t dcic; // Debug and Cache Invalidate Control
    uint32_t badAddr; // Bad Address
    uint32_t bdam; // Breakpoint Data Address Mask
    uint32_t status; // Status Register
    CauseRegister cause; // Cause of last exception
    uint32_t epc; // Exception Program Counter
    uint32_t prid; // Processor Revision ID
} Cop0;

typedef struct {
    uint32_t pc; // program counter
    uint32_t next_pc; // next program counter

    uint32_t r[32]; // general purpose registers
    uint32_t hi; // high register
    uint32_t lo; // low register

    uint8_t load_reg; // register to load after delay slot
    uint32_t load_value; // value to load after delay slot
    uint8_t next_load_reg; // next register to load after delay slot
    uint32_t next_load_value; // next value to load after delay slot

    bool in_delay_slot; // flag to indicate if the CPU is in a delay slot
    bool branch_taken; // flag to indicate if a branch was taken
    uint32_t branch_target; // target address of the branch

    bool next_delay_slot; // flag to indicate if the next instruction is in a delay slot
    bool next_branch_taken; // flag to indicate if the next branch was taken
    uint32_t next_branch_target; // target address of the next branch

    uint32_t next_exc_code; // next exception code to raise

    Cop0 cop0; // coprocessor 0 state

    uint32_t inst; // current instruction
} Cpu;

Cpu* create_cpu();
void destroy_cpu(Cpu *cpu);

uint32_t cpu_step(Cpu *cpu, Bus *bus);

#endif // CPU_H
