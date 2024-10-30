/**
 * @file hexahedron/include/kernel/multiboot.h
 * @brief Multiboot header
 * 
 * This isn't architecture-specific. GRUB uses it to boot multiple architectures.
 * 
 * 
 * @copyright
 * No copyright on this file. The structure was sourced from GRUB documentation.
 */

#ifndef _KERNEL_MULTIBOOT_H
#define _KERNEL_MULTIBOOT_H

#include <stdint.h>

typedef struct _multiboot_t
{
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t boot_device;
	uint32_t cmdline;
	uint32_t mods_count;
	uint32_t mods_addr;

	uint32_t num;
	uint32_t size;
	uint32_t addr;
	uint32_t shndx;

	uint32_t mmap_length;
	uint32_t mmap_addr;

	uint32_t drives_length;
	uint32_t drives_addr;

	uint32_t config_table;

	uint32_t boot_loader_name;

	uint32_t apm_table;

	uint32_t vbe_control_info;
	uint32_t vbe_mode_info;
	uint16_t vbe_mode;
	uint16_t vbe_interface_seg;
	uint16_t vbe_interface_off;
	uint16_t vbe_interface_len;

	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t  framebuffer_bpp;
	uint8_t  framebuffer_type;
} __attribute__ ((packed)) multiboot_t;


// Numbers bootloader provides
#define MULTIBOOT_MAGIC 0x2BADB002
#define MULTIBOOT2_MAGIC 0x36D76289

#endif