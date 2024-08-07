// ==================================================================
// vmm.c - 86th time's the charm, right? (Virtual Memory Mangement)
// ==================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.
// NOTICE: Paging file structure and design sourced from BrokenThorn Entertainment. MOST code is written by me, but PLEASE credit them!

#include <kernel/vmm.h> // Main header file


#include <kernel/panic.h> // Kernel panicking

// Variables
pagedirectory_t *currentDirectory = 0; // Current page directory 
uint32_t currentPDBR = 0; // Current page directory base register address
bool pagingEnabled = false;


// vmm_tableLookupEntry(pagetable_t *table, uint32_t virtual_addr) - Look up an entry within the page table.
pte_t *vmm_tableLookupEntry(pagetable_t *table, uint32_t virtual_addr) {
    if (table) return &table->entries[PAGETBL_INDEX(virtual_addr)];
    else serialPrintf("vmm_tableLookupEntry: Invalid page table detected.\n");
    return 0;
}

// vmm_directoryLookupEntry(pagedirectory_t *directory, uint32_t virtual_addr) - Look up an entry within the page directory.
pde_t *vmm_directoryLookupEntry(pagedirectory_t *directory, uint32_t virtual_addr) {
    if (directory) return &directory->entries[PAGEDIR_INDEX(virtual_addr)];
    else serialPrintf("vmm_directoryLookupEntry: Invalid page directory detected.\n");
    return 0;
}

// vmm_loadPDBR(uint32_t pdbr_addr) - Loads a new value into the PDBR address
void vmm_loadPDBR(uint32_t pdbr_addr) {
    asm ("mov %0, %%cr3" :: "r"(pdbr_addr));
}

// vmm_switchDirectory(pagedirectory_t *directory) - Changes the current page directory.
bool vmm_switchDirectory(pagedirectory_t *directory) {
    if (!directory) return false;
    currentDirectory = directory;
    vmm_loadPDBR(currentDirectory); // Load the new directory into the page directory base register (CR3)
    return true;
}


// vmm_flushTLBEntry(uint32_t addr) - Invalidate the current TLB entry
void vmm_flushTLBEntry(uint32_t addr) {
    asm volatile ("cli");
    asm volatile ("invlpg (%0)" :: "r"(addr));
    asm volatile ("sti");
}

// vmm_getCurrentDirectory() - Returns the current page directory
pagedirectory_t *vmm_getCurrentDirectory() {
    return currentDirectory;
}

// vmm_allocatePage(pte_t *entry) - Allocate VMM pages
bool vmm_allocatePage(pte_t *entry) {
    // Allocate a free physical frame
    void *p = pmm_allocateBlock();
    if (!p) return false; // Failed to get the block

    // Map it to the page.
    pte_setframe(entry, (uint32_t)p);
    pte_addattrib(entry, PTE_PRESENT);

    return true;
}

// vmm_freePage() - Frees a page.
void vmm_freePage(pte_t *entry) {
    // Very simple - just free the memory block and set the PRESENT bit to 0.
    void *ptr = (void*)pte_getframe(*entry);
    if (ptr) pmm_freeBlock(ptr);

    pte_delattrib(entry, PTE_PRESENT);
}

// vmm_getPageTable(void *virtual_address) - Returns the page table
pde_t *vmm_getPageTable(void *virtual_address) {
    pagedirectory_t *pageDirectory = vmm_getCurrentDirectory();
    return &pageDirectory->entries[PAGEDIR_INDEX((uint32_t)virtual_address)];
}

