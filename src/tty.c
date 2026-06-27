#include "tty.h"
#include <stdio.h>

TTY *tty_create() {
    TTY *tty = (TTY *)malloc(sizeof(TTY));
    if (!tty) {
        fprintf(stderr, "Failed to allocate TTY structure\n");
        exit(EXIT_FAILURE);
    }
    tty->buffer_index = 0;
    tty->strlen = 0;
    tty->buffer[0] = '\0';
    tty->message[0] = '\0';
    return tty;
}

void tty_destroy(TTY *tty) {
    if (!tty) return;
    free(tty);
}

void tty_reset(TTY *tty) {
    tty->buffer_index = 0;
    tty->strlen = 0;
    tty->buffer[0] = '\0';
    tty->message[0] = '\0';
}

void tty_putchar(TTY *tty, Cpu *cpu) {
    char c = (char)(cpu->r[4] & 0xFF);
    if (tty->buffer_index < TTY_BUFFER_SIZE - 1) {
        tty->buffer[tty->buffer_index++] = c;
        tty->buffer[tty->buffer_index] = '\0';
    }
    if (c == '\n' || tty->buffer_index >= TTY_BUFFER_SIZE - 1) {
        snprintf(tty->message, TTY_BUFFER_SIZE, "%s", tty->buffer);
        printf("%s", tty->message);
        tty->buffer_index = 0;
        tty->buffer[0] = '\0';
    }
}

void tty_maybe_putchar(TTY *tty, Cpu *cpu) {
    if (cpu->pc == 0xA0) {
        uint8_t func_code = cpu->r[9] & 0xFF;
        if (func_code == 0x3C) {
            tty_putchar(tty, cpu);
        }
        return;
    }
    if (cpu->pc == 0xB0) {
        uint8_t func_code = cpu->r[9] & 0xFF;
        if (func_code == 0x3D) {
            tty_putchar(tty, cpu);
        }
        return;
    }
}
