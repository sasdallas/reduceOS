// hal.h - Header file for the HAL.c file (Hardware Abstraction Layer)

#ifndef HAL_H
#define HAL_H


// Includes
#include "include/idt.h" // Interrupt Descriptor Table
#include "include/pic.h" // Programmable Interrupt Controller
#include "include/pit.h" // Programmable interval Timer
#include "include/libc/stdint.h" // Integer declarations (like uint8_t, uint16_t, etc..)

// Definitions

#define far
#define near

// Functions

extern void setVector(int intNo, uint32_t vect); // Sets a new interrupt vector.
extern void interruptCompleted(uint32_t intNo); // Notifies HAL interrupt is done.
extern void enableHardwareInterrupts(); // Enable hardware interrupts
extern void disableHardwareInterrupts(); // Disable hardware interrupts
extern uint8_t inportb(uint16_t port); // Read data from a device using port mapped IO
extern void outportb(uint16_t port, uint8_t value); // Write data to a device using port mapped IO
extern void __cpuid(uint32_t type, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx); // Returns an assembly cpuid instruction.

#endif