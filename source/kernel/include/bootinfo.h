// bootinfo.h - header file defining multiboot_info, the structure passed by loadkernel and kernel assembly files.
// All credit from this file goes to BrokenThorn Entertainment.

#ifndef BOOTINFO_H
#define BOOTINFO_H

#include "include/libc/stdint.h" // Definitions of integer types, like uint8_t

#define MULTIBOOT_MAGIC 0x1BADB002
#define MULTIBOOT2_MAGIC 0x2BADB002 // this is probably wrong but whatever

typedef struct {
	uint32_t	m_flags; 			  // m_flags - Multiboot flags (useless without a multiboot bootloader like GRUB)
	uint32_t	m_memoryLo;			  // m_memoryLo - Low address of memory
	uint32_t	m_memoryHi;			  // m_memoryHi - High address of memory (usually 0)
	uint32_t	m_bootDevice;		  // m_bootDevice - Value BIOS passed
	uint32_t	m_cmdLine;			  // m_cmdLine - command used to boot OS
	uint32_t	m_modsCount;		  // m_modsCount - Amount of mods passed (used with initrd)
	uint32_t	m_modsAddr;			  // m_modsAddr - Address of mods passed (used with initrd)
	uint32_t	m_syms0;			  // m_syms0 - Unused
	uint32_t	m_syms1;			  // m_syms1 - Unused
	uint32_t	m_syms2;			  // m_syms2 - Unused
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
} multiboot_info;



#endif

