#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include "bus.h"
#include "cpu.h"
#include "loader.h"
#include "tty.h"
#include <time.h>
#include <string.h>

#define MAX_STEPS 10000000
#define TARGET_PC 0x80030000

void benchmark(Cpu *cpu, Bus *bus, TTY *tty) {
    uint32_t instructions_executed = 0;
    uint32_t max_steps = 8000000;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (max_steps-- > 0) {
        // tty_maybe_putchar(tty, cpu);
        cpu_step(cpu, bus);
        // printf("PC: 0x%08X, Instruction: 0x%08X\n", cpu->pc, cpu->inst);
        instructions_executed++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    double elapsed_ms = (seconds * 1000.0) + (nanoseconds / 1000000.0);

    printf("Instructions executed: %u\n", instructions_executed);
    printf("Elapsed time: %.2f ms\n", elapsed_ms);
    double mips = instructions_executed / (elapsed_ms * 1e-3) / 1e6;
    printf("MIPS: %.2f\n", mips);
}

void run_until_kernel_init(Cpu *cpu, Bus *bus, TTY *tty) {
    uint32_t steps = 0;
    while (steps++ < MAX_STEPS) {
        cpu_step(cpu, bus);
        if (cpu->pc == TARGET_PC) {
            break;
        }
    }
}

int main() {
    Bus* bus = bus_create();
    Cpu* cpu = cpu_create();
    TTY* tty = tty_create();

    for (int i = 0; i < 10; i++) {
        bus_reset(bus);
        cpu_reset(cpu);
        tty_reset(tty);
        load_bios(bus, "roms/SCPH1001.BIN");
        run_until_kernel_init(cpu, bus, tty);
        load_exe(cpu, bus, "roms/psxtest_cpu.exe");

        printf("Benchmark iteration %d\n", i + 1);
        benchmark(cpu, bus, tty);
    }

    tty_destroy(tty);
    cpu_destroy(cpu);
    bus_destroy(bus);

    return 0;
}
