#include <kernel/terminal.h>
#include <kernel/mod.h>
#include <libk_reduced/stdio.h>



int init(int argc, char *argv[]) {
    serialPrintf("[module example2] Hello, module world!\n");
    return 0;
}

int deinit() {
    serialPrintf("[module example2] Deinitializing...\n");
    return 0;
}

struct Metadata data = {
    .name = "Example2 Module",
    .description = "Example2 module for reduceOS",
    .init = init,
    .deinit = deinit
};
