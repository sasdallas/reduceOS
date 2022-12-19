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
#include "include/graphics.h" // Graphics handling
#include "include/terminal.h" // Terminal handling
#include "include/idt.h" // Interrupt Descriptor Table
#include "include/pic.h" // Programmable Interrupt Controller
#include "include/pit.h" // Programmable Interval Timer
#include "include/hal.h" // Hardware Abstraction Layer
#include "include/keyboard.h" // Keyboard driver
#include "include/panic.h" // Kernel panicking
#include "include/bootinfo.h"

#endif