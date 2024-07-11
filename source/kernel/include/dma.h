// dma.h - header file for the direct memory access controller

#ifndef DMA_H
#define DMA_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/hal.h" // Hardware Abstraction Layer


// Definitions


// Start address & count registers (slave)
#define DMA_SLAVE_STARTADDR0_REG 0x00   // (W) Start address register for channel 0 (unusable)
#define DMA_SLAVE_COUNT0_REG 0x01       // (W) Count register for channel 0 (unusable)
#define DMA_SLAVE_STARTADDR1_REG 0x02   // (W) Start address register for channel 1
#define DMA_SLAVE_COUNT1_REG 0x03       // (W) Count register for channel 1
#define DMA_SLAVE_STARTADDR2_REG 0x04   // (W) Start address register for channel 2
#define DMA_SLAVE_COUNT2_REG 0x05       // (W) Count register for channel 2
#define DMA_SLAVE_STARTADDR3_REG 0x06   // (W) Start address register for channel 3
#define DMA_SLAVE_COUNT3_REG 0x07       // (W) Count register for channel 3


// Start address & count registers (master)
#define DMA_MASTER_STARTADDR4_REG 0xC0  // (W) Start address register for channel 4 (unusable)
#define DMA_MASTER_COUNT4_REG 0xC2      // (W) Count register for channel 4 (unusable)
#define DMA_MASTER_STARTADDR5_REG 0xC4  // (W) Start address register for channel 5
#define DMA_MASTER_COUNT5_REG 0xC6      // (W) Count register for channel 5
#define DMA_MASTER_STARTADDR6_REG 0xC8  // (W) Start address register for channel 6
#define DMA_MASTER_COUNT6_REG 0xCA      // (W) Count register for channel 6
#define DMA_MASTER_STARTADDR7_REG 0xCC  // (W) Start address register for channel 7
#define DMA_MASTER_COUNT7_REG 0xCE      // (W) Count register for channel 7

// Other registers (slave)
#define DMA_SLAVE_STATUS_REG 0x08       // (R) Status register
#define DMA_SLAVE_COMMAND_REG 0x08      // (W) Command register
#define DMA_SLAVE_REQUEST_REG 0x09      // (W) Request register
#define DMA_SLAVE_MASKCHANNEL_REG 0x0A  // (W) Single channel mask register
#define DMA_SLAVE_MODE_REG 0x0B         // (W) Mode register
#define DMA_SLAVE_RESETFLOP_REG 0x0C    // (W) Flip-flop reset register
#define DMA_SLAVE_INTERMED_REG 0x0D     // (R) Intermediate register
#define DMA_SLAVE_RESETMASTER_REG 0x0D  // (W) Master reset register
#define DMA_SLAVE_MASKRESET_REG 0x0E    // (W) Mask reset register
#define DMA_SLAVE_MULTICHNL_REG 0x0F    // (RW) Multichannel mask register

// Other registers (MASTER)
#define DMA_MASTER_STATUS_REG 0x08      // (R) Status register
#define DMA_MASTER_COMMAND_REG 0x08     // (W) Command register
#define DMA_MASTER_REQUEST_REG 0x09     // (W) Request register
#define DMA_MASTER_MASKCHANNEL_REG 0x0A // (W) Single channel mask register
#define DMA_MASTER_MODE_REG 0x0B        // (W) Mode register
#define DMA_MASTER_RESETFLOP_REG 0x0C   // (W) Flip-flop reset register
#define DMA_MASTER_INTERMED_REG 0x0D    // (R) Intermediate register
#define DMA_MASTER_RESETMASTER_REG 0x0D // (W) Master reset register
#define DMA_MASTER_MASKRESET_REG 0x0E   // (W) Mask reset register
#define DMA_MASTER_MULTICHNL_REG 0x0F   // (RW) Multichannel mask register


