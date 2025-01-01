/**
 * @file hexahedron/include/kernel/loader/elf_loader.h
 * @brief ELF loader header
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_LOADER_ELF_LOADER_H
#define KERNEL_LOADER_ELF_LOADER_H

/**** INCLUDES ****/
#include <stdint.h>
#include <kernel/fs/vfs.h>

/**** DEFINITIONS ****/

#define ELF_KERNEL  0   // Load an ELF file with full kernel access (drivers)
#define ELF_USER    1   // Load an ELF file with usermode access (programs)

#define ELF_RELOC_FAIL  (uintptr_t)-1       // Ugliest value ever...

/**** FUNCTIONS ****/

/**
 * @brief Find a specific symbol by name and get its value
 * @param ehdr_address The address of the EHDR (as elf64/elf32 could be in use)
 * @param name The name of the symbol
 * @returns A pointer to the symbol or NULL
 * 
 * @warning Make sure you've initialized the file!
 */
uintptr_t elf_findSymbol(uintptr_t ehdr_address, char *name);

/**
 * @brief Load an ELF file fully
 * @param fbuf The buffer to load the file from
 * @param flags The flags to use (ELF_KERNEL or ELF_USER)
 * @returns A pointer to the file that can be passed to @c elf_startExecution or @c elf_findSymbol - or NULL if there was an error. 
 */
uintptr_t elf_loadBuffer(uint8_t *fbuf, int flags);

/**
 * @brief Load an ELF file into memory
 * @param file The file to load into memory
 * @param flags The flags to use (ELF_KERNEL or ELF_USER)
 * @returns A pointer to the file that can be passed to @c elf_startExecution or @c elf_findSymbol - or NULL if there was an error. 
 */
uintptr_t elf_load(fs_node_t *node, int flags);



#endif