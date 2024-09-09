/**
 * @file source/kmods/ps2.c
 * @brief The Intel 8042 PS/2 driver
 * 
 * This file controlls setting up and initializing the 8042 PS/2 driver
 * 
 * @copyright
 * This file is part of the reduceOS C kernel, which is licensed under the terms of GPL.
 * Please credit me if this code is used.
 * 
 * Copyright (C) 2024 Samuel S.
 */


#include "ps2.h"
#include <kernel/mod.h>

static uint8_t ps2_sendCommand(uint8_t command, uint8_t byte2, int getResponse) {
    // Send the command to the port
    outportb(PS2_CMD_PORT, command);


    // If there's a second byte, handle that.
    if (byte2) {
        while ((inportb(PS2_STATUS_PORT) & PS2_STATUS_INPUTBUF)); // Wait until bit 1 is clear
        outportb(PS2_CMD_PORT, byte2);
    }

    if (getResponse) {
        while (!(inportb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUTBUF));
        return inportb(PS2_DATA_PORT);
    } else {
        return 0;
    }
}

int init(int argc, char *argv[]) {
    // A lot of people think just tossing in an IRQ handler will work, it will, but not on real hardware.
    // We need to actually initialize the i/8042 controller
    // This is unfortunately a complex process, but not too bad.
    // As I've said before, we can't trust the BIOS to do literally anything for us, nor can we trust anything else:
    // - QEMU will lead you astray! This is a problem that will only come up much later, sadly

    // TODO: Read the FADT to determine if a PS/2 controller actually exists.

    // Disable the devices at both PS/2 ports
    /*ps2_sendCommand(PS2_COMMAND_DISABLE1, NULL, 0);
    ps2_sendCommand(PS2_COMMAND_DISABLE2, NULL, 0);

    // Now we'll flush the output output buffer by reading from port 60
    inportb(0x60); 

    // Set the controller configuration byte to stop it from sending IRQs and using translations.
    uint8_t ccb = ps2_sendCommand(PS2_COMMAND_GETCCB, NULL, 1);
    uint8_t old_ccb = ccb;
    ccb &= ~(PS2_PORT1_INTERRUPT | PS2_PORT2_INTERRUPT | 0x10); // Disable interrupts and clear bit 4 to ensure clock
    ps2_sendCommand(PS2_COMMAND_WRITECCB, ccb, 0);

    // Perform the self-test
    uint8_t selftest = ps2_sendCommand(PS2_COMMAND_TEST, NULL, 1);
    if (selftest != 0x55) {
        serialPrintf("[module usb] The PS/2 controller did not complete a successful self-test - returned 0x%x.\n", selftest);
        printf("PS/2 controller did not succeed in a self-test.\n");
        return 0;
    }

    // Write the old CCB back
    ps2_sendCommand(PS2_COMMAND_WRITECCB, old_ccb, 0);

    // Determine if there are two-channels
    // Start by enabling PS/2 port two
    ps2_sendCommand(PS2_COMMAND_ENABLE2, NULL, 0);
    ccb = ps2_sendCommand(PS2_COMMAND_GETCCB, NULL, 1);
    uint8_t dc = -1;
    if (ccb & 0x20) {
        dc = 1;
        // Reset the old PS/2 configuration
        ps2_sendCommand(PS2_COMMAND_DISABLE2, NULL, 0);
        ccb &= ~(0x22); // Clear bits 1 and 5 to disable port 2 interrupts and enable its clock signal
        ps2_sendCommand(PS2_COMMAND_WRITECCB, ccb, 0);
    } else {
        dc = 0;
    }

    // Perform interface tests
    uint8_t port1test = ps2_sendCommand(PS2_COMMAND_TEST1, NULL, 1);

    if (port1test != 0x00) {
        serialPrintf("[module usb] PS/2 port #1 test failed - returned 0x%x\n", port1test);
        printf("PS/2 controller reported that port #1 has failed the self-test.\n");
        return 0;
    }

    if (dc) {
        uint8_t port2test = ps2_sendCommand(PS2_COMMAND_TEST2, NULL, 1);
        if (port2test != 0x00) {
            serialPrintf("[module usb] PS/2 port #2 test failed - returned 0x%x\n", port2test);
            printf("PS/2 controller reported that port #2 has failed the self-test.\n");
            return 0;
        }
    }

    // Everything appears to work, let's turn them on.
    ps2_sendCommand(PS2_COMMAND_ENABLE1, NULL, 0);
    ps2_sendCommand(PS2_COMMAND_ENABLE2, NULL, 0);
    ccb = ps2_sendCommand(PS2_COMMAND_GETCCB, NULL, 0);
    ccb |= (PS2_PORT1_INTERRUPT & PS2_PORT2_INTERRUPT);
    ps2_sendCommand(PS2_COMMAND_WRITECCB, ccb, 0);*/

    // TODO: We need to reset these devices.    

    ps2_kbd_init();
    serialPrintf("[module ps2] PS/2 module initialized and ready.\n");
    return 0;
}

void deinit() {
}


struct Metadata data = {
    .name = "PS/2 Driver",
    .description = "PS/2 driver for reduceOS",
    .init = init,
    .deinit = deinit
};
