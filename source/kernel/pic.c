// =========================================================================
// pic.c - Programmable Interrupt Controller
// This file handles setting up the Programmable Interrupt Controller (PIC)
// =========================================================================

#include "include/pic.h" // Main header file


// i86_picSendCommand(uint8_t cmd, uint8_t picNum) - Send a command to a certain PIC
void i86_picSendCommand(uint8_t cmd, uint8_t picNum) {
    if (picNum > 1) return; // Don't bother if the picNum is greater than one (there are only two!)

    uint8_t reg = (picNum == 1) ? I86_PIC2_REG_COMMAND : I86_PIC1_REG_COMMAND; // Choose the right thing to send based on the picNum
    outportb(reg, cmd);
}

// i86_picSendData(uint8_t data, uint8_t picNum) - Send data to PICs
void i86_picSendData(uint8_t data, uint8_t picNum) {
    if (picNum > 1) return; // Don't bother if the picNum is greater than one (there are only two)

    uint8_t reg = (picNum==1) ? I86_PIC2_REG_DATA : I86_PIC1_REG_DATA;
    outportb(reg, data);
}

// uint8_t i86_picReadData(uint8_t picNum) - Read data from PICs
uint8_t i86_picReadData(uint8_t picNum) {
    if (picNum > 1) return; // Don't bother if the picNum is greater than one (there are only two)

    uint8_t reg = (picNum == 1) ? I86_PIC2_REG_DATA : I86_PIC1_REG_DATA;
    return inportb(reg);
}

// i86_picInit(uint8_t base0, uint8_t base1) - Initialize the PIC
void i86_picInit(uint8_t base0, uint8_t base1) {
    uint8_t icw = 0;

    disableHardwareInterrupts(); // Disable hardware interrupts
    
    // Begin initialization of PIC
    icw = (icw & ~I86_PIC_ICW1_MASK_INIT) | I86_PIC_ICW1_INIT_YES;
    icw = (icw & ~I86_PIC_ICW1_MASK_IC4) | I86_PIC_ICW1_IC4_EXPECT;

    // Send commands to both PICs
    i86_picSendCommand(icw, 0);
    i86_picSendCommand(icw, 1);

    // Send initialization control word 2 (base addresses of the IRQs)
    i86_picSendData(base0, 0);
    i86_picSendData(base1, 1);

    // Send initialization control word 3 (connect between master + slave)
    i86_picSendData(0x04, 0);
    i86_picSendData(0x02, 1);

    // Send initialization control word 4 (enabled I86 mode)
    
    icw = (icw & ~I86_PIC_ICW4_MASK_UPM) | I86_PIC_ICW4_UPM_86MODE;

    i86_picSendData(icw, 0);
    i86_picSendData(icw, 1);
    printf("Programmable Interrupt Controller initialized.\n");
}