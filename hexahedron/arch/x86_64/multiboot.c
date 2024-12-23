/**
 * @file hexahedron/arch/x86_64/multiboot.c
 * @brief Multiboot functions
 * 
 * @warning This code is messy af. If you want to understand what it's doing, please just RTFM.
 *          It will make your life so much easier.
 *          https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
 *          https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
 * 
 * @warning x86_64 has a specific quirk. See bottom of file
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <stdint.h>
#include <string.h>

#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/generic_mboot.h>
#include <kernel/multiboot.h>
#include <kernel/multiboot2.h>
#include <kernel/mem/pmm.h>

extern uintptr_t arch_allocate_structure(size_t bytes);
extern uintptr_t arch_relocate_structure(uintptr_t structure_ptr, size_t size);


/**
 * @brief Find a tag
 * @param header The header of the structure to start from. Can be a tag. If you're providing bootinfo make sure to adjust += 8
 * @param type The type of tag to find
 * @todo Check against total_size for invalid MB2 tags?
 */
struct multiboot_tag *multiboot2_find_tag(void *header, uint32_t type) {
    uint8_t *start = (uint8_t*)header;
    
    while ((uintptr_t)start & 7) start++; // Skip over padding

    struct multiboot_tag *tag = (struct multiboot_tag*)start;
    while (tag->type != 0) {
        // Start going through the tags
        if (tag->type == type) return tag;
        start += tag->size;
        while ((uintptr_t)start & 7) start++;
        tag = (struct multiboot_tag*)start;
    }

    return NULL;
}


/** 
 * @brief Parse a Multiboot 2 header and packs into a @c generic_parameters structure
 * @param bootinfo A pointer to the multiboot informatino
 * @returns A generic parameters structure
 */
generic_parameters_t *arch_parse_multiboot2(multiboot_t *bootinfo) {
    void *updated_bootinfo = (void*)((uint8_t*)bootinfo + 8); // TODO: ugly af

    // First, get some bytes for a generic_parameters structure
    generic_parameters_t *parameters = (generic_parameters_t*)arch_allocate_structure(sizeof(generic_parameters_t));
    
    // Find the memory map, we'll parse it first.
    struct multiboot_tag_mmap *mmap_tag = (struct multiboot_tag_mmap*)multiboot2_find_tag(updated_bootinfo, MULTIBOOT_TAG_TYPE_MMAP);
    if (mmap_tag == NULL) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "arch", "*** The kernel requires a memory map to startup properly. A memory map was not found in the Multiboot structure.\n");
        __builtin_unreachable();
    }

    // Setup parameters for generic
    parameters->mmap_start = (generic_mmap_desc_t*)arch_allocate_structure(sizeof(generic_mmap_desc_t));
    generic_mmap_desc_t *last_mmap_descriptor = parameters->mmap_start;
    generic_mmap_desc_t *descriptor = parameters->mmap_start;

    // Start iterating!
    multiboot_memory_map_t *mmap;
    for (mmap = mmap_tag->entries; 
            (uint8_t*)mmap < (uint8_t*)mmap_tag + mmap_tag->size;
            mmap = (multiboot_memory_map_t*)((unsigned long)mmap + mmap_tag->entry_size)) {

        descriptor->address = mmap->addr;
        descriptor->length = mmap->len;
        switch (mmap->type) {
            case MULTIBOOT_MEMORY_AVAILABLE:
                descriptor->type = GENERIC_MEMORY_AVAILABLE;
                break;
            
            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                descriptor->type = GENERIC_MEMORY_ACPI_RECLAIM;
                break;
            
            case MULTIBOOT_MEMORY_NVS:
                descriptor->type = GENERIC_MEMORY_ACPI_NVS;
                break;
            
            case MULTIBOOT_MEMORY_BADRAM:
                descriptor->type = GENERIC_MEMORY_BADRAM;
                break;

            case MULTIBOOT_MEMORY_RESERVED:
            default:
                descriptor->type = GENERIC_MEMORY_RESERVED;
                break;
        }

        // Debugging output
        dprintf(DEBUG, "Memory descriptor 0x%x - 0x%016llX len 0x%016llX type 0x%x last descriptor 0x%x\n", descriptor, descriptor->address, descriptor->length, descriptor->type, last_mmap_descriptor);

        // If we're not the first update the last memory map descriptor
        if (mmap != mmap_tag->entries) {
            last_mmap_descriptor->next = descriptor;
            last_mmap_descriptor = descriptor;
        }

        // Reallocate & repeat
        descriptor = (generic_mmap_desc_t*)arch_allocate_structure(sizeof(generic_mmap_desc_t));
    }

    // Next, get the basic meminfo structure
    struct multiboot_tag_basic_meminfo *meminfo_tag = (struct multiboot_tag_basic_meminfo*)multiboot2_find_tag(updated_bootinfo, MULTIBOOT_TAG_TYPE_BASIC_MEMINFO);
    if (meminfo_tag == NULL) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "arch", "*** The kernel requires a Multiboot2 tag that was not provided (BASIC_MEMINFO)\n");
    }

    parameters->mem_size = meminfo_tag->mem_lower + meminfo_tag->mem_upper;

    // Parse modules
    struct multiboot_tag_module *mod_tag = (struct multiboot_tag_module*)multiboot2_find_tag(updated_bootinfo, MULTIBOOT_TAG_TYPE_MODULE);
    
    if (mod_tag == NULL) goto _done_modules; // Let generic deal with this...

    parameters->module_start = (generic_module_desc_t*)arch_allocate_structure(sizeof(generic_module_desc_t));
    generic_module_desc_t *module = parameters->module_start;
    generic_module_desc_t *last_module = module;
    while (mod_tag) {
        // Relocate cmdline
        module->cmdline = (char*)arch_relocate_structure((uintptr_t)mod_tag->cmdline, strlen((char*)(uintptr_t)mod_tag->cmdline));
        module->cmdline[strlen((char*)(uintptr_t)module->cmdline) - 1] = 0; // TODO: need to do this?
        
        // Relocate the module's contents
        module->mod_start = arch_relocate_structure((uintptr_t)mod_tag->mod_start, (uintptr_t)(mod_tag->mod_end - mod_tag->mod_start));
        module->mod_end = module->mod_start + (mod_tag->mod_end - mod_tag->mod_start);

        struct multiboot_tag_module *next_tag = (struct multiboot_tag_module*)multiboot2_find_tag((void*)mod_tag, MULTIBOOT_TAG_TYPE_MODULE);
        if (next_tag == NULL) break;
        mod_tag = next_tag;

        // Allocate the next one
        last_module = module;
        module = (generic_module_desc_t*)arch_allocate_structure(sizeof(generic_module_desc_t));
        last_module->next = module;
    }

