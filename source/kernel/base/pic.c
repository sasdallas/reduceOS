// =========================================================================
// pic.c - Programmable Interrupt Controller
// This file handles setting up the Programmable Interrupt Controller (PIC)
// =========================================================================


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// DOES NOT WORK
// This file does not work. PIC is not initialized correctly, rendering keyboard and other things INOPERABLE.
// DO NOT USE - SEE IDT.C FOR TEMPORARY WORK AROUND
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#include <kernel/pic.h> // Main header file


// picSendCommand(uint8_t cmd, uint8_t picNum) - Send a command to a certain PIC
void picSendCommand(uint8_t cmd, uint8_t picNum) {
    if (picNum > 1) return; // Don't bother if the picNum is greater than one (there are only two!)

    uint8_t reg = (picNum == 1) ? PIC2_REG_COMMAND : PIC1_REG_COMMAND; // Choose the right thing to send based on the picNum
    outportb(reg, cmd);
}

// picSendData(uint8_t data, uint8_t picNum) - Send data to PICs
void picSendData(uint8_t data, uint8_t picNum) {
    if (picNum > 1) return; // Don't bother if the picNum is greater than one (there are only two)

    uint8_t reg = (picNum==1) ? PIC2_REG_DATA : PIC1_REG_DATA;
    outportb(reg, data);
}

// uint8_t picReadData(uint8_t picNum) - Read data from PICs
uint8_t picReadData(uint8_t picNum) {
    if (picNum > 1) return; // Don't bother if the picNum is greater than one (there are only two)

    uint8_t reg = (picNum == 1) ? PIC2_REG_DATA : PIC1_REG_DATA;
    return inportb(reg);
}

// picInit(uint8_t base0, uint8_t base1) - Initialize the PIC
void picInit(uint8_t base0, uint8_t base1) {
    uint8_t icw = 0;

    disableHardwareInterrupts(); // Disable hardware interrupts
    
    // Begin initialization of PIC
    icw = (icw & ~PIC_ICW1_MASK_INIT) | PIC_ICW1_INIT_YES;
    icw = (icw & ~PIC_ICW1_MASK_IC4) | PIC_ICW1_IC4_EXPECT;

    // Send commands to both PICs
    picSendCommand(icw, 0);
    picSendCommand(icw, 1);

    // Send initialization control word 2 (base addresses of the IRQs)
    picSendData(base0, 0);
    picSendData(base1, 1);

    // Send initialization control word 3 (connect between master + slave)
    picSendData(0x04, 0);
    picSendData(0x02, 1);

    // Send initialization control word 4 (enabled I86 mode)
    
    icw = (icw & ~PIC_ICW4_MASK_UPM) | PIC_ICW4_UPM_86MODE;

    picSendData(icw, 0);
    picSendData(icw, 1);
    printf("Programmable Interrupt Controller initialized.\n");
}
