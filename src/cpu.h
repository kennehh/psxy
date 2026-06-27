#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "bus.h"

typedef struct {
    uint32_t bpc; // Breakpoint Program Counter
    uint32_t bda; // Breakpoint Data Address
    uint32_t tar; // Target Address
    uint32_t dcic; // Debug and Cache Invalidate Control
    uint32_t badAddr; // Bad Address
    uint32_t bdam; // Breakpoint Data Address Mask
    uint32_t status; // Status Register
    uint32_t cause; // Cause of last exception
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

Cpu* cpu_create();
void cpu_destroy(Cpu *cpu);

uint32_t cpu_step(Cpu *cpu, Bus *bus);

#endif // CPU_H