_done_modules:

    // Let's get the framebuffer
    struct multiboot_tag_framebuffer *fb_tag = (struct multiboot_tag_framebuffer*)multiboot2_find_tag(updated_bootinfo, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);
    if (fb_tag) {
        parameters->framebuffer = (generic_fb_desc_t*)arch_allocate_structure(sizeof(generic_fb_desc_t));
        parameters->framebuffer->framebuffer_addr = fb_tag->common.framebuffer_addr;
        parameters->framebuffer->framebuffer_width = fb_tag->common.framebuffer_width;
        parameters->framebuffer->framebuffer_height = fb_tag->common.framebuffer_height;
        parameters->framebuffer->framebuffer_bpp = fb_tag->common.framebuffer_bpp;
        parameters->framebuffer->framebuffer_pitch = fb_tag->common.framebuffer_pitch;
    }

    // Get command line and bootloader name
    struct multiboot_tag_string *cmdline = (struct multiboot_tag_string*)multiboot2_find_tag(updated_bootinfo, MULTIBOOT_TAG_TYPE_CMDLINE);
    struct multiboot_tag_string *bootldr = (struct multiboot_tag_string*)multiboot2_find_tag(updated_bootinfo, MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME);

    // TODO: Unsafe strcpy
    if (cmdline && strlen((char*)cmdline->string) > 0) {
        parameters->kernel_cmdline = (char*)arch_relocate_structure((uintptr_t)cmdline->string, strlen((char*)cmdline->string));
        parameters->kernel_cmdline[strlen(parameters->kernel_cmdline) - 1] = 0;
    } else {
        parameters->kernel_cmdline = (char*)arch_allocate_structure(1); // !!!: wtf am i doing
    }
    

    if (bootldr && strlen((char*)bootldr->string) > 0) {
        parameters->bootloader_name = (char*)arch_relocate_structure((uintptr_t)bootldr->string, strlen((char*)bootldr->string));
        parameters->bootloader_name[strlen(parameters->bootloader_name) - 1] = 0;
    } else {
        parameters->bootloader_name = (char*)arch_allocate_structure(1);
    }


    // Finished!
    return parameters;
}







