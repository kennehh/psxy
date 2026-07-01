#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "tty.h"
#include "psx.h"

PSX *psx_create(void) {
    PSX *psx = (PSX *)malloc(sizeof(PSX));
    if (!psx) {
        fprintf(stderr, "Failed to allocate PSX structure\n");
        exit(EXIT_FAILURE);
    }
    psx->cpu = cpu_create();
    psx->bus = bus_create();
    psx->tty = tty_create();
    return psx;
}

void psx_destroy(PSX *psx) {
    if (!psx) return;
    cpu_destroy(psx->cpu);
    bus_destroy(psx->bus);
    tty_destroy(psx->tty);
    free(psx);
}

void psx_reset(PSX *psx) {
    if (!psx) return;
    cpu_reset(psx->cpu);
    bus_reset(psx->bus);
    bus_install_bios_trampolines(psx);
    tty_reset(psx->tty);
}

void psx_run(PSX *psx, uint32_t cycles) {
    cpu_run(psx, cycles);
}