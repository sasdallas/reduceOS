#include <kernel/terminal.h>
#include <kernel/mod.h>




int init(int argc, char *argv[]) {
    terminalWriteString("Hello, world!");
    return 0;
}

mod_metadata_t metadata = {
    .name = "example",
    .init = init
};