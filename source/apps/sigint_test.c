#include <stdio.h>
#include <sys/signal.h>

static void sig_handler(int sig) {
    
}

int main(int argc, char **argv) {
    open("/device/stdin", 0);
    open("/device/console", 1);
    open("/device/console", 1); // stderr will be forced anyways, this doesn't really matter

    printf("Now your OS is hanging and there's nothing you can do about it!\n");
    printf("But maybe you could press CTRL C, just for funsies.\n");
    fprintf(stderr, "Can't close me!\n");

    signal(SIGINT, sig_handler);

    for (;;);

    return 0;
}