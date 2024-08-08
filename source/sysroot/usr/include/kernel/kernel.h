// kernel.h - Includes and function declarations for the kernel

#ifndef KERNEL_H
#define KERNEL_H

// Includes
#include <stddef.h> // size_t declaration
#include <stdint.h> // Integer type declarations
#include <stdbool.h> // Boolean declarations
#include <string.h> // String functions
#include <limits.h> // Limits on integers and more.
#include <stdarg.h> // va argument handling (for ... on printf)
#include <va_list.h> // va_list declared here.
#include <assert.h> // Assertion macro
#include <udivdi3.h> // udivdi3
#include <sleep.h> // Sleeping
#include <kernel/graphics.h> // Graphics handling
#include <kernel/terminal.h> // Terminal handling
#include <kernel/idt.h> // Interrupt Descriptor Table
#include <kernel/gdt.h> // Global Descriptor Table
#include <kernel/pic.h> // Programmable Interrupt Controller
#include <kernel/pit.h> // Programmable Interval Timer
#include <kernel/hal.h> // Hardware Abstraction Layer
#include <kernel/keyboard.h> // Keyboard driver
#include <kernel/panic.h> // Kernel panicking
#include <kernel/bootinfo.h> // Boot information
#include <kernel/command.h> // Command parser
#include <kernel/pci.h> // PCI driver
#include <kernel/serial.h> // Serial logging
#include <kernel/initrd.h> // Initial ramdisk management.
#include <kernel/vfs.h> // Virtual file system.
#include <kernel/ide_ata.h> // ATA driver
#include <kernel/rtc.h> // Real-time clock.
#include <kernel/bios32.h> // BIOS32 calls
#include <kernel/processor.h> // Procesor handler
#include <kernel/vesa.h> // VESA VBE graphics driver
#include <kernel/syscall.h> // System call
#include <kernel/bitmap.h> // Bitmap image controls
#include <kernel/CONFIG.h> // reduceOS configuration
#include <kernel/floppy.h> // FDC driver
#include <kernel/test.h> // Test command

#endif
