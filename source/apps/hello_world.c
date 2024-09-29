#include <stdio.h>

int main(int argc, char **argv) {
    open("/device/keyboard", 0);
    open("/device/console", 1);
    open("/device/console", 1); 

    write(stdout, "Hello, world!", strlen("Hello, world!"));

    printf("Hello, world from newlib function!\n");
    printf("Received %i arguments:\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("\t- %s\n", argv[i]);
    }

    for (;;);
}