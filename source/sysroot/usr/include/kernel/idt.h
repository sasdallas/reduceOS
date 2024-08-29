// idt.h - IDT header file, contains includes/declarations for the idt.c file.

#ifndef IDT_H
#define IDT_H

// Includes
#include <libk_reduced/stddef.h> // size_t declaration
#include <libk_reduced/stdint.h> // Integer type declarations
#include <libk_reduced/stdbool.h> // Boolean declarations
#include <libk_reduced/string.h> // String functions
#include <libk_reduced/limits.h> // Limits on integers and more.
#include <libk_reduced/stdarg.h> // va argument handling (for ... on printf)
#include <libk_reduced/va_list.h> // va_list declared here.
#include <kernel/terminal.h> // printf() and other terminal functions

// Definitions

#define I86_MAX_INTERRUPTS 256 // 256 maximum interrupts

// These must be in the format 0D110 where D is the descriptor type
#define I86_IDT_DESC_BIT16 0x06 // 00000110
#define I86_IDT_DESC_BIT32 0x0E // 00001110
#define I86_IDT_DESC_RING1 0x40 // 01000000
#define I86_IDT_DESC_RING2 0x20 // 00100000
#define I86_IDT_DESC_RING3 0x60 // 01100000
#define I86_IDT_DESC_PRESENT 0x80 // 10000000


// Typedefs

typedef void (*IDT_IRQ_HANDLER)(void); // Interrupt handler without an error code. Interrupt handlers are called by the processor. Since the stack setup may change, we leave it up to the interrupts' implementation to handle it and properly return.

typedef struct {
    uint16_t baseLow; // Lower 16 bits (0-15) of the interrupt routine address (address to jump when interrupt fires.)
    uint16_t segmentSelector; // Code segment selector in GDT
    uint8_t reserved; // Reserved. Should be 0.
    uint8_t flags; // Bit flags.
    uint16_t baseHigh; // Higher 16 bits (16-31) of the interrupt routine address (see baseLow)
} __attribute__((packed)) idtEntry_t;

typedef struct {
    uint16_t limit; // size of the IDT
    uint32_t base_addr; // base address of the IDT
} __attribute__((packed)) idtPtr_t; 




// Functions


extern int idtInstallIR(uint8_t i, uint8_t flags, uint16_t segmentSelector, uint32_t base); // Installs interrupt handler. When INT is fired, it calls this
extern void idtInit(); // Initializes basic IDT.



#endif
