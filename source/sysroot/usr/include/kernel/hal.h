// hal.h - Header file for the HAL.c file (Hardware Abstraction Layer)

#ifndef HAL_H
#define HAL_H


// Includes
#include <kernel/idt.h> // Interrupt Descriptor Table
#include <kernel/pic.h> // Programmable Interrupt Controller
#include <kernel/pit.h> // Programmable interval Timer
#include <libk_reduced/stdint.h> // Integer declarations (like uint8_t, uint16_t, etc..)
#include <libk_reduced/stddef.h> // size_t declaration

// Definitions

#define far
#define near


// BDA definitions (see https://wiki.osdev.org/Memory_Map_(x86)#BIOS_Data_Area_.28BDA.29)
#define HAL_BDA_START 0x400
#define HAL_BDA_END 0x498

// Functions

void setVector(int intNo, uint32_t vect); // Sets a new interrupt vector.
void interruptCompleted(uint32_t intNo); // Notifies HAL interrupt is done.
void enableHardwareInterrupts(); // Enable hardware interrupts
void disableHardwareInterrupts(); // Disable hardware interrupts
uint8_t inportb(uint16_t port); // Read data from a device using port mapped IO
void outportb(uint16_t port, uint8_t value); // Write data to a device using port mapped IO
uint16_t inportw(uint16_t port); // Read data from a device using port mapped IO
void outportw(uint16_t port, uint16_t data); // Write data to a device using port mapped IO
void __cpuid(uint32_t type, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx); // Returns an assembly cpuid instruction.
uint32_t inportl(uint16_t port); // Read data from a device via port mapped IO
void outportl(uint16_t port, uint32_t data); // Write data to a device via port mapped IO
size_t msb(size_t i); // Returns the most significant bit.
void setVector_flags(int intNo, uint32_t vect, int flags); // Sets a new interrupt vector using flags.
unsigned short hal_getBIOSArea(unsigned short offset); // Returns the value at the offset in the BDA

#endif
