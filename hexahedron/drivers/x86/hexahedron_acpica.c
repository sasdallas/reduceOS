/**
 * @file hexahedron/drivers/x86/acpica/hexahedron_acpica.c
 * @brief ACPICA interface for Hexahedron
 * 
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * ACPICA and its components are licensed under BSD license, the same as Hexahedron.
 * ACPICA is created by Intel corporation.
 * Licensing of ACPICA is available in hexahedron/drivers/x86/acpica/LICENSE
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <acpica/acpi.h>

#include <kernel/arch/i386/hal.h>
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/alloc.h>
#include <kernel/misc/spinlock.h>
#include <kernel/misc/semaphore.h>

#include <stdarg.h>
#include <errno.h>


#define FUNC_UNIMPLEMENTED(FUNCTION)    \
    dprintf(WARN, "[ACPICA] %s: Unimplemented\n", FUNCTION);    \
    kernel_panic_extended(UNSUPPORTED_FUNCTION_ERROR, "acpica", "*** %s not implemented\n", FUNCTION);   \
    __builtin_unreachable();

/* Exposed to kernel */
int ACPICA_Initialize() {
    dprintf(INFO, "[ACPICA] ACPICA was compiled into kernel. Initializing ACPICA\n");
    dprintf(INFO, "[ACPICA] AcpiInitializeSubsystem\n");
    
    ACPI_STATUS status;
    
    status = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(status)) {
        dprintf(ERR, "[ACPICA] AcpiInitializeSubsystem did not succeed - status %i\n", status);
        return -1; 
    }

    // Initialize tables
    status = AcpiInitializeTables(NULL, 16, FALSE);
    if (ACPI_FAILURE(status)) {
        dprintf(ERR, "[ACPICA] AcpiInitializeTables did not succeed - status %i\n", status);
        AcpiTerminate();
        return -1;
    }

    // Load tables
    status = AcpiLoadTables();
    if (ACPI_FAILURE(status)) {
        dprintf(ERR, "[ACPICA] AcpiLoadTables did not succeed - status %i\n", status);
        AcpiTerminate();
        return -1;
    }

    // Enable subsystem
    status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status)) {
        dprintf(ERR, "[ACPICA] AcpiEnableSystem did not succeed - status %i\n", status);
        AcpiTerminate();
        return -1;
    }

    dprintf(INFO, "[ACPICA] Initialization completed successfully.\n");
    return 0;
}


/* INITIALIZE/TERMINATE FUNCTIONS */

ACPI_STATUS AcpiOsInitialize() {
    return AE_OK;
}

ACPI_STATUS AcpiOsTerminate() {
    return AE_OK;
}

/* Get the RSDP */
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
    // Try to ask HAL (loading from EFI provides RSDP in ST)
    ACPI_PHYSICAL_ADDRESS rsdp = (ACPI_PHYSICAL_ADDRESS)hal_getRSDP();
    if (rsdp == 0x0) {
        ACPI_STATUS status = AcpiFindRootPointer(&rsdp); // Use ACPICA to find the pointer.
        if (ACPI_FAILURE(status)) return 0x0;
    }

    return rsdp;
}

/* OVERRIDE FUNCTIONS */

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue) {
    *NewValue = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
    *NewTable = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength) {
    *NewAddress = 0x0;
    return AE_OK;
}

/* MEMORY FUNCTIONS */

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
    // just like pmm it
    return (void*)mem_remapPhys((uintptr_t)PhysicalAddress);
}

/* Unmap memory */
void AcpiOsUnmapMemory(void *where, ACPI_SIZE Length) {
    // Considering we're returning PMM-mapped memory, nah.
}

/* Get a physical address */
ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress) {
    // It is possible to just call mem_getPhysicalAddress as well
    *PhysicalAddress = (ACPI_PHYSICAL_ADDRESS)(LogicalAddress - (void*)MEM_IDENTITY_MAP_REGION);
    return AE_OK;
}

/* Allocate */
void *AcpiOsAllocate(ACPI_SIZE Size) {
    return kmalloc(Size);
}

/* Free */
void AcpiOsFree(void *Memory) {
    return kfree(Memory);
}

/* Are the memory regions readable? */
BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length) {
    FUNC_UNIMPLEMENTED("AcpiOsReadable");
}

/* Are the memory regions writable? */
BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length) {
    FUNC_UNIMPLEMENTED("AcpiOsWritable");
}

/* UNIMPLEMENTED THREAD FUNCTIONS (SINGLE-THREADED) */

ACPI_THREAD_ID AcpiOsGetThreadId() {
    return 1; // Constant thread. According to ACPICA docs this should work.
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context) {
    return AE_OK;
}

void AcpiOsSleep(UINT64 Milliseconds) {
    // TODO: We can implement this
    FUNC_UNIMPLEMENTED("AcpiOsSleep");
}

void AcpiOsStall(UINT32 Microseconds) {
    FUNC_UNIMPLEMENTED("AcpiOsStall");
}

void AcpiOsWaitEventsComplete() {
    dprintf(WARN, "Unimplemented AcpiOsWaitEventsComplete - not critical\n");
}

