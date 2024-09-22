// mem.h - A header file for the entire memory management system

#ifndef MEM_H
#define MEM_H

// Includes
#include <kernel/vmm.h>
#include <libk_reduced/stdint.h> // Integer declarations
#include <libk_reduced/string.h> // String functions
#include <libk_reduced/assert.h>

#if defined(ARCH_I386) || defined(__INTELLISENSE__)
#include <kernel/vmm_pte.h>
#include <kernel/vmm_pde.h>
#include <kernel/arch/i386/page.h>
#else
#error "Unknown or unsupported architecture"
#endif

// Macros
#define MEM_ALIGN_PAGE(addr) (((uint32_t)addr & 0xFFFFF000) + 4096)
#define MEM_PAGEDIR_INDEX(x) (((x) >> 22) & 0x3ff) // Returns the index of x within the PDE
#define MEM_PAGETBL_INDEX(x) (((x) >> 12) & 0x3ff) // Returns the index of x within the PTE
#define MEM_VIRTUAL_TO_PHYS(addr) (*addr & ~0xFFF) // Returns the physical address of addr.

// Flags for the architecture memory mapper
// By default, if created, a page will be given user-mode and write access.
#define MEM_CREATE              0x01    // Create the page. Commonly used with mem_getPage during mappings
#define MEM_KERNEL              0x02    // The page is kernel-mode only
#define MEM_READONLY            0x04    // The page is read-only
#define MEM_WRITETHROUGH        0x08    // The page is marked as writethrough
#define MEM_NOT_CACHEABLE       0x10    // The page is not cacheable
#define MEM_NOALLOC             0x20    // Do not allocate the page and instead use what was given
#define MEM_NOT_PRESENT         0x40    // The page is not present in memory

// errno doesn't have specifications for MEM, and this is kernel only.
// These are the possible error values
#define MEM_ERR_PRESENT -1      // The page isn't present in memory


/******************* FUNCTIONS *******************/


/**
 * @brief Get the physical address of a virtual address
 * @param dir Can be NULL to use the current directory.
 * @param virtaddr The virtual address
 */
uintptr_t mem_getPhysicalAddress(pagedirectory_t *dir, uintptr_t virtaddr);

/**
 * @brief Returns the page entry requested as a PTE
 * @param dir The directory to search. Specify NULL for current directory
 * @param addr The virtual address of the page
 * @param flags The flags of the page to look for
 * 
 * @warning Specifying MEM_CREATE will only create needed structures, it will NOT allocate the page!
 *          Please use a function such as mem_allocatePage to do that.
 */
pte_t *mem_getPage(pagedirectory_t *dir, uintptr_t addr, uintptr_t flags);

/**
 * @brief Switch the memory management directory
 * @param pagedir The page directory to switch to
 * @note This will also reload the PDBR.
 * @returns -EINVAL on invalid, 0 on success.
 */
int mem_switchDirectory(pagedirectory_t *pagedir);

/**
 * @brief Allocate a page using the physical memory manager
 * 
 * @param page The page object to use. Can be obtained with mem_getPage
 * @param flags The flags to follow when setting up the page
 * 
 * @note You can also use this function to set bits of a specific page - just specify @c MEM_NOALLOC in @p flags.
 * @warning The function will automatically allocate a PMM block if NOALLOC isn't specified.
 */
void mem_allocatePage(pte_t *page, uint32_t flags);


/**
 * @brief Die in the cold winter
 * @param pages How many pages were trying to be allocated
 * @param seq The sequence of failure
 */
void mem_outofmemory(int pages, char *seq);

/**
 * @brief Get the current page directory
 */
pagedirectory_t *mem_getCurrentDirectory();

/**
 * @brief Clone a page directory.
 * 
 * This is a full PROPER page directory clone. The old one just memcpyd the pagedir.
 * That's like 50% fine and 50% completely boinked (or 100%, honestly none of this vmm should work).
 * This function does it properly and clones the page directory, its tables, and their respective entries fully.
 * 
 * @param pd_in The source page directory. Keep as NULL to clone the current page directory.
 * @param pd_out The output page directory.
 * @returns 0 on success, anything else is failure (usually an errno)
 */
int mem_clone(pagedirectory_t *pd_in, pagedirectory_t *pd_out);

/**
 * @brief Initialize the memory management subsystem
 * 
 * This function will identity map the kernel into memory and setup page tables.
 */
void mem_init();

/**
 * @brief Free a page
 * 
 * @param page The page to free 
 */
void mem_freePage(pte_t *page);


/**
 * @brief Expand/shrink the kernel heap
 * 
 * @param b The amount of bytes to allocate, needs to a multiple of PAGE_SIZE
 * @returns Address of the start of the bytes 
 */
void *mem_sbrk(int b);

// Functions (liballoc)
void enable_liballoc();
void *kmalloc(size_t size);
void *krealloc(void *a, size_t b);
void *kcalloc(size_t a, size_t b);
void kfree(void *a);

#endif
