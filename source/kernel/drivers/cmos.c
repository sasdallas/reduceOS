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
    hal_disableHardwareInterrupts();
    cmos_selectRegister(cmosreg);
    uint8_t ret = inportb(0x71); 
    hal_enableHardwareInterrupts();
    return ret;
}

void cmos_writeRegister(uint8_t cmosreg, uint8_t value) {
    hal_disableHardwareInterrupts();
    cmos_selectRegister(cmosreg);
    outportb(CMOS_DATA, value);
    hal_enableHardwareInterrupts();
}


// cmos_dump(uint16_t *values) - Dumps the contents of CMOS into values
void cmos_dump(uint16_t *values) {
    for (uint16_t index = 0; index < 128; index++) {
        outportb(CMOS_ADDRESS, index);
        values[index] = inportb(CMOS_DATA);
    }
}


// cmos_isUpdateInProgress() - Returns whether an update is in progress
int cmos_isUpdateInProgress() {
    outportb(CMOS_ADDRESS, 0x0a);
    return inportb(CMOS_DATA) & 0x80;
}
