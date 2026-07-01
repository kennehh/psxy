#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "tty.h"
#include "psx.h"
#include "loader.h"

PSX *psx_create(void) {
    PSX *psx = (PSX *)malloc(sizeof(PSX));
    if (!psx) {
        fprintf(stderr, "Failed to allocate PSX structure\n");
        exit(EXIT_FAILURE);
    }
    bus_init(&psx->bus);
    cpu_reset(&psx->cpu);
    tty_init(&psx->tty);
    load_bios_trampolines(psx);
    return psx;
}

void psx_destroy(PSX *psx) {
    if (!psx) return;
    free(psx);
}

void psx_reset(PSX *psx) {
    if (!psx) return;
    cpu_reset(&psx->cpu);
    bus_reset(&psx->bus);
    tty_reset(&psx->tty);
    load_bios_trampolines(psx);
}

void psx_run(PSX *psx, uint32_t cycles) {
    cpu_run(psx, cycles);
}