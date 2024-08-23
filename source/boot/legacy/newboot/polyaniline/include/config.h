// config.h - Similar to reduceOS' config.h, but simpler (bootloader is in alpha stage)

#ifndef POLY_CONFIG_H
#define POLY_CONFIG_H

// You may modify the below values
#define VERSION "1.0"
#define CODENAME "Redline"

// These values are for loading the kernel

#define KERNEL_LOAD_ADDR    0x4000000ULL            // This address is guranteed to be available, for some reason. The linker.ld should play fine with it.
#define LOAD_SIZE           134217728               // 128MB being loaded. Probably bad
#define KERNEL_PAGES        8192                    // Bunch o' pages
#define KERNEL_ACTUAL_LOAD  0x1000000               // The load address as defined in linker.ld
// The ramdisk load address is calculated based off KERNEL_LOAD_ADDRESS



#endif