// Page access registers (contains upper 8 bits of the 24-bit transfer memory address)
#define DMA_CHNL0_PAGEACCESS_REG 0x87  
#define DMA_CHNL1_PAGEACCESS_REG 0x83
#define DMA_CHNL2_PAGEACCESS_REG 0x81
#define DMA_CHNL3_PAGEACCESS_REG 0x82
#define DMA_CHNL4_PAGEACCESS_REG 0x8F
#define DMA_CHNL5_PAGEACCESS_REG 0x8B
#define DMA_CHNL6_PAGEACCESS_REG 0x89
#define DMA_CHNL7_PAGEACCESS_REG 0x8A


// Command register bits (literally none of these work lmao)
#define DMA_CMD_MEMORYTOMEMORY 0x1      // Memory transfers, hardware framebuffering - doesn't work
#define DMA_CMD_ADDRHOLDCHNL0 0x2       // Holds the channel 0 address - doesn't work.
#define DMA_CMD_ENABLE 0x4              // Does actually work! Disables DMA controller if set
#define DMA_CMD_COMP 0x8                // Controls whether DMA timing is compressed - doesn't work.
#define DMA_CMD_PRIORITY 0x10           // Fixed/normal DMA priority - doesn't work.
#define DMA_CMD_EXTW 0x20               // Controls whether write selection is late/extended - doesn't work.
#define DMA_CMD_DROP 0x40               // DMA request sense control - doesn't work.
#define DMA_CMD_ACK 0x80                // DMA acknowledged - doesn't work.

// Mode register bits (ugly bitmask but works)
#define DMA_MODE_SELECT_MASK 0x3        // Channel selection bits
#define DMA_MODE_TRANSFER_MASK 0xC      // Transfer type
#define DMA_MODE_SELFTEST 0             // (transfer type) Tells the DMA controller to perform a self-test
#define DMA_MODE_READTRANSFER 4         // (transfer type) Tells the DMA controller to do a read transfer
#define DMA_MODE_WRITETRANSFER 8        // (transfer type) Tells the DMA controller to do a write transfer
#define DMA_MODE_AUTO_MASK 0x10         // Automatically reinitialize after transfer completion
#define DMA_MODE_IDEC_MASK 0x20         // Reverses the memory order of the data.
#define DMA_MODE_MASK 0xC0              // Actual DMA mode
#define DMA_MODE_TRANSFERONDEMAND 0     // (mode) Transfer on demand
#define DMA_MODE_SINGLETRANSFER 0x40    // (mode) Single DMA transfer
#define DMA_MODE_BLOCKTRANSFER 0x80     // (mode) Block transfer
#define DMA_MODE_CASCADETRANSFER 0xC0   // (mode) Cascading DMA chips


// Functions
void dma_setStartAddress(uint8_t channel, uint8_t addrLow, uint8_t addrHi); // Set the starting address for the DMA controller
void dma_setCount(uint8_t channnel, uint8_t countLow, uint8_t countHi); // Sets the channel count
void dma_setPageAccess(uint8_t channel, uint8_t addr); // Set the page access bits
void dma_disableDMAC(int port, bool disabled); // Disables a DMAC (probably doesn't even work)
void dma_maskChannel(uint8_t channel); // Masks a channel in the DMAC
void dma_unmaskChannel(uint8_t channel); // Unmasks a channel in the DMAC
void dma_resetFlipFlop(int dma); // Resets the DMA flip flop (b/n 16-bit transfers)
void dma_resetDMA(int dma); // Resets the DMAC
void dma_resetMask(int dma); // Resets the masks in the DMAC, unmasking all channels
void dma_setRead(uint8_t channel); // Puts a channel into read transfer mode
void dma_setWrite(uint8_t channel); // Puts a channel into write transfer mode
void dma_initPool(int pool_size); // Initialize a DMA pool of memory, that processes can pull from for their channels
void *dma_allocPool(size_t size); // Gets a chunk from the DMA pool. Note: This chunk is statutory! You cannot put it back!

#endif