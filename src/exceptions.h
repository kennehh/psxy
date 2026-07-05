#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdint.h>
#include "common.h"

#define EXC_NONE 0xFF // No exception
#define EXC_INT  0x00 // Interrupt
#define EXC_MOD  0x01 // TLB modification
#define EXC_TLBL 0x02 // TLB load/fetch
#define EXC_TLBS 0x03 // TLB store
#define EXC_ADEL 0x04 // Address error load/fetch
#define EXC_ADES 0x05 // Address error store
#define EXC_IBE  0x06 // Bus error instruction fetch
#define EXC_DBE  0x07 // Bus error data load/store
#define EXC_SYS  0x08 // Syscall
#define EXC_BP   0x09 // Breakpoint
#define EXC_RI   0x0A // Reserved instruction
#define EXC_CPU  0x0B // Coprocessor unusable
#define EXC_OV   0x0C // Arithmetic overflow
#define EXC_TR   0x0D // Trap

void raise_exception(PSX *psx, uint8_t exc_code);

#endif // EXCEPTIONS_H
