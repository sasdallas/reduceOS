#include <kernel/terminal.h>
#include <kernel/mod.h>




int init(int argc, char *argv[]) {
    terminalWriteString("Hello, world!");
    return 0;
}

struct Metadata data = {
    .name = "example",
    .init = init
};