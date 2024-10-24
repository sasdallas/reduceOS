// =========================================================
// elf.c - Handles loading/booting an ELF32 file
// =========================================================
// This file is part of the Polyaniline bootloader. Please credit me if you use this code.

#include <bootelf.h> // Main header file
#include <boot_terminal.h>
#include <config.h>

#include <libc_string.h>

// checkELF() - Checks if the ELF file is valid, in both architecture-wise and identification wise
int checkELF(Elf32_Ehdr *ehdr) {
    if (!ehdr) return -1; // No header to check

    // Validate MAG0-3
    if (ehdr->e_ident[EI_MAG0] != ELF_MAG0) {
        return -1;
    }

    if (ehdr->e_ident[EI_MAG1] != ELF_MAG1) {
        return -1;
    }

    if (ehdr->e_ident[EI_MAG2] != ELF_MAG2) {
        return -1;
    }

    if (ehdr->e_ident[EI_MAG3] != ELF_MAG3) {
        return -1;
    }

    // Architecture checking

    if (ehdr->e_ident[EI_CLASS] != ELF_CLASS32) {
        return -1;
    }

    if (ehdr->e_ident[EI_DATA] != ELF_DATA2LSB) {
        return -1;
    }

    if (ehdr->e_machine != EM_386) {
        return -1;
    }

    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
        return -1;
    }

    if (ehdr->e_type != ET_REL && ehdr->e_type != ET_EXEC) {
        return -1;
    }

    return 0;
}


// loadELF(Elf32_Ehdr *ehdr) - Load an ELF file into memory
uintptr_t loadELF(Elf32_Ehdr *ehdr) {
    // Check to make sure the file is legitimate
    if (checkELF(ehdr) != 0) {
        setColor(0x04);
        boot_printf("Load fail - invalid ELF32 binary\n");
        return 0x0;
    }

    uintptr_t end = 0;

    // We're deviating a little bit from the original code and we're just going to parse PHDRs.
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf32_Phdr *phdr = (Elf32_Phdr*)(KERNEL_LOAD_ADDR + ehdr->e_phoff + (i * ehdr->e_phentsize));

        switch (phdr->p_type) {
            case PT_NULL:
                break;  // NULL type, nothing to do.
            case PT_LOAD:
                // We need to load it into memory
                memcpy((uint8_t*)(uintptr_t)phdr->p_vaddr, KERNEL_LOAD_ADDR + phdr->p_offset, phdr->p_filesize);
                
                // Zero out the rest of the section
                memset(phdr->p_vaddr + phdr->p_filesize, 0, phdr->p_memsize - phdr->p_filesize);

                if (phdr->p_vaddr + phdr->p_memsize > end) end = phdr->p_vaddr + phdr->p_memsize;
                break;
            default:
                // Unknown type
                setColor(0x04);
                boot_printf("Load fail - unknown PHDR type 0x%x\n", phdr->p_type);
                return 0x0;
        }
    }

    // We'll return the ending variable, and then the other program can figure out entrypoint (they did pass us an EHDR)
    end = (end & ~(0xFFF)) + ((end & 0xFFF) ? 0x1000 : 0); // Round it to neraest page though
    return end;
}