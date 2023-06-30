// kernel.h - Includes and function declarations for the kernel

#ifndef KERNEL_H
#define KERNEL_H

// Includes
#include "include/libc/stddef.h" // size_t declaration
#include "include/libc/stdint.h" // Integer type declarations
#include "include/libc/stdbool.h" // Boolean declarations
#include "include/libc/string.h" // String functions
#include "include/libc/limits.h" // Limits on integers and more.
#include "include/libc/stdarg.h" // va argument handling (for ... on printf)
#include "include/libc/va_list.h" // va_list declared here.
#include "include/libc/assert.h" // Assertion macro
#include "include/libc/udivdi3.h" // udivdi3
#include "include/libc/sleep.h" // Sleeping
#include "include/graphics.h" // Graphics handling
#include "include/terminal.h" // Terminal handling
#include "include/idt.h" // Interrupt Descriptor Table
#include "include/gdt.h" // Global Descriptor Table
#include "include/pic.h" // Programmable Interrupt Controller
#include "include/pit.h" // Programmable Interval Timer
#include "include/hal.h" // Hardware Abstraction Layer
#include "include/keyboard.h" // Keyboard driver
#include "include/panic.h" // Kernel panicking
#include "include/bootinfo.h" // Boot information
#include "include/command.h" // Command parser
#include "include/pci.h" // PCI
#include "include/serial.h" // Serial logging
#include "include/initrd.h" // Initial ramdisk management.
#include "include/vfs.h" // Virtual file system.
#include "include/ide_ata.h" // ATA driver
#include "include/rtc.h" // Real-time clock.
#include "include/bios32.h" // BIOS32 calls
#include "include/processor.h" // Procesor handler
#include "include/vesa.h"
#include "include/syscall.h" // System call

#endif