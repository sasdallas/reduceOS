// bootelf.h - Handles booting ELF files, mostly stolen from reduceOS.

#ifndef POLY_ELF_H
#define POLY_ELF_H

// Includes
#include <libc_types.h>

/* Copy-pasted from the reduceOS ELF loader with parts stripped out */

// ELF data types (these can ediffer across machine types)
typedef uint16_t Elf32_Half;        // Unsigned half-int
typedef uint32_t Elf32_Off;         // Unsigned offset
typedef uint32_t Elf32_Addr;        // Unsigned address
typedef uint32_t Elf32_Word;        // Unsigned int
typedef int32_t Elf32_Sword;        // Signed int (or sword)

// Enums
enum Elf_Ident {
    EI_MAG0         = 0,            // 0x7F
    EI_MAG1         = 1,            // 'E'
    EI_MAG2         = 2,            // 'L'
    EI_MAG3         = 3,            // 'F'
    EI_CLASS        = 4,            // Architecture
    EI_DATA         = 5,            // Byte Order
    EI_VERSION      = 6,            // Version of ELF used
    EI_OSABI        = 7,            // OS-specific
    EI_ABIVERSION   = 8,            // OS-specific
    EI_PAD          = 9             // Padding
};

enum Elf_Type {
    ET_NONE         = 0,            // Unknown type
    ET_REL          = 1,            // Relocatable file
    ET_EXEC         = 2             // Executable file
};


// ELF Definitions
#define ELF_NIDENT  16              // Number of ELF identifiers

// MAG0-3
#define ELF_MAG0    0x7F
#define ELF_MAG1    'E'
#define ELF_MAG2    'L'
#define ELF_MAG3    'F'

// Byte Orders
#define ELF_DATA2LSB    (1)

// i386 (32-bit) architecture values
#define ELF_CLASS32     (1)         // 32-bit architecture
#define EM_386          (3)         // x86 machine type
#define EV_CURRENT      (1)         // Current ELF version

// Program header types
#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5   
#define PT_PHDR         6
#define PT_TLS          7
#define PT_LOOS         0x60000000
#define PT_HIOS         0x6FFFFFFF
#define PT_LOPROC       0x70000000
#define PT_HIPROC       0x7FFFFFFF



// Headers

// EHDR
typedef struct {
    uint8_t     e_ident[ELF_NIDENT];
    Elf32_Half  e_type;
    Elf32_Half  e_machine;
    Elf32_Word  e_version;
    Elf32_Addr  e_entry;
    Elf32_Off   e_phoff;
    Elf32_Off   e_shoff;
    Elf32_Word  e_flags;
    Elf32_Half  e_ehsize;
    Elf32_Half  e_phentsize;
    Elf32_Half  e_phnum;
    Elf32_Half  e_shentsize;
    Elf32_Half  e_shnum;
    Elf32_Half  e_shstrndx;
} Elf32_Ehdr;

// PHDR
typedef struct {
    Elf32_Word  p_type;
    Elf32_Off   p_offset;
    Elf32_Addr  p_vaddr;
    Elf32_Addr  p_paddr;
    Elf32_Word  p_filesize;
    Elf32_Word  p_memsize;
    Elf32_Word  p_flags;
    Elf32_Word  p_align;
} Elf32_Phdr;

// Functions
int checkELF(Elf32_Ehdr *ehdr);
uintptr_t loadELF(Elf32_Ehdr *ehdr);


#endif