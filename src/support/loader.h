#ifndef LOADER_H
#define LOADER_H

#include "core/common.h"

void load_bios(PSX *psx, const char *bios_path);
void load_exe(PSX *psx, const char *exe_path);
void load_bios_trampolines(PSX *psx);
void clear_bios_trampolines(PSX *psx);

#endif // LOADER_H
