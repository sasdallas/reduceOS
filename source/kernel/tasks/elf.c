// =======================================================
// elf.c - Executable & Linkable Format loader
// =======================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

// Implementation source: https://wiki.osdev.org/ELF_Tutorial

#include <kernel/elf.h> // Main header file


/* EHDR FUNCTIONS */

// elf_checkFile(Elf32_Ehdr *ehdr) - Validates the EHDR header for an ELF file
int elf_checkFile(Elf32_Ehdr *ehdr) {
    if (!ehdr) return -1; // No header to check

    // Validate MAG0-3
    if (ehdr->e_ident[EI_MAG0] != ELF_MAG0) {
        serialPrintf("elf_checkFile: EHDR check fail - EI_MAG0 incorrect (given 0x%x, expected 0x%x).\n", ehdr->e_ident[EI_MAG0], ELF_MAG0);
        return -1;
    }

    if (ehdr->e_ident[EI_MAG1] != ELF_MAG1) {
        serialPrintf("elf_checkFile: EHDR check fail - EI_MAG1 incorrect (given 0x%x, expected 0x%x).\n", ehdr->e_ident[EI_MAG1], ELF_MAG1);
        return -1;
    }

    if (ehdr->e_ident[EI_MAG2] != ELF_MAG2) {
        serialPrintf("elf_checkFile: EHDR check fail - EI_MAG2 incorrect (given 0x%x, expected 0x%x).\n", ehdr->e_ident[EI_MAG2], ELF_MAG2);
        return -1;
    }

    if (ehdr->e_ident[EI_MAG3] != ELF_MAG3) {
        serialPrintf("elf_checkFile: EHDR check fail - EI_MAG3 incorrect (given 0x%x, expected 0x%x).\n", ehdr->e_ident[EI_MAG3], ELF_MAG3);
        return -1;
    }

    // Success!
    return 0;
}

// elf_isCompatible(Elf32_Ehdr *ehdr) - Returns whether the ELF file is supported by the current architecture
int elf_isCompatible(Elf32_Ehdr *ehdr) {
    if (elf_checkFile(ehdr) != 0) {
        serialPrintf("elf_isCompatible: EHDR check failed. Cannot continue\n");
        return -1;
    }

    if (ehdr->e_ident[EI_CLASS] != ELF_CLASS32) {
        serialPrintf("elf_isCompatible: EI_CLASS is not for i386 (reported 0x%x)\n", ehdr->e_ident[EI_CLASS]);
        return -1;
    }

    if (ehdr->e_ident[EI_DATA] != ELF_DATA2LSB) {
        serialPrintf("elf_isCompatible: EI_DATA is not little endian (reported 0x%x)\n", ehdr->e_ident[EI_DATA]);
        return -1;
    }

    if (ehdr->e_machine != EM_386) {
        serialPrintf("elf_isCompatible: Machine is not x86 (reported 0x%x)\n", ehdr->e_machine);
        return -1;
    }

    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
        serialPrintf("elf_isCompatible: Unsupported ELF version (reported 0x%x)\n", ehdr->e_ident[EI_VERSION]);
        return -1;
    }

    if (ehdr->e_type != ET_REL && ehdr->e_type != ET_EXEC) {
        serialPrintf("elf_isCompatible: Unknown type of ELF file (only rel and exec are supported, reported 0x%x)\n", ehdr->e_type);
        return -1;
    }

    return 0;
}


/* SECTION FUNCTIONS */

// (static) elf_sheader(Elf32_Ehdr *ehdr) - Returns the section header for the EHDR
static inline Elf32_Shdr *elf_sheader(Elf32_Ehdr *ehdr) {
    return (Elf32_Shdr *)((int)ehdr + ehdr->e_shoff);
}

// (static) elf_section(Elf32_Ehdr *ehdr, int index) - Returns the section header at index
static inline Elf32_Shdr *elf_section(Elf32_Ehdr *ehdr, int index) {
    return &elf_sheader(ehdr)[index];
}

// (static) elf_getStrTable(Elf32_Ehdr *ehdr) - Returns the string table for the EHDR header
static inline char *elf_getStrTable(Elf32_Ehdr *ehdr) {
    // EHDR has a field for the string table's index
    if (ehdr->e_shstrndx == SHN_UNDEF) return NULL; // No string table

    return (char *)ehdr + elf_section(ehdr, ehdr->e_shstrndx)->sh_offset;
}

