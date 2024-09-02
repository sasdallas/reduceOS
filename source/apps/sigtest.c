#include <sys/signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


void setup_console() {
    open("/device/debug", 0);
    open("/device/debug", 1);
    open("/device/debug", 1); // stderr will be forced anyways, this doesn't really matter
}

void SIGINT_Handler(int sig) {
    signal(sig, SIG_IGN);
    printf("Handling SIGINT\n");
    signal(sig, SIGINT_Handler);
}



int main(int argc, char **argv) {
    setup_console();
    printf("let's go!!!\n");
    
    if (signal(SIGINT, SIGINT_Handler) == SIG_ERR) {
        printf("aur naur.. SIGINT install error\n"); // i don't even remember implementing this being possible...
        exit(1);
    }

    pid_t pid = getpid();
    int i;
    for (i = 0;;i++) {
        raise(SIGINT);
    }
}

