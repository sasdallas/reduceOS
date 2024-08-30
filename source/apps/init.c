#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    // Step one is to setup the file descriptor outputs
    open("/device/stdin", 0);
    open("/device/console", 1);
    open("/device/console", 1); // stderr will be forced anyways, this doesn't really matter

    int module = open("/device/modules/mod0", 1);

    printf("Hello!\n");
    fprintf(stderr, "Hello to the debug console!");



    for (;;);
}