extern char **environ;
#include <stdio.h>

int main(int argc, char **argv) {
    open("/device/console", 0);
    open("/device/console", 0);
    open("/device/console", 0);

    printf("argc = %i\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("\t%s\n", argv[i]);
    }

    printf("environ dump\n");

    char **s = environ;

    for (; *s; s++) {
        printf("%s\n", *s);
    }
    for (;;);
}