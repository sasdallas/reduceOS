// memtest.c - reduceOS memory testing application
// memory's final boss

#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>

#define RUNTEST(test) if (test() != 0) { printf("*** MEMTEST FAILED - HALT\n"); for (;;); } 
#define PAGE_SIZE 4096
#define PATTERN 0xAA // it is screaming for help

// sbrk() wrapper
static void *dosbrk(ssize_t size) {
    void *p = sbrk(size);

    if (p == (void*)-1) {
        printf("FAIL! SBRK returned -1\n");
        return NULL;
    }

    if (p == NULL) {
        printf("FAIL! SBRK returned NULL\n");
        return NULL;
    }

    return p;
}

// Chunk filler with bytes
static void fill_chunk(void *chunk, unsigned int pageoffset, unsigned int size) {
    volatile char *ptr = chunk;

    // If a page offset was specified, take care of it
    if (pageoffset != 0) {
        ptr += PAGE_SIZE * pageoffset;
    }

    for (size_t i = 0; i < size; i++) {
        ptr[i] = PATTERN;
    }
}

// Chunk checker
static int check_chunk(void *chunk, unsigned int pageoffset, unsigned int size) {
    volatile uint8_t *ptr = chunk;

    // If a page offset was specified, take care of it
    if (pageoffset != 0) {
        ptr += PAGE_SIZE * pageoffset;
    }

    for (size_t i = 0; i < size; i++) {
        if (ptr[i] != PATTERN) {
            printf("FAIL! mismatch at offset 0x%x, expected 0x%x but got 0x%x\n", ptr + i, PATTERN, ptr[i]);
            return -1;
        }
    }

    return 0;
}


int memtest_testmalloc() {
    printf("- Running test: malloc...");

    void *a = malloc(512);
    if (a) {
        printf("pass (ptr: 0x%x)!\n", a); // don't free because why not
        return 0; 
    } else {
        printf("LMAO\n");
        return -1;
    }
}

/* SBRK TESTS */

int memtest_sbrktest1() {
    // Allocate & leak 1 page
    printf("- Running test: allocate and leak page...");
    void *p = dosbrk(PAGE_SIZE); // expand memory size by one page
    fill_chunk(p, 0, PAGE_SIZE);
    
    // Check to make sure there is no memory corruption
    if (check_chunk(p, 0, PAGE_SIZE) != 0) {
        return -1;
    } 

    printf("pass (leaked)\n");
    return 0;
}

int memtest_sbrktest2() {
    // Allocate multiple pages and leak them all (in this case 6)
    printf("- Running test: allocate 6 pages and leak them...");

    void *origp = dosbrk(0); // sbrk heap back to 0.

    // Let's begin the fun
    void *p = dosbrk(PAGE_SIZE * 6);
    if (p != origp) {
        printf("FAIL! sbrk did not restore origp (got %p expected %p)\n", p, origp);
        return -1;
    }

    // Mark the pages and check them
    for (int i = 0; i < 6; i++) {
        fill_chunk(p, i, PAGE_SIZE);
        if (check_chunk(p, i, PAGE_SIZE)) {
            printf("FAIL! Data corrupt on page %u\n", i);
            return -1;
        }
    }

    printf("pass (leaked)\n");
    return 0;
}

int memtest_sbrktest3() {
    // stub test
    printf("- Running test: allocate and free page...");
    printf("pass (stub)\n");
    return 0;
}