// vmm_mapPage(void *physical_addr, void *virtual_addr) - Maps a page from the physical address to its virtual address
void vmm_mapPage(void *physical_addr, void *virtual_addr) {
    // Get page directory
    pagedirectory_t *pageDirectory = vmm_getCurrentDirectory();

    // Get the page table
    pde_t *entry = &pageDirectory->entries[PAGEDIR_INDEX((uint32_t)virtual_addr)];
    if ((*entry & PTE_PRESENT) != PTE_PRESENT) {
        serialPrintf("vmm_mapPage: entry not present, allocating it...\n");

        pagetable_t *table = (pagetable_t*)pmm_allocateBlock();
        if (!table) return; // failed to get the block

        // Clear the page table.
        memset(table, 0, sizeof(pagetable_t));

        // Create a new entry
        pde_t *e = &pageDirectory->entries[PAGEDIR_INDEX((uint32_t)virtual_addr)];

        // Map it in the table
        pde_addattrib(e, PDE_PRESENT);
        pde_addattrib(e, PDE_WRITABLE);
        pde_setframe(e, (uint32_t)table);
    }

    // Get the table
    pagetable_t *table = (pagetable_t*)VIRTUAL_TO_PHYS(entry);

    // Get the page.
    pte_t *page = &table->entries[PAGETBL_INDEX((uint32_t)virtual_addr)];

    // Map it in
    pte_setframe(page, (uint32_t)physical_addr);
    pte_addattrib(page, PTE_PRESENT);
}

// vmm_enablePaging() - Turns on paging
void vmm_enablePaging() {
    uint32_t cr0, cr4;

    asm volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 = cr4 & 0xffffffef;
    asm volatile ("mov %0, %%cr4" :: "r"(cr4));

    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 = cr0 | 0x80000000;

    asm volatile ("mov %0, %%cr0" :: "r"(cr0));

    pagingEnabled = true;
}


// vmm_disablePaging() - Disables paging
void vmm_disablePaging() {
    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 = cr0 & 0x7FFFFFFF;
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));

    pagingEnabled = false;
}

// vmm_allocateRegion(uint32_t physical_address, uint32_t virtual_address, size_t size) - Identity map a region
void vmm_allocateRegion(uint32_t physical_address, uint32_t virtual_address, size_t size) {
    if (size < 1) return; // Size 0. I hate users.


    // Allocate a page table.
    pagetable_t *table = (pagetable_t*)pmm_allocateBlock();

    // Clear the page table
    memset(table, 0, sizeof(pagetable_t));

    // Identity map physical_address to virtual_address.
    for (int i = 0, frame=physical_address, virt=virtual_address; i < size / 4096; i++, frame += 4096, virt += 4096) {
        pte_t page = 0;
        pte_addattrib(&page, PTE_PRESENT);
        pte_addattrib(&page, PTE_WRITABLE);
        pte_addattrib(&page, PTE_USER);
        pte_setframe(&page, frame);

        // Add the above page to the page table.
        table->entries[PAGETBL_INDEX(virt)] = page;
    }

    // Create a new page directory entry
    pde_t *pagedir_entry = &(vmm_getCurrentDirectory())->entries[PAGEDIR_INDEX(virtual_address)];
    pde_addattrib(pagedir_entry, PDE_PRESENT);
    pde_addattrib(pagedir_entry, PDE_WRITABLE);
    pde_addattrib(pagedir_entry, PDE_USER);
    pde_setframe(pagedir_entry, (uint32_t)table);
}




