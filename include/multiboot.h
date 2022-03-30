#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "stdint.h"

#define MULTIBOOT_MAGIC_HEADER      0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC  0x2BADB002

/* The Multiboot header. */
typedef struct {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
} MULTIBOOT_HEADER;

/* The symbol table for a.out. */
typedef struct {
    uint32_t tabsize;
    uint32_t strsize;
    uint32_t addr;
    uint32_t reserved;
} AOUT_SYMBOL_TABLE;

/* The section header table for ELF. */
typedef struct {
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
} ELF_SECTION_HEADER_TABLE;

typedef struct {
    /* required, defined in entry.asm */
    uint32_t flags;

    /* available low-high memory from BIOS, present if flags[0] is set(MEMINFO in entry.asm) */
    uint32_t mem_low;
    uint32_t mem_high;

    /* "root" partition, present if flags[1] is set(BOOTDEVICE in entry.asm) */
    uint32_t boot_device;

    /* kernel command line, present if flags[2] is set(CMDLINE in entry.asm) */
    uint32_t cmdline;

    /* no of modules loaded, present if flags[3] is set(MODULECOUNT in entry.asm) */
    uint32_t modules_count;
    uint32_t modules_addr;

    /* symbol table info, present if flags[4] & flags[5] is set(SYMT in entry.asm) */
    union {
        AOUT_SYMBOL_TABLE aout_sym;
        ELF_SECTION_HEADER_TABLE elf_sec;
    } u;

    /* memory mapping, present if flags[6] is set(MEMMAP in entry.asm) */
    uint32_t mmap_length;
    uint32_t mmap_addr;

    /* drive info, present if flags[7] is set(DRIVE in entry.asm) */
    uint32_t drives_length;
    uint32_t drives_addr;

    /* ROM configuration table, present if flags[8] is set(CONFIGT in entry.asm) */
    uint32_t config_table;

    /* boot loader name, present if flags[9] is set(BOOTLDNAME in entry.asm) */
    uint32_t boot_loader_name;

    /* Advanced Power Management(APM) table, present if flags[10] is set(APMT in entry.asm) */
    uint32_t apm_table;

    /* video info, present if flags[11] is set(VIDEO in entry.asm) */
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    /* video framebufer info, present if flags[12] is set(VIDEO_FRAMEBUF in entry.asm)  */
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;  // indexed = 0, RGB = 1, EGA = 2

} MULTIBOOT_INFO;


typedef enum {
    MULTIBOOT_MEMORY_AVAILABLE = 1,
    MULTIBOOT_MEMORY_RESERVED,
    MULTIBOOT_MEMORY_ACPI_RECLAIMABLE,
    MULTIBOOT_MEMORY_NVS,
    MULTIBOOT_MEMORY_BADRAM
} MULTIBOOT_MEMORY_TYPE;

typedef struct {
    uint32_t size;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t len_low;
    uint32_t len_high;
    MULTIBOOT_MEMORY_TYPE type;
} MULTIBOOT_MEMORY_MAP;

#endif