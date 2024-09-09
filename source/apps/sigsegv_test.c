#include <stdio.h>
#include <string.h>


int main(int argc, char *argv[]) {
    *(volatile int*)0xAAAAAAAA = 42;
    fprintf(stderr, "Something isn't quite right..\n");

    return 0;
}