// bootinfo.h - header file defining multiboot_info, the structure passed by loadkernel and kernel assembly files.

#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <libk_reduced/stdint.h> // Definitions of integer types, like uint8_t

#define MULTIBOOT_MAGIC 0x1BADB002


typedef struct {
	uint32_t tab_size;
	uint32_t string_size;
	uint32_t address;
	uint32_t reserved;
} aoutSymbolTable_t;

typedef struct {
	uint32_t number;
	uint32_t size;
	uint32_t address;
	uint32_t shndx;
} elfHeader_t;

typedef struct {
	uint32_t	m_flags; 			  // m_flags - Multiboot flags (useless without a multiboot bootloader like GRUB)
	uint32_t	m_memoryLo;			  // m_memoryLo - Low address of memory
	uint32_t	m_memoryHi;			  // m_memoryHi - High address of memory (usually 0)
	uint32_t	m_bootDevice;		  // m_bootDevice - Value BIOS passed
	uint32_t	m_cmdLine;			  // m_cmdLine - command used to boot OS
	uint32_t	m_modsCount;		  // m_modsCount - Amount of mods passed (used with initrd)
	uint32_t	m_modsAddr;			  // m_modsAddr - Address of mods passed (used with initrd)
	union {
        aoutSymbolTable_t aout_sym;
        elfHeader_t elf_sec;
    } u;
	uint32_t	m_mmap_length;		  // m_mmap_length - Length of memory map
	uint32_t	m_mmap_addr;		  // m_mmap_addr - Address of memory map
	uint32_t	m_drives_length;	  // m_drives_length - Length of drive
	uint32_t	m_drives_addr;		  // m_drives_addr - Address of drive
	uint32_t	m_config_table;		  // m_config_table - Unused
	uint32_t	m_bootloader_name;	  // m_bootloader_name - The name of the bootloader that loaded the kernel
	uint32_t	m_apm_table;		  // m_apm_table - APM information
	uint32_t	m_vbe_control_info;   // m_vbe_control_info - VBE control info 
	uint32_t	m_vbe_mode_info;      // m_vbe_mode_info - VBE mode info
	uint16_t	m_vbe_mode;			  // m_vbe_mode - VBE mode
	uint32_t	m_vbe_interface_addr; // m_vbe_interface_addr - VBE interface address
	uint16_t	m_vbe_interface_len;  // m_vbe_interface_len - VBE interface length
	uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;  // indexed = 0, RGB = 1, EGA = 2
} __attribute__((packed)) multiboot_info;

typedef struct {
	uint32_t mod_start;
	uint32_t mod_end;
	uint32_t cmdline;
	uint32_t padding;
} multiboot_mod_t;


#endif

