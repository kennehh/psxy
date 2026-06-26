#ifndef LOADER_H
#define LOADER_H

#include "bus.h"
#include "cpu.h"

void load_bios(Bus *bus, const char *bios_path);
void load_exe(Cpu *cpu, Bus *bus, const char *exe_path);

#endif // LOADER_H
