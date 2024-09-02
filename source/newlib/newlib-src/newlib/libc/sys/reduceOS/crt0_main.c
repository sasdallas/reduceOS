#include <fcntl.h>

extern void exit(int code);
extern int main(int argc, char *argv[]);

extern char **environ;

void crt0_main(int envc, int argc, int args) {

    // Copy envc out to env
    int *env_start = &args;
    char *env[envc];
    for (int i = 0; i < envc; i++) {
        env[i] = (*(env_start + i));
    }

    environ = env;

    // We need to copy args out to argv
    int *args_start = env_start + envc + 1;
    char *argv[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = (*(args_start + i));
    }

    int ex = main(argc, argv);
    _exit(ex);
}