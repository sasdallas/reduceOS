// ahci.h - Header file for the AHCI driver

#ifndef AHCI_H
#define AHCI_H

// Includes
#include <libk_reduced/stdint.h>

/**** Definitions ****/

// Biggest difference between SATA and parallel ATA is that SATA uses FIS
// FIS stands for Frame Information Structure. It's a type of packet used to transport data between host and device.
// FIS can be viewed as a data set of traditional task files.

typedef enum {
    FIS_TYPE_REG_H2D    = 0x27,     // Register FIS - Host to device
    FIS_TYPE_REG_D2H    = 0x34,     // Register FIS - Device to host
    FIS_TYPE_DMA_ACT    = 0x39,     // DMA activate FIS - Device to host
    FIS_TYPE_DMA_SETUP  = 0x41,     // DMA setup FIS - bidirectional
    FIS_TYPE_DATA       = 0x46,     // Data FIS - bidirectional
    FIS_TYPE_BIST       = 0x58,     // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP  = 0x5F,     // PIO setup FIS - Device to host
    FIS_TYPE_DEV_BITS   = 0xA1,     // Set device bits FIS - Device to host
} FIS_TYPE;


// Register FIS structure for host to device
typedef struct _ahci_fis_reg_h2d {
    // DWORD 0
    uint8_t     fis_type;           // Should be FIS_TYPE_REG_H2D
    uint8_t     pmport:4;           // Port multiplier
    uint8_t     reserved0:3;        // Reserved
    uint8_t     c:1;                // If this is 1, it means command, 0 is control

    uint8_t     command;            // Command register
    uint8_t     feature_lo;         // Low bytes of the feature register
    
    // DWORD 1
    uint8_t     lba0;               // LBA low register, bits 0-7
    uint8_t     lba1;               // LBA mid register, bits 8-15
    uint8_t     lba2;               // LBA high register, bits 16-23
    uint8_t     device;             // Device register

    // DWORD 2
    uint8_t     lba3;               // LBA register, bits 24-31
    uint8_t     lba4;               // LBA register, bits 32-39
    uint8_t     lba5;               // LBA register, bits 40-47
    uint8_t     feature_hi;         // High bytes of the feature register

    // DWORD 3
    uint8_t     count_lo;           // Low bytes of the count register
    uint8_t     count_hi;           // High bytes of the count register
    uint8_t     icc;                // Isochronous command completion
    uint8_t     control;            // Control register

    // DWORD 4
    uint8_t     reserved1[4];       // Reserved
} ahci_fis_reg_h2d_t;

// Register FIS structure for device to host
typedef struct _ahci_fis_reg_d2h {
    // DWORD 0
    uint8_t     fis_type;           // Should be FIS_TYPE_REG_D2H

    uint8_t     pmport:4;           // Port multiplier
    uint8_t     reserved0:2;        // Reserved
    uint8_t     i:1;                // Interrupt bit
    uint8_t     reserved1:1;        // Reserved

    uint8_t     status;             // Status register
    uint8_t     error;              // Error register
    
    // DWORD 1
    uint8_t     lba0;               // LBA low register, bits 0-7
    uint8_t     lba1;               // LBA mid register, bits 8-15
    uint8_t     lba2;               // LBA high register, bits 16-23
    uint8_t     device;             // Device register

    // DWORD 2
    uint8_t     lba3;               // LBA register, bits 24-31
    uint8_t     lba4;               // LBA register, bits 32-39
    uint8_t     lba5;               // LBA register, bits 40-47
    uint8_t     reserved2;          // High bytes of the feature register

    // DWORD 3
    uint8_t     count_lo;           // Low bytes of the count register
    uint8_t     count_hi;           // High bytes of the count register
    uint8_t     reserved3[2];       // Reserved

    // DWORD 4
    uint8_t     reserved4[4];       // Reserved
} ahci_fis_reg_d2h_t;


// Data FIS - bidirectional structure
typedef struct _ahci_fis_data {
    // DWORD 0
    uint8_t     fis_type;           // Should be FIS_TYPE_DATA

    uint8_t     pmport:4;           // Port multiplier
    uint8_t     reserved0:4;        // Reserved

    uint8_t     reserved1[2];       // Reserved

    // DWORD 1 ~ N
    uint32_t    data[1];            // Payload
} ahci_fis_data_t;


