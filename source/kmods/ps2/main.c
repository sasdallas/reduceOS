// (PS/2 module) main.c - main function of the PS/2 device module

#include "ps2.h"
#include <kernel/mod.h>

int init(int argc, char *argv[]) {
    ps2_kbd_init();
    serialPrintf("[module ps2] PS/2 module initialized and ready.\n");
    return 0;
}

void deinit() {
}


struct Metadata data = {
    .name = "PS/2 Module",
    .description = "PS/2 driver for reduceOS",
    .init = init,
    .deinit = deinit
};
