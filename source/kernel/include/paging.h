// paging.h - header file for paging.c.
// Implementation of the paging file is based on James Molloy's kernel dev tutorials (http://www.jamesmolloy.co.uk/tutorial_html/7.-The%20Heap.html)

#ifndef PAGING_H
#define PAGING_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/string.h" // String functions

#include "include/physical_memory.h" // Some useful frame functions
#include "include/panic.h" // Kernel panicking

// Typedefs

typedef struct {
    uint32_t present : 1; // Page present in memory
    uint32_t rw : 1; // The page is read-only if this is clear, but it's read-write if set.
    uint32_t user : 1; // Supervisor level only if clear.
    uint32_t accessed : 1; // Has the page been accessed since the last refresh?
    uint32_t dirty : 1; // Has the page been written to since the last refresh?
    uint32_t unused : 7; // This value represents an amalgamation of unused + reserved bits.
    uint32_t frame : 20; // Frame address (needs to be shifted right 12 bits)
} page;

typedef struct {
    page pages[1024];
} page_table;

typedef struct {
    page_table *tables[1024]; // Array of pointers to page tables.
    uint32_t tablePhysical[1024]; // Array of pointers to physical location of page tables.
    uint32_t physicalAddress; // Physical address of tablePhysical.
} page_directory;

// Macros

// (from JamesM's kernel dev tutorials)
#define INDEX_BIT(a) (a/(8*4))
#define OFFSET_BIT(a) (a%(8*4))

// Definitions
#define PAGE_ALIGN 4096 // Aligned to 4096.
#define PAGE_SIZE PAGE_ALIGN // Page size is 4096.


// Functions

void allocateFrame(page *page, int kernel, int writable); // Allocating a frame.
void freeFrame(page *page); // Deallocate (or "free") a frame.
void initPaging(uint32_t physicalMemorySize, uint32_t kernelSize); // Initialize paging
void switchPageDirectory(page_directory *dir); // Switches the page directory using inline assembly
page *getPage(uint32_t addr, int make, page_directory *dir); // Returns a page from an address, directory (creates one if make is non-zero).
#endif