/* SEMAPHORE FUNCTIONS */

/* Create a semaphore */
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
    *OutHandle = semaphore_create("acpica_sem", InitialUnits, MaxUnits);
    return AE_OK;
}

/* Delete semaphore */
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
    semaphore_destroy((semaphore_t*)Handle);
    return AE_OK;
}

/* Wait on semaphore */
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
    if (Timeout == 0) {
        // Don't wait at all? Just check if there are enough units
        if ((UINT32)semaphore_getItems(Handle) >= Units) {
            semaphore_wait(Handle, Units);
            return AE_OK;
        }

        return AE_TIME;
    }

    time_t start_time = now(); // Start time in ms
    UINT32 remaining = Units;
    while (remaining > 0 && now() - start_time < Timeout) {
        remaining -= semaphore_wait(Handle, remaining);
    }

    if (remaining > 0) {
        // Timeout exceeded
        semaphore_signal(Handle, Units - remaining);
        return AE_TIME; 
    }

    return AE_OK;
}

/* Signal the semaphore */
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
    int added = semaphore_signal(Handle, Units);
    if ((UINT32)added != Units) return AE_LIMIT;
    return AE_OK;
}

/* LOCK FUNCTIONS */

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
    // this we can actually do
    spinlock_t *lock = kmalloc(sizeof(spinlock_t));
    
    // TODO: NOT DISABLING INTERRUPTS 
    *OutHandle = lock;
    return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle) {
    kfree(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
    spinlock_acquire((spinlock_t*)Handle);
    return (ACPI_CPU_FLAGS)NULL;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {
    spinlock_release((spinlock_t*)Handle);
}

/* INTERRUPT FUNCTIONS */

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context) {
    FUNC_UNIMPLEMENTED("AcpiOsInstallInterruptHandler");
}
ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler) {
    FUNC_UNIMPLEMENTED("AcpiOsRemoveInterruptHandler");
}

/* LOGGING */
void AcpiOsPrintf(const char *Format, ...) {
    va_list ap;
    va_start(ap, Format);
    xvasprintf(debug_print, NULL, Format, ap);
    va_end(ap);
}

void AcpiOsVprintf(const char *Format, va_list Args) {
    xvasprintf(debug_print, NULL, Format, Args);
}

/* MORE MEMORY */

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {

    void *ptr = (void*)mem_remapPhys((uintptr_t)Address);

    switch (Width) {
        case 8:
            *Value = *(UINT8*)ptr; 
            break;
        case 16:
            *Value = *(UINT16*)ptr; 
            break;
        case 32:
            *Value = *(UINT32*)ptr; 
            break;
        case 64:
            *Value = *(UINT64*)ptr; 
            break;
        default:
            kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "acpica", "*** AcpiOsReadMemory received bad width argument 0x%x\n", Width);
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
    void *ptr = (void*)mem_remapPhys((uintptr_t)Address);

    switch (Width) {
        case 8:
            *(UINT8*)ptr = Value;
            break;
        case 16:
            *(UINT16*)ptr = Value;
            break;
        case 32:
            *(UINT32*)ptr = Value;
            break;
        case 64:
            *(UINT64*)ptr = Value;
            break;
        default:
            kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "acpica", "*** AcpiOsReadMemory received bad width argument 0x%x\n", Width);
    }

    return AE_OK;
}


/* I/O FUNCTIONS */

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    switch (Width) {
        case 8:
            *Value = inportb(Address);
            break;
        case 16:
            *Value = inportw(Address);
            break;
        case 32:
            *Value = inportl(Address);
            break;
        default:
            kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "acpica", "*** AcpiOsReadPort received bad width argument 0x%x\n", Width);
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    switch (Width) {
        case 8:
            outportb(Address, Value);
            break;
        case 16:
            outportw(Address, Value);
            break;
        case 32:
            outportl(Address, Value);
            break;
        default:
            kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "acpica", "*** AcpiOsReadPort received bad width argument 0x%x\n", Width);
    }

    return AE_OK;
}

/* (UNIMPLEMENTED) PCI FUNCTIONS */

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width) {
    FUNC_UNIMPLEMENTED("AcpiOsReadPciConfiguration");
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width) {
    FUNC_UNIMPLEMENTED("AcpiOsWritePciConfiguration");
    return AE_OK;
}

/* MISC. */

UINT64 AcpiOsGetTimer() {
    return now() * 10 * 1000;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
    /* It might send other stuff but we need to catch fatal signals */
    switch (Function) {
        case ACPI_SIGNAL_FATAL:
            ACPI_SIGNAL_FATAL_INFO *error_info = Info;
            kernel_panic_extended(ACPI_SYSTEM_ERROR, "acpica", "*** ACPI AML error: Fatal error detected. Type: 0x%x Code: 0x%x Argument: 0x%x", 
                    error_info->Type,
                    error_info->Code,
                    error_info->Argument);
            break;
        default:
            dprintf(DEBUG, "ACPI AML signal 0x%x received\n", Function);
    }

    return AE_OK;
}