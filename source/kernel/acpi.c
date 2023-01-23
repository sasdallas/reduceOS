// ====================================================================================
// acpi.c - reduceOS ACPI handler (Advanced Configuration and Power Interface)
// ====================================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/acpi.h" // ACPI header file

RSDPDescriptor_t rsdpDescriptor_v1;
RSDPDescriptor2_t rsdpDescriptor_v2;

// Function prototypes
void acpiParseRSDT(ACPI_SDTHeader *rsdtDescriptor); // Parse the Root System Description Tables.
void acpiParseTable(ACPI_SDTHeader *rsdtTable); // Parse an ACPI table.
void acpiParseFACP(ACPI_FADT *facp); // Parse a FACP table from the RSDT.
bool acpiParseRSDP(uint8_t *rsdpAddress); // Parses the Root System Descriptor Pointer to check if it is valid.
void acpiParseXSDT(ACPI_SDTHeader *xsdtDescriptor); // Parse the eXtended System Description Tables

// acpiParseRSDP() - Parses the Root System Descriptor Pointer to check if it is valid.
bool acpiParseRSDP(uint8_t *rsdpAddress) {
    // We need to validate the RSDP by checking the signature.
    // Also, we need to check the revision and parse the RSDT (Root System Description Table) XSDT (eXtended System Description Table)

    // First, verify the checksum.
    uint8_t checksum = 0;
    for (int i = 0; i < 20; i++) {
        checksum += rsdpAddress[i];
    }

    if (checksum) {
        // Checksum failed to validate.
        serialPrintf("acpiParseRSDP: Checksum validation failed.\n");
        return false;
    }

    
    // Second, print the OEMID.
    char oem[7];
    memcpy(oem, rsdpAddress + 9, 6);
    oem[6] = '\0';
    serialPrintf("RSDP OEM: %s\n", oem);

    

    // Check the revision to see if we need to parse the XSDT.
    uint8_t revision = rsdpAddress[15];

    if (revision == 0) {
        // Identified as version 1.0.
        serialPrintf("acpiParseRSDP: Identified RSDP as ACPI version 1.0\n");
        uint32_t rsdtAddress = *(uint32_t *)(rsdpAddress + 16);
        rsdpDescriptor_v1.checksum = checksum;
        strcpy(rsdpDescriptor_v1.OEM_ID, oem);
        rsdpDescriptor_v1.signature = 0x2052545020445352;
        rsdpDescriptor_v1.revision = 0;
        rsdpDescriptor_v1.rsdtAddress = rsdtAddress;
        rsdpDescriptor_v2.beginning = rsdpDescriptor_v1;

        acpiParseRSDT((ACPI_SDTHeader*)(uint32_t*)rsdtAddress);
    } else if (revision == 2) {
        serialPrintf("acpiParseRSDP: Identified RSDP as ACPI version 2.0\n");
        uint32_t rsdtAddress = *(uint32_t*)(rsdpAddress + 16);
        uint64_t xsdtAddress = *(uint64_t*)(rsdpAddress + 24);

        if (xsdtAddress) {
            acpiParseXSDT((ACPI_SDTHeader*)(unsigned*)xsdtAddress);
        } else {
            acpiParseRSDT((ACPI_SDTHeader*)(unsigned*)rsdtAddress);
        }
    } else {
        serialPrintf("acpiParseRSDP: Terminating due to unsupported revision %i\n", revision);
    }
    return true;
}

// acpiParseRSDT(ACPI_SDTHeader *rsdtDescriptor) - Parse the Root System Description Tables
void acpiParseRSDT(ACPI_SDTHeader *rsdtDescriptor) {
    uint32_t *start = (uint32_t*)(rsdtDescriptor + 1);
    uint32_t *end = (uint32_t*)((uint8_t)&rsdtDescriptor + rsdtDescriptor->length);

    while (start < end) {
        uint32_t address = *start++;
        acpiParseTable((ACPI_SDTHeader *)(unsigned *)rsdtDescriptor);
    }
}

// acpiParseXSDT(ACPI_SDTHeader *xsdtDescriptor) - Parse the eXtended System Description Tables
void acpiParseXSDT(ACPI_SDTHeader *xsdtDescriptor) {
    // Main difference b/n RSDT and XSDT (in coding) is that XSDT uses 64-bit unsigned values.
    uint64_t *start = (uint64_t*)(xsdtDescriptor + 1);
    uint64_t *end = (uint64_t*)((uint8_t*)xsdtDescriptor + xsdtDescriptor->length);

    while (start < end) {
        uint64_t address = *start++;
        acpiParseTable((ACPI_SDTHeader *)(unsigned *)xsdtDescriptor);
    }
} 

// acpiParseFACP(ACPI_FADT) - Parse a FACP table from the RSDT.
void acpiParseFACP(ACPI_FADT *facp) {
    if (facp->SMI_CommandPort) {
        serialPrintf("Enabling ACPI...\n");
        outportb(facp->SMI_CommandPort, facp->AcpiEnable);
    } else {
        serialPrintf("acpiParseFACP: ACPI is already enabled.\n");
    }
}

// TODO: Implement APIC support

// acpiParseTable(ACPI_SDTHeader *rsdtTable) - Parse an ACPI RSDT header.
void acpiParseTable(ACPI_SDTHeader *rsdtTable) {
    uint32_t tableSignature = rsdtTable->signature;
    
    // Validate table signature.
    /* char signatureString[5];
    memcpy(signatureString, &tableSignature, 4);
    signatureString[4] = '\0';
    serialPrintf("RSDT table %s (0x%x)\n", signatureString, tableSignature);*/

    if (tableSignature == 0x50434146) {
        acpiParseFACP((ACPI_FADT*)rsdtTable);
    } else if (tableSignature == 0x43495041) {
        serialPrintf("acpiParseTable: requested to parse non-facp table but we don't have APIC yet.\n");
    }
}


// acpiInit() - Initializes ACPI by searching for the RSDP in the main BIOS area and parsing it.
void acpiInit() {
    // Search the main BIOS area.
    uint8_t *start = (uint8_t*)0x000E0000;
    uint8_t *end = (uint8_t*)0x000FFFFFF;

    while (start < end) {
        uint64_t signature = *(uint64_t*)start;

        // 0x2052545020445352 is the RSDP signature.
        if (signature == 0x2052545020445352) {
            serialPrintf("acpiInit: Found RSDP at address 0x%x\n", start);
            if (acpiParseRSDP(start)) break;
        }

        start += 16;
    }
}