/*
 * Hexahedron ACPICA header 
 * Original source: Acces2
 */

#ifndef __ACHEXAHEDRON_H__
#define __ACHEXAHEDRON_H__

#include <kernel/misc/spinlock.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#define ACPI_SINGLE_THREADED // TODO: Uncomment

// Use the system's C library
#define ACPI_USE_SYSTEM_CLIBRARY



#define ACPI_USE_DO_WHILE_0

#define ACPI_USE_NATIVE_DIVIDE

// Use local ACPICA cache
#define ACPI_CACHE_T                ACPI_MEMORY_LIST
#define ACPI_USE_LOCAL_CACHE        1

#if defined(__ARCH_X86_64__)
#define ACPI_MACHINE_WIDTH          64
#elif defined(__ARCH_I386__)
#define ACPI_MACHINE_WIDTH          32
#else
#error "Unknown architecture"
#endif

#define ACPI_SPINLOCK               spinlock_t*
#define ACPI_UINTPTR_T              uintptr_t


// Certain compilation errors occur of multiple snprintf/vsnprintf definitions
#define snprintf _snprintf
#define vsnprintf _vsnprintf

// CPU cache suppot not yet
#define ACPI_FLUSH_CPU_CACHE() 

#include "acgcc.h"

#endif /* __ACHEXAHEDRON_H__ */