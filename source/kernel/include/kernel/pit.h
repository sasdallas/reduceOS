// pit.h - Header file for pit.c (Programmable Interval Timer)

#ifndef PIT_H
#define PIT_H

// Includes
#include <kernel/idt.h> // Interrupt Descriptor Table
#include <kernel/pic.h> // Programmable Interrupt Controller
#include <kernel/hal.h> // Hardware Abstraction Layer
#include <kernel/terminal.h> // Terminal output (printf)
#include <kernel/isr.h> // Interrupt Service Routines
#include <stdint.h> // Integer declarations

// Definitions

// Controller registers
#define PIT_REG_COUNTER0        0x40
#define PIT_REG_COUNTER1        0x41
#define PIT_REG_COUNTER2        0x42
#define PIT_REG_COMMAND         0x43


// Operational command bit masks

#define PIT_OCW_MASK_BINCOUNT       1 // 00000001
#define PIT_OCW_MASK_MODE           0xE // 00001110
#define PIT_OCW_MASK_RL             0x30 // 00110000
#define PIT_OCW_MASK_COUNTER        0xC0 // 11000000


// Operational command control bits

// Use these when setting binary count mode
#define PIT_OCW_BINCOUNT_BINARY         0   // 0
#define PIT_OCW_BINCOUNT_BCD            1   // 1

// Use these when setting counter mode
#define PIT_OCW_MODE_TERMINALCOUNT      0   // 0000
#define PIT_OCW_MODE_ONESHOT            0x2 // 0010
#define PIT_OCW_MODE_RATEGEN            0x4 // 0100
#define PIT_OCW_MODE_SQUAREWAVEGEN      0x6 // 0110
#define PIT_OCW_MODE_SOFTWARETRIG       0x8 // 1000
#define PIT_OCW_MODE_HARDWARETRIG       0xA // 1010

// Use these when setting data transfer
#define PIT_OCW_RL_LATCH                0   // 000000
#define PIT_OCW_RL_LSBONLY              0x10 // 010000
#define PIT_OCW_RL_MSBONLY              0x20 // 100000
#define PIT_OCW_RL_DATA                 0x30 // 110000

// Use these when setting the counter we're working with
#define PIT_OCW_COUNTER_0               0    // 00000000
#define PIT_OCW_COUNTER_1               0x40 // 01000000
#define PIT_OCW_COUNTER_2               0x80 // 10000000



// Functions


void pitSendCommand(uint8_t cmd); // Send operational command to PIT.
void pitSendData(uint16_t data, uint8_t counter); // Write data byte to a counter.
uint64_t pitSetTickCount(uint64_t i); // Sets new PIT tick count and returns prev. value.
uint64_t pitGetTickCount(); // Returns current tick count.
void pitStartCounter(uint32_t freq, uint8_t counter, uint8_t mode); // Starts a counter (counter continues until another call)
void pitInit(); // Initialize PIT
bool pitIsInitialized(); // Check if the PIT is initialized.


#endif
