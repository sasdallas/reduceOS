// paging.h - Header file for paging.c (handles memory)
// This implementation is from BrokenThorn Entertainment, modified to support C.

#ifndef PAGING_H
#define PAGING_H

// Includes
#include "include/libc/stdint.h" // Integer definitions, like uint8_t
#include "include/libc/string.h" // String functions
#include "include/terminal.h" // printf()
#include "include/paging_pde.h" // PDE functions (page directory entry)
#include "include/paging_pte.h" // PTE functions (page table entry)
#include "include/panic.h" // Panicking
// Definitions

// x86 arch defines 1024 entries per table.
#define PAGES_PER_TABLE 1024
#define PAGES_PER_DIR 1024

#define PTABLE_ADDR_SPACE_SIZE 0x400000 // Page table represents 4 mb address space
#define DTABLE_ADDR_SPACE_SIZE 0x100000000 // Directory table represents 4 gb address space
#define PAGE_SIZE 4096 // Page sizes are 4k

// Macros
#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0xFFF)


// Typedefs
typedef uint32_t virtualAddress;

// Page table
typedef struct {
    pt_entry m_entries[PAGES_PER_TABLE];
} page_table_t;

// Page directory
typedef struct {
    pd_entry m_entries[PAGES_PER_DIR];
} page_directory_t;


// Functions:
pt_entry *paging_tableLookupEntry(page_table_t *p, virtualAddress addr); // Searches the page table p for a virtual address.
pd_entry *paging_directoryLookupEntry(page_directory_t *p, virtualAddress addr); // Searches the page directory p for a virtual address.
bool paging_switchDirectory(page_directory_t *dir); // Switches the current page directory to dir.
void paging_flushTlbEntry(virtualAddress addr); // Flushes a tlb entry at virtual address address
page_directory_t *paging_getDirectory(); // Returns address of current directory.
bool paging_allocatePage(pt_entry *e); // Allocate a page
void paging_freePage(pt_entry *e); // Free a page.
void paging_mapPage(void *phys, void *virt); // Map a page to a physical and virtual address
void paging_initialize(); // Initialize paging, set variables, allocate default pages, etc.


#endif