/**
 * @brief Parse a Multiboot 1 header and packs into a @c generic_parameters structure
 * @param bootinfo The Multiboot information
 * @returns A generic parameters structure
 */
generic_parameters_t *arch_parse_multiboot1(multiboot_t *bootinfo) {
    // First, get some bytes for a generic_parameters structure
    generic_parameters_t *parameters = (generic_parameters_t*)arch_allocate_structure(sizeof(generic_parameters_t));
    
    // Get the easy stuff out of the way - copy some strings.
    if (strlen((char*)(uintptr_t)bootinfo->cmdline) > 0) {
        parameters->kernel_cmdline = (char*)arch_relocate_structure(bootinfo->cmdline, strlen((char*)(uintptr_t)bootinfo->cmdline));
        parameters->kernel_cmdline[strlen(parameters->kernel_cmdline) - 1] = 0;
    } else {
        parameters->kernel_cmdline = (char*)arch_allocate_structure(1); // !!!: wtf am i doing
    }

    if (strlen((char*)(uintptr_t)bootinfo->boot_loader_name) > 0) {
        parameters->bootloader_name = (char*)arch_relocate_structure(bootinfo->boot_loader_name, strlen((char*)(uintptr_t)bootinfo->boot_loader_name));
        parameters->bootloader_name[strlen(parameters->bootloader_name) - 1] = 0;
    } else {
        parameters->bootloader_name = (char*)arch_allocate_structure(1);
    }

    // Make a framebuffer tag
    parameters->framebuffer = (generic_fb_desc_t*)arch_allocate_structure(sizeof(generic_fb_desc_t));
    parameters->framebuffer->framebuffer_addr = bootinfo->framebuffer_addr;
    parameters->framebuffer->framebuffer_width = bootinfo->framebuffer_width;
    parameters->framebuffer->framebuffer_height = bootinfo->framebuffer_height;
    parameters->framebuffer->framebuffer_bpp = bootinfo->framebuffer_bpp;
    parameters->framebuffer->framebuffer_pitch = bootinfo->framebuffer_pitch;

    // If we don't have any modules, womp womp
    if (bootinfo->mods_count == 0) goto _done_modules;

    // Construct the initial module
    multiboot1_mod_t *module = (multiboot1_mod_t*)(uintptr_t)bootinfo->mods_addr;
    parameters->module_start = (generic_module_desc_t*)arch_allocate_structure(sizeof(generic_module_desc_t));
    parameters->module_start->cmdline = (char*)arch_relocate_structure((uintptr_t)module->cmdline, strlen((char*)(uintptr_t)module->cmdline));
    parameters->module_start->mod_start = arch_relocate_structure((uintptr_t)module->mod_start, (uintptr_t)module->mod_end - (uintptr_t)module->mod_start);
    parameters->module_start->mod_end = parameters->module_start->mod_start + (module->mod_end - module->mod_start);

    if (bootinfo->mods_count == 1) goto _done_modules;

    // Iterate
    generic_module_desc_t *last_mod_descriptor = parameters->module_start;
    for (uint32_t i = 1; i < bootinfo->mods_count; i++) {
        module++;

        generic_module_desc_t *mod_descriptor = (generic_module_desc_t*)arch_allocate_structure(sizeof(generic_module_desc_t));
        mod_descriptor->cmdline = (char*)arch_relocate_structure((uintptr_t)module->cmdline, strlen((char*)(uintptr_t)module->cmdline));
        
        // Relocate the module's contents
        mod_descriptor->mod_start = arch_relocate_structure((uintptr_t)module->mod_start, (uintptr_t)module->mod_end - (uintptr_t)module->mod_start);
        mod_descriptor->mod_end = mod_descriptor->mod_start + (module->mod_end - module->mod_start);

        // Null-terminate cmdline
        mod_descriptor->cmdline[strlen(mod_descriptor->cmdline) - 1] = 0;

        last_mod_descriptor->next = mod_descriptor;
        last_mod_descriptor = mod_descriptor;
    }

_done_modules:

    // Done with the modules, handle the memory map now.
    if (!(bootinfo->flags & 0x040)) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "arch", "*** The kernel requires a memory map to startup properly. A memory map was not found in the Multiboot structure.\n");
        __builtin_unreachable();
    }

    parameters->mem_size = bootinfo->mem_upper + bootinfo->mem_lower;

    parameters->mmap_start = (generic_mmap_desc_t*)arch_allocate_structure(sizeof(generic_mmap_desc_t));

    multiboot1_mmap_entry_t *mmap;
    generic_mmap_desc_t *last_mmap_descriptor = parameters->mmap_start;
    generic_mmap_desc_t *descriptor = parameters->mmap_start;
    

    for (mmap = (multiboot1_mmap_entry_t*)(uintptr_t)bootinfo->mmap_addr;
                (uintptr_t)mmap < bootinfo->mmap_addr + bootinfo->mmap_length;
                mmap = (multiboot1_mmap_entry_t*) ((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
                

        descriptor->address = mmap->addr;
        descriptor->length = mmap->len;
        
        // Translate mmap type to generic type 
        switch (mmap->type) {
            case MULTIBOOT_MEMORY_AVAILABLE:
                descriptor->type = GENERIC_MEMORY_AVAILABLE;
                break;
            
            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                descriptor->type = GENERIC_MEMORY_ACPI_RECLAIM;
                break;
            
            case MULTIBOOT_MEMORY_NVS:
                descriptor->type = GENERIC_MEMORY_ACPI_NVS;
                break;
            
            case MULTIBOOT_MEMORY_BADRAM:
                descriptor->type = GENERIC_MEMORY_BADRAM;
                break;

            case MULTIBOOT_MEMORY_RESERVED:
            default:
                descriptor->type = GENERIC_MEMORY_RESERVED;
                break;
            
        }

        // Debugging output
        dprintf(DEBUG, "Memory descriptor 0x%x - 0x%016llX len 0x%016llX type 0x%x last descriptor 0x%x\n", descriptor, descriptor->address, descriptor->length, descriptor->type, last_mmap_descriptor);

        // If we're not the first update the last memory map descriptor
        if ((uintptr_t)mmap != bootinfo->mmap_addr) {
            last_mmap_descriptor->next = descriptor;
            last_mmap_descriptor = descriptor;
        }

        // Reallocate & repeat
        descriptor = (generic_mmap_desc_t*)arch_allocate_structure(sizeof(generic_mmap_desc_t));
    }

    last_mmap_descriptor->next = NULL;

      
    return parameters;    
}


/**** x86_64 specific ****/

#include <kernel/mem/mem.h>

static multiboot_t *stored_bootinfo = NULL;
static int is_mb2 = 0;

/**
 * @brief Mark/unmark valid spots in memory
 * @param highest_address The highest kernel address
 * @param mem_size The memory size.
 */
void arch_mark_memory(uintptr_t highest_address, uintptr_t mem_size) {
    if (!stored_bootinfo) {
        kernel_panic(KERNEL_BAD_ARGUMENT_ERROR, "multiboot");
        __builtin_unreachable();    
    }

    if (is_mb2) {
        dprintf(INFO, "Mb2 unimplemented\n");
        
    } else {
        // Bootinfo needs to be remapped using memory
        multiboot_t *bootinfo = (multiboot_t*)mem_remapPhys((uintptr_t)stored_bootinfo, MEM_ALIGN_PAGE(sizeof(multiboot_t)));

        multiboot1_mmap_entry_t *mmap = (multiboot1_mmap_entry_t*)mem_remapPhys(bootinfo->mmap_addr, MEM_ALIGN_PAGE(bootinfo->mmap_length));

        while ((uintptr_t)mmap < (uintptr_t)mem_remapPhys(bootinfo->mmap_addr + bootinfo->mmap_length, MEM_ALIGN_PAGE(bootinfo->mmap_length))) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                // Available!
                dprintf(DEBUG, "Marked memory descriptor %016llX - %016llX (%i KB) as available memory\n", mmap->addr, mmap->addr + mmap->len, mmap->len / 1024);
                pmm_initializeRegion((uintptr_t)mmap->addr, (uintptr_t)mmap->len);
            } else {
                // Make sure mmap->addr isn't out of memory - most emulators like to have reserved
                // areas outside of their actual memory space, which the PMM really does not like.
                if (mmap->addr + mmap->len < mem_size) {
                    dprintf(DEBUG, "Marked memory descriptor %016llX - %016llX (%i KB) as unavailable memory\n", mmap->addr, mmap->addr + mmap->len, mmap->len / 1024);
                    pmm_deinitializeRegion((uintptr_t)mmap->addr, (uintptr_t)mmap->len);
                } else {
                    dprintf(DEBUG, "Cannot mark memory descriptor %016llX - %016llX (%i KB)\n", mmap->addr, mmap->addr + mmap->len, mmap->len / 1024);
                }
            }

            mmap = (multiboot1_mmap_entry_t*)((uintptr_t)mmap + mmap->size + sizeof(uint32_t)); 
        }
    }

    // Unmark kernel region
    extern uintptr_t __text_start;
    uintptr_t kernel_start = (uintptr_t)&__text_start;
    dprintf(DEBUG, "Marked memory descriptor %016X - %016X (%i KB) as kernel memory\n", kernel_start, highest_address, (highest_address - kernel_start) / 1024);
    pmm_deinitializeRegion(kernel_start, highest_address - kernel_start);    

    dprintf(DEBUG, "Marked valid memory - PMM has %i free blocks / %i max blocks\n", pmm_getFreeBlocks(), pmm_getMaximumBlocks());
}




