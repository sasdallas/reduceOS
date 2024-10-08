// =====================================================================
// acpi.c - handles the Advanced Configuration Power Interface
// =====================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/acpi.h> // Main header file
#include <libk_reduced/stdio.h>

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast" // THIS IS BAD AND TO BE REMOVED

// Variables
int ACPI_cpuCount = 0;
uint8_t ACPI_cpuIDs[16];
ACPI_MADT *madt;


// Functions

// (static) acpiRSDPChecksum(uint8_t *ptr) - Validate the RSDP checksum.
static bool acpiRSDPChecksum(uint8_t *ptr) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < 20; i++) {
        sum += ptr[i];
    }

    if (sum) return false;
    return true;
}

// acpiParseFacp(ACPI_FADT *facp) - Parse the FACP (Fixed ACPI Description Table, its signature is FACP)
void acpiParseFacp(ACPI_FADT *facp) {
    if (facp->SMI_CommandPort != 0) {
        // Enable ACPI.
        // outportb(facp->SMI_CommandPort, facp->AcpiEnable);
    } else {
        serialPrintf("acpiParseFacp: Cannot enable ACPI, already enabled.\n");
    }
    return;
}

// acpiParseApic(ACPI_MADT *table) - Parse an APIC table for the RSDT.
void acpiParseApic(ACPI_MADT *table) {
    madt = table;
    
    // Set up the local APIC address
    serialPrintf("acpiParseApic: local APIC address is 0x%x\n", &madt->localAPIC_addr);
    localAPICAddress = (uint8_t*)(void*)&madt->localAPIC_addr;
    

    // Iterate through each APIC header, checking its type.
    uint8_t *ptr = (uint8_t*)(madt + 1);
    uint8_t *endAddr = (uint8_t *)madt + table->header.length;
    while (ptr < endAddr) {
        APICHeader *header = (APICHeader*)ptr;
        uint8_t headerLength = header->length;

        if ((uint8_t)header->type == APIC_TYPE_LOCAL_APIC) {
            APICLocal *local = (APICLocal*)ptr;

            serialPrintf("\t- Found CPU: %d %d %x\n", local->ACPIProcessorID, local->apicID, local->flags);
            if (ACPI_cpuCount < 16) {
                ACPI_cpuIDs[ACPI_cpuCount] = local->apicID;
                ACPI_cpuCount++;
            }
        } else if ((uint8_t)header->type == APIC_TYPE_IO_APIC) {
            APICIO *io = (APICIO*)ptr;
            serialPrintf("\t- Found I/O APIC: %d 0x%x %d\n", io->ioAPIC_id, &io->ioAPIC_addr, io->globalSystemInterruptBase);
            
            ioAPIC_addr = (uint8_t*)&io->ioAPIC_addr;
        } else if ((uint8_t)header->type == APIC_TYPE_IO_INT_OVERRIDE) {
            APICInterruptOverride *into = (APICInterruptOverride*)ptr;
            serialPrintf("\t- Found interrupt override: %d %d %d 0x%x\n", into->bus, into->source, into->interrupt, into->flags);
        } else if ((uint8_t)header->type == APIC_TYPE_IO_NMI_SOURCE) {
            APICIO_NMISource *nmi = (APICIO_NMISource*)ptr;
            serialPrintf("\t- Found I/O APIC NMI source - %02x %04x %08x\n", nmi->nmi, nmi->flags, nmi->interrupt);
        } else if ((uint8_t)header->type == APIC_TYPE_LOCAL_NMI) {
            APICLocalNMI *nmi = (APICLocalNMI*)ptr;
            serialPrintf("\t- Found local APIC NMI source - %02x %04x %02x\n", nmi->processor_id, nmi->flags, nmi->lint);
        } else if ((uint8_t)header->type == APIC_TYPE_LOCAL_ADDR) {
            APICLocalAddressOverride *override = (APICLocalAddressOverride*)ptr;
            serialPrintf("\t- Found local APIC address override - %016llx\n", override->address);
        } else if ((uint8_t)header->type == APIC_TYPE_LOCALX2_APIC) {
            APICLocalx2 *apic = (APICLocalx2*)ptr;
            serialPrintf("\t- Found Local x2APIC - %08x %08x %08x\n", apic->apic_id, apic->flags, apic->acpi_id);
        } else {
            serialPrintf("\t- Found unknown APIC structure type %d\n", (uint8_t)header->type);
        }
        ptr += headerLength;
    }
}


// acpiParseRSDT(ACPIHeader *rsdt) - Parses the RSDT (Root System Description Table)
void acpiParseRSDT(ACPIHeader *rsdt) {
    // Iterate through all tables and parse each one.
    uint32_t *tablePtr = (uint32_t *)(rsdt + 1);
    uint32_t *endAddr = (uint32_t*)((uint8_t*)rsdt + rsdt->length);
    


    serialPrintf("ACPI table signatures (RSDT):\n");

    while (tablePtr < endAddr) {
        uint32_t address = *tablePtr++;
        ACPIHeader *table = (ACPIHeader*)address;

        // Parse the table signature first (like in acpiInit do our trick where we convert to uint32 and check against hex instead of calling strcmp).
        uint32_t signature = (uint32_t)table->signature;

        // Get the string of the signature.
        char signature_str[4];
        memcpy(signature_str, table->signature, 4);
        signature_str[4] = '\0';
        serialPrintf("\t%s 0x%x\n", signature_str, signature);


        // Check signature - can either be FACP or APIC.
        if (!strncmp(signature_str, "FACP", 4)) {
            acpiParseFacp((ACPI_FADT*)table);
        } else if (!strncmp(signature_str, "APIC", 4)) {
            acpiParseApic((ACPI_MADT*)table);
        }
    }
}