// vmmInit() - Initialize the VMM.
void vmmInit() {
    // Allocate the default page table.
    pagetable_t *table = (pagetable_t*)pmm_allocateBlock();
    if (!table) return; // failed to get the block

    // Allocate a second page table
    pagetable_t *table2 = (pagetable_t*)pmm_allocateBlock();
    if (!table2) return; // failed to get the block

    // Allocate a third for our big boy kernel
    pagetable_t *table3 = (pagetable_t*)pmm_allocateBlock();
    if (!table3) return; // failed to get the block


    // Why do we do this instead of calling vmm_allocateRegion? Well that is a very simple question, and here is my answer:
    pagetable_t *table4 = (pagetable_t*)pmm_allocateBlock();
    if (!table4) return; // failed to get the block

    // Clear the page table.
    memset(table, 0, sizeof(pagetable_t));
    memset(table2, 0, sizeof(pagetable_t));
    memset(table3, 0, sizeof(pagetable_t));
    memset(table4, 0, sizeof(pagetable_t));

    // Identity map the first 4MB, since when paging is enabled, all addresses are made virtual.
    for (int i = 0, frame=0x1000, virt=0x00001000; i < 1024; i++, frame += 4096, virt += 4096) {
        // Create a new page and change its frame.
        pte_t page = 0;
        pte_addattrib(&page, PTE_PRESENT);
        pte_addattrib(&page, PTE_WRITABLE);
        pte_addattrib(&page, PTE_USER);
        pte_setframe(&page, frame);

        // Add the above page to the page table.
        table->entries[PAGETBL_INDEX(virt)] = page;
    }
    
    // Remove the present flag for the first 4KB, for debugging, as if a pointer goes to 0x0, we'll know
    /*pte_t page = 0;
    pte_setframe(&page, 0x0);
    if (pte_ispresent(page)) pte_delattrib(page, PTE_PRESENT);
    if (pte_iswritable(page)) pte_delattrib(page, PTE_WRITABLE);

    table->entries[PAGETBL_INDEX(0x00000000)] = page;*/


    for (int i = 0, frame=0x400000, virt=0x00400000; i < 1024; i++, frame += 4096, virt += 4096) {
        // Create a new page and change its frame.
        pte_t page = 0;
        pte_addattrib(&page, PTE_PRESENT);
        pte_addattrib(&page, PTE_WRITABLE);
        pte_addattrib(&page, PTE_USER);
        pte_setframe(&page, frame);

        // Add the above page to the page table.
        table2->entries[PAGETBL_INDEX(virt)] = page;
    }

    for (int i = 0, frame=0x800000, virt=0x00800000; i < 1024; i++, frame += 4096, virt += 4096) {
        // Create a new page and change its frame.
        pte_t page = 0;
        pte_addattrib(&page, PTE_PRESENT);
        pte_addattrib(&page, PTE_WRITABLE);
        pte_addattrib(&page, PTE_USER);
        pte_setframe(&page, frame);

        // Add the above page to the page table.
        table3->entries[PAGETBL_INDEX(virt)] = page;
    }

    for (int i = 0, frame=0xC00000, virt=0x00C00000; i < 1024; i++, frame += 4096, virt += 4096) {
        // Create a new page and change its frame.
        pte_t page = 0;
        pte_addattrib(&page, PTE_PRESENT);
        pte_addattrib(&page, PTE_WRITABLE);
        pte_addattrib(&page, PTE_USER);
        pte_setframe(&page, frame);

        // Add the above page to the page table.
        table4->entries[PAGETBL_INDEX(virt)] = page;
    }

    // Create the default page directory and clear it.
    pagedirectory_t *dir = (pagedirectory_t*)pmm_allocateBlocks(6);
    if (!dir) return; // failed to get the block

    memset(dir, 0, sizeof(pagedirectory_t));

    // Now, since we identity mapped the first 4MB, we need to create a PDE for that (three for all tables).
    // The first PDE is for the first 4MB, the second is for the next, ...
    pde_t *entry = &dir->entries[PAGEDIR_INDEX(0x00000000)];
    pde_addattrib(entry, PDE_PRESENT);
    pde_addattrib(entry, PDE_WRITABLE);
    pde_addattrib(entry, PDE_USER);
    pde_setframe(entry, (uint32_t)table);

    pde_t *entry2 = &dir->entries[PAGEDIR_INDEX(0x00400000)];
    pde_addattrib(entry2, PDE_PRESENT);
    pde_addattrib(entry2, PDE_WRITABLE);
    pde_addattrib(entry2, PDE_USER);
    pde_setframe(entry2, (uint32_t)table2);

    pde_t *entry3 = &dir->entries[PAGEDIR_INDEX(0x00800000)];
    pde_addattrib(entry3, PDE_PRESENT);
    pde_addattrib(entry3, PDE_WRITABLE);
    pde_addattrib(entry3, PDE_USER);
    pde_setframe(entry3, (uint32_t)table3);

    pde_t *entry4 = &dir->entries[PAGEDIR_INDEX(0x00C00000)];
    pde_addattrib(entry4, PDE_PRESENT);
    pde_addattrib(entry4, PDE_WRITABLE);
    pde_addattrib(entry4, PDE_USER);
    pde_setframe(entry4, (uint32_t)table4);

    // Set our ISR interrupt handler
    isrRegisterInterruptHandler(14, pageFault);

    // Store the current PDBR
    currentPDBR = (uint32_t)&dir->entries;

    // Switch to the page directory.
    vmm_switchDirectory(dir);





    // Enable paging!
    vmm_enablePaging();

    serialPrintf("vmmInit: Successfully initialized paging.\n");
}

