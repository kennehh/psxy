#include "exceptions.h"

#define CAUSE_IP_MASK 0x0000FF00 // Mask for the interrupt pending bits in the Cause register
#define CAUSE_BT_MASK 0x40000000 // Mask for the branch delay bit in the Cause register
#define CAUSE_BD_MASK 0x80000000 // Mask for the branch delay bit in the Cause register


uint32_t raise_exception(Cpu *cpu, uint8_t exc_code) {

    uint32_t cause = cpu->cop0.cause & CAUSE_IP_MASK; // preserve the interrupt pending bits
    cause |= exc_code << 2; // set the exception code

    if (exc_code != EXC_IBE && exc_code != EXC_DBE) {
        cause |= ((cpu->inst >> 26) & 0x3) << 28; // set the coprocessor number bits based on opcode
    }

    cpu->cop0.epc = cpu->pc; // Save the current program counter to the EPC register
    if (cpu->in_delay_slot) {
        cause |= CAUSE_BD_MASK; // Set the branch delay bit if the exception occurred in a delay slot
        cpu->cop0.tar = cpu->branch_target; // Save the branch target address to the TAR register
        cpu->cop0.epc -= 4; // Adjust the EPC to point to the instruction before the delay slot

        if (cpu->branch_taken) {
            cause |= CAUSE_BT_MASK; // Set the branch taken bit if a branch was taken
        }
    }

    if (exc_code == EXC_ADEL || exc_code == EXC_ADES) {
        cpu->cop0.badAddr = cpu->pc; // Save the bad virtual address for address errors
    }

    uint32_t stat = cpu->cop0.status;
    cpu->cop0.status = (stat & ~0x3F) | ((stat & 0x3F) << 2); // Shift the current interrupt mask and mode bits left by 2
    cpu->cop0.cause = cause; // Update the Cause register with the new value

    // clear pending loads and delay slot flags
    cpu->next_load_reg = 0;
    cpu->next_load_value = 0;
    cpu->next_delay_slot = false;
    cpu->next_branch_taken = false;
    cpu->next_branch_target = 0;
    cpu->next_exc_code = 0; // Clear the next exception code after handling

    return 0x80000080; // Return the address of the exception handler
}