// (static) elf_lookupString(ELf32_Ehdr *ehdr, int offset) - Lookup a string in the string table
static inline char *elf_lookupString(Elf32_Ehdr *ehdr, int offset) {
    char *str_table = elf_getStrTable(ehdr);
    if (str_table == NULL) return NULL; // No string table
    return str_table + offset;
}

/* SYMBOL TABLE FUNCTIONS */


// elf_lookupSymbol(const char *name) - This is a big boy! We need to search through both our reduced libc and our kernel symbols to find this bad boy.
void *elf_lookupSymbol(const char *name) {
    // We basically have three main places to check:
    // 1. Our kernel symbols, something might try to hook into the kernel such as a driver
    // 2. Our reduced libc, something like the init process might try to hook there.
    // 3. Other libraries that should be loaded (TODO)

    return NULL;
}


// (static) elf_getSymbolValue(Elf32_Ehdr *ehdr, int table, uint32_t index) - Returns the symbol value (absolute address)
static int elf_getSymbolValue(Elf32_Ehdr *ehdr, int table, uint32_t index) {
    // The symbols do contain a st_value parameter but it is a relative address.
    // We need to compute the full address if needed

    if (table == SHN_UNDEF || index == SHN_UNDEF) return 0; // Stupid users.

    // Get the section
    Elf32_Shdr *symtab = elf_section(ehdr, table);

    // Calculate the symbol table entry & sanity check
    uint32_t symtab_entries = symtab->sh_size / symtab->sh_entrysize;
    if (index >= symtab_entries) {
        serialPrintf("elf_getSymbolValue: Symbol index out of range! (%d:%u)\n", table, index);
        return -1;
    }

    // Calculate the symbol address and get it
    int symbol_address = (int)ehdr + symtab->sh_offset;
    Elf32_Sym *symbol = &((Elf32_Sym*)symbol_address)[index];

    // Now we can check the type and calculate accordingly
    if (symbol->st_shndx == SHN_UNDEF) {
        // External symbol, lookup the value
        Elf32_Shdr *strtab = elf_section(ehdr, symtab->sh_link);
        const char *symname = (const char *)ehdr + strtab->sh_offset + symbol->st_name;

        void *target = elf_lookupSymbol(symname);

        if (target == NULL) {
            // We could not find the external symbol :(

            // It might be weak though, meaning we can ignore it until it actually becomes a problem.
            if (ELF32_ST_BIND(symbol->st_info) & STB_WEAK) {
                // It is - let's ignore it!
                return 0;
            } else {
                // No, it's not - this is fatal.
                serialPrintf("elf_getSymbolValue: External symbol '%s' not found.\n", symname);
                return ELF_RELOC_ERROR;
            }
        } else {
            return (int)target;
        }
    } else if (symbol->st_shndx == SHN_ABS) {
        // Absolute symbol, we're done.
        return symbol->st_value;
    } else {
        // Internally defined symbol
        Elf32_Shdr *target = elf_section(ehdr, symbol->st_shndx);
        return (int)ehdr + symbol->st_value + target->sh_offset;
    }
}



/* PHDR FUNCTIONS */

// (static) elf_getPHDR(Elf32_Ehdr *ehdr) - Returns the ELF program header
static Elf32_Phdr *elf_getPHDR(Elf32_Ehdr *ehdr) {
    return (Elf32_Phdr*)((int)ehdr + ehdr->e_phoff);
}




/* LOADING FUNCTIONS */

// (static) elf_loadStage1(Elf32_Ehdr *ehdr) - Performs the first stage of loading an ELF file by alloacting memory for the sections.
static int elf_loadStage1(Elf32_Ehdr *ehdr) {
    Elf32_Shdr *shdr = elf_sheader(ehdr);

    // TODO: This is a very simple implementation.

    // Iterate over the section headers
    for (unsigned int i = 0; i < ehdr->e_shnum; i++) {
        Elf32_Shdr *section = &shdr[i];
        
        // If the section isn't present in the file
        if (section->sh_type == SHT_NOBITS) {
            // Skip it if the section is empty
            if (!section->sh_size) continue; // No need allocate anything.

            if (section->sh_flags & SHF_ALLOC) {
                // Alloacte and zero out some memory.
                void *memory = kmalloc(section->sh_size);
                memset(memory, 0, section->sh_size);

                // Assign the memory offset to the section offset
                section->sh_offset = (int)memory -(int)ehdr;
            }
        }
    }

    return 0;
}

