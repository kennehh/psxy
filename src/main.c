#include <stdint.h>
#include <stdio.h>
#include "bus.h"
#include "cpu.h"
#include "loader.h"
#include "tty.h"

int main() {
    Bus* bus = bus_create();
    Cpu* cpu = cpu_create();
    TTY* tty = tty_create();

    load_bios(bus, "../roms/SCPH1001.BIN");
    uint32_t max_steps = 10000000;
    uint32_t target_pc = 0x80030000;

    while (max_steps-- > 0) {
        tty_maybe_putchar(tty, cpu);
        cpu_step(cpu, bus);
        if (cpu->pc == target_pc) {
            printf("Reached target PC: 0x%08X\n", target_pc);
            break;
        }
    }

    tty_destroy(tty);
    cpu_destroy(cpu);
    bus_destroy(bus);

    return 0;
}
