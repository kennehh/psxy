#include "loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_bios(Bus *bus, const char *bios_path) {
    FILE *bios_file = fopen(bios_path, "rb");
    if (!bios_file) {
        fprintf(stderr, "Failed to open BIOS file: %s\n", bios_path);
        exit(EXIT_FAILURE);
    }

    size_t read_size = fread(bus->bios, 1, BIOS_SIZE, bios_file);
    if (read_size != BIOS_SIZE) {
        fprintf(stderr, "Failed to read BIOS file: %s\n", bios_path);
        exit(EXIT_FAILURE);
    }

    bus_clear_bios_trampolines(bus);
    memcpy(bus->page_table[BIOS_PHYS_START >> BUS_PAGE_SHIFT], bus->bios, BIOS_SIZE);
    fclose(bios_file);
}

void load_exe(Cpu *cpu, Bus *bus, const char *exe_path) {
    FILE *exe_file = fopen(exe_path, "rb");
    if (!exe_file) {
        fprintf(stderr, "Failed to open EXE file: %s\n", exe_path);
        exit(EXIT_FAILURE);
    }

    // Check for "PS-X EXE" magic number
    char magic[8];
    fread(magic, 1, 8, exe_file);
    if (memcmp(magic, "PS-X EXE", 8) != 0) {
        fprintf(stderr, "Invalid EXE file: %s\n", exe_path);
        exit(EXIT_FAILURE);
    }

    fseek(exe_file, 0x10, SEEK_SET); // Skip to the header

    // Read EXE header
    uint32_t pc, gp, dest, size, memfill_start, memfill_size, sp_base, sp_offset;
    fread(&pc, sizeof(uint32_t), 1, exe_file);
    fread(&gp, sizeof(uint32_t), 1, exe_file);
    fread(&dest, sizeof(uint32_t), 1, exe_file);
    fread(&size, sizeof(uint32_t), 1, exe_file);
    fread(&memfill_start, sizeof(uint32_t), 1, exe_file);
    fread(&memfill_size, sizeof(uint32_t), 1, exe_file);
    fread(&sp_base, sizeof(uint32_t), 1, exe_file);
    fread(&sp_offset, sizeof(uint32_t), 1, exe_file);

    cpu->pc = pc;
    cpu->next_pc = pc + 4;
    cpu->r[28] = gp; // gp register
    cpu->r[29] = sp_base + sp_offset; // sp register

    if (memfill_size > 0) {
        uint32_t start_phys = memfill_start & 0x1FFFFFFF;
        memset(bus->ram + start_phys, 0, memfill_size);
    }

    if (size > 0) {
        uint32_t dest_phys = dest & 0x1FFFFFFF;
        fseek(exe_file, 0x800, SEEK_SET); // Skip to the data section of the EXE
        fread(bus->ram + dest_phys, 1, size, exe_file);
    }
}
