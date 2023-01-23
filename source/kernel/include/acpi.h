// acpi.h - header file for the Advanced Configuration and Power Interface handler.

#ifndef ACPI_H
#define ACPI_H

// Includes
#include "include/libc/stdint.h" // Integer definitions
#include "include/libc/stdbool.h" // Boolean definitions
// Typedefs

// RSDP descriptors
// RSDP stands for Root System Description Pointer. It is a data structure used in the ACPI programming interface.

// Version 1.0 RSDP descriptor
struct RSDPDescriptor_struct {
    uint32_t signature;
    uint8_t checksum;
    char OEM_ID[6];
    uint8_t revision;
    uint32_t rsdtAddress;
} __attribute__ ((packed));

typedef struct RSDPDescriptor_struct RSDPDescriptor_t;




// Version 2.0 RSDP descriptor
struct RSDPDescriptor_v2 {
    RSDPDescriptor_t beginning;
    uint32_t length;
    uint64_t XSDTAddress;
    uint8_t extendedChecksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

typedef struct RSDPDescriptor_v2 RSDPDescriptor2_t;

// RSDT - Root system description table (version 1.0)
// Note: While RSDT is the main System Description Table, there are many kinds of SDT - so call this structure ACPI_SDTHeader.
typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char OEM_ID[6];
    char OEMTableID[8];
    uint32_t oemRevision;
    uint32_t creatorID;
    uint32_t creatorRevision;
} ACPI_SDTHeader;

// FADT - Fixed ACPI Description Table
// https://wiki.osdev.org/FADT - for both ACPI_GenericAddress and ACPI_FADT

typedef struct {
    uint8_t AddressSpace;
    uint8_t BitWidth;
    uint8_t BitOffset;
    uint8_t AccessSize;
    uint64_t Address;
} ACPI_GenericAddress;

typedef struct {
    ACPI_SDTHeader header;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;
 
    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t  Reserved;
 
    uint8_t  PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t  AcpiEnable;
    uint8_t  AcpiDisable;
    uint8_t  S4BIOS_REQ;
    uint8_t  PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t  PM1EventLength;
    uint8_t  PM1ControlLength;
    uint8_t  PM2ControlLength;
    uint8_t  PMTimerLength;
    uint8_t  GPE0Length;
    uint8_t  GPE1Length;
    uint8_t  GPE1Base;
    uint8_t  CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t  DutyOffset;
    uint8_t  DutyWidth;
    uint8_t  DayAlarm;
    uint8_t  MonthAlarm;
    uint8_t  Century;
 
    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t BootArchitectureFlags;
 
    uint8_t  Reserved2;
    uint32_t Flags;
 
    // 12 byte structure; see below for details
    ACPI_GenericAddress ResetReg;
 
    uint8_t  ResetValue;
    uint8_t  Reserved3[3];
 
    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_FirmwareControl;
    uint64_t                X_Dsdt;
    ACPI_GenericAddress X_PM1aEventBlock;
    ACPI_GenericAddress X_PM1bEventBlock;
    ACPI_GenericAddress X_PM1aControlBlock;
    ACPI_GenericAddress X_PM1bControlBlock;
    ACPI_GenericAddress X_PM2ControlBlock;
    ACPI_GenericAddress X_PMTimerBlock;
    ACPI_GenericAddress X_GPE0Block;
    ACPI_GenericAddress X_GPE1Block;
} ACPI_FADT;

// Functions
void acpiInit();
void acpiParseRSDT(ACPI_SDTHeader *rsdtDescriptor); // Parse the Root System Description Tables.
void acpiParseTable(ACPI_SDTHeader *rsdtTable); // Parse an ACPI table.
void acpiParseFACP(ACPI_FADT *facp); // Parse a FACP table from the RSDT.
bool acpiParseRSDP(uint8_t *rsdpAddress); // Parses the Root System Descriptor Pointer to check if it is valid.
void acpiParseXSDT(ACPI_SDTHeader *xsdtDescriptor); // Parse the eXtended System Description Tables


#endif
