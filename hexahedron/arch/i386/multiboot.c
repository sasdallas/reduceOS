/**
 * @file hexahedron/arch/i386/multiboot.c
 * @brief 
 * 
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
 * @brief Parse a Multiboot 2 header and packs into a @c generic_parameters structure
 * @param bootinfo A pointer to the multiboot informatino
 * @returns A generic parameters structure
 */
generic_parameters_t *arch_parse_multiboot2(multiboot_t *bootinfo) {
    return NULL;
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
    if (strlen((char*)bootinfo->cmdline) > 0) {
        parameters->kernel_cmdline = (char*)arch_relocate_structure(bootinfo->cmdline, strlen((char*)bootinfo->cmdline));
        parameters->kernel_cmdline[strlen(parameters->kernel_cmdline) - 1] = 0;
    } else {
        parameters->kernel_cmdline = (char*)arch_allocate_structure(1); // XXX: wtf am i doing
    }

    if (strlen((char*)bootinfo->boot_loader_name) > 0) {
        parameters->bootloader_name = (char*)arch_relocate_structure(bootinfo->boot_loader_name, strlen((char*)bootinfo->boot_loader_name));
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
    multiboot1_mod_t *module = (multiboot1_mod_t*)bootinfo->mods_addr;
    parameters->module_start = (generic_module_desc_t*)arch_allocate_structure(sizeof(generic_module_desc_t));
    parameters->module_start->cmdline = (char*)arch_relocate_structure((uintptr_t)module->cmdline, strlen((char*)module->cmdline));
    parameters->module_start->mod_start = module->mod_start;
    parameters->module_start->mod_end = module->mod_end;
    
    
    if (bootinfo->mods_count == 1) goto _done_modules;

    // Iterate
    generic_module_desc_t *last_mod_descriptor = parameters->module_start;
    for (uint32_t i = 1; i < bootinfo->mods_count; i++) {
        module++;

        generic_module_desc_t *mod_descriptor = (generic_module_desc_t*)arch_allocate_structure(sizeof(generic_module_desc_t));
        mod_descriptor->cmdline = (char*)arch_relocate_structure((uintptr_t)module->cmdline, strlen((char*)module->cmdline));
        
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
    if (bootinfo->mmap_length == 0) kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "arch", "The kernel requires a memory map to startup properly. A memory map was not found in the Multiboot structure.\n");


    parameters->mem_size = bootinfo->mem_upper + bootinfo->mem_lower;

    parameters->mmap_start = (generic_mmap_desc_t*)arch_allocate_structure(sizeof(generic_mmap_desc_t));

    multiboot1_mmap_entry_t *mmap;
    generic_mmap_desc_t *last_mmap_descriptor = parameters->mmap_start;
    generic_mmap_desc_t *descriptor = parameters->mmap_start;
    

    for (mmap = (multiboot1_mmap_entry_t*)bootinfo->mmap_addr;
                (uint32_t)mmap < bootinfo->mmap_addr + bootinfo->mmap_length;
                mmap = (multiboot1_mmap_entry_t*) ((uint32_t)mmap + mmap->size + sizeof(mmap->size))) {
                

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

        dprintf(DEBUG, "Memory descriptor 0x%x - 0x%016llX len 0x%016llX type 0x%x last descriptor 0x%x\n", descriptor, descriptor->address, descriptor->length, descriptor->type, last_mmap_descriptor);

        if ((uintptr_t)mmap != bootinfo->mmap_addr) {
            last_mmap_descriptor->next = descriptor;
            last_mmap_descriptor = descriptor;
        }

        descriptor = (generic_mmap_desc_t*)arch_allocate_structure(sizeof(generic_mmap_desc_t));
    }

    last_mmap_descriptor->next = NULL;

      
    return parameters;    
}

/**
 * @brief Mark/unmark valid spots in memory
 * @todo Work in tandem with mem.h to allow for a maximum amount of blocks to be used
 */
void arch_mark_memory(generic_parameters_t *parameters) {
    generic_mmap_desc_t *mmap = parameters->mmap_start;
    while (mmap) {
        // Working with 64-bits in a 32-bit environment is scary...
        if (mmap->address > UINT32_MAX) {
            dprintf(WARN, "Bad memory descriptor encountered - %016llX length %016llX (32-bit - 64-bit overflow)\n", mmap->address, mmap->length);
            mmap = mmap->next;
            continue;
        }

        if (mmap->type == GENERIC_MEMORY_AVAILABLE) {
            dprintf(DEBUG, "Marked memory descriptor %016llX - %016llX (%i KB)\n", mmap->address, mmap->address + mmap->length, mmap->length / 1024);
            pmm_initializeRegion((uintptr_t)mmap->address, (uintptr_t)mmap->length);
        } 

        mmap = mmap->next;
    }

    // Unmark kernel region
    

    dprintf(DEBUG, "Marked valid memory - PMM has %i free blocks / %i max blocks\n", pmm_getFreeBlocks(), pmm_getMaximumBlocks());
}