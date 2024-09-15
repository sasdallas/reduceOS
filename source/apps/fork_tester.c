#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void sig_handler(int signum) { write(2, "SIGCHLD", 7); }

int main(int argc, char** argv) {
    // Step one is to setup the file descriptor outputs
    open("/device/console", 0);
    open("/device/console", 1);
    open("/device/console", 1); // stderr will be forced anyways, this doesn't really matter

    // Step two is to print out the loading message
    printf("reduceOS is loading, please wait...\n");
    fprintf(stderr, "/bin/init process running, please wait...\n");

    // Fork our current process and try to run /stage2
    int cpid = fork();

    signal(SIGCHLD, sig_handler);

    if (!cpid) {
        write(2, "We are the child process\n", strlen("We are the child process\n"));
        exit(0);
    }

    printf("We are the main process\n");
    return 0;
}
