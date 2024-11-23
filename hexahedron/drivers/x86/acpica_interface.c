/**
 * @file hexahedron/drivers/x86/acpica_interface.c
 * @brief Provides the interface to ACPICA, exposed to the kernel.
 * 
 * @see acpica_osl.c for the OS layer of ACPICA
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifdef ACPICA_ENABLED


#include <acpica/acpi.h>
#include <acpica/actypes.h>

#include <kernel/arch/i386/hal.h>
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/alloc.h>
#include <stdarg.h>
#include <errno.h>

#define LOG(status, message, ...) dprintf_module(status, "ACPICA:KRN", message, ## __VA_ARGS__)


/* Exposed to kernel */
int ACPICA_Initialize() {
    LOG(INFO, "ACPICA was compiled into kernel. Initializing ACPICA\n");
    
    ACPI_STATUS status;
    
    status = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(status)) {
        LOG(ERR, "AcpiInitializeSubsystem did not succeed - status %i\n", status);
        return -1; 
    }

    // Initialize tables
    status = AcpiInitializeTables(NULL, 16, FALSE);
    if (ACPI_FAILURE(status)) {
        LOG(ERR, "AcpiInitializeTables did not succeed - status %i\n", status);
        AcpiTerminate();
        return -1;
    }

    // Load tables
    status = AcpiLoadTables();
    if (ACPI_FAILURE(status)) {
        LOG(ERR, "AcpiLoadTables did not succeed - status %i\n", status);
        AcpiTerminate();
        return -1;
    }

    // Enable subsystem
    status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status)) {
        LOG(ERR, "AcpiEnableSubsystem did not succeed - status %i\n", status);
        AcpiTerminate();
        return -1;
    }

    // Initialize objects
    status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status)) {
        LOG(ERR, "AcpiInitializeObjects did not succeed - status %i\n", status);
        AcpiTerminate();
        return -1;
    }

    LOG(INFO, "Initialization completed successfully.\n");
    return 0;
}

/* SMP */

/**
 * @brief Start SMP. Communicates with x86 SMP system
 */
int ACPICA_StartSMP() {
    // The MADT has the signature 'APIC'. Cast to ACPI_TABLE_MADT
    ACPI_TABLE_MADT *MadtTable;
    ACPI_STATUS Status;

    Status = AcpiGetTable("APIC", 1, (ACPI_TABLE_HEADER**)&MadtTable);
    if (!ACPI_SUCCESS(Status)) {
        LOG(WARN, "No MADT table was found (AcpiGetTable returned %i) - does system not support SMP?\n", Status);
        return -ENOTSUP;
    }

    LOG(DEBUG, "MADT Local APIC address = 0x%x\n", MadtTable->Address);

    // Start iterating through entries
    UINT8 *StartPointer = (UINT8*)MadtTable + sizeof(ACPI_TABLE_MADT);
    UINT8 *EndPointer = (UINT8*)MadtTable + MadtTable->Header.Length;
    ACPI_SUBTABLE_HEADER *Subtable;

    while (StartPointer < EndPointer) {
        Subtable = (ACPI_SUBTABLE_HEADER*)StartPointer;
    
        switch (Subtable->Type) {
            case ACPI_MADT_TYPE_LOCAL_APIC:
                ACPI_MADT_LOCAL_APIC *LocalApic = (ACPI_MADT_LOCAL_APIC*)Subtable;
                LOG(DEBUG, "LOCAL APIC - ID 0x%x FLAGS 0x%x PROCESSOR ID 0x%x\n", LocalApic->Id, LocalApic->LapicFlags, LocalApic->ProcessorId);
                break;
            case ACPI_MADT_TYPE_IO_APIC:
                ACPI_MADT_IO_APIC *IoApic = (ACPI_MADT_IO_APIC*)Subtable;
                LOG(DEBUG, "I/O APIC - ADDR 0x%x GLOBAL IRQ 0x%x ID 0x%x\n", IoApic->Address, IoApic->GlobalIrqBase, IoApic->Id);
                break;
            case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
                ACPI_MADT_INTERRUPT_OVERRIDE *IntOverride = (ACPI_MADT_INTERRUPT_OVERRIDE*)Subtable;
                LOG(DEBUG, "INTERRUPT OVERRIDE - SRCIRQ 0x%x BUS 0x%x GLOBAL IRQ 0x%x INTI FLAGS 0x%x\n", IntOverride->SourceIrq, IntOverride->Bus, IntOverride->GlobalIrq, IntOverride->IntiFlags);
                break;
            case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
                ACPI_MADT_LOCAL_APIC_NMI *LocalApicNmi = (ACPI_MADT_LOCAL_APIC_NMI*)Subtable;
                LOG(DEBUG, "LOCAL APIC NMI - INTI FLAGS 0x%x LINT 0x%x PROCESSOR ID 0x%x\n", LocalApicNmi->IntiFlags, LocalApicNmi->Lint, LocalApicNmi->ProcessorId);
                break;
            default:
                LOG(DEBUG, "UNKNOWN TYPE - 0x%x\n", Subtable->Type);
        }

        StartPointer += Subtable->Length;
    }

    return 0;
}


/* NAMESPACE ENUMERATION */

ACPI_STATUS AcpiWalkCallback(ACPI_HANDLE Object, UINT32 NestingLevel, void *Context, void **ReturnValue) {
    ACPI_STATUS Status;
    ACPI_DEVICE_INFO *Info;
    ACPI_BUFFER Name;
    char buffer[256];
    Name.Length = sizeof(buffer);
    Name.Pointer = buffer;

    Status = AcpiGetName(Object, ACPI_FULL_PATHNAME, &Name);
    if (ACPI_SUCCESS(Status)) {
        LOG(DEBUG, "Enumeration of object: %s\n", buffer);
    } else {

    }

    Status = AcpiGetObjectInfo(Object, &Info);
    if (ACPI_SUCCESS(Status)) {
        LOG(DEBUG, "\t\tHID %08x ADR: %08x\n", Info->HardwareId, Info->Address);
    } else {
        LOG(DEBUG, "\t\tAcpiGetObjectInfo returned ACPI_STATUS 0x%x\n", Status);
    }

    *ReturnValue = NULL;
    return AE_OK;
}

/**
 * @brief Print the ACPICA namespace to serial (for debug)
 */
void ACPICA_PrintNamespace() {
    AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, 256, AcpiWalkCallback, NULL, NULL, NULL);
}



#endif
