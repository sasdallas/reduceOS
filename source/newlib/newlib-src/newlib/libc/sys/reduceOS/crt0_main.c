#include <fcntl.h>

extern void exit(int code);
extern int main(int argc, char *argv[]);

void crt0_main(int argc, int args) {

    // We need to copy args out to argv
    int *args_start = &args;

    char *argv[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = (*(args_start + i));
    }

    _init_signal();
    int ex = main(argc, argv);
    _exit(ex);
}