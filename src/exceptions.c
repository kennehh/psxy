#include "psx.h"
#include "exceptions.h"

#define CAUSE_IP_MASK 0x0000FF00 // Mask for the interrupt pending bits in the Cause register
#define CAUSE_BT_MASK 0x40000000 // Mask for the branch delay bit in the Cause register
#define CAUSE_BD_MASK 0x80000000 // Mask for the branch delay bit in the Cause register

void raise_exception(PSX *psx, uint8_t exc_code) {
    Cpu *cpu = &psx->cpu;
    cpu->cop0.cause &= CAUSE_IP_MASK; // preserve the interrupt pending bits
    cpu->cop0.cause |= exc_code << 2; // set the exception code

    if (exc_code != EXC_IBE && exc_code != EXC_DBE) {
        cpu->cop0.cause |= ((cpu->inst >> 26) & 0x3) << 28; // set the coprocessor number bits based on opcode
    }


    if (IS_IN_DELAY_SLOT(cpu->branch_state)) {
        cpu->cop0.cause |= CAUSE_BD_MASK; // Set the branch delay bit if the exception occurred in a delay slot
        cpu->cop0.tar = cpu->branch_target; // Save the branch target address to the TAR register
        cpu->cop0.epc = cpu->pc - 4; // Save the program counter of the instruction in the delay slot to the EPC register

        if (IS_BRANCH_TAKEN(cpu->branch_state)) {
            cpu->cop0.cause |= CAUSE_BT_MASK; // Set the branch taken bit if a branch was taken
        }
    } else {
        cpu->cop0.epc = cpu->pc; // Save the current program counter to the EPC register
    }

    if (exc_code == EXC_ADEL || exc_code == EXC_ADES) {
        cpu->cop0.badAddr = cpu->pc; // Save the bad virtual address for address errors
    }

    uint32_t status = cpu->cop0.status;
    status = (status & ~0x3F) | ((status << 2) & 0x3F); // Shift the current interrupt mask and mode bits left by 2
    set_cop0_status(cpu, status);

    // reset CPU state for exception handling
    #ifdef SINGLE_STEP_TEST_MODE
    cpu->branch_target = 0; // not necessary to clear in normal operation, but single step tests expect this to be cleared
    #endif
    cpu->branch_state = BRANCH_STATE_NO_DELAY;
    cpu->pc = 0x80000080; // Set the program counter to the exception handler address
    cpu->next_pc = 0x80000084;
    cpu->next_exc_code = EXC_NONE;
}
