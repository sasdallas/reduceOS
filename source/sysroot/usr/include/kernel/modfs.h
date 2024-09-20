// modfs.h - header file for multiboot module filesystem driver

#ifndef MODFS_H
#define MODFS_H

// Includes
#include <libk_reduced/stdint.h>
#include <kernel/vfs.h>
#include <kernel/bootinfo.h>

// Functions
void mountModfs(multiboot_mod_t *mod, char *mountpoint);
void modfs_init(multiboot_info *info);

#endif
