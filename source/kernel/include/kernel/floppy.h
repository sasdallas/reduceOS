// floppy.h - Header file for the floppy driver

#ifndef FLOPPY_H
#define FLOPPY_H

// Includes
#include <stdint.h> // Integer definitions
#include <kernel/hal.h> // Hardware Abstraction Layer
#include <kernel/dma.h> // Direct Memory Addressing (floppies CAN use PIO mode to transfer, but its better to do ISA DMA)

// Definitions (note: not gonna even bother with 5.25-inch drives, sticking with 3.5-inch)
#define FLOPPY_HEADS 2
#define FLOPPY_144MB_CYLINDERS 80
#define FLOPPY_144MB_SECTORS_PER_TRACK 18

#define FLOPPY_IRQ 6
#define FLOPPY_DMA_CHANNEL 2


// Floppy registers
#define FLOPPY_STATUS_A 0x3F0               // (read-only) Status register A
#define FLOPPY_STATUS_B 0x3F1               // (read-only) Status register B
#define FLOPPY_DIGITALOUTPUT 0x3F2          // Digital output registers 
#define FLOPPY_TAPEDRIVE 0x3F3              // Tape drive
#define FLOPPY_MAINSTATUS 0x3F4             // (read-only) MSR
#define FLOPPY_DATARATE_SEL 0x3F4           // (write-only) Data rate selection
#define FLOPPY_DATA_FIFO 0x3F5              // Data FIFO buffer
#define FLOPPY_DIGITALINPUT 0x3F7           // (read-only) Digital input register
#define FLOPPY_CONFIGCTRL 0x3F7             // (write-only) Config control register

// Digital Output bitflags
#define FLOPPY_DIGITALOUTPUT_MOTOR3 0x80    // Turns on drive 3's motor
#define FLOPPY_DIGITALOUTPUT_MOTOR2 0x40    // Turns on drive 2's motor
#define FLOPPY_DIGITALOUTPUT_MOTOR1 0x20    // Turns on drive 1's motor
#define FLOPPY_DIGITALOUTPUT_MOTOR0 0x10    // Turns on drive 0's motor
#define FLOPPY_DIGITALOUTPUT_IRQ 0x8        // Enables IRQs and DMA
#define FLOPPY_DIGITALOUTPUT_RESET 0x4      // If cleared, enter reset mode. Else, normal operation
#define FLOPPY_DIGITALOUTPUT_DRIVESEL01 0x3 // Select drive number for next access

// MSR bitflags
#define FLOPPY_MSR_RQM 0x80                 // OK to exchange bytes with FIFO IO port?
#define FLOPPY_MSR_DIO 0x40                 // Does the FIFO IO port expect an IN opcode?
#define FLOPPY_MSR_NDMA 0x20                // Set in execution phase of PIO mode read/write commands only
#define FLOPPY_MSR_CB 0x10                  // Command Busy
#define FLOPPY_MSR_SEEK3 0x8                // Drive 3 is seeking
#define FLOPPY_MSR_SEEK2 0x4                // Drive 2 is seeking
#define FLOPPY_MSR_SEEK1 0x2                // Drive 1 is seeking
#define FLOPPY_MSR_SEEK0 0x1                // Drive 0 is seeking

// Floppy commands (written to the DATA_FIFO)
#define FLOPPY_CMD_READTRACK 2              // (generates IRQ6)
#define FLOPPY_CMD_SPECIFY 3                // Set drive parameters
#define FLOPPY_CMD_SENSESTATUS 4            // Check the drive status
#define FLOPPY_CMD_WRITEDATA 5              // Write data
#define FLOPPY_CMD_READDATA 6               // Read data
#define FLOPPY_CMD_RECALIBRATE 7            // Seeks to cylinder 0
#define FLOPPY_CMD_SENSEINT 8               // Acknowledge IRQ6, get the status of the last command.
#define FLOPPY_CMD_WRITEDELETED 9           // TBD
#define FLOPPY_CMD_READID 10                // (generates IRQ6)
#define FLOPPY_CMD_READDELETED 12           // TBD
#define FLOPPY_CMD_TRACKFORMAT 13           // Format a track
#define FLOPPY_CMD_DUMPREG 14               // TBD
#define FLOPPY_CMD_SEEK 15                  // Seek both heads to cylinder X
#define FLOPPY_CMD_VERSION 16               // Get the floppy controller version
#define FLOPPY_CMD_SCANEQ 17                // TBD
#define FLOPPY_CMD_PERPENDICULAR 18         // Turn on/off perpendicular mode(?)
#define FLOPPY_CMD_CONFIGURE 19             // Set controller parameters
#define FLOPPY_CMD_LOCK 20                  // Prevent controller parameters from resetting
#define FLOPPY_CMD_VERIFY 22                // TBD
#define FLOPPY_CMD_SCANLOE 25               // TBD
#define FLOPPY_CMD_SCANHOE 29               // TBD

// Extended command bitmasks
#define FLOPPY_CMD_EXT_SKIP 0x20            // Skip deleted data address marks
#define FLOPPY_CMD_EXT_DENSITY 0x40         // Operate in FM (single density) or MFM (double density) mode
#define FLOPPY_CMD_EXT_MULTITRACK 0x80      // Operate on one track or both tracks of the cylinder


// GAP 3 (gap length - space b/n sectors on the disk)
#define FLOPPY_GAP3_STD 42                  // tbd, i can't read documentation
#define FLOPPY_GAP3_3_5 27                  // 3.5" drives

// When sending commands to the FDC, they must follow a specific formula: 2 ^ n * 128
// According to the documentation, some drives support reading up to 16KB per sector, but most don't, so we'll use a list of the most common
#define FLOPPY_BPS_128 0                    // 128 bytes per sector
#define FLOPPY_BPS_256 1                    // 256 bytes per sector
#define FLOPPY_BPS_512 2                    // 512 bytes per sector
#define FLOPPY_BPS_1024 3                   // 1024 bytes per sector

// Error codes floppyReadSector might return.
#define FLOPPY_OK 0
#define FLOPPY_ERROR -1
#define FLOPPY_INVALID_CMD -2
#define FLOPPY_DRIVE_NOT_READY -3
#define FLOPPY_SEEK_FAIL -4
#define FLOPPY_DRIVE_READ_ONLY -5

// Functions
void floppy_init(); // Initialize the floppy drive
void floppy_reset(); // Resets the floppy drive
void floppy_disableFDC(); // Disables the FDC
void floppy_enableFDC(); // Enables the FDC
int floppy_seek(uint32_t cylinder, uint32_t head); // Seek a floppy drive's head to a cylinder
int floppy_calibrateDrive(uint32_t drive); // Calibrate a floppy drive
void floppy_setDrive(uint8_t drive); // Set the current drive
void floppy_driveData(uint32_t steprate, uint32_t loadtime, uint32_t unloadtime, bool isDMA); // Passes control info to the FDC about the drive
int floppy_readSectorInternal(uint8_t head, uint8_t track, uint8_t sector); // Read a sector (internal use, should be static but eh)
void floppy_dmaInit(uint8_t *buffer, size_t length); // Initialize the floppy drive for DMA
void floppy_stopMotor(); // Stop the floppy motor
void floppy_startMotor(uint8_t drive); // Start the motor on a drive
void floppy_IRQ(); // Called when floppy IRQ is present
void floppy_waitIRQ(); // Waits for an IRQ, then resets the value
void floppy_acknowledgeIRQ(uint32_t *st0, uint32_t *cyl); // Acknowledges to the FDC an interrupt was received
int floppy_readSector(int lba, uint8_t *buffer); // Read a sector from the floppy disk.


#endif
