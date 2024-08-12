#include <kernel/mod.h>
#include "multifile2.h"
#include <stdio.h>

int init(int argc, char *args[]) {
    printf("Multifile 1 says hello!\n");
    int ret = sayHello();
    printf("sayHello returns %i\n", ret);
    return 0;
}

int deinit() {
    printf("Multifile shutting down\n");
    return 0;
}

struct Metadata data = {
    .name = "Multifile Module",
    .description = "A test for multiple file modules",
    .init = init,
    .deinit = deinit
};