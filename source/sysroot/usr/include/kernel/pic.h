// pic.h - Header file for pic.c (Programmable Interrupt Controller)

#ifndef PIC_H
#define PIC_H

// Includes
#include <kernel/idt.h> // Interrupt Descriptor Table

#include <kernel/hal.h> // Hardware Abstraction Layer
#include <kernel/terminal.h> // Terminal output (printf)
#include <libk_reduced/stdint.h> // Integer types like uint8_t, uint16_t, etc...


// Definitions




// PIC 1 register port addresses
#define PIC1_REG_COMMAND    0x20
#define PIC1_REG_STATUS     0x20
#define PIC1_REG_DATA       0x21
#define PIC1_REG_IMR        0x21

// PIC 2 register port addresses
#define PIC2_REG_COMMAND    0xA0
#define PIC2_REG_STATUS     0xA0
#define PIC2_REG_DATA       0xA1
#define PIC2_REG_IMR        0xA1

// Initialization control word 1 bit masks
#define PIC_ICW1_MASK_IC4           0x1 // 00000001
#define PIC_ICW1_MASK_SNGL          0x2 // 00000010
#define PIC_ICW1_MASK_ADI           0x4 // 00000100
#define PIC_ICW1_MASK_LTIM          0x8 // 00001000
#define PIC_ICW1_MASK_INIT          0x10 // 00010000

// Note: Initialization control words 2 and 3 don't require bit masks, so we don't need to define them
// Control word 4, on the other hand does, so we do need to define them.

// Initialization control word 4 bit masks
#define PIC_ICW4_MASK_UPM           0x1 // 00000001
#define PIC_ICW4_MASK_AEOI          0x2 // 00000010
#define PIC_ICW4_MASK_MS            0x4 // 00000100
#define PIC_ICW4_MASK_BUF           0x8 // 00001000
#define PIC_ICW4_MASK_SFNM          0x10 // 00010000



// Moving on to the control bits...
// Initialization command 1 control bits
#define PIC_ICW1_IC4_EXPECT             1   // 1
#define PIC_ICW1_IC4_NO                 0   // 0
#define PIC_ICW1_SNGL_YES               2   // 10
#define PIC_ICW1_SNGL_NO                0   // 00 
#define PIC_ICW1_ADI_CALLINTERVAL4      4   // 100
#define PIC_ICW1_ADI_CALLINTERVAL8      0   // 000
#define PIC_ICW1_LTIM_LEVELTRIGGERED    8   // 1000
#define PIC_ICW1_LTIM_EDGETRIGGERED     0   // 0000
#define PIC_ICW1_INIT_YES               0x10 // 10000
#define PIC_ICW1_INIT_NO                0   // 00000

// Initialization command 4 control bits
#define PIC_ICW4_UPM_86MODE             1   // 1
#define PIC_ICW4_UPM_MCSMODE            0   // 0
#define PIC_ICW4_AEOI_AUTOEOI           2   // 10
#define PIC_ICW4_AEOI_NOAUTOEOI         0   // 00
#define PIC_ICW4_MS_BUFFERMASTER        4   // 100
#define PIC_ICW4_MS_BUFFERSLAVE         0   // 0
#define PIC_ICW4_BUF_MODEYES            8   // 1000
#define PIC_ICW4_BUF_MODENO             0   // 0
#define PIC_ICW4_SFNM_NESTEDMODE        0x10 // 10000
#define PIC_ICW4_SFNM_NOTNESTED         0   // 0

// Devices

// These devices use PIC 1 to generate interrupts
#define PIC_IRQ_TIMER                   0
#define PIC_IRQ_KEYBOARD                1
#define PIC_IRQ_SERIAL2                 3
#define PIC_IRQ_SERIAL1                 4
#define PIC_IRQ_PARALLEL2               5
#define PIC_IRQ_DISKETTE                6
#define PIC_IRQ_PARALLEL1               7

// These devices use PIC 2 to generate interrupts
#define PIC_IRQ_CMOSTTIMER              0
#define PIC_IRQ_CGARETRACE              1
#define PIC_IRQ_AUXILIARY               4
#define PIC_IRQ_FPU                     5
#define PIC_IRQ_HDC                     6


// Command word bit masks

// Command word 2 bit masks - use when sending commands
#define PIC_OCW2_MASK_L1                1           // 00000001
#define PIC_OCW2_MASK_L2                2           // 00000010
#define PIC_OCW2_MASK_L3                4           // 00000100
#define PIC_OCW2_MASK_EOI               0x20        // 00100000
#define PIC_OCW2_MASK_SL                0x40        // 01000000
#define PIC_OCW2_MASK_ROTATE            0x80        // 10000000

// Command word 3 bit masks - use when sending commands
#define PIC_OCW3_MASK_RIS               1           // 00000001
#define PIC_OCW3_MASK_RIR               2           // 00000010
#define PIC_OCW3_MASK_MODE              4           // 00000100
#define PIC_OCW3_MASK_SMM               0x20        // 00100000
#define PIC_OCW3_MASK_ESMM              0x40        // 01000000
#define PIC_OCW3_MASK_D7                0x80        // 10000000



// Function definitions

// Read data byte from PIC
extern uint8_t picReadData(uint8_t picNum);

// Send a data byte from PIC
extern void picSendData(uint8_t data, uint8_t picNum);

// Send operational command to PIC
extern void picSendCommand(uint8_t cmd, uint8_t picNum);

// Enables and disables interrupts
extern void picMaskIRQ(uint8_t irqmask, uint8_t picNum);

// Initialize pic
extern void picInit(uint8_t base0, uint8_t base1);

#endif
