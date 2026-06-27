#include <stdint.h>
#include <stdio.h>
#include "bus.h"
#include "cpu.h"
#include "loader.h"

int main() {
    Bus* bus = bus_create();
    Cpu* cpu = create_cpu();

    return 0;
}
