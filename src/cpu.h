#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#define BRANCH_STATE_NO_DELAY 0b00
#define BRANCH_STATE_IN_DELAY_SLOT 0b01
#define BRANCH_STATE_TAKEN 0b11

#define IS_BRANCH_TAKEN(state) ((state) == BRANCH_STATE_TAKEN)
#define IS_IN_DELAY_SLOT(state) ((state) & BRANCH_STATE_IN_DELAY_SLOT)
#define IS_NO_DELAY(state) ((state) == BRANCH_STATE_NO_DELAY)

typedef struct Cpu {
    uint32_t fetch_page; // current page for instruction fetch
    uint8_t *fetch_page_ptr; // pointer to the current page for instruction fetch

    uint32_t pc; // program counter
    uint32_t next_pc; // next program counter
    uint32_t r[32]; // general purpose registers
    uint32_t hi; // high register
    uint32_t lo; // low register

    uint32_t inst; // current instruction

    uint8_t load_reg; // register to load after delay slot
    uint8_t next_load_reg; // next register to load after delay slot
    uint32_t load_value; // value to load after delay slot

    uint32_t next_load_value; // next value to load after delay slot

    uint8_t branch_state; // state of the branch (0b00: no delay, 0b01: in delay slot, 0b11: branch taken)
    uint32_t branch_target; // target address of the branch

    uint8_t next_branch_state; // next state of the branch (0b00: no delay, 0b01: in delay slot, 0b11: branch taken)
    uint32_t next_branch_target; // target address of the next branch

    uint8_t next_exc_code; // next exception code to raise
} Cpu;

void cpu_reset(Cpu *cpu);

uint32_t cpu_step(PSX *psx);
uint32_t cpu_run(PSX *psx, uint32_t cycles);

#endif // CPU_H
