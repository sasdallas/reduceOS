// acpi.h - header file for the Advanced Configuration Power Interface manager

#ifndef ACPI_H
#define ACPI_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations.
#include <kernel/local_apic.h> // Local APIC (different from ACPI)
#include <kernel/io_apic.h> // IO APIC
#include <libk_reduced/string.h> // Misc. functions
#include <kernel/bios32.h> // BIOS32 functions

// Typedefs

// ACPI header
typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char OEM[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t creatorID;
    uint32_t creatorRevision;
} __attribute__((packed)) ACPIHeader;

// ACPI FADT (Fixed ACPI Description Table)
// https://wiki.osdev.org/FADT

typedef struct {
  uint8_t AddressSpace;
  uint8_t BitWidth;
  uint8_t BitOffset;
  uint8_t AccessSize;
  uint64_t Address;
} GenericAddressStructure;

typedef struct {
    ACPIHeader h;
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
    GenericAddressStructure ResetReg;
 
    uint8_t  ResetValue;
    uint8_t  Reserved3[3];
 
    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_FirmwareControl;
    uint64_t                X_Dsdt;
 
    GenericAddressStructure X_PM1aEventBlock;
    GenericAddressStructure X_PM1bEventBlock;
    GenericAddressStructure X_PM1aControlBlock;
    GenericAddressStructure X_PM1bControlBlock;
    GenericAddressStructure X_PM2ControlBlock;
    GenericAddressStructure X_PMTimerBlock;
    GenericAddressStructure X_GPE0Block;
    GenericAddressStructure X_GPE1Block;
} ACPI_FADT;

// APIC headers
typedef struct {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) APICHeader;

typedef struct {
    APICHeader header;
    uint8_t ACPIProcessorID;
    uint8_t apicID;
    uint32_t flags;
} __attribute__((packed)) APICLocal;

typedef struct {
    APICHeader header;
    uint8_t ioAPIC_id;
    uint8_t reserved;
    uint32_t ioAPIC_addr;
    uint32_t globalSystemInterruptBase;
} __attribute__((packed)) APICIO;

typedef struct {
    APICHeader header;
    uint8_t bus;
    uint8_t source;
    uint32_t interrupt;
    uint16_t flags;
} __attribute__((packed)) APICInterruptOverride;

typedef struct {
    ACPIHeader header;
    uint8_t nmi;            // NMI source
    uint8_t reserved;       // Reserved
    uint16_t flags;         // Flags
    uint32_t interrupt;     // Global System Interrupt
} __attribute__((packed)) APICIO_NMISource;

typedef struct {
    ACPIHeader header;
    uint8_t processor_id;   // CPU ID
    uint16_t flags;         // Flags
    uint8_t lint;           // LINT#
} __attribute__((packed)) APICLocalNMI;

typedef struct {
    ACPIHeader header;
    uint16_t reserved;      // Reserved
    uint64_t address;       // 64-bit address of local APIC
} __attribute__((packed)) APICLocalAddressOverride;

typedef struct {
    ACPIHeader header;
    uint16_t reserved;      // Reserved
    uint32_t apic_id;       // APIC ID
    uint32_t flags;         // Flags
    uint32_t acpi_id;       // ACPI ID (not to be confused with APIC_ID)
} __attribute__((packed)) APICLocalx2;

// RSDP (Root System Description Pointer)
// ACPI version 1.0
typedef struct {
    char signature[8];
    uint8_t checksum;
    char OEMID[6];
    uint8_t revision;
    uint32_t rsdtAddress;
} __attribute__((packed)) RSDPDescriptor;

// ACPI version 2.0
typedef struct {
    RSDPDescriptor start;
    uint32_t length;
    uint64_t xsdtAddress;
    uint8_t extendedChecksum;
    uint8_t reserved[3];
} __attribute__((packed)) RSDPDescriptor_v2;

typedef struct {
    ACPIHeader header;
    uint32_t localAPIC_addr;
    uint32_t flags;
} __attribute__((packed)) ACPI_MADT;

typedef struct {
    ACPIHeader h;
    uint32_t pointers[];
} RSDT;


// Definitions
#define APIC_TYPE_LOCAL_APIC            0
#define APIC_TYPE_IO_APIC               1
#define APIC_TYPE_IO_INT_OVERRIDE       2
#define APIC_TYPE_IO_NMI_SOURCE         3
#define APIC_TYPE_LOCAL_NMI             4
#define APIC_TYPE_LOCAL_ADDR            5
#define APIC_TYPE_LOCALX2_APIC          9



// External variables
extern uint8_t *localAPICAddress;
extern uint8_t *ioAPIC_addr;


// Functions
void acpiInit();

#endif
