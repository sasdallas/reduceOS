// heap.h - header file for heap.c (manages the kernel heap)
// Implementation of the heap file is based on James Molloy's kernel dev tutorials (http://www.jamesmolloy.co.uk/tutorial_html/7.-The%20Heap.html)
#ifndef HEAP_H
#define HEAP_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/string.h" // String functions
#include "include/libc/ordered_array.h" // Ordered array handling
#include "include/libc/assert.h"

#include "include/paging.h"
#include "include/panic.h" // Handles panicking
// Definitions

// (actual #defines)
#define HEAP_START 0xC0000000 // Heap start, must always be 0xC0000000
#define HEAP_INITIAL_SIZE 0x100000 // Heap's initial size (may later change)
#define HEAP_INDEX_SIZE 0x20000 // Size of the index for the heap.
#define HEAP_MAGIC 0x123890AB // magik nombeer (used for error checking and identification)
#define HEAP_MINIMUM_SIZE 0x70000 // The heap must have a size of at least 0x70000 or greater.

#define PLACEMENT_ALIGN 0x1000 // Placement align (always should be 4096 or 4k)

// (variables)
extern uint32_t end; // end is defined in the linker.ld script we added (thanks James Molloy!)


// Typedefs

// header typedef  - Size information for a hole or block.
typedef struct {
    uint32_t magic; // Magic number (0x123890AB)
    uint8_t isHole; // If isHole is non-zero, this is a hole. If it's 0, it's a block.
    uint32_t size; // Hole or block size (includes the end footer!).
} header_t;

// footer typedef - contains some misc stuff
typedef struct {
    uint32_t magic; // Magic number (0x123890AB)
    header_t *header; // Pointer to the block header.
} footer_t;

// heap typedef - the main thing!
typedef struct {
    ordered_array_t index;
    uint32_t startAddress; // Start of the allocaed space
    uint32_t endAddress; // End of the allocated space (may be expanded up to maxAddress)
    uint32_t maxAddress; // Maximum address for heap expansion
    uint8_t supervisor; // (user-mode stuff) Should extra pages requested be mapped as supervisor-only?)
    uint8_t readonly; // Should extra pages requested be mapped as read-only?
} heap_t;


// Functions

uint32_t kmalloc_int(uint32_t size, int align, uint32_t *phys); // Allocates size blocks on placement_address (a uint32_t pointer to end)
uint32_t kmalloc_a(uint32_t size);
uint32_t kmalloc_p(uint32_t size, uint32_t *phys);
uint32_t kmalloc_ap(uint32_t size, uint32_t *phys);
uint32_t kmalloc(uint32_t size); 
uint32_t kmalloc_heap(uint32_t size, int align, uint32_t *phys);
heap_t *createHeap(uint32_t startAddress, uint32_t endAddress, uint32_t maxAddress, uint8_t supervisor, uint8_t readonly); // Create a heap!
void *alloc(uint32_t size, uint8_t pageAlign, heap_t *heap); // Allocate to a heap (of size "size" and page align if pageAlign != 0)
void free(void *p, heap_t *heap); // Free *p from *heap.
#endif