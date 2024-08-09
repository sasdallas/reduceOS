// ===============================================================
// cmos.c - CMOS driver, simiaar to RTC
// ===============================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/cmos.h> // CMOS header file

// We don't really need to handle a lot of CMOS because it's done for us in RTC
// This file just has some basic functions.

// NMIs or Non-Maskable Interrupts can be handled very differently for each OS.
// They send a panic signal to the CPU that it can't ignore, and the CMOS has a built-in register to disable them.
// reduceOS will not disable NMIs.


static void cmos_selectRegister(uint8_t cmosreg) {
    outportb(CMOS_ADDRESS,  cmosreg);
}

uint8_t cmos_readRegister(uint8_t cmosreg) {
    disableHardwareInterrupts();
    cmos_selectRegister(cmosreg);
    uint8_t ret = inportb(0x71);
    enableHardwareInterrupts();
}

void cmos_writeRegister(uint8_t cmosreg, uint8_t value) {
    disableHardwareInterrupts();
    cmos_selectRegister(cmosreg);
    outportb(CMOS_DATA, value);
    enableHardwareInterrupts();
}

