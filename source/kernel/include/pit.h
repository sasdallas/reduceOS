// pit.h - Header file for pit.c (Programmable Interval Timer)

#ifndef PIT_H
#define PIT_H

// Includes
#include "include/idt.h" // Interrupt Descriptor Table
#include "include/pic.h" // Programmable Interrupt Controller
#include "include/hal.h" // Hardware Abstraction Layer
#include "include/terminal.h" // Terminal output (printf)
#include "include/isr.h" // Interrupt Service Routines
#include "include/libc/stdint.h" // Integer declarations

// Definitions

// Controller registers
#define I86_PIT_REG_COUNTER0        0x40
#define I86_PIT_REG_COUNTER1        0x41
#define I86_PIT_REG_COUNTER2        0x42
#define I86_PIT_REG_COMMAND         0x43


// Operational command bit masks

#define I86_PIT_OCW_MASK_BINCOUNT       1 // 00000001
#define I86_PIT_OCW_MASK_MODE           0xE // 00001110
#define I86_PIT_OCW_MASK_RL             0x30 // 00110000
#define I86_PIT_OCW_MASK_COUNTER        0xC0 // 11000000


// Operational command control bits

// Use these when setting binary count mode
#define I86_PIT_OCW_BINCOUNT_BINARY         0   // 0
#define I86_PIT_OCW_BINCOUNT_BCD            1   // 1

// Use these when setting counter mode
#define I86_PIT_OCW_MODE_TERMINALCOUNT      0   // 0000
#define I86_PIT_OCW_MODE_ONESHOT            0x2 // 0010
#define I86_PIT_OCW_MODE_RATEGEN            0x4 // 0100
#define I86_PIT_OCW_MODE_SQUAREWAVEGEN      0x6 // 0110
#define I86_PIT_OCW_MODE_SOFTWARETRIG       0x8 // 1000
#define I86_PIT_OCW_MODE_HARDWARETRIG       0xA // 1010

// Use these when setting data transfer
#define I86_PIT_OCW_RL_LATCH                0   // 000000
#define I86_PIT_OCW_RL_LSBONLY              0x10 // 010000
#define I86_PIT_OCW_RL_MSBONLY              0x20 // 100000
#define I86_PIT_OCW_RL_DATA                 0x30 // 110000

// Use these when setting the counter we're working with
#define I86_PIT_OCW_COUNTER_0               0    // 00000000
#define I86_PIT_OCW_COUNTER_1               0x40 // 01000000
#define I86_PIT_OCW_COUNTER_2               0x80 // 10000000



// Functions


void i86_pitSendCommand(uint8_t cmd); // Send operational command to PIT.
void i86_pitSendData(uint16_t data, uint8_t counter); // Write data byte to a counter.
uint32_t i86_pitSetTickCount(uint32_t i); // Sets new PIT tick count and returns prev. value.
uint32_t i86_pitGetTickCount(); // Returns current tick count.
void i86_pitStartCounter(uint32_t freq, uint8_t counter, uint8_t mode); // Starts a counter (counter continues until another call)
void i86_pitInit(); // Initialize PIT
bool i86_pitIsInitialized(); // Check if the PIT is initialized.


#endif