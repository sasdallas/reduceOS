// =======================================================
// local_apic.c - Handles setting up the local APIC.
// =======================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/local_apic.h> // Main header file



// First, what is the local APIC? Well, APIC stands for "Advanced Programmable Interrupt Controller", or a newer version of the 8259 PIC chip.
// Intel's more recent processors now use the APIC standard, so it's essential to implement in a kernel.
// Second, why is it a "local" APIC? There are two versions of the APIC, IO and local.
// In an APIC-based system (either with a more recent processor or a multiprocessor system), each CPU is made of a core and local APIC.
// The local APIC is responsible for handling CPU-specific interrupt configuration and a little more. The IO APIC is part of the chipset and provides multi-processor interrupt management.
// https://wiki.osdev.org/APIC

// NOTE: localAPICAddress should be defined by the ACPI and/or linker script!
uint8_t *localAPICAddress = 0x0;

// Now, onto the static functions!

// (static) localAPIC_read(uint32_t reg) - Reads from the local APIC.
static uint32_t localAPIC_read(uint32_t reg) {
    return *(volatile uint32_t *)(localAPICAddress + reg);
}

// (static) localAPIC_write(uint32_t reg, uint32_t data)
static void localAPIC_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t *)(localAPICAddress + reg) = data;
}

// And finally, the main funtcions:


// localAPIC_init() - Initialize the local APIC
void localAPIC_init() {
    // Make sure ACPI actually set something
    if (localAPICAddress == 0x0) {
        serialPrintf("localAPIC_init: Cannot initialize if localAPIC is 0x0.\n");    
        return;
    }

    // Clear task priority to enable all interrupts.
    localAPIC_write(LOCAL_APIC_TPR, 0);

    // Enable logical destination mode
    localAPIC_write(LOCAL_APIC_DFR, 0xFFFFFFFF); // Flat mode
    localAPIC_write(LOCAL_APIC_LDR, 0x01000000); // All CPUs use logical ID 1

    // Configure the SPI register.
    localAPIC_write(LOCAL_APIC_SVR, 0x100 | 0xFF);  
}


// localAPIC_getID() - returns the ID of the local APIC.
uint8_t localAPIC_getID() {
    return localAPIC_read(LOCAL_APIC_ID) >> 24;
}

// localAPIC_sendInit(uint8_t apicID) - Sends an init request to local APIC.
void localAPIC_sendInit(uint8_t apicID) {
    localAPIC_write(LOCAL_APIC_ICRHI, apicID << ICR_DESTINATION_SHIFT);
    localAPIC_write(LOCAL_APIC_ICRLO, ICR_INIT | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);

    // Wait until APIC is done.
    while (localAPIC_read(LOCAL_APIC_ICRLO) & ICR_SEND_PENDING);
}


// localAPIC_sendStartup(uint8_t apicID, uint8_t vector) - Sends a startup request to an APIC with a vector.
void localAPIC_sendStartup(uint8_t apicID, uint8_t vector) {
    localAPIC_write(LOCAL_APIC_ICRHI, apicID << ICR_DESTINATION_SHIFT);
    localAPIC_write(LOCAL_APIC_ICRLO, vector | ICR_STARTUP | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);

    while (localAPIC_read(LOCAL_APIC_ICRLO) & ICR_SEND_PENDING);
}