int memtest_sbrktest4() {
    // Allocate multiple pages and free them all (in this case 6)
    printf("- Running test: allocate 6 pages and free them...");

    void *origp = dosbrk(0); // sbrk heap back to 0.

    // Let's begin the fun
    void *p = dosbrk(PAGE_SIZE * 6);
    if (p != origp) {
        printf("FAIL! sbrk did not restore origp (got %p expected %p)\n", p, origp);
        return -1;
    }

    // Mark the pages and check them
    for (int i = 0; i < 6; i++) {
        fill_chunk(p, i, PAGE_SIZE);
        if (check_chunk(p, i, PAGE_SIZE)) {
            printf("FAIL! Data corrupt on page %u\n", i);
            return -1;
        }
    }

    // Restore
    p = dosbrk(0);

    // Free the pages
    void *q = dosbrk(-PAGE_SIZE * 6);
    if (q != p) {
        printf("FAIL! sbrk did not shrink the heap back down (got %p, expected %p)\n", q, p);
        return -1;
    }

    q = dosbrk(0);
    if (q != origp) {
        printf("FAIL! sbrk did not restore the heap to original state (got %p, expected %p)\n", q, origp);
        return -1;
    }

    printf("pass (allocated and freed)\n");
    return 0;
} 

int memtest_heaptest() {
    // This test has 6 parts:
    // - Checking how much can be allocated
    // - Writing one byte to each page
    // - Checking that
    // - Freeing the memory
    // - Rellocating the memory
    // - Freeing it again
    // Represented by periods
    printf("- Running test: full heap destruction");

    // Part 1 - check how much memory is available
    size_t size;
    void *p;
    for (size = (1024 * 1024 * 1024); (p = sbrk(size)) == (void*)-1; size = size / 2) {
        fprintf(stderr, "memtest: Heap allocation reached but did not succeed.\n");
        fprintf(stderr, "Allocation failed on %9lu bytes\n", size);
        printf(". FAIL! Could not calculate total memory\n");
        return -1;
    }

    fprintf(stderr, "memtest: %9lu bytes allocation ok\n", size);
    printf(".");

    // Touch each page
    fprintf(stderr, "memtest: begin %i page touch", size / PAGE_SIZE);
    for (int i = 0; i < size / PAGE_SIZE; i++) {
        *(uint8_t*)(p + (i * PAGE_SIZE)) = PATTERN;
        fprintf(stderr, ".");
    }

    fprintf(stderr, "done\n");
    printf(".");

    // Check each page
    fprintf(stderr, "memtest: begin %i page check", size / PAGE_SIZE);
    void *ptr = p;
    for (int i = 0; i < size / PAGE_SIZE; i++) {
        ptr = p + (i * PAGE_SIZE);
        if (((uint8_t*)ptr)[0] != PATTERN) {
            fprintf(stderr, "fail on page %i - expected 0x%x got 0x%x\n", i, PATTERN, *((uint8_t*)p + (i * PAGE_SIZE)));
            printf("FAIL! Page touch/check failed (page %i)\n", i);
            return -1;
        }
        fprintf(stderr, ".");
    }

    fprintf(stderr, "done\n");
    printf(".");


    fprintf(stderr, "memtest: free mem...");
    dosbrk(-size);
    fprintf(stderr, "ok\n");
    printf(".");

    fprintf(stderr, "memtest: realloc...");
    dosbrk(size);
    fprintf(stderr, "ok\n");
    printf(".");

    fprintf(stderr, "memtest: freeing...");
    dosbrk(-size);
    fprintf(stderr, "ok\n");
    printf(".");

    printf("pass\n");
    return 0;
}


int main(int argc, char **argv) {
    // Setup a console
    open("/device/keyboard", 0); // stdin
    open("/device/console", 0); // stdout
    open("/device/serial/COM1", 0); // stderr

    printf("memtest v1.0 by @sasdallas\n");

    // malloc and friends functions
    RUNTEST(memtest_testmalloc);

    // sbrk tests
    RUNTEST(memtest_sbrktest1); // Allocate and leak page
    RUNTEST(memtest_sbrktest2); // Allocate and leak 6 pages
    RUNTEST(memtest_sbrktest3); // Allocate and free page
    RUNTEST(memtest_sbrktest4); // Allocate and free 6 pages

    // heap tests
    RUNTEST(memtest_heaptest); // Big bertha

    for (;;);
}