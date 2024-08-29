#include <kernel/terminal.h>
#include <kernel/mod.h>
#include <libk_reduced/stdio.h>



int init(int argc, char *argv[]) {
    serialPrintf("[module example] Hello, module world!\n");
    return 0;
}

int deinit() {
    serialPrintf("[module example] Deinitializing...\n");
    return 0;
}

struct Metadata data = {
    .name = "Example Module",
    .description = "Example module for reduceOS",
    .init = init,
    .deinit = deinit
};
