#ifndef TTY_H
#define TTY_H

#include "cpu.h"
#include <stdlib.h>

#define TTY_BUFFER_SIZE 256

typedef struct {
    char buffer[TTY_BUFFER_SIZE];
    size_t buffer_index;
    size_t strlen;
} TTY;

TTY *tty_create(void);
void tty_destroy(TTY *tty);
void tty_reset(TTY *tty);
void tty_putchar(TTY *tty, Cpu *cpu);
void tty_maybe_putchar(TTY *tty, Cpu *cpu);

#endif // TTY_H
