// elf.h - Header file for the Executable and Linkable Format driver

#ifndef ELF_H
#define ELF_H

// Includes
#include <libk_reduced/stdint.h>
#include <libk_reduced/stddef.h>
#include <kernel/serial.h>
#include <kernel/ksym.h>
#include <kernel/mem.h>



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

enum St_Binding {
    STB_LOCAL       = 0,            // Local scope
    STB_GLOBAL      = 1,            // Global scope
    STB_WEAK        = 2             // Weak
};

enum St_Types {
    STT_NOTYPE      = 0,            // No type
    STT_OBJECT      = 1,            // Variables, arrays, etc.
    STT_FUNC        = 2             // Methods or functions
};

enum Rt_Types {
    R_386_NONE      = 0,            // No relocation
    R_386_32        = 1,            // Symbol + Offset
    R_386_PC32      = 2,            // Symbol + Offset - Section Offset
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

// Section Header Definitions
#define SHN_UNDEF       0           // Undefined/Not Present
#define SHN_LORESERVE   0xFF00      // ?
#define SHN_LOPROC      0xFF00      // ?
#define SHN_HIPROC      0xFF1F      // ?
#define SHN_ABS         0xFFF1      // Absolute symbol
#define SHN_COMMON      0xFFF2      // Common
#define SHN_HIRESERVE   0xFFFF      // ?

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


// Errors
#define ELF_RELOC_ERROR -1          // Failure to relocate
#define ELF_PARSE_ERROR -2          // Failure to parse

// Macros
#define ELF32_ST_BIND(INFO)     ((INFO) >> 4)       // Get the binding
#define ELF32_ST_TYPE(INFO)     ((INFO) & 0x0F)     // Get the type   

#define ELF32_R_SYM(INFO)       ((INFO) >> 8)       // Get the relocatable symbol
#define ELF32_R_TYPE(INFO)      ((uint8_t)(INFO))   // Get the relocation type

#define DO_386_32(S, A)         ((S) + (A))         // Perform relocation for 386
#define DO_386_PC32(S, A, P)    ((S) + (A) - (P))   // Perform relocation for 386 but subtract section offset



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

// Symbol Table
typedef struct {
    Elf32_Word  st_name;
    Elf32_Addr  st_value;
    Elf32_Word  st_size;
    uint8_t     st_info;
    uint8_t     st_other;
    Elf32_Half  st_shndx;
} Elf32_Sym;


// Relocatable Section (no explicit added)
typedef struct {
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;
} Elf32_Rel;


// Relocatable Section (explicit added)
typedef struct {
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;
    Elf32_Sword r_addend;
} Elf32_Rel_Added;

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
void *elf_loadFileFromBuffer(void *buf); // Loads an ELF file from a buffer with its contents
void *elf_loadFile(fsNode_t *file); // Loads a file. 
void *elf_findSymbol(Elf32_Ehdr *ehdr, char *name); // Locates and returns the location of a symbol
void elf_cleanupFile(char *buffer); // Frees memory where the file was relocated, depending on what it was

// Pointer functions
int execve(char *filename, int argc, char *argv[], char *envp[]); // just points to createProcess pretty much
int system(char *filename, int argc, char *argv_in[]);


// Internal functions, should only be used by process scheduler
// (scheduler performs memory mapping itself and disregards our method)
Elf32_Phdr *elf_getPHDR(Elf32_Ehdr *ehdr, int offset);
int elf_isCompatible(Elf32_Ehdr *ehdr);



#endif
