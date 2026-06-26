#include "loader.h"
#include <stdio.h>
#include <stdlib.h>

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
    memcpy(bus->page_table[BIOS_PHYS_START >> BUS_PAGE_SHIFT], bus->bios, BIOS_SIZE);
    fclose(bios_file);
}

void load_exe(Cpu *cpu, Bus *bus, const char *exe_path) {
    FILE *exe_file = fopen(exe_path, "rb");
    if (!exe_file) {
        fprintf(stderr, "Failed to open EXE file: %s\n", exe_path);
        exit(EXIT_FAILURE);
    }

    uint32_t exe_header[8];
    size_t read_size = fread(exe_header, sizeof(uint32_t), 8, exe_file);
    if (read_size != 8) {
        fprintf(stderr, "Failed to read EXE header: %s\n", exe_path);
        exit(EXIT_FAILURE);
    }

    uint32_t pc = exe_header[0];
    uint32_t gp = exe_header[1];
    uint32_t dest = exe_header[2];
    uint32_t size = exe_header[3];
    uint32_t memfill_start = exe_header[4];
    uint32_t memfill_size = exe_header[5];
    uint32_t sp_base = exe_header[6];
    uint32_t sp_offset = exe_header[7];

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
        read_size = fread(bus->ram + dest_phys, 1, size, exe_file);
        if (read_size != size) {
            fprintf(stderr, "Failed to read EXE data: %s\n", exe_path);
            exit(EXIT_FAILURE);
        }
    }
}
