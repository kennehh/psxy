#include "tty.h"
#include <stdio.h>

TTY *tty_create(void) {
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
    uint16_t pc_func = ((cpu->pc & 0xFF) << 8) | (cpu->r[9] & 0xFF);
    switch (pc_func) {
        case 0xA03C: // PC = 0xA0, func_code = 0x3C
        case 0xB03D: // PC = 0xB0, func_code = 0x3D
            tty_putchar(tty, cpu);
            break;
        default:
            break;
    }
}
