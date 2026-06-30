#ifndef LOADER_H
#define LOADER_H

#include "common.h"

void load_bios(PSX *psx, const char *bios_path);
void load_exe(PSX *psx, const char *exe_path);

#endif // LOADER_H
