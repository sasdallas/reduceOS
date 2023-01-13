// ide_ata.h - header file for the PCI IDE controller

#ifndef IDE_ATA__H
#define IDE_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/hal.h" // Hardware abstraction layer
#include "include/isr.h" // IRQ handling
#include "include/libc/sleep.h" // Sleep function


// Definitions

// Status port bit masks
#define ATA_STATUS_BSY 0x80             // Busy
#define ATA_STATUS_DRDY 0x40            // Drive ready
#define ATA_STATUS_DF 0x20              // Drive write fault
#define ATA_STATUS_DSC 0x10             // Drive seek complete
#define ATA_STATUS_DRQ 0x08             // Data request ready
#define ATA_STATUS_CORR 0x04            // Corrected data
#define ATA_STATUS_IDX 0x02             // Index
#define ATA_STATUS_ERR 0x01             // Error


// Error register bits
#define ERR_AMNF 0x01                  // Address mark not found.
#define ERR_TKZNF 0x02                 // Track zero not found.
#define ERR_ABRT 0x04                  // Aborted command.
#define ERR_MCR 0x08                   // Media change request.
#define ERR_IDNF 0x10                  // ID not found
#define ERR_MC 0x20                    // Media changed.
#define ERR_UNC 0x40                   // Unrecoverable data error.
#define ERR_BBK 0x80                   // Bad block.

// IDE commands (https://wiki.osdev.org/PCI_IDE_Controller)
#define ATA_READ_PIO          0x20
#define ATA_READ_PIO_EXT      0x24
#define ATA_READ_DMA          0xC8
#define ATA_READ_DMA_EXT      0x25
#define ATA_WRITE_PIO         0x30
#define ATA_WRITE_PIO_EXT     0x34
#define ATA_WRITE_DMA         0xCA
#define ATA_WRITE_DMA_EXT     0x35
#define ATA_CACHE_FLUSH       0xE7
#define ATA_CACHE_FLUSH_EXT   0xEA
#define ATA_PACKET            0xA0
#define ATA_IDENTIFY_PACKET   0xA1
#define ATA_IDENTIFY          0xEC

// ATAPI-specific commands
#define ATAPI_READ 0xA8
#define ATAPI_EJECT 0x18

// Identification space information (identification space is the buffer of 512 bytes returned by ATA_IDENTIFY_PACKET and ATA_IDENTIFY)
#define ATA_IDENT_DEVICETYPE 0
#define ATA_IDENT_CYLINDERS 2
#define ATA_IDENT_HEADS 6
#define ATA_IDENT_SECTORS 12
#define ATA_IDENT_SERIAL 20
#define ATA_IDENT_MODEL 54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID 106
#define ATA_IDENT_MAX_LBA 120
#define ATA_IDENT_COMMANDSETS 164
#define ATA_IDENT_MAX_LBA_EXT 200


// IDE/ATA interface types
#define IDE_ATA 0x00
#define IDE_ATAPI 0x01

#define ATA_MASTER 0x00
#define ATA_SLAVE 0x01

// ATA registers
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

// ATA channels
#define ATA_PRIMARY 0x00
#define ATA_SECONDARY 0x01

// ATA directions
#define ATA_READ 0x00
#define ATA_WRITE 0x01

// Typedefs

// IDE channel registers
typedef struct {
    uint16_t ioBase; // IO base
    uint16_t controlBase; // Control base
    uint16_t busMasterIDE; // Bus master IDE
    uint8_t nIEN; // nIEN (no interrupt)
} ideChannelRegisters_t;


typedef struct {
    uint8_t reserved; // Defines whether the drive realy exists (non-zero if it does)
    uint8_t channel; // Defines whether the drive is primary channel or secondary (non-zero if secondary)
    uint8_t drive; // Defines whether the drive is a master drive or a slave drive (non-zero if slave)
    uint16_t type; // Defines what the drive is (non-zero if ATAPI)
    uint16_t signature; // Drive signature
    uint16_t features; // Drive features
    uint32_t commandSets; // Command sets supported
    uint32_t size; // Size (in sectors)
    uint8_t model[41]; // Drive model.
} ideDevice_t;

// Functions
uint8_t ideRead(uint8_t channel, uint8_t reg); // Reads in a register
void ideWrite(uint8_t channel, uint8_t reg, uint8_t data); // Writes to a register
void ideReadBuffer(uint8_t channel, uint8_t reg, uint32_t buffer, uint32_t quads); // Reads the identification space and copies it to a buffer.
void insl(uint16_t reg, uint32_t *buffer, int quads); // Reads a long word from a register port for quads times.
void outsl(uint16_t reg, uint32_t *buffer, int quads); // Writes a long word to a register port for quads times
uint8_t idePolling(uint8_t channel, uint32_t advancedCheck); // Returns whether there was an error.
uint8_t idePrintErrors(uint32_t drive, uint8_t err); // Prints the errors that may have occurred.
void ideInit(uint32_t bar0, uint32_t bar1, uint32_t bar2, uint32_t bar3, uint32_t bar4); // Initialize IDE drives.
void printIDESummary(); // Print a summary of all IDE drives found.
uint8_t ideAccessATA(uint8_t direction, uint8_t drive, uint32_t lba, uint8_t sectorNum, uint16_t selector, uint32_t edi); // Access an ATA drive (direction specifies what operation to perform)
uint8_t ideReadATAPI(uint8_t drive, uint32_t lba, uint8_t sectorNum, uint16_t selector, uint32_t edi); // Read from an ATAPI drive.
void ideReadSectors(uint8_t drive, uint8_t sectorNum, uint32_t lba, uint16_t es, uint32_t edi); // Read from an ATA/ATAPI drive.
void ideWriteSectors(uint8_t drive, uint8_t sectorNum, uint32_t lba, uint16_t es, uint32_t edi); // Write to an ATA drive.

#endif