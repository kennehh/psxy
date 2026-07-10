#include "psx.h"
#include "cop0.h"

#define CAUSE_IP_MASK 0x0000FF00 // Mask for the interrupt pending bits in the Cause register
#define CAUSE_BT_BIT 0x40000000 // Bit for the branch taken bit in the Cause register
#define CAUSE_BD_BIT 0x80000000 // Bit for the branch delay bit in the Cause register
#define CAUSE_IP_BIT 0x00008000 // Bit in the Cause register indicating an interrupt is pending

static inline void cop0_set_pending_interrupts(Cop0 *cop0) {
    uint32_t status = cop0->status;
    if (!(status & 0x1)) { // Check if interrupts are enabled
        cop0->pending_interrupts = 0; // Clear pending interrupts if interrupts are disabled
        return;
    }

    uint32_t pending = cop0->cause & status & 0xFF00; // Check for pending interrupts
    cop0->pending_interrupts = pending ? 1 : 0;
}

static inline void cop0_set_status(Cop0 *cop0, uint32_t value) {
    cop0->status = value;
    cop0->cache_isolated = (value & 0x00010000) ? 1 : 0;
    cop0_set_pending_interrupts(cop0); // Update pending interrupts based on the new status
}

void cop0_raise_exception(PSX *psx, uint8_t exc_code) {
    Cpu *cpu = &psx->cpu;
    Cop0 *cop0 = &psx->cop0;
    cop0->cause &= CAUSE_IP_MASK; // preserve the interrupt pending bits
    cop0->cause |= exc_code << 2; // set the exception code

    if (exc_code != EXC_IBE && exc_code != EXC_DBE) {
        cop0->cause |= ((cpu->inst >> 26) & 0x3) << 28; // set the coprocessor number bits based on opcode
    }

    if (IS_IN_DELAY_SLOT(cpu->branch_state)) {
        cop0->cause |= CAUSE_BD_BIT; // Set the branch delay bit if the exception occurred in a delay slot
        cop0->tar = cpu->branch_target; // Save the branch target address to the TAR register
        cop0->epc = cpu->pc - 4; // Save the program counter of the instruction in the delay slot to the EPC register

        if (IS_BRANCH_TAKEN(cpu->branch_state)) {
            cop0->cause |= CAUSE_BT_BIT; // Set the branch taken bit if a branch was taken
        }
    } else {
        cop0->epc = cpu->pc; // Save the current program counter to the EPC register
    }

    if (exc_code == EXC_ADEL || exc_code == EXC_ADES) {
        cop0->badAddr = cpu->pc; // Save the bad virtual address for address errors
    }

    uint32_t status = cop0->status;
    status = (status & ~0x3F) | ((status << 2) & 0x3F); // Shift the current interrupt mask and mode bits left by 2
    cop0_set_status(cop0, status);

    // reset CPU state for exception handling
    #ifdef PSXY_SINGLE_STEP_TEST_MODE
    cpu->branch_target = 0; // not necessary to clear in normal operation, but single step tests expect this to be cleared
    #endif
    cpu->branch_state = BRANCH_STATE_NO_DELAY;
    cpu->pc = 0x80000080; // Set the program counter to the exception handler address
    cpu->next_pc = 0x80000084;
    cpu->next_exc_code = EXC_NONE;
    cop0->pending_interrupts = 0; // Clear the pending interrupts flag after raising an exception
}

void cop0_write(Cop0 *cop0, uint8_t rd, uint32_t value) {
    switch (rd) {
        case 3:  cop0->bpc = value;  break;
        case 5:  cop0->bda = value;  break;
        case 6:  cop0->tar = value;  break;
        case 7:  cop0->dcic = value; break;
        case 9:  cop0->bdam = value; break;
        case 12: cop0_set_status(cop0, value); break;
    }
}

uint32_t cop0_read(Cop0 *cop0, uint8_t rd) {
    switch (rd) {
        case 3:  return cop0->bpc;
        case 5:  return cop0->bda;
        case 6:  return cop0->tar;
        case 7:  return cop0->dcic;
        case 8:  return cop0->badAddr;
        case 12: return cop0->status;
        case 13: return cop0->cause;
        case 14: return cop0->epc;
        case 15: return cop0->prid;
        default: return 0; // Return zero for unimplemented registers
    }
}

void cop0_rfe(Cop0 *cop0) {
    uint32_t stat = cop0->status;
    cop0->status = (stat & ~0x0F) | ((stat >> 2) & 0x0F);
    cop0_set_pending_interrupts(cop0); // Update pending interrupts based on the new status
}

void cop0_reset(Cop0 *cop0) {
    if (!cop0) return;
    memset(cop0, 0, sizeof(Cop0)); // Clear COP0 registers
    cop0->prid = 2; // Set the Processor Revision ID to 2 (for PlayStation)
}

void cop0_update_interrupts(PSX *psx) {
    Irq *irq = &psx->irq;
    Cop0 *cop0 = &psx->cop0;
    uint16_t pending = irq->stat & irq->mask;

    if (pending) {
        cop0->cause |= CAUSE_IP_BIT; // Set the interrupt pending bit
        cop0->pending_interrupts = (cop0->status & 0x1) ? 1 : 0;
    } else {
        cop0->cause &= ~CAUSE_IP_BIT; // Clear the interrupt pending bit
        cop0->pending_interrupts = 0;
    }
}
