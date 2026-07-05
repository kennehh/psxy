#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "tty.h"
#include "psx.h"
#include "loader.h"
#include "timers.h"

PSX *psx_create(void) {
    PSX *psx = (PSX *)malloc(sizeof(PSX));
    if (!psx) {
        fprintf(stderr, "Failed to allocate PSX structure\n");
        exit(EXIT_FAILURE);
    }
    bus_init(&psx->bus);
    cpu_reset(&psx->cpu);
    gpu_init(&psx->gpu);
    tty_init(&psx->tty);
    timers_reset(&psx->timers);

#ifndef PSXY_SINGLE_STEP_TEST_MODE
    load_bios_trampolines(psx);
#endif

    return psx;
}

void psx_destroy(PSX *psx) {
    if (!psx) return;
    bus_destroy(&psx->bus);
    gpu_destroy(&psx->gpu);
    free(psx);
}

void psx_reset(PSX *psx) {
    if (!psx) return;
    cpu_reset(&psx->cpu);
    cop0_reset(&psx->cop0);
    bus_reset(&psx->bus);
    tty_reset(&psx->tty);
    gpu_reset(&psx->gpu);
    irq_reset(&psx->irq);
    timers_reset(&psx->timers);

#ifndef PSXY_SINGLE_STEP_TEST_MODE
    load_bios_trampolines(psx);
#endif
}

void psx_run(PSX *psx, uint32_t cycles) {
    cpu_run(psx, cycles);
}
