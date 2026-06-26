#include "exc.h"

#define CAUSE_IP_MASK 0x0000FF00 // Mask for the interrupt pending bits in the Cause register
#define CAUSE_BT_MASK 0x40000000 // Mask for the branch delay bit in the Cause register
#define CAUSE_BD_MASK 0x80000000 // Mask for the branch delay bit in the Cause register


uint32_t raise_exception(Cpu *cpu, uint8_t exc_code) {
    cpu->cop0.cause.raw &= CAUSE_IP_MASK; // preserve the interrupt pending bits
    cpu->cop0.cause.fields.exc_code = exc_code;

    if (exc_code != EXC_IBE && exc_code != EXC_DBE) {
        // set coprocessor bits for all exceptions except for bus errors
        cpu->cop0.cause.fields.ce = 0b11;
    }

    cpu->cop0.epc = cpu->pc; // Save the current program counter to the EPC register
    if (cpu->in_delay_slot) {
        cpu->cop0.cause.fields.bd = 1; // Set the branch delay bit if the exception occurred in a delay slot
        cpu->cop0.epc -= 4; // Adjust the EPC to point to the instruction before the delay slot
    }

    if (exc_code == EXC_ADEL || exc_code == EXC_ADES) {
        cpu->cop0.badAddr = cpu->pc; // Save the bad virtual address for address errors
    }

    uint32_t stat = cpu->cop0.status;
    cpu->cop0.status = (stat & ~0x3F) | ((stat & 0x3F) << 2); // Shift the current interrupt mask and mode bits left by 2

    // clear pending loads and delay slot flags
    cpu->next_load_reg = 0;
    cpu->next_load_value = 0;
    cpu->next_delay_slot = false;
    cpu->next_branch_taken = false;
    cpu->next_branch_target = 0;
    cpu->next_exc_code = 0; // Clear the next exception code after handling

    return 0x80000080; // Return the address of the exception handler
}
