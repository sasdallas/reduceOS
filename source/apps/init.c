#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char** argv) {
    // Step one is to setup the file descriptor outputs
    open("/device/console", 0);
    open("/device/console", 1);
    open("/device/console", 1); 

    // Step two is to print out the loading message
    printf("reduceOS is loading, please wait...\n");
    fprintf(stderr, "/bin/init process running, please wait...\n");

    // Fork our current process and try to run /stage2
    int cpid = fork();

    if (!cpid) {
        // We're the child process. Start execution
        char* cargv[] = {"/stage2"};
        int res = execve("/stage2", cargv, 1);
        if (res != 0) {
            printf("error: child failed\n");
            exit(0);
        }
    } else {
        printf("The child process finished or was terminated.\n");
    }

    for (;;);
}