// PIO setup - data to host structure
typedef struct _ahci_fis_pio_setup {
    // DWORD 0
    uint8_t     fis_type;           // Should be FIS_TYPE_PIO_SETUP

    uint8_t     pmport:4;           // Port multiplier
    uint8_t     reserved0:1;        // Reserved
    uint8_t     d:1;                // Data transfer direction
    uint8_t     i:1;                // Interrupt bit
    uint8_t     reserved1:1;        // Reserved

    uint8_t     status;             // Status register
    uint8_t     error;              // Error register

    // DWORD 1
    uint8_t     lba0;               // LBA low register, bits 0-7
    uint8_t     lba1;               // LBA mid register, bits 8-15
    uint8_t     lba2;               // LBA high register, bits 16-23
    uint8_t     device;             // Device register

    // DWORD 2
    uint8_t     lba3;               // LBA register, bits 24-31
    uint8_t     lba4;               // LBA register, bits 32-39
    uint8_t     lba5;               // LBA register, bits 40-47
    uint8_t     reserved2;          // Reserved

    // DWORD 3
    uint8_t     count_lo;           // Low bytes of count register
    uint8_t     count_hi;           // High bytes of count register
    uint8_t     reserved3;          // Reserved
    uint8_t     e_status;           // New value of status register

    // DWORD 4
    uint16_t    tc;                 // Transfer count
    uint8_t     reserved4[2];       // Reserved
} ahci_fis_pio_setup_t;

typedef struct _ahci_fis_dma_setup {
    // DWORD 0
    uint8_t     fis_type;           // Should be FIS_TYPE_DMA_SETUP

    uint8_t     pmport:4;           // Port multiplier
    uint8_t     reserved0:1;        // Reserved
    uint8_t     d:1;                // Data transfer direction, 1 - device to host
    uint8_t     i:1;                // Interrupt bit
    uint8_t     a:1;                // Auto-activate

    uint8_t     reserved1[2];       // Reserved

    // DWORD 1 and DWORD 2

    uint64_t    DMABufferID;        // DMA buffer identifier. Used to identify the DMA buffer in the host's memory.
                                    // According to the SATA specification, this is host-specific.

    // DWORD 3
    uint32_t    reserved2;          // Reserved

    // DWORD 4
    uint32_t    DMABufferOffset;    // Byte offset into buffer. First two bits should be 0.

    // DWORD 5
    uint32_t    TransferCount;      // Number of bytes to transfer.

    // DWORD 6
    uint32_t    reserved3;          // Reserved 
} ahci_fis_dma_setup_t;

// HBA registers
// Link to how these thingies work: https://wiki.osdev.org/AHCI#/media/File:HBA_registers.jpg

// The host communicates with the AHCI controller through system memory and mm registers.
// PCI BAR5 of the controller should point to base memory (called ABAR, stands for AHCI Base Memory Register)
// Other PCI base address registers act the same as a traditional IDE controller.

// These structures are sourced from OSDev wiki.

typedef volatile struct _ahci_hba_port
{
	uint32_t clb;		// 0x00, command list base address, 1K-byte aligned
	uint32_t clbu;		// 0x04, command list base address upper 32 bits
	uint32_t fb;		// 0x08, FIS base address, 256-byte aligned
	uint32_t fbu;		// 0x0C, FIS base address upper 32 bits
	uint32_t is;		// 0x10, interrupt status
	uint32_t ie;		// 0x14, interrupt enable
	uint32_t cmd;		// 0x18, command and status
	uint32_t rsv0;		// 0x1C, Reserved
	uint32_t tfd;		// 0x20, task file data
	uint32_t sig;		// 0x24, signature
	uint32_t ssts;		// 0x28, SATA status (SCR0:SStatus)
	uint32_t sctl;		// 0x2C, SATA control (SCR2:SControl)
	uint32_t serr;		// 0x30, SATA error (SCR1:SError)
	uint32_t sact;		// 0x34, SATA active (SCR3:SActive)
	uint32_t ci;		// 0x38, command issue
	uint32_t sntf;		// 0x3C, SATA notification (SCR4:SNotification)
	uint32_t fbs;		// 0x40, FIS-based switch control
	uint32_t rsv1[11];	// 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4];	// 0x70 ~ 0x7F, vendor specific
} ahci_hba_port_t;