// (static) elf_relocate(Elf32_Ehdr *ehdr, Elf32_Rel *rel, Elf32_Shdr *reltab) - Perform relocation
static int elf_relocate(Elf32_Ehdr *ehdr, Elf32_Rel *rel, Elf32_Shdr *reltab) {
    Elf32_Shdr *target = elf_section(ehdr, reltab->sh_info);

    int addr = (int)ehdr + target->sh_offset;
    int *ref = (int*)(addr + rel->r_offset);

    // Symbol value
    int symval = 0;
    if (ELF32_R_SYM(rel->r_info) != SHN_UNDEF) {
        // We actually should get this
        symval = elf_getSymbolValue(ehdr, reltab->sh_link, ELF32_R_SYM(rel->r_info));
        if (symval == ELF_RELOC_ERROR) return ELF_RELOC_ERROR;
    }

    // Relocate based on the type
    switch (ELF32_R_TYPE(rel->r_info)) {
        case R_386_NONE:
            // No relocation needed
            break;
        case R_386_32:
            // Symbol + Offset
            *ref = DO_386_32(symval, *ref);
            break;
        case R_386_PC32:
            // Symbol + Offset - Section Offset
            *ref = DO_386_PC32(symval, *ref, (int)ref);
            break;
        default:
            serialPrintf("elf_relocate: Unsupported relocation type (%d)\n", ELF32_R_TYPE(rel->r_info));
            return ELF_RELOC_ERROR;
    }

    return symval;
}


// (static) elf_loadStage2(Elf32_Ehdr *ehdr) - Loads the second stage by processing all relocatable symbols
static int elf_loadStage2(Elf32_Ehdr *ehdr) {
    Elf32_Shdr *shdr = elf_sheader(ehdr);

    // Iterate over the section headers
    unsigned int i, idx;
    for (i = 0; i < ehdr->e_shnum; i++) {
        Elf32_Shdr *section = &shdr[i];

        // Check if the section is relocatable
        if (section->sh_type == SHT_REL) {
            // Process each entry in the table
            for (idx = 0; idx < section->sh_size / section->sh_entrysize; idx++) {
                Elf32_Rel *reltab = &((Elf32_Rel*)((int)ehdr + section->sh_offset))[idx];
                int result = elf_relocate(ehdr, reltab, section);

                // On result, display message and return
                if (result == ELF_RELOC_ERROR) {
                    serialPrintf("elf_loadStage2: Could not relocate symbol\n");
                }
            }
        }
    }

    return 0;
}


// (static) elf_loadRelocatable(Elf32_Ehdr *ehdr) - Loads a relocatable ELF file into memory
static inline void *elf_loadRelocatable(Elf32_Ehdr *ehdr) {
    int result;

    // Load stage 1 and stage 2, and parse the program header
    result = elf_loadStage1(ehdr);
    if (result == ELF_RELOC_ERROR) {
        serialPrintf("elf_loadRelocatable: Failed to load ELF file (stage 1 error).\n");
        return NULL;
    }

    result = elf_loadStage2(ehdr);
    if (result == ELF_RELOC_ERROR) {
        serialPrintf("elf_loadRelocatable: Failed to load ELF file (stage 2 error)\n");
        return NULL;
    }

    // Parse the program header

    return (void*)ehdr->e_entry;
}


// elf_loadFileFromBuffer(void *buf) - Loads an ELF file from a buffer with its contents
void *elf_loadFileFromBuffer(void *buf) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr*)buf;
    
    // Check to make sure it is actually supported
    if (elf_isCompatible(ehdr) != 0) {
        serialPrintf("elf_loadFileFromBuffer: ELF file cannot be loaded.\n");
        return NULL;
    }

    // Check the type and do what's necessary for the file.
    switch (ehdr->e_type) {
        case ET_EXEC:
            // TODO: actually do this lol
            return NULL;
        case ET_REL:
            return elf_loadRelocatable(ehdr);
        default:
            serialPrintf("elf_loadFileFromBuffer: This should've been caught by isCompatible.\n");
            return NULL;
    }

    return NULL;
}