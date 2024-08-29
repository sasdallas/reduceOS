#include <fcntl.h>

extern void exit(int code);
extern int main();

void _start() {
    _init_signal();
    int ex = main();
    write(3, "_crt0 completed\n", strlen("_crt0 completed\n"));
    _exit(ex);
}