/**
 * @brief x86_64-specific parser function for Multiboot1
 * @param bootinfo The boot information
 * @param mem_size Output pointer to mem_size
 * @param kernel_address Output pointer to highest valid kernel address
 * 
 * This is here because paging is already enabled in x86_64, meaning
 * we have to initialize the allocator. It's very hacky, but it does end up working.
 * (else it will overwrite its own page tables and crash or something, i didn't do much debugging)
 */
void arch_parse_multiboot1_early(multiboot_t *bootinfo, uintptr_t *mem_size, uintptr_t *kernel_address) {
    // Store structure for later use
    stored_bootinfo = bootinfo;
    is_mb2 = 0;

    extern uintptr_t __kernel_end;
    uintptr_t kernel_addr = (uintptr_t)&__kernel_end;
    uintptr_t msize = (uintptr_t)&__kernel_end;

    // Check if memory map was provided 
    if (!(bootinfo->flags & 0x040)) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "arch", "*** The kernel requires a memory map to startup properly. A memory map was not found in the Multiboot structure.\n");
        __builtin_unreachable();
    }
    
    multiboot1_mmap_entry_t *mmap = (void*)(uintptr_t)bootinfo->mmap_addr;

    // Handle the memory map in relation to highest kernel address
    if ((uintptr_t)mmap + bootinfo->mmap_length > kernel_addr) {
        kernel_addr = (uintptr_t)mmap + bootinfo->mmap_length;
    }

    while ((uintptr_t)mmap < bootinfo->mmap_addr + bootinfo->mmap_length) {
        if (mmap->type == 1 && mmap->len && mmap->addr + mmap->len - 1 > msize) {
            msize = mmap->addr + mmap->len - 1;
        }

        mmap = (multiboot1_mmap_entry_t*)((uintptr_t)mmap + mmap->size + sizeof(uint32_t));
    }

    if (bootinfo->mods_count) {
        multiboot1_mod_t *mods = (multiboot1_mod_t*)(uintptr_t)bootinfo->mods_addr;
        for (uint32_t i = 0; i < bootinfo->mods_count; i++) {
            if ((uintptr_t)mods[i].mod_end > kernel_addr) {
                kernel_addr = mods[i].mod_end;
            }
        }
    }

    // Round maximum kernel address up a page
    kernel_addr = (kernel_addr + 0x1000) & ~0xFFF;

    // Set pointers
    *kernel_address = kernel_addr;
    *mem_size = msize; 
}

/**
 * @brief x86_64-specific parser function for Multiboot2
 * @param bootinfo The boot information
 * @param mem_size Output pointer to mem_size
 * @param kernel_address Output pointer to highest valid kernel address
 * 
 * This is here because paging is already enabled in x86_64, meaning
 * we have to initialize the allocator. It's very hacky, but it does end up working.
 * (else it will overwrite its own page tables and crash or something, i didn't do much debugging)
 */
void arch_parse_multiboot2_early(multiboot_t *bootinfo, uintptr_t *mem_size, uintptr_t *kernel_address) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "multiboot");
    __builtin_unreachable();
}