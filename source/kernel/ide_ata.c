// =========================================================
// ide_ata.c - reduceOS IDE + ATA driver
// =========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/ide_ata.h" // Main header file

// What is IDE?
// According to the OSDev wiki, IDE is a keyword which refers to the electrical specification of the cables which connect ATA drives (like hard drives) to another device. The drives use the ATA (Advanced Technology Attachment) interface. An IDE cable also can terminate at an IDE card connected to PCI.
// ATAPI is an extension to ATA (recently renamed to PATA), adding support for the SCSI command set.
// More sources: https://wiki.osdev.org/PCI_IDE_Controller


// Variables
ideChannelRegisters_t channels[2]; // Primary and secondary channels
uint8_t ideBuffer[2048] = {0}; // Buffer to read the identification space into (see ide_ata.h).
static volatile uint8_t ideIRQ = 0; // Set to 1 when an IRQ is received.
static uint8_t atapiPacket[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // ATAPI drives
ideDevice_t ideDevices[4]; // Maximum devices supported is 4.



// Function prototypes
uint8_t ideRead(uint8_t channel, uint8_t reg); // Reads in a register
void ideWrite(uint8_t channel, uint8_t reg, uint8_t data); // Writes to a register
void ideReadBuffer(uint8_t channel, uint8_t reg, uint32_t buffer, uint32_t quads); // Reads the identification space and copies it to a buffer.
void insl(uint16_t reg, uint32_t *buffer, int quads); // Reads a long word from a register port for quads times.
void outsl(uint16_t reg, uint32_t *buffer, int quads); // Writes a long word to a register port for quads times
uint8_t idePolling(uint8_t channel, uint32_t advancedCheck); // Returns whether there was an error.
uint8_t idePrintErrors(uint32_t drive, uint8_t err); // Prints the errors that may have occurred.

// ideInit(uint32_t bar0, uint32_t bar1, uint32_t bar2, uint32_t bar3, uint32_t bar4) - Initializes the IDE controller/driver.
void ideInit(uint32_t bar0, uint32_t bar1, uint32_t bar2, uint32_t bar3, uint32_t bar4) {
    // Parameters:
    // bar0 is the start of the IO ports used by the primary ATA channel
    // bar1 is the start of the IO ports that control the primary ATA channel
    // bar2 is the start of the IO ports used by the secondary ATA channel
    // bar3 is the start of the IO ports that control the secondary ATA channel
    // bar4 is the start of 8 IO ports that control the primary channel's bus master IDE.
    // bar4 + 8 is the start of the 8 IO ports that control the secondary channel's bus master IDE.

    int count = 0;

    // First, detect IO ports which interface the IDE controller.
    channels[ATA_PRIMARY].ioBase = (bar0 & 0xFFFFFFFC) + 0x1F0 * (!bar0); // 0x1F0 is the default IO base
    channels[ATA_PRIMARY].controlBase = (bar1 & 0xFFFFFFFC) + 0x3F6 * (!bar1); // 0x3F6 is the default control base
    channels[ATA_SECONDARY].ioBase = (bar2 & 0xFFFFFFFC) + 0x170 * (!bar2);
    channels[ATA_SECONDARY].controlBase = (bar3 & 0xFFFFFFFC) + 0x376 * (!bar3);
    channels[ATA_PRIMARY].busMasterIDE = (bar4 & 0xFFFFFFFC) + 0; // Bus master IDE
    channels[ATA_SECONDARY].busMasterIDE = (bar4 & 0xFFFFFFFC) + 8;

    // Next, disable IRQs in both channels (via setting bit 1 in the control port)
    ideWrite(ATA_PRIMARY, ATA_REG_CONTROL, 2);
    ideWrite(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    // Now, detect ATA/ATAPI drives.
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            uint8_t err = 0, type = IDE_ATA, status;
            ideDevices[count].reserved = 0; // Assuming there is no drive.

            // Step 1 - select the drive.
            ideWrite(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select the drive
            sleep(1); // Wait 1 ms for drive select to wokr.

            // Next, send the ATA identify command
            ideWrite(i, ATA_REG_COMMAND, ATA_IDENTIFY);
            sleep(1); // Wait 1 ms for identify to work.

            // Now, poll.
            if (ideRead(i, ATA_REG_STATUS) == 0) continue; // If status is 0, no device.
            
            while (1) {
                status = ideRead(i, ATA_REG_STATUS);
                if (status & ATA_STATUS_ERR) { err = 1; break; } // If err, device is not ATA.
                if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRQ)) break; // Everything is right.
            }

            // Next, probe for ATAPI devices
            if (err != 0) {
                uint8_t cl = ideRead(i, ATA_REG_LBA1);
                uint8_t ch = ideRead(i, ATA_REG_LBA2);

                if (cl == 0x14 && ch == 0xEB) {
                    type = IDE_ATAPI;
                } else if (cl == 0x69 && ch == 0x96) {
                    type = IDE_ATAPI;
                } else {
                    continue; // Unknown type.
                }

                ideWrite(i, ATA_REG_COMMAND, ATA_IDENTIFY_PACKET);
                sleep(1);
            }

            // Read the identification space of the device.
            ideReadBuffer(i, ATA_REG_DATA, (uint32_t)ideBuffer, 128);

            // Next, read the device parameters.
            ideDevices[count].reserved = 1;
            ideDevices[count].type = type;
            ideDevices[count].channel = i;
            ideDevices[count].drive = j;
            ideDevices[count].signature = *((uint16_t*)(ideBuffer + ATA_IDENT_DEVICETYPE));
            ideDevices[count].features = *((uint16_t*)(ideBuffer + ATA_IDENT_CAPABILITIES));
            ideDevices[count].commandSets = *((uint32_t *)(ideBuffer + ATA_IDENT_COMMANDSETS));

            // Now, get the device size.
            if (ideDevices[count].commandSets & (1 << 26)) {
                // Device uses 48-bit addressing.
                ideDevices[count].size = *((uint32_t*)(ideBuffer + ATA_IDENT_MAX_LBA_EXT));
            } else {
                // Device uses CHS or 28-bit addressing
                ideDevices[count].size = *((uint32_t*)(ideBuffer + ATA_IDENT_MAX_LBA));
            }

            // Finally (for this function), get the model string.
            for (int k = 0; k < 40; k += 2) {
                ideDevices[count].model[k] = ideBuffer[ATA_IDENT_MODEL + k + 1];
                ideDevices[count].model[k + 1] = ideBuffer[ATA_IDENT_MODEL + k];
            }

            ideDevices[count].model[40] = '\0'; // Zero terminate the string.

            count++;
        }
    }

    // Finally (for all of the driver), print a summary of the ATA devices
    int drives = 0;
    for (int i = 0; i < 4; i++) {
        if (ideDevices[i].reserved == 1) {
            serialPrintf("Found %s drive - %s\n", (ideDevices[i].type == 0) ? "ATA" : "ATAPI", ideDevices[i].model);
            // Quick maths to find out the drive capacity.
            int capacityGB = ideDevices[i].size / 1024 / 1024;
            int capacityMB = ideDevices[i].size / 1024 - (capacityGB * 1024);
            int capacityKB = ideDevices[i].size - (capacityMB * 1024);
            serialPrintf("\tCapacity: %i GB %i MB %i KB\n", capacityGB, capacityMB, capacityKB);
            drives++;
        }
    }
    printf("IDE driver initialized - found %i drives.\n", drives);
}

