#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include "psx.h"
#include "loader.h"
#include "tty.h"
#include "hrtime.h"
#include <string.h>

#define MAX_STEPS 10000000
#define TARGET_PC 0x80030000

static void benchmark(PSX *psx) {
    uint32_t instructions_executed = 8000000;
    uint32_t max_steps = 8000000;

    uint64_t start = get_time_ns();

    psx_run(psx, max_steps);

    uint64_t end = get_time_ns();

    double elapsed_ms = (end - start) / 1000000.0;
    printf("Instructions executed: %u\n", instructions_executed);
    printf("Elapsed time: %.2f ms\n", elapsed_ms);
    double mips = instructions_executed / (elapsed_ms * 1e-3) / 1e6;
    printf("MIPS: %.2f\n", mips);
}

static void run_until_kernel_init(PSX *psx) {
    // uint32_t steps = 0;
    // while (steps++ < MAX_STEPS) {
    //     // tty_maybe_putchar(tty, cpu);
    //     // cpu_step(cpu, bus);
    //     // tty_maybe_putchar(psx->tty, psx->cpu);
    //     // cpu_step_block(psx);
    //     cpu_step(psx);
    //     if (psx->cpu->pc == TARGET_PC) {
    //         break;
    //     }
    // }

    psx_run(psx, 2000000);
}

int main(void) {
    PSX* psx = psx_create();

    for (int i = 0; i < 10; i++) {
        psx_reset(psx);
        load_bios(psx, "roms/SCPH1001.BIN");
        run_until_kernel_init(psx);
        // load_exe(psx, "roms/psxtest_cpu.exe");

        // printf("Benchmark iteration %d\n", i + 1);
        // benchmark(psx);
    }

    psx_destroy(psx);

    return 0;
}
