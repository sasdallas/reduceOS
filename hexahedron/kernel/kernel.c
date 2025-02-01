/**
 * @file hexahedron/kernel/kernel.c
 * @brief Start of the generic parts of Hexahedron
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

// libpolyhedron
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

// Kernel includes
#include <kernel/kernel.h>
#include <kernel/arch/arch.h>
#include <kernel/debug.h>
#include <kernel/panic.h>

// Loaders
#include <kernel/loader/driver.h>

// Memory
#include <kernel/mem/mem.h>
#include <kernel/mem/alloc.h>

// VFS
#include <kernel/fs/vfs.h>
#include <kernel/fs/tarfs.h>
#include <kernel/fs/ramdev.h>

// Drivers
#include <kernel/drivers/font.h>

// Graphics
#include <kernel/gfx/term.h>
#include <kernel/gfx/gfx.h>

// Misc.
#include <kernel/misc/ksym.h>
#include <kernel/misc/args.h>


/* Log method of generic */
#define LOG(status, ...) dprintf_module(status, "GENERIC", __VA_ARGS__)


/**
 * @brief Mount the initial ramdisk to /device/initrd/
 */
void kernel_mountRamdisk(generic_parameters_t *parameters) {
    // Find the initial ramdisk and mount it to RAM.
    fs_node_t *initrd_ram = NULL;
    generic_module_desc_t *mod = parameters->module_start;

    while (mod) {
        if (mod->cmdline && !strncmp(mod->cmdline, "type=initrd", 9)) {
            // Found it, mount the ramdev.
            initrd_ram = ramdev_mount(mod->mod_start, mod->mod_end - mod->mod_start);
            break;
        }

        mod = mod->next;
    }

    if (!initrd_ram) {
        // We didn't find it. Panic.
        LOG(ERR, "Module with type=initrd not found\n");

        kernel_panic(INITIAL_RAMDISK_CORRUPTED, "kernel");
        __builtin_unreachable();
    }

    // Now we have to mount tarfs to it.
    char devpath[64];
    snprintf(devpath, 64, "/device/%s", initrd_ram->name);
    if (vfs_mountFilesystemType("tarfs", devpath, "/device/initrd") == NULL) {
        // Oops, we couldn't mount it.
        LOG(ERR, "Failed to mount initial ramdisk (tarfs)\n");
        kernel_panic(INITIAL_RAMDISK_CORRUPTED, "kernel");
        __builtin_unreachable();
    }

    LOG(INFO, "Mounted initial ramdisk to /device/initrd\n");
    printf("Mounted initial ramdisk successfully\n");
}

/**
 * @brief Load kernel drivers
 */
void kernel_loadDrivers() {
    driver_initialize(); // Initialize the driver system

    fs_node_t *conf_file = kopen(DRIVER_DEFAULT_CONFIG_LOCATION, O_RDONLY);
    if (!conf_file) {
        kernel_panic_extended(INITIAL_RAMDISK_CORRUPTED, "kernel", "*** Missing driver configuration file (%s)\n", DRIVER_DEFAULT_CONFIG_LOCATION);
        __builtin_unreachable();
    }
    
    driver_loadConfiguration(conf_file); // Load the configuration
    fs_close(conf_file);
}

/**
 * @brief Kernel main function
 */
void kmain() {
    LOG(INFO, "Reached kernel main, starting Hexahedron...\n");
    generic_parameters_t *parameters = arch_get_generic_parameters();

    // All architecture-specific stuff is done now. We need to get ready to initialize the whole system.
    // Do some sanity checks first.
    if (!parameters->module_start) {
        LOG(ERR, "No modules detected - cannot continue\n");
        kernel_panic(INITIAL_RAMDISK_CORRUPTED, "kernel");
        __builtin_unreachable();
    }

    // Now, initialize the VFS.
    vfs_init();

    // Startup the builtin filesystem drivers    
    tarfs_init();

    // Now we need to mount the initial ramdisk
    kernel_mountRamdisk(parameters);

    // Try to load new font file
    fs_node_t *new_font = kopen("/device/initrd/ter-112n.psf", O_RDONLY);
    if (new_font) {
        // Load PSF
        if (!font_loadPSF(new_font)) {
            // Say hello
            gfx_drawLogo(TERMINAL_DEFAULT_FG);
            arch_say_hello(0);
        } else {
            fs_close(new_font);
            LOG(ERR, "Failed to load font file \"/device/initrd/ter-112n.psf\".\n");
        }
    } else {
        LOG(ERR, "Could not find new font file \"/device/initrd/ter-112n.psf\", using old font\n");
    }

    // Load symbols
    fs_node_t *symfile = kopen("/device/initrd/hexahedron-kernel-symmap.map", O_RDONLY);
    if (!symfile) {
        kernel_panic_extended(INITIAL_RAMDISK_CORRUPTED, "kernel", "*** Missing hexahedron-kernel-symmap.map\n");
        __builtin_unreachable();
    }

    int symbols = ksym_load(symfile);
    fs_close(symfile);

    LOG(INFO, "Loaded %i symbols from symbol map\n", symbols);

    // Load drivers
    if (!kargs_has("--no-load-drivers")) {
        kernel_loadDrivers();
    } else {
        LOG(WARN, "Not loading any drivers, found argument \"--no-load-drivers\".\n");
    }

    
}