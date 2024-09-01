// arg_tester.c - Test argc and argv

#include <stdio.h>

void setup_console() { 
    open("/device/stdin", 0);
    open("/device/console", 1);
    open("/device/console", 1); // stderr will be forced anyways, this doesn't really matter
}

int main(int argc, char **argv) {
    setup_console();

    if (!argc) {
        printf("argc is 0 or NULL, error.\n");
        return 0;
    }

    printf("argc: %i\n", argc);
    printf("argv:\n");
    for (int i = 0; i < argc; i++) {
        printf("\t- %s\n", argv[i]);
    }

    for (;;);
}