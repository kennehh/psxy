#ifndef TTY_H
#define TTY_H

#include <stdlib.h>
#include "cpu.h"

#define TTY_BUFFER_SIZE 128

typedef struct {
    char buffer[TTY_BUFFER_SIZE];
    size_t buffer_index;
    size_t strlen;
} TTY;

TTY *tty_create(void);
void tty_destroy(TTY *tty);
void tty_reset(TTY *tty);
void tty_maybe_putchar(TTY *tty, Cpu *cpu);
void tty_maybe_printf(TTY *tty, Cpu *cpu, Bus* bus);

#endif // TTY_H
