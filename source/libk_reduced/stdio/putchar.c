// putchar.c - prints characters to string

#include <libk_reduced/stdio.h>

// putchar(int ic) - terminalPutchar with return values.
int putchar(int ic) {
    char c = (char) ic;
    terminalWrite(&c, sizeof(c));
    return ic;
}
