#include <kernel/terminal.h>
#include <kernel/mod.h>
#include <stdio.h>



int init(int argc, char *argv[]) {
    terminalWriteString("Hello, module world!\n");
    printf("Libc go brrr\n");
    return 0;
}

int deinit() {
    printf("Deinitializing module...\n");
    return 0;
}

struct Metadata data = {
    .name = "Example Module",
    .description = "Example module for reduceOS",
    .init = init,
    .deinit = deinit
};