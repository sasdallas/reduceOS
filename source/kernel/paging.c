// ==========================================================
// paging.c - Manages kernel virtual memory
// ==========================================================
// This file is a part of the reduceOS C kernel. If you use this code, please credit the implementation's original owner (if you directly copy and paste this, also add me please)
// This implementation is not by me - credits for the implementation are present in paging.h

#include "include/paging.h" // Main header file

page_directory_t* kernelDir = 0; // The kernel's page directory
page_directory_t* currentDir = 0; // The current page directory




extern uint32_t placement_address; // Defined in heap.c
extern heap_t* kernelHeap;

// A stack for our kernel. 16 is max tasks and 16384 is the stack size.
static char stack[16-1][16384];

// Function prototypes.
static page_table_t *clonePageTable(page_table_t *src, uint32_t *physicalAddress); // Clone a page table.

// A few addresses we need to preallocate.
extern uint32_t *vbeBuffer;


// Moving on to the functions...


// initPaging() - Initialize paging
void initPaging() {
    // Make a page directory.
    uint32_t phys;
    kernelDir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t)); // Allocate a page directory for the kernel...
    memset(kernelDir, 0, sizeof(page_directory_t));
    kernelDir->physicalAddress = (uint32_t)kernelDir->tablePhysical;


    // Now, map some pages in the kernel's heap area - here we call getPage but not allocateFrame, causing page tables to be created where necessary.
    int i = 0;
    for (i = HEAP_START; i < HEAP_START + HEAP_INITIAL_SIZE; i += PAGE_ALIGN) {
        getPage(i, 1, kernelDir);
    }


    // Now we can identity map using allocateFrame
    i = 0;
    while (i < placement_address + PAGE_ALIGN) {
        allocateFrame(getPage(i, 1, kernelDir), 0, 0);
        i += PAGE_ALIGN;
    }



    // Allocate the pages we mapped earlier
    for (i = HEAP_START; i < HEAP_START + HEAP_INITIAL_SIZE; i += PAGE_ALIGN) {
        allocateFrame(getPage(i, 1, kernelDir), 0, 0);
    }


    // Register interrupt handlers and enable paging.
    isrRegisterInterruptHandler(14, pageFault); // pageFault is defined in panic.c
    uint32_t *ptr = (uint32_t*)0xA0000000;
    uint32_t do_page_fault = *ptr;
    switchPageDirectory(kernelDir); // Switch the page directory to kernel directory.

    printf("Paging initialized!\n");
    
    
    // Create the kernel heap
    kernelHeap = createHeap(HEAP_START, HEAP_START + HEAP_INITIAL_SIZE, 0xCFFFF000, 0, 0);
    printf("Kernel heap initialized!\n");

}

// switchPageDirectory(page_directory_t *dir) - Switches the page directory using inline assembly
void switchPageDirectory(page_directory_t* dir) {
    currentDir = dir;
    ASSERT(dir->physicalAddress, "switchPageDirectory", "physicalAddress not present - cannot switch.");
    asm volatile ("mov %0, %%cr3" :: "r"(dir->physicalAddress));
}

void enablePaging() {
    uint32_t cr0;
    uint32_t cr4;
    
    asm volatile ("mov %%cr4, %0" : "=r"(cr4)); // Set cr4 to the value of reg cr4
    cr4 = cr4 & 0xFFFFFFEF; // Clear PSE bit (is this necessary?)
    asm volatile ("mov %0, %%cr4" :: "r"(cr4));

    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 = cr0 | 0x80000000; // Set paging bit
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));
}


// getPage(uint32_t addr, int make, page_directory_t *dir) - Returns a page from an address, directory (creates one if make is non-zero).
page_t* getPage(uint32_t addr, int make, page_directory_t* dir) {
    addr /= PAGE_ALIGN; // Turn the address into an index (aligned to 4096).

    // Find the page table containing this address.
    uint32_t tableIndex = addr / 1024;
    if (dir->tables[tableIndex]) {
        // If this table is already assigned...
        return &dir->tables[tableIndex]->pages[addr % 1024];
    }
    else if (make) {
        uint32_t tmp;
        dir->tables[tableIndex] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
        memset(dir->tables[tableIndex], 0, 0x1000);
        dir->tablePhysical[tableIndex] = tmp | 0x7;
        return &dir->tables[tableIndex]->pages[addr % 1024];
    }
    else { return 0; } // Nothing we can do.
}


// clonePageDirectory(page_directory_t *src) - Clone a page directory.
page_directory_t *clonePageDirectory(page_directory_t *src) {
    uint32_t phys; // Physical address
    
    // We first need to make a new page directory and store its physical address.
    page_directory_t *dir = (page_directory_t*)kmalloc_ap(sizeof(page_directory_t), &phys);

    // Blank the page directory.
    memset(dir, 0, sizeof(page_directory_t));

    // Get the offset of the physical tables from the start of the structure.
    uint32_t offset = (uint32_t)dir->tablePhysical - (uint32_t)dir;
    
    // Calculate the physical address.
    dir->physicalAddress = phys + offset;

    // Copy each page table.
    for (int i = 0; i < 1024; i++) {
        if (!src->tables[i]) continue;
        
        if (kernelDir->tables[i] == src->tables[i]) {
            // It's in the kernel - use the same pointer.
            dir->tables[i] = src->tables[i];
            dir->tablePhysical[i] = src->tablePhysical[i];

            

        } else {
            // Copy the table.
            uint32_t phys;
            dir->tables[i] = clonePageTable(src->tables[i], &phys);
            dir->tablePhysical[i] = phys | 0x07;
        }
    }

    return dir;
}


// DEPRECATED:
static page_table_t *clonePageTable(page_table_t *src, uint32_t *physicalAddress) {
    // Make a new page table (which is page aligned)
    page_table_t *table = (page_table_t*)kmalloc_ap(sizeof(page_table_t), physicalAddress);

    // Blank the page table.
    memset(table, 0, sizeof(page_directory_t));

    // Iterate through all pages in the table
    for (int i = 0; i < 1024; i++) {
        if (src->pages[i].frame) {
            // Get a new frame.
            allocateFrame(&table->pages[i], 0, 0);

            // Clone the flags from src to dest.
            if (src->pages[i].present) table->pages[i].present = 1;
            if (src->pages[i].rw) table->pages[i].rw = 1;
            if (src->pages[i].user) table->pages[i].user = 1;
            if (src->pages[i].accessed) table->pages[i].accessed = 1;
            if (src->pages[i].dirty) table->pages[i].dirty = 1;

            // Physically copy the data across (this is defined in assembly/process.asm)
            // copyPagePhysical(src->pages[i].frame*0x1000, table->pages[i].frame*0x1000);
        } 
    }

    return table;
}

// createStack(unsigned int id) - Create a stack for paging.
void *createStack(unsigned int id) {
    if (!id) return NULL;
    if (id >= 16) return NULL; // Max tasks is 16.

    return (void*)stack[id-1];  
}

