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


#define FUNC_UNIMPLEMENTED(FUNCTION)    \
    dprintf(WARN, "ACPICA %s: Unimplemented\n", FUNCTION);    \
    kernel_panic_extended(UNSUPPORTED_FUNCTION_ERROR, "acpica", "*** %s not implemented\n", FUNCTION);   \
    __builtin_unreachable();

/* Initialize */
ACPI_STATUS AcpiOsInitialize() {
    return AE_OK;
}

/* Terminate */
ACPI_STATUS AcpiOsTerminate() {
    return AE_OK;
}

/* Get the RSDP */
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
    // Try to ask HAL (loading from EFI provides RSDP in ST)
    ACPI_PHYSICAL_ADDRESS rsdp = (ACPI_PHYSICAL_ADDRESS)hal_getRSDP();
    if (rsdp == 0x0) {
        AcpiFindRootPointer(&rsdp); // Use ACPICA to find the pointer.
    }

    return rsdp;
}

/* Override certain ACPI variables (unused) */
ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue) {
    *NewValue = NULL;
    return AE_OK;
}

/* Override certain ACPI tables (unused) */
ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
    *NewTable = NULL;
    return AE_OK;
}

/* Map memory */
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

/* UNIMPLEMENTED THREAD FUNCTIONS */

ACPI_THREAD_ID AcpiOsGetThreadId() {
    FUNC_UNIMPLEMENTED("AcpiOsGetThreadId");
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context) {
    FUNC_UNIMPLEMENTED("AcpiOsExecute");
}

void AcpiOsSleep(UINT64 Milliseconds) {
    // TODO: We can implement this
    FUNC_UNIMPLEMENTED("AcpiOsSleep");
}

void AcpiOsStall(UINT32 Microseconds) {
    FUNC_UNIMPLEMENTED("AcpiOsStall");
}


/* SEMAPHORE FUNCTIONS */

/* Create a semaphore */
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle) {
    FUNC_UNIMPLEMENTED("AcpiOsCreateSemaphore");
}

/* Delete semaphore */
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
    FUNC_UNIMPLEMENTED("AcpiOsDeleteSemaphore");
}

/* Wait on semaphore */
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout) {
    FUNC_UNIMPLEMENTED("AcpiOsWaitSemaphore");
}

/* Signal the semaphore */
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
    FUNC_UNIMPLEMENTED("AcpiOsSignalSemaphore");
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

