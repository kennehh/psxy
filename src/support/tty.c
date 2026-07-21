#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "support/tty.h"
#include "cpu/cpu.h"
#include "core/psx.h"
#include "memory/bus.h"

void tty_reset(TTY *tty) {
    tty->buffer_index = 0;
    tty->strlen = 0;
    tty->buffer[0] = '\0';
}

static inline uint32_t get_arg_value(PSX *psx, uint8_t arg_idx) {
    Cpu *cpu = &psx->cpu;
    if (arg_idx < 4) {
        // R4, R5, R6, R7
        return cpu->r[4 + arg_idx];
    }

    // [SP+10h...]
    uint32_t sp = cpu->r[29];
    uint32_t arg_addr = sp + 0x10 + ((arg_idx - 4) << 2);
    return bus_read32(psx, &psx->bus, arg_addr);
}

static inline void tty_putchar(TTY *tty, char c) {
    if (tty->buffer_index < TTY_BUFFER_SIZE - 1) {
        tty->buffer[tty->buffer_index++] = c;
        tty->buffer[tty->buffer_index] = '\0';
    }
    if (c == '\n' || tty->buffer_index >= TTY_BUFFER_SIZE - 1) {
        printf("%s", tty->buffer);
        tty->buffer_index = 0;
        tty->buffer[0] = '\0';
    }
}

static inline char* arg_number(TTY *tty, PSX *psx, uint8_t arg_idx, char specifier) {
    uint32_t val = get_arg_value(psx, arg_idx);
    static char num_buf[TTY_BUFFER_SIZE];

    switch (specifier) {
        case 'd':
        case 'i':
            snprintf(num_buf, sizeof(num_buf), "%d", (int32_t)val);
            break;
        case 'u':
            snprintf(num_buf, sizeof(num_buf), "%u", val);
            break;
        case 'x':
            snprintf(num_buf, sizeof(num_buf), "%x", val);
            break;
        case 'X':
            snprintf(num_buf, sizeof(num_buf), "%X", val);
            break;
        default:
            return "";
    }

    return num_buf;
}

static inline char* arg_string(TTY *tty, PSX *psx, uint8_t arg_idx, size_t max_length) {
    uint32_t str_addr = get_arg_value(psx, arg_idx);
    static char buffer[TTY_BUFFER_SIZE];

    for (size_t i = 0; i < max_length; i++) {
        char c = bus_read8(psx, &psx->bus, str_addr + i);
        buffer[i] = c;
        if (c == '\0') {
            break;
        }
    }

    return buffer;
}

static inline char* arg_char(TTY *tty, PSX *psx, uint8_t arg_idx) {
    static char buffer[TTY_BUFFER_SIZE];
    buffer[0] = (char)(get_arg_value(psx, arg_idx) & 0xFF);
    buffer[1] = '\0';
    return buffer;
}

static inline void tty_printf(TTY *tty, PSX *psx) {
    Cpu *cpu = &psx->cpu;
    static char buffer[TTY_BUFFER_SIZE];
    uint32_t addr = cpu->r[4];
    uint8_t arg_idx = 1;
    uint8_t max_msg_length = TTY_BUFFER_SIZE - 1;
    uint8_t buffer_idx = 0;

    for (int i = 0; i < max_msg_length; i++) {
        char c = bus_read8(psx, &psx->bus, addr + i);
        if (c == '\0') {
            break;
        }
        if (c != '%') {
            buffer[buffer_idx++] = c;
            continue;
        }

        char c_next = bus_read8(psx, &psx->bus, addr + i + 1);
        if (c_next == '%') {
            buffer[buffer_idx++] = '%';
            i++;
            continue;
        }

        i++;
        c = bus_read8(psx, &psx->bus, addr + i);

        char pad_ch = ' ';
        bool left_align = false;
        uint8_t width = 0;

        if (c == '-') {
            left_align = true;
            i++;
        }
        if (c == '0') {
            pad_ch = '0';
            i++;
        }

        while (i < max_msg_length) {
            c = bus_read8(psx, &psx->bus, addr + i);
            if (c < '0' || c > '9') {
                break;
            }
            width = width * 10 + (c - '0');
            i++;
        }

        if (c == '.') {
            i++;
            // Precision is ignored for now
            while (i < max_msg_length) {
                c = bus_read8(psx, &psx->bus, addr + i);
                if (c < '0' || c > '9') {
                    break;
                }
                i++;
            }
        }

        // ignore length modifiers for now
        while (c == 'l' || c == 'h') {
            i++;
        }

        char *arg_value_buffer = NULL;
        size_t max_arg_length = max_msg_length - buffer_idx;

        switch (c) {
            case 'c':
                arg_value_buffer = arg_char(tty, psx, arg_idx++);
                break;
            case 's':
                arg_value_buffer = arg_string(tty, psx, arg_idx++, max_arg_length);
                break;
            case 'd': case 'i': case 'u': case 'x': case 'X':
                arg_value_buffer = arg_number(tty, psx, arg_idx++, c);
                break;
            default:
                // Unsupported specifier, just print it as is
                buffer[buffer_idx++] = '%';
                buffer[buffer_idx++] = c;
                break;
        }

        uint8_t arg_value_length = strlen(arg_value_buffer);

        if (width > arg_value_length) {
            uint8_t pad_length = width - arg_value_length;
            if (buffer_idx + pad_length > TTY_BUFFER_SIZE - 1) {
                // Not enough space in the buffer to accommodate padding, truncate the padding
                pad_length = TTY_BUFFER_SIZE - 1 - buffer_idx;
            }

            if (left_align) {
                // pad on the right
                for (uint8_t j = 0; j < pad_length; j++) {
                    arg_value_buffer[arg_value_length + j] = pad_ch;
                }
            } else {
                // shift existing content to the right and pad on the left
                memmove(arg_value_buffer + pad_length, arg_value_buffer, arg_value_length + 1); // +1 to include null terminator
                for (uint8_t j = 0; j < pad_length; j++) {
                    arg_value_buffer[j] = pad_ch;
                }
            }
            arg_value_length = strlen(arg_value_buffer);
        }

        snprintf(buffer + buffer_idx, TTY_BUFFER_SIZE - buffer_idx, "%s", arg_value_buffer);
        buffer_idx += arg_value_length;
    }

    for (uint8_t j = 0; j < buffer_idx; j++) {
        tty_putchar(tty, buffer[j]);
    }
}

void tty_maybe_putchar(TTY *tty, Cpu *cpu) {
    uint32_t pc = physical_address(cpu->pc);
    uint8_t func_code = cpu->r[9] & 0xFF;
    uint32_t pc_func = (pc << 8) | func_code;

    switch (pc_func) {
        case 0xA03C: // PC = 0xA0, func_code = 0x3C
        case 0xB03D: { // PC = 0xB0, func_code = 0x3D
            char c = (char)(cpu->r[4] & 0xFF);
            tty_putchar(tty, c);
            break;
        }
        default:
            break;
    }
}

void tty_maybe_printf(TTY *tty, PSX *psx) {
    Cpu *cpu = &psx->cpu;
    uint32_t pc = physical_address(cpu->pc);
    uint8_t func_code = cpu->r[9] & 0xFF;
    uint32_t pc_func = (pc << 8) | func_code;

    switch (pc_func) {
        case 0xA03F: // PC = 0xA0, func_code = 0x3F
            tty_printf(tty, psx);
            break;
        default:
            break;
    }
}