// printIDESummary() - Print a basic summary of all available IDE drives.
void printIDESummary() {
    for (int i = 0; i < 4; i++) {
        if (ideDevices[i].reserved == 1) {
            printf("Found %s drive - %s\n", (ideDevices[i].type == 0) ? "ATA" : "ATAPI", ideDevices[i].model);
            // Quick maths to find out the drive capacity.
            int capacityGB = ideDevices[i].size / 1024 / 1024;
            int capacityMB = ideDevices[i].size / 1024 - (capacityGB * 1024);
            int capacityKB = ideDevices[i].size - (capacityMB * 1024);
            printf("\tCapacity: %i GB %i MB %i KB\n", capacityGB, capacityMB, capacityKB);
        }
    }
}

// ideRead(uint8_t channel, uint8_t reg) - Reads in a register
uint8_t ideRead(uint8_t channel, uint8_t reg) {
    uint8_t returnValue;
    if (reg > 0x07 && reg < 0x0C) {
        ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    }

    if (reg < 0x08) {
        returnValue = inportb(channels[channel].ioBase + reg - 0x00);
    } else if (reg < 0x0C) {
        returnValue = inportb(channels[channel].ioBase + reg - 0x06);
    } else if (reg < 0x0E) {
        returnValue = inportb(channels[channel].controlBase + reg - 0x0A);
    } else if (reg < 0x16) {
        returnValue = inportb(channels[channel].busMasterIDE + reg - 0x0E);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    }

    return returnValue;
}

// ideWrite(uint8_t channel, uint8_t reg, uint8_t data) - Writes to an IDE register.
void ideWrite(uint8_t channel, uint8_t reg, uint8_t data) {
    if (reg > 0x07 && reg < 0x0C)
        ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    
    if (reg < 0x08) {
        outportb(channels[channel].ioBase + reg - 0x00, data);
    } else if (reg < 0x0C) {
        outportb(channels[channel].ioBase + reg - 0x06, data);
    } else if (reg < 0x0E) {
        outportb(channels[channel].controlBase + reg - 0x0A, data);
    } else if (reg < 0x16) {
        outportb(channels[channel].busMasterIDE + reg - 0x0E, data);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    }
}



