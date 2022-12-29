// =======================================================
// paging.c - reduceOS paging handler, manages memory
// =======================================================
// This implementation is not by me. Credits in the header file.

#include "include/paging.h" // Main header file.

// Variables
page_directory_t *currentDirectory = 0; // Current directory table
physicalAddress currentPDBR = 0; // Current page directory base register


// paging_tableLookupEntry(page_table_t *p, virtualAddress addr) - Searches the page table p for a virtual address.
pt_entry *paging_tableLookupEntry(page_table_t *p, virtualAddress addr) {
    if (p) return &p->m_entries[PAGE_TABLE_INDEX(addr)];
    return 0;
}

// paging_directoryLookupEntry(page_directory_t *p, virtualAddress addr) - Searches the page directory p for a virtual address.
pd_entry *paging_directoryLookupEntry(page_directory_t *p, virtualAddress addr) {
    if (p) return &p->m_entries[PAGE_TABLE_INDEX(addr)];
    return 0;
}

// paging_switchDirectory(page_directory_t *dir) - Switches the current page directory to dir.
bool paging_switchDirectory(page_directory_t *dir) {
    if (!dir) return false; // Invalid directory.

    // Switch current directory and load PDBR
    currentDirectory = dir;
    loadPDBR(currentPDBR);
    return true;
}

// paging_flushTlbEntry(virtualAddress addr) - Flushes a tlb entry at virtual address address
void paging_flushTlbEntry(virtualAddress addr) {
    // Use the invlpg instruction to flush tlb entry
    asm volatile ("invlpg %0" :: "m"(addr));   
}


// paging_getDirectory() - Returns address of current directory.
page_directory_t *paging_getDirectory() {
    return currentDirectory;
}

// paging_allocatePage(pt_entry *e) - Allocate a page
bool paging_allocatePage(pt_entry *e) {
    // First, allocate a free physical frame
    void *p = memPhys_allocateBlock();
    if (!p) return false; // Allocation failed or not enough memory.

    // Next, map it to the page.
    pt_entry_set_frame(e, (physicalAddress)p);
    pt_entry_add_attribute(e, I86_PTE_PRESENT);
    // Doesn't set write flag.

    return true;
}

// paging_freePage(pt_entry *e) - Free a page.
void paging_freePage(pt_entry *e) {
    // First, get the pfn of the pt_entry.
    void *p = (void*)pt_entry_pfn(*e);
    if (p) memPhys_freeBlock(p);

    // Remove the present attribute
    pt_entry_del_attribute(e, I86_PTE_PRESENT);
}


// paging_mapPage(void *phys, void *virt) - Map a page to a physical and virtual address
void paging_mapPage(void *phys, void *virt) {
    // First, get the page directory.
    page_directory_t *pageDirectory = paging_getDirectory();

    // Next, get the page table.
    pd_entry *e = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((uint32_t)virt)];
    
    // Check if the page table isn't present.
    if ((*e & I86_PTE_PRESENT) != I86_PTE_PRESENT) {
        // Page table not present, allocate it.
        page_table_t *table = (page_table_t*)memPhys_allocateBlock();
        if (!table) return;

        // Clear page table.
        memset(table, 0, sizeof(page_table_t));

        // Create a new entry
        pd_entry *entry = &pageDirectory->m_entries[PAGE_DIRECTORY_INDEX((uint32_t) virt)];

        // Map in the table (you can also do *entry |= 3) to enable these bits
        pd_entry_add_attribute(entry, I86_PDE_PRESENT);
        pd_entry_add_attribute(entry, I86_PDE_WRITABLE);
        pd_entry_set_frame(entry, (physicalAddress)table);
    }

    // Get the table and page
    page_table_t *table = (page_table_t*) PAGE_GET_PHYSICAL_ADDRESS(e);
    pt_entry *page = &table->m_entries[PAGE_TABLE_INDEX((uint32_t)virt)];

    // Map it in.
    pt_entry_set_frame(page, (physicalAddress)phys);
    pt_entry_add_attribute(page, I86_PTE_PRESENT);
}

// paging_initialize() - Initialize paging, set variables, allocate default pages, etc.
void paging_initialize() {
    // First, allocate the default page table
    page_table_t* table = (page_table_t*)memPhys_allocateBlock();
    if (!table) return;

    // Next, allocate 3 gb page table
    page_table_t* table2 = (page_table_t*)memPhys_allocateBlock();
    if (!table2) return;

    // Clear the page table.
    memset(table, 0, sizeof(page_table_t));
    

    // Identity map the 1st 4mb.
    for (int i = 0, frame = 0x0, virt=0x00000000; i < 1024; i++, frame += 4096, virt += 4096) {
        // Create a new page
        pt_entry page = 0;
        pt_entry_add_attribute(&page, I86_PTE_PRESENT);
        pt_entry_set_frame(&page, frame);

        // Add it to the page table.
        table2->m_entries[PAGE_TABLE_INDEX(virt)] = page;
    }

    // Now, map 1.1 kb to 3 gb (where we are at)
    for (int i=0, frame=0x100000, virt=0xc0000000; i<1024; i++, frame+=4096, virt+=4096) {
        // Create a new page
        pt_entry page =0;
        pt_entry_add_attribute(&page, I86_PTE_PRESENT);
        pt_entry_set_frame(&page, frame);
        
        // Add it to page table...
        table->m_entries[PAGE_TABLE_INDEX(virt)] = page;
    }

    // Create default directory table
    page_directory_t *dir = (page_directory_t*)memPhys_allocateBlocks(3);
    if (!dir) return; // Not enough memory or failed to allocat.e

    // Clear directory table and set it as current
    memset(dir, 0, sizeof(page_directory_t));

    // Get first entry in directory table and set it up to point to our table.
    pd_entry *entry = &dir->m_entries[PAGE_DIRECTORY_INDEX(0xC0000000)];
    pd_entry_add_attribute(entry, I86_PDE_PRESENT);
    pd_entry_add_attribute(entry, I86_PDE_WRITABLE);
    pd_entry_set_frame(entry, (physicalAddress)table);

    pd_entry *entry2 = &dir->m_entries[PAGE_DIRECTORY_INDEX(0x00000000)];
    pd_entry_add_attribute(entry2, I86_PDE_PRESENT);
    pd_entry_add_attribute(entry2, I86_PDE_WRITABLE);
    pd_entry_set_frame(entry2, (physicalAddress)table2);

    // Store current PDBR.
    currentPDBR = (physicalAddress)&dir->m_entries;

    // Switch to our page directory
    paging_switchDirectory(dir);

    // Don't enable paging, we already have it enabled.

}