// acpiParseXSDT(ACPIHeader *xsdt) - Parses the XSDT (eXtended System Description Table)
void acpiParseXSDT(ACPIHeader *xsdt) {
    // It's close to the same thing as the RSDT but we use long values instead.

    uint64_t *tablePtr = (uint64_t*)(xsdt+1);
    uint64_t *endAddr = (uint64_t*)((uint8_t*)xsdt + xsdt->length);

    serialPrintf("ACPI table signatures (XSDT):");
    while (tablePtr < endAddr) {
        uint64_t address = *tablePtr++;
        ACPIHeader *table = (ACPIHeader*)address;

        // Parse the table signature first (like in acpiInit do our trick where we convert to uint32 and check against hex instead of calling strcmp).
        uint32_t signature = (uint32_t)table->signature;

        // Get the string of the signature.
        char signature_str[4];
        memcpy(signature_str, table->signature, 4);
        signature_str[4] = '\0';
        serialPrintf("\t%s 0x%x\n", signature_str, signature);

        // Check signature - can either be FACP or APIC.
        if (signature == 0x50434146) {
            acpiParseFacp((ACPI_FADT*)table);
        } else if (signature == 0x43495041) {
            acpiParseApic((ACPI_MADT*)table);
        }
        

    }
}


// acpiParseRSDP(uint8_t *ptr) - Parses the RSDP (Root System Description Pointer)
bool acpiParseRSDP(uint8_t *ptr) {
    // Validate the checksum first.
    if (!acpiRSDPChecksum(ptr)) {
        serialPrintf("acpiParseRSDP: checksum validation failed\n");
        return false;
    }
    
    
    
    // Check the version and parse accordingly.
    // We either need to parse the RSDT or XSDT (either standing for Root System Description Table or eXtended System Description Table)
    // Note: Convert to v1 header here because v2 is just an extension, so if header revision is 2 then just reconvert to RSDP v2.

    RSDPDescriptor *header = (RSDPDescriptor*)ptr;
    
    
    // OEMID is bugged (properly because it's not null terminated?) so we just copy the OEM to a char
    char oem[7];
    memcpy(oem, header->OEMID, 6);
    oem[6] = '\0';
    serialPrintf("acpiParseRSDP: (dbg) OEM is %s\n", oem);
    


    if (header->revision == 0) {
        // (for debugging purposes)
        serialPrintf("acpiParseRSDP: found ACPI version 1.0, parsing RSDT...\n");

        vmm_allocateRegionFlags(header->rsdtAddress, header->rsdtAddress, (uint32_t)(header->rsdtAddress + (sizeof(char)*4)), 1, 0, 0);

        uint32_t addr = *(uint32_t*)(ptr + 16);

        ACPIHeader *rsdt = (ACPIHeader*)addr;

        acpiParseRSDT(rsdt);
    } else if (header->revision == 2) {
        // (for debugging purposes)
        serialPrintf("acpiParseRSDP: found ACPI version 2.0, parsing XSDT...\n");

        // Get RSDT and XSDT address for one final check.
        uint32_t rsdtAddress = *(uint32_t*)(ptr + 16);
        uint64_t xsdtAddress = *(uint64_t*)(ptr + 24);
        
        vmm_allocateRegionFlags(rsdtAddress, rsdtAddress, (uint32_t)(rsdtAddress + (sizeof(char) * 4)), 1, 0, 0);
        vmm_allocateRegionFlags(xsdtAddress, xsdtAddress, (uint32_t)(xsdtAddress + (sizeof(char) * 4)), 1, 0, 0);

        if (xsdtAddress) {
            // Parse the XSDT.
            acpiParseXSDT((ACPIHeader*)xsdtAddress);
        } else {
            // Parse the RSDT
            acpiParseRSDT((ACPIHeader*)rsdtAddress);
        }
    } else {
        serialPrintf("apciParseRSDP: Unsupported ACPI version %d.\n", header->revision);
    }

    return true;

}


// acpiInit() - Initializes ACPI
void acpiInit() {
    // We need to search the BIOS area for the RSDP (root system description pointer)
    uint8_t *i = (uint8_t*)0x000E0000;
    uint8_t *endArea = (uint8_t *)0x000FFFFF;

    while (i < endArea) {
        // Find the signature of whatever BIOS area we are in.
        // The signature is actually characters that say 'RSD PTR ' (not null terminated).
        // For now, to make things easier, we'll coonvert it to 0x2052545020445352 (the same string in unsigned long form.)

        uint64_t signature = *(uint64_t*)i;
        if (signature == 0x2052545020445352) {
            serialPrintf("acpiInit: Found RSDP signature at 0x%x\n", i);
            // We (might've) found the RSDP. Parse it.
            if (acpiParseRSDP(i)) {
                serialPrintf("acpiInit: Successfully enabled ACPI.\n");
                printf("ACPI enabled successfully.\n");
                break;
            }
        }

        // We didn't find it, continue searching.
        i += 16;
    }
    
    // TODO: Implement AML checking
}