typedef volatile struct _ahci_hba_mem
{
	// 0x00 - 0x2B, Generic Host Control
	uint32_t cap;		// 0x00, Host capability
	uint32_t ghc;		// 0x04, Global host control
	uint32_t is;		// 0x08, Interrupt status
	uint32_t pi;		// 0x0C, Port implemented
	uint32_t vs;		// 0x10, Version
	uint32_t ccc_ctl;	// 0x14, Command completion coalescing control
	uint32_t ccc_pts;	// 0x18, Command completion coalescing ports
	uint32_t em_loc;		// 0x1C, Enclosure management location
	uint32_t em_ctl;		// 0x20, Enclosure management control
	uint32_t cap2;		// 0x24, Host capabilities extended
	uint32_t bohc;		// 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	uint8_t  rsv[0xA0-0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	uint8_t  vendor[0x100-0xA0];

	// 0x100 - 0x10FF, Port control registers
	ahci_hba_port_t	ports[1];	// 1 ~ 32
} ahci_hba_mem_t;

typedef volatile struct _ahci_hba_fis
{
	// 0x00
	ahci_fis_dma_setup_t	dsfis;		// DMA Setup FIS
	uint8_t         pad0[4];

	// 0x20
	ahci_fis_pio_setup_t	psfis;		// PIO Setup FIS
	uint8_t         pad1[12];

	// 0x40
	ahci_fis_reg_d2h_t	rfis;		// Register – Device to Host FIS
	uint8_t         pad2[4];

	// 0x58
	uint8_t	        sdbfis;		// Set Device Bit FIS
	
	// 0x60
	uint8_t         ufis[64];

	// 0xA0
	uint8_t   	rsv[0x100-0xA0];
} ahci_hba_fis_t;


typedef struct _ahci_hba_cmdheader
{
	// DWORD 0
	uint8_t  cfl:5;		// Command FIS length in DWORDS, 2 ~ 16
	uint8_t  a:1;		// ATAPI
	uint8_t  w:1;		// Write, 1: H2D, 0: D2H
	uint8_t  p:1;		// Prefetchable

	uint8_t  r:1;		// Reset
	uint8_t  b:1;		// BIST
	uint8_t  c:1;		// Clear busy upon R_OK
	uint8_t  rsv0:1;		// Reserved
	uint8_t  pmp:4;		// Port multiplier port

	uint16_t prdtl;		// Physical region descriptor table length in entries

	// DWORD 1
	volatile
	uint32_t prdbc;		// Physical region descriptor byte count transferred

	// DWORD 2 & DWORD 3
	uint32_t ctba;		// Command table descriptor base address
	uint32_t ctbau;		// Command table descriptor base address upper 32 bits

	// DWORD 4-7
	uint32_t rsv1[4];	// Reserved
} ahci_hba_cmdheader_t;

typedef struct _ahci_hba_prdt_entry
{
	uint32_t dba;		// Data base address
	uint32_t dbau;		// Data base address upper 32 bits
	uint32_t rsv0;		// Reserved

	// DW3
	uint32_t dbc:22;		// Byte count, 4M max
	uint32_t rsv1:9;		// Reserved
	uint32_t i:1;		// Interrupt on completion
} ahci_hba_prdt_t;

typedef struct _ahci_hba_cmdtable
{
	// 0x00
	uint8_t  cfis[64];	// Command FIS

	// 0x40
	uint8_t  acmd[16];	// ATAPI command, 12 or 16 bytes

	// 0x50
	uint8_t  rsv[48];	// Reserved

	// 0x80
	ahci_hba_prdt_t	prdt_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
} ahci_hba_cmdtable_t;


// PX commands
#define AHCI_PXCMD_ST       (1 << 0UL)
#define AHCI_PXCMD_SUD      (1 << 1UL)
#define AHCI_PXCMD_POD      (1 << 2UL)
#define AHCI_PXCMD_CLO      (1 << 3UL)
#define AHCI_PXCMD_FRE      (1 << 4UL)
#define AHCI_PXCMD_MPSS     (1 << 13UL)
#define AHCI_PXCMD_FR       (1 << 14UL)
#define AHCI_PXCMD_CR       (1 << 15UL)

// Port signatures
#define AHCI_PORTSIG_ATAPI  0xEB140101
#define AHCI_PORTSIG_HDD    0x00000101
#define AHCI_PORTSIG_NONE   0xFFFF0101

#endif 

