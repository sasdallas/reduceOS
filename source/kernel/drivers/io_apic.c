// =======================================================
// io_apic.c - Handles setting up the I/O APIC.
// =======================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/io_apic.h> // Main header file
#include <kernel/serial.h>

// (copy and pasted from local_apic.c)
// First, what is the local APIC? Well, APIC stands for "Advanced Programmable Interrupt Controller", or a newer version of the 8259 PIC chip.
// Intel's more recent processors now use the APIC standard, so it's essential to implement in a kernel.
// Second, why is it a "local" APIC? There are two versions of the APIC, IO and local.
// In an APIC-based system (either with a more recent processor or a multiprocessor system), each CPU is made of a core and local APIC.
// The local APIC is responsible for handling CPU-specific interrupt configuration and a little more. The IO APIC is part of the chipset and provides multi-processor interrupt management.
// https://wiki.osdev.org/APIC


// Global variables
uint8_t *ioAPIC_addr = 0x0; // SHOULD BE SET BY ACPI

// Static functions

// (static) ioAPIC_write(uint8_t *base, uint8_t reg, uint32_t val) - Write to the IO APIC.
static void ioAPIC_write(uint8_t *base, uint8_t reg, uint32_t val) {
    *(volatile uint32_t *)(base + IO_APIC_REGSEL) = reg;
    *(volatile uint32_t *)(base + IO_APIC_WIN) = val;
}

// (static) ioAPIC_read(uint8_t *base, uint8_t reg) - Read from the IO APIC.
static uint32_t ioAPIC_read(uint8_t *base, uint8_t reg) {
    *(volatile uint32_t *)(base + IO_APIC_REGSEL) = reg;
    return *(volatile uint32_t *)(base + IO_APIC_WIN);
}

// Functions.

// ioAPIC_setEntry(uint8_t *base, uint8_t index, uint64_t data) - Set an entry in the IO APIC.
void ioAPIC_setEntry(uint8_t *base, uint8_t index, uint64_t data) {
    ioAPIC_write(base, IO_APIC_REDTBL + index * 2, (uint32_t)data);
    ioAPIC_write(base, IO_APIC_REDTBL + index * 2 + 1, (uint32_t)(data >> 32));
}

// ioAPIC_init() - Initialize the IO APIC.
void ioAPIC_init() {
    // Make sure ACPI actually set this variable
    if (ioAPIC_addr == 0x0) {
        serialPrintf("ioAPIC_init: Cannot initialize I/O APIC when ioAPIC_addr == 0x0\n");
        return;
    }

    // First, get the number of entries supported by the IO APIC.
    uint32_t x = ioAPIC_read(ioAPIC_addr, IO_APIC_VER);
    uint32_t count = ((x >> 16) & 0xFF) + 1;

    // serialPrintf("ioAPIC_init: got %d entries supported for the IO APIC\n", count);

    // Disable all entries.
    for (uint32_t i = 0; i < count; i++) { ioAPIC_setEntry(ioAPIC_addr, i, 1 << 16); }
}
