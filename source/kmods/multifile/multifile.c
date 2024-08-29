#include <kernel/mod.h>
#include "multifile2.h"
#include <libk_reduced/stdio.h>

int init(int argc, char *args[]) {
    serialPrintf("[module multifile] Multifile 1 says hello!\n");
    int ret = sayHello();
    return 0;
}

int deinit() {
    serialPrintf("[module multifile] Multifile shutting down\n");
    return 0;
}

struct Metadata data = {
    .name = "Multifile Module",
    .description = "A test for multiple file modules",
    .init = init,
    .deinit = deinit
};