// ideReadBuffer(uint8_t channel, uint8_t reg, uint32_t buffer, uint32_t quads) - Reads the identification space and copies it to a buffer.
void ideReadBuffer(uint8_t channel, uint8_t reg, uint32_t buffer, uint32_t quads) {
    // Like most of the previous functions, if reg is greater than 0x07 but less than 0x0C, call ideWrite()
    if (reg > 0x07 && reg < 0x0C) {
        ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    }

    asm ("pushw %es; movw %ds, %ax; movw %ax, %es");

    if (reg < 0x08) {
        insl(channels[channel].ioBase + reg - 0x00, buffer, quads);
    } else if (reg < 0x0C) {
        insl(channels[channel].ioBase + reg - 0x06, buffer, quads);
    } else if (reg < 0x0E) {
        insl(channels[channel].controlBase + reg - 0x0A, buffer, quads);
    } else if (reg < 0x16) {
        insl(channels[channel].busMasterIDE + reg - 0x0E, buffer, quads);
    }

    asm ("popw %es");
    if (reg > 0x07 && reg < 0x0C) {
        ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    }
}



// insl(uint16_t reg, uint32_t *buffer, int quads) - Reads a long word from a register port for quads times.
void insl(uint16_t reg, uint32_t *buffer, int quads) {
    for (int index = 0; index < quads; index++) {
        buffer[index] = inportl(reg);
    }
}

// outsl(uint16_t reg, uint32_t *buffer, int quads) -  Writes a long word to a register port for quads times
void outsl(uint16_t reg, uint32_t *buffer, int quads) {
    for (int index = 0; index < quads; index++) {
        outportl(reg, buffer[index]);
    }
}

// idePolling(uint8_t channel, uint32_t advancedCheck) - Returns whether there was an error.
uint8_t idePolling(uint8_t channel, uint32_t advancedCheck) {
    // First, delay 400 ns for BSY to be set.
    for (int i = 0; i < 4; i++) ideRead(channel, ATA_REG_ALTSTATUS);

    // Next, wait for BSY to be cleared.
    while (ideRead(channel, ATA_REG_STATUS) * ATA_STATUS_BSY);

    if (advancedCheck) {
        // The user wants us to do an advanced check.
        uint8_t state = ideRead(channel, ATA_REG_STATUS); // read the status register
        
        // Check for errors
        if (state & ATA_STATUS_ERR) return 2; // Error.

        // Next, check if device fault
        if (state & ATA_STATUS_DF) return 1; // Device fault.

        // Finally, check if data request ready.
        // BSY is 0, DF is 0, and ERR is 0, so check for DRQ now.

        if ((state & ATA_STATUS_DRQ) == 0) return 3; // DRQ should be set.
    }

    return 0; // No error.
}


// idePrintErrors(uint32_t drive, uint8_t err) - Prints the errors that may have occurred.
uint8_t idePrintErrors(uint32_t drive, uint8_t err) {
    if (err == 0) return err; // Pesky users!!
    serialPrintf("ide: encountered an error on drive 0x%x. error:", drive);
    printf("IDE encountered error");

    if (err == 1) { printf(" - device fault.\n"); err = 19; serialPrintf(" device fault.\n"); }
    else if (err == 2) {
        uint8_t st = ideRead(ideDevices[drive].channel, ATA_REG_ERROR);
        if (st & ERR_AMNF) { printf(" - no address mark found.\n"); err = 7; serialPrintf(" no address mark found.\n"); }
        if (st & ERR_TKZNF) { printf(" - no media or media error.\n"); err = 3; serialPrintf(" no media or media error (track zero or not found).\n"); }
        if (st & ERR_ABRT) { printf(" - command aborted.\n"); err = 20; serialPrintf(" command aborted.\n"); }
        if (st & ERR_MCR) { printf(" - no media or media error.\n"); err = 3; serialPrintf(" no media or media error (media change request)\n"); }
        if (st & ERR_IDNF) { printf(" - ID mark not found.\n"); err = 21; serialPrintf(" ID mark not found.\n"); }
        if (st & ERR_MC) { printf(" - no media or media error.\n"); err = 3; serialPrintf(" no media or media error (media changed)\n"); }
        if (st & ERR_UNC) { printf(" - uncorrectable data error.\n"); err = 22; serialPrintf(" uncorrectable data error.\n"); }
        if (st & ERR_BBK) { printf(" - bad sectors.\n"); err = 13; serialPrintf(" bad sectors.\n"); }
    } else if (err == 3) { printf("- reads nothing.\n"); err = 23; serialPrintf(" reads nothing.\n"); }
    else if (err == 4) { printf("- write protected drive.\n"); err = 8; serialPrintf(" write protected drive.\n"); }

    printf("Drive - [%s %s] %s\n", 
    (const char *[]){"Primary", "Secondary"}[ideDevices[drive].channel],
    (const char *[]){"Master", "Slave"}[ideDevices[drive].channel],
    ideDevices[drive].model);

    return err;
}