// =====================================================================
// acpi.c - handles the Advanced Configuration Power Interface
// =====================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/acpi.h" // Main header file

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
        outportb(facp->SMI_CommandPort, facp->AcpiEnable);
        serialPrintf("acpiParseFacp: ACPI enabled succesfully.\n");
    } else {
        serialPrintf("acpiParseFacp: Cannot enable ACPI, already enabled.\n");
    }
}

// acpiParseApic(ACPI_MADT *table) - Parse an APIC table for the RSDT.
void acpiParseApic(ACPI_MADT *table) {
    madt = &table;
    

    serialPrintf("acpiParseApic: local APIC address is 0x%x\n", madt->localAPIC_addr);
    localAPICAddress = (uint8_t*)&madt->localAPIC_addr;
    


    uint8_t *ptr = (uint8_t*)(madt + 1);
    uint8_t *endAddr = (uint8_t *)madt + madt->header.length;
    while (ptr < endAddr) {
        APICHeader *header = (APICHeader*)ptr;
        
        if ((uint8_t)header->type == APIC_TYPE_LOCAL_APIC) {
            APICLocal *local = (APICLocal*)ptr;

            serialPrintf("Found CPU: %d %d %x\n", local->ACPIProcessorID, local->apicID, local->flags);
            if (ACPI_cpuCount < 16) {
                ACPI_cpuIDs[ACPI_cpuCount] = local->apicID;
                ACPI_cpuCount++;
            }
        } else if ((uint8_t)header->type == APIC_TYPE_IO_APIC) {
            APICIO *io = (APICIO*)ptr;
            serialPrintf("Found I/O APIC: %d 0x%x %d\n", io->ioAPIC_id, io->ioAPIC_addr, io->globalSystemInterruptBase);
            
            ioAPIC_addr = (uint8_t*)&io->ioAPIC_addr;
        } else if ((uint8_t)header->type == APIC_TYPE_INT_OVERRIDE) {
            APICInterruptOverride *into = (APICInterruptOverride*)ptr;
            serialPrintf("Found interrupt override: %d %d %d 0x%x\n", into->bus, into->source, into->interrupt, into->flags);
        } else {
            serialPrintf("Found unknown APIC structure type %d\n", (uint8_t)header->type);
        }

        ptr += header->length;
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
        
        //serialPrintf("\t%s 0x%x\n", table->signature, signature);
        
        // Check signature - can either be FACP or APIC.
        if (signature == 0x50434146) {
            serialPrintf("Parse FACP\n");
            acpiParseFacp((ACPI_FADT*)table);
        } else if (signature == 0x43495041) {
            serialPrintf("Parse APIC\n");
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
        ACPIHeader *table = (ACPIHeader*)&address;

        // Parse the table signature first (like in acpiInit do our trick where we convert to uint32 and check against hex instead of calling strcmp).
        uint32_t signature = (uint32_t)table->signature;

        //serialPrintf("\t%s 0x%x\n", table->signature, signature);

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
    memcpy(oem, ptr + 9, 6);
    oem[6] = '\0';
    serialPrintf("acpiParseRSDP: (dbg) OEM is %s\n", oem);
    


    if (header->revision == 0) {
        // (for debugging purposes)
        serialPrintf("acpiParseRSDP: found ACPI version 1.0, parsing RSDT...\n");
        acpiParseRSDT((ACPIHeader*)ptr + 16);
    } else if (header->revision == 2) {
        // (for debugging purposes)
        serialPrintf("acpiParseRSDP: found ACPI version 2.0, parsing XSDT...\n");

        // Convert header to v2
        RSDPDescriptor_v2 *new_header = (RSDPDescriptor_v2*)&ptr;

        // Parse the XSDT.
        acpiParseXSDT((ACPIHeader*)new_header->xsdtAddress);
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
            serialPrintf("Found RSDP signature at 0x%x\n", i);
            // We (might've) found the RSDP. Parse it.
            if (acpiParseRSDP(i)) break;
        }

        // We didn't find it, continue searching.
        i += 16;
    }
}