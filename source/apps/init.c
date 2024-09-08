#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>


int main(int argc, char **argv) {
    // Step one is to setup the file descriptor outputs
    open("/device/stdin", 0);
    open("/device/console", 1);
    open("/device/console", 1); // stderr will be forced anyways, this doesn't really matter

    // Step two is to print out the loading message
    printf("reduceOS is loading, please wait...\n");
    fprintf(stderr, "/bin/init process running, please wait...\n");

    // Fork our current process and try to run /stage2
    int cpid = fork();

    if (!cpid) {
        // We're the child process. Start execution
        char *cargv[] = {"/stage2"};
        int cargc = 1;
        execve("/stage2", cargv, cargc);

        fprintf(stderr, "child process execve failed or succeeded\n");

        // Failed?
        exit(0);
    }

    int pid = 0;
    do {
        pid = wait(NULL);
        if (pid == -1 && errno == ECHILD) {
            break;
        }

        if (pid == cpid) {
            break;
        }
    } while ((pid > 0) || (pid == -1 && errno == EINTR));

    
    printf("The child process finished or was terminated.\n");
    for (;;);
}
