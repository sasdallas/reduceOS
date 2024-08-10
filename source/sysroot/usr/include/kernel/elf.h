// elf.h - Header file for the Executable and Linkable Format driver

#ifndef ELF_H
#define ELF_H

// Includes
#include <stdint.h>


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

enum Sh_Type {
    SHT_NULL        = 0,            // Null section
    SHT_PROGBITS    = 1,            // Program information
    SHT_SYMTAB      = 2,            // Symbol table
    SHT_STRTAB      = 3,            // String table
    SHT_REL_A       = 4,            // Relocation (with addend)
    SHT_NOBITS      = 8,            // Not present in file
    SHT_REL         = 9             // Relocation (no addend)
};

enum Sh_Attr {
    SHF_WRITE       = 0x01,         // Writable section
    SHF_ALLOC       = 0x02          // Exists in memory
};

// Definitions
#define ELF_NIDENT  16              // Number of ELF identifiers

#define SHN_UNDEF   (0x00)          // Undefined/Not Present

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

// Section Header (SHDR)
typedef struct {
    Elf32_Word  sh_name;
    Elf32_Word  sh_type;
    Elf32_Word  sh_flags;
    Elf32_Addr  sh_addr;
    Elf32_Off   sh_offset;
    Elf32_Word  sh_size;
    Elf32_Word  sh_link;
    Elf32_Word  sh_info;
    Elf32_Word  sh_addralign;
    Elf32_Word  sh_entrysize;
} Elf32_Shdr;



// Functions


#endif