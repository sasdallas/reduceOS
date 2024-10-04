// =========================================================
// ide_ata.c - reduceOS IDE + ATA driver
// =========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.
// TODO: This file should be replaced with a global drive interface that will manage VFS and such.


#include <kernel/ide_ata.h> // Main header file
#include <libk_reduced/stdio.h>

// What is IDE?
// According to the OSDev wiki, IDE is a keyword which refers to the electrical specification of the cables which connect ATA drives (like hard drives) to another device. The drives use the ATA (Advanced Technology Attachment) interface. An IDE cable also can terminate at an IDE card connected to PCI.
// ATAPI is an extension to ATA (recently renamed to PATA), adding support for the SCSI command set.
// More sources: https://wiki.osdev.org/PCI_IDE_Controller


// Variables
ideChannelRegisters_t channels[2]; // Primary and secondary channels
uint8_t ideBuffer[2048] = {0}; // Buffer to read the identification space into (see ide_ata.h).
static volatile unsigned char ideIRQ = 0; // Set to 1 when an IRQ is received.
static uint8_t atapiPacket[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // ATAPI drives
ideDevice_t ideDevices[4]; // Maximum devices supported is 4.



// Function prototypes
uint8_t ideRead(uint8_t channel, uint8_t reg); // Reads in a register
void ideWrite(uint8_t channel, uint8_t reg, uint8_t data); // Writes to a register
void ideReadBuffer(uint8_t channel, uint8_t reg, uint32_t *buffer, uint32_t quads); // Reads the identification space and copies it to a buffer.
void insl(uint16_t reg, uint32_t *buffer, int quads); // Reads a long word from a register port for quads times.
void outsl(uint16_t reg, uint32_t *buffer, int quads); // Writes a long word to a register port for quads times
uint8_t idePolling(uint8_t channel, uint32_t advancedCheck); // Returns whether there was an error.
uint8_t idePrintErrors(uint32_t drive, uint8_t err); // Prints the errors that may have occurred.

void ideWaitIRQ() {
    while (!ideIRQ);
    ideIRQ = 0;
}

void ideIRQHandler() {
    ideIRQ = 1;
}


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
            ideReadBuffer(i, ATA_REG_DATA, (uint32_t*)ideBuffer, 128);

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
    serialPrintf("ide: IDE driver completed initialization successfully.\n");
    for (int i = 0; i < 4; i++) {
        if (ideDevices[i].reserved == 1) {
            serialPrintf("ide: Found %s drive - %s\n", (ideDevices[i].type == 0) ? "ATA" : "ATAPI", ideDevices[i].model);
            // Quick maths to find out the drive capacity.
            int capacityGB = ideDevices[i].size / 1024 / 1024;
            int capacityMB = ideDevices[i].size / 1024 - (capacityGB * 1024);
            int capacityKB = ideDevices[i].size - (capacityMB * 1024);
            serialPrintf("\tCapacity: %i GB %i MB %i KB\n", capacityGB, capacityMB, capacityKB);
            drives++;
        }
    }

    isrRegisterInterruptHandler(15, ideIRQHandler);

    printf("IDE driver initialized - found %i drives.\n", drives);

}


// ideGetVFSNode(int driveNum) - Returns VFS node for an IDE drive
fsNode_t *ideGetVFSNode(int driveNum) {
    fsNode_t *ret = kmalloc(sizeof(fsNode_t));
    
    // First, make sure the drive actually exists.
    if (ideDevices[driveNum].reserved != 1) { ret->impl = -1; return ret; }
    
    

    ret->gid = ret->uid = ret->inode = ret->length = ret->mask = 0;
    ret->flags = VFS_BLOCKDEVICE;
    ret->impl = driveNum;
    ret->open = NULL;
    ret->close = NULL;
    ret->finddir = NULL;
    ret->create = NULL;
    ret->read = &ideRead_vfs;
    ret->write = &ideWrite_vfs;
    ret->readdir = NULL;
    ret->mkdir = NULL;
    strcpy(ret->name, "IDE/ATA drive");
    return ret;
}


// ide_fs_mount(const char *device, const char *mount_path) - Initializes the filesystem on a device
fsNode_t *ide_fs_mount(const char *device, const char *mount_path) {
    serialPrintf("ide_fs_mount: Trying to mount drive %s on %s...\n", device, mount_path);

    // Convert it back to a number
    int i = (int)strtol(device, (char**)NULL, 10);

    // Now, we try to get the VFS node for the device.
    fsNode_t *vfsNode = ideGetVFSNode(i);

    // Check if it's actually a valid node
    if ((int)vfsNode->impl == -1) {
        kfree(vfsNode);
        return NULL;
    }

    return vfsNode;
}

// ide_install(int argc, char *argv[]) - Installs the IDE driver to initialize on any compatible drives
int ide_install(int argc, char *argv[]) {
    vfs_registerFilesystem("ide", ide_fs_mount);
    return 0;
}


// ideRead_vfs(struct fsNode *node, off_t off, uint32_t size, uint8_t *buffer) - Read function for the VFS
uint32_t ideRead_vfs(struct fsNode *node, off_t off, uint32_t size, uint8_t *buffer) {
    // Create a temporary buffer that rounds up size to the nearest 512 multiple.
    uint8_t *temporary_buffer = kmalloc((size + 512) - ((size + 512) % 512)); 

    // Calculate the LBA, based off of offset rounded down (TODO: Make this conversion a longer value)
    int lba = (off - (off % 512)) / 512;

    // Read in the sectors
    int ret = ideReadSectors(((fsNode_t*)node)->impl, ((size + 512) - ((size + 512) % 512)) / 512, lba, (uint32_t)temporary_buffer);
    if (ret != IDE_OK) return ret;

    // We've now read in about one sector greater than our size, for offset purposes
    // We can now copy the contents to our buffer.
    int offset = off - (lba * 512); // This is the offset we need to memcpy() our buffer to

    memcpy(buffer, temporary_buffer + offset, size);

    kfree(temporary_buffer);
    return IDE_OK;
}

// ideWrite_vfs(struct fsNode *node, off_t off, uint32_t size, uint8_t *buffer) - Write function for the VFS
uint32_t ideWrite_vfs(struct fsNode *node, off_t off, uint32_t size, uint8_t *buffer) {
    // First, create a padded buffer that rounds up size to the nearest 512 multiple.
    // This buffer will serve as the actual things to write to the sector.
    // We're first going to read in the sector at LBA, then replace the contents at offset to be the input buffer.
    uint8_t padded_buffer[((size + 512) - ((size + 512) % 512))]; 

    // Calculate the LBA, based off of offset rounded down
    int lba = (off - (off % 512)) / 512;

    // Read in the sector for our offset
    int ret = ideReadSectors(((fsNode_t*)node)->impl, 1, lba, (uint32_t)padded_buffer);
    if (ret != IDE_OK) return ret;
 
    // Copy the contents of buffer to padded_buffer with an offset
    memcpy(padded_buffer + (off-(lba*512)), buffer, size);

    // Write the sectors
    ret = ideWriteSectors(((fsNode_t*)node)->impl, ((size + 512) - ((size + 512) % 512)) / 512, lba, (uint32_t)padded_buffer);
    if (ret != IDE_OK) return ret;

    return IDE_OK;
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
        outportb(channels[channel].controlBase + reg - 0x0C, data);
    } else if (reg < 0x16) {
        outportb(channels[channel].busMasterIDE + reg - 0x0E, data);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    }
}



// ideReadBuffer(uint8_t channel, uint8_t reg, uint32_t *buffer, uint32_t quads) - Reads the identification space and copies it to a buffer.
void ideReadBuffer(uint8_t channel, uint8_t reg, uint32_t *buffer, uint32_t quads) {
    // Like most of the previous functions, if reg is greater than 0x07 but less than 0x0C, call ideWrite()
    if (reg > 0x07 && reg < 0x0C) {
        ideWrite(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    }

    //asm ("pushw %es; movw %ds, %ax; movw %ax, %es");

    asm("pushw %es; pushw %ax; movw %ds, %ax; movw %ax, %es; popw %ax;");

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
    while (ideRead(channel, ATA_REG_STATUS) & ATA_STATUS_BSY);
    
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


// ideAccessATA(uint8_t direction, uint8_t drive, uint64_t lba, uint8_t sectorNum, uint32_t edi) - Read/write sectors to an ATA drive (if direction is 0 we read, else write)
uint8_t ideAccessATA(uint8_t direction, uint8_t drive, uint64_t lba, uint8_t sectorNum, uint32_t edi) {
    /* A bit of explanation about the parameters:
    Drive is the drive number (can be 0-3)
    lba is the LBA address which allows us to access disks (up to 2TB supported)
    sectorNum is the number of sectors to be read 
    edi is the offset in that segment (data buffer memory address)*/


    // First, we define a few variables
    uint8_t lbaMode, dma, cmd; // lbaMode: 0: CHS, 1: LBA28, 2: LBA48. dma: 0: No DMA, 1: DMA
    uint8_t lbaIO[6];
    uint32_t channel = ideDevices[drive].channel; // Read the channel
    uint32_t slaveBit = ideDevices[drive].drive; // Read the drive (master or slave)
    uint32_t bus = channels[channel].ioBase; // Bus base (the data port)
    uint32_t words = 256; // Almost every ATA drive has a sector size of 512 bytes
    uint16_t cylinder;
    uint8_t head, sect, err;

    // Disable IRQs to prevent problems.
    ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ideIRQ = 0x0) + 0x02);

    // Now, let's read the paramters

    // Select one from LBA28, LBA48, or CHS
    if (lba >= 0x10000000) {
        // Drive supports LBA48. Use that.
        
        serialPrintf("WARNING: USAGE OF LBA48 DETECTED\n");
        lbaMode = 2;
        lbaIO[0] = (lba & 0x000000FF) >> 0;
        lbaIO[1] = (lba & 0x0000FF00) >> 8;
        lbaIO[2] = (lba & 0x00FF0000) >> 16;
        lbaIO[3] = (lba & 0xFF000000) >> 24;
        lbaIO[4] = 0; // LBA28 is integer, so 32 bits are enough to access 2TB
        lbaIO[5] = 0;
        head = 0; // (lower 4 bits of ATA_REG_HDDEVSEL)
    } else if (ideDevices[drive].features & 0x200) {
        // Drive supports LBA28.

        lbaMode = 1;
        lbaIO[0] = (lba & 0x00000FF) >> 0;
        lbaIO[1] = (lba & 0x000FF00) >> 8;
        lbaIO[2] = (lba & 0x0FF0000) >> 16;
        lbaIO[3] = 0; // These registers are not used here.
        lbaIO[4] = 0;
        lbaIO[5] = 0;
        head = (lba & 0xF000000) >> 24;
    } else {
        // Drive uses CHS.
        lbaMode = 0;
        sect = (lba % 63) + 1;
        cylinder = (lba + 1 - sect) / (16 * 63);
        lbaIO[0] = sect;
        lbaIO[1] = (cylinder >> 0) & 0xFF;
        lbaIO[2] = (cylinder >> 8) & 0xFF;
        lbaIO[3] = 0;
        lbaIO[4] = 0;
        lbaIO[5] = 0;
        head = (lba + 1 - sect) % (16 * 63) / (63);
    }

    dma = 0; // Don't use DMA.


    // Now we wait if the drive is busy.
    while (ideRead(channel, ATA_REG_STATUS) & ATA_STATUS_BSY) {

    }  

    // Select the drive from the controller.
    if (lbaMode == 0) ideWrite(channel, ATA_REG_HDDEVSEL, 0xA0 | (slaveBit << 4) | head); // Drive & CHS.
    else ideWrite(channel, ATA_REG_HDDEVSEL, 0xE0 | (slaveBit << 4) | head);


    // Next, write the parameters to the registers.
    if (lbaMode == 2) {
        // Make sure to write a few extra parameters if we use LBA48.
        // ideWrite makes it pretty simple if we want to write to the LBA0 and LBA3 registers.
        ideWrite(channel, ATA_REG_SECCOUNT1, 0);
        ideWrite(channel, ATA_REG_LBA3, lbaIO[3]);
        ideWrite(channel, ATA_REG_LBA4, lbaIO[4]);
        ideWrite(channel, ATA_REG_LBA5, lbaIO[5]);
    }

    ideWrite(channel, ATA_REG_SECCOUNT0, sectorNum);
    ideWrite(channel, ATA_REG_LBA0, lbaIO[0]);
    ideWrite(channel, ATA_REG_LBA1, lbaIO[1]);
    ideWrite(channel, ATA_REG_LBA2, lbaIO[2]);


    // Now, select the command and send it.
    // According to the ATA/ATAPI-8 specification, these are the available commands (and the routines that are followed)
    // If DMA & LBA48, send DMA_EXT
    // If DMA & LBA28, send DMA_LBA
    // If DMA & LBA28, send DMA_CHS
    // If not DMA & LBA48, send PIO_EXT.
    // If not DMA & LBA28, send PIO_LBA
    // If not DMA & LBA28, send PIO_CHS

    if (lbaMode == 0 && dma == 0 && direction == 0) cmd = ATA_READ_PIO;
    if (lbaMode == 1 && dma == 0 && direction == 0) cmd = ATA_READ_PIO;
    if (lbaMode == 2 && dma == 0 && direction == 0) cmd = ATA_READ_PIO_EXT;
    if (lbaMode == 0 && dma == 1 && direction == 0) cmd = ATA_READ_DMA;
    if (lbaMode == 1 && dma == 1 && direction == 0) cmd = ATA_READ_DMA;
    if (lbaMode == 2 && dma == 1 && direction == 0) cmd = ATA_READ_DMA_EXT;
    if (lbaMode == 0 && dma == 0 && direction == 1) cmd = ATA_WRITE_PIO;
    if (lbaMode == 1 && dma == 0 && direction == 1) cmd = ATA_WRITE_PIO;
    if (lbaMode == 2 && dma == 0 && direction == 1) cmd = ATA_WRITE_PIO_EXT;
    if (lbaMode == 0 && dma == 1 && direction == 1) cmd = ATA_WRITE_DMA;
    if (lbaMode == 1 && dma == 1 && direction == 1) cmd = ATA_WRITE_DMA;
    if (lbaMode == 2 && dma == 1 && direction == 1) cmd = ATA_WRITE_DMA_EXT;

    ideWrite(channel, ATA_REG_COMMAND, cmd); // Send the command.

    
    // Now that we have sent the command, we should poll and read/write a sector until all sectors are read/written.
    if (dma) {
        // TODO: Implement DMA reading + writing.
        
    } else {
        if (direction == 0) {
            // PIO read.
            for (int i = 0; i < sectorNum; i++) {
                err = idePolling(channel, 1);
                if (err) {
                    serialPrintf("ideAccessATA (direction read): IDE polling returned non-zero value %i\n", err);
                    return err; // Return if an error occurred. 
                }
                asm ("pushw %es");
                asm ("rep insw" :: "c"(words), "d"(bus), "D"(edi)); // Receive data.
                asm ("popw %es");
                edi += (words * 2);
            }
        } else {
            // PIO write
            for (int i = 0; i < sectorNum; i++) {
                idePolling(channel, 0); // Poll the channel.
                asm ("pushw %ds");
                asm ("rep outsw" :: "c"(words), "d"(bus), "S"(edi)); // Send data.
                asm ("popw %ds");
                edi += (words*2);
            }
            ideWrite(channel, ATA_REG_COMMAND, (char []) { ATA_CACHE_FLUSH, ATA_CACHE_FLUSH, ATA_CACHE_FLUSH_EXT }[lbaMode]);
            idePolling(channel, 0);
        }
    }

    return 0; // Done!
}

// ideReadATAPI(uint8_t drive, uint32_t lba, uint8_t sectorNum, uint32_t edi) - Read from an ATAPI drive.
uint8_t ideReadATAPI(uint8_t drive, uint32_t lba, uint8_t sectorNum, uint32_t edi) {
    uint32_t channel = ideDevices[drive].channel;
    uint32_t slaveBit = ideDevices[drive].drive;
    uint32_t bus = channels[channel].ioBase;
    uint32_t words = 1024; // Sector size (ATAPI drives have a sector size of 2048 bytes)
    uint8_t err;
    int i;

    // Enable IRQs.
    ideWrite(channel, ATA_REG_CONTROL, channels[channel].nIEN = ideIRQ = 0x0);

    // Now, setup the SCSI packet.
    atapiPacket[0] = ATAPI_READ;
    atapiPacket[1] = 0x0;
    atapiPacket[2] = (lba >> 24) & 0xFF;
    atapiPacket[3] = (lba >> 16) & 0xFF;
    atapiPacket[4] = (lba >> 8) & 0xFF;
    atapiPacket[5] = (lba >> 0) & 0xFF;
    atapiPacket[6] = 0x0;
    atapiPacket[7] = 0x0;
    atapiPacket[8] = 0x0;
    atapiPacket[9] = sectorNum;
    atapiPacket[10] = 0x0;
    atapiPacket[11] = 0x0;

    // Now, select the drive.
    ideWrite(channel, ATA_REG_HDDEVSEL, slaveBit);

    // Delay 400ns for select to coplete.
    for (i = 0; i < 4; i++) {
        ideRead(channel, ATA_REG_ALTSTATUS);
    }   

    // Inform controller we use PIO mode.
    ideWrite(channel, ATA_REG_FEATURES, 0);

    // Notify controller size of buffer.
    ideWrite(channel, ATA_REG_LBA1, (words*2) & 0xFF); // Lower byte of sector size.
    ideWrite(channel, ATA_REG_LBA1, (words*2) >> 8); // Upper byte of sector size.

    // First, send the packet command (not the actual packet)
    ideWrite(channel, ATA_REG_COMMAND, ATA_PACKET);

    // Wait for the drive to finish or return an error.
    if ((err = idePolling(channel, 1))) return err; // Error.

    // Send the packet data.
    asm ("rep outsw" :: "c"(6), "d"(bus), "S"(atapiPacket));

    // Receive the data.
    for (i = 0; i < sectorNum; i++) {
        ideWaitIRQ(); // Wait for an IRQ.
        if ((err = idePolling(channel, 1))) return err; // There was an error.
        asm ("pushw %es");
        asm ("rep insw" :: "c"(words), "d"(bus), "D"(edi)); // Receive the data.
        asm ("popw %es");
        edi += (words * 2);
    }

    // Wait for an IRQ.
    ideWaitIRQ();
    // Wait for BSY and DRQ to clear.
    while (ideRead(channel, ATA_REG_STATUS) & (ATA_STATUS_BSY | ATA_STATUS_DRQ));

    return 0;
}


// Note that the above functions (specifically ATA/ATAPI reading and writing) are not supposed to be used outside of ide_ata.c, but are there just in case.
// THESE are the functions supposed to be used outside of ATA:

uint8_t package[8]; // package[0] contains err code

// ideReadSectors(uint8_t drive, uint8_t sectorNum, uint64_t lba, uint32_t edi) - Read from an ATA/ATAPI drive.
int ideReadSectors(uint8_t drive, uint8_t sectorNum, uint64_t lba, uint32_t edi) {

    // Check if the drive is present.
    if (drive > 3 || ideDevices[drive].reserved == 0) {
        package[0] = 0x1;
        serialPrintf("ideReadSectors: drive not found - cannot continue.\n");
        return IDE_DRIVE_NOT_FOUND;
    }
    
    // Check if the inputs are valid.
    else if (((lba + sectorNum) > ideDevices[drive].size) && (ideDevices[drive].type == IDE_ATA)) { 
        package[0] = 0x2; // Seeking to invalid position.
        serialPrintf("ideReadSectors: LBA address invalid - greater than available sectors.\n");
        return IDE_LBA_INVALID;
    }
    // Read in PIO mode through polling and IRQs.
    else {
        uint8_t error;
        if (ideDevices[drive].type == IDE_ATA) {
            error = ideAccessATA(ATA_READ, drive, lba, sectorNum, edi);
            return error;
        } else if (ideDevices[drive].type == IDE_ATAPI) {
            for (int i = 0; i < sectorNum; i++) {
                error = ideReadATAPI(drive, lba + i, 1, edi + (i*2048));
            }
            package[0] = idePrintErrors(drive, error);
            
            return package[0];
        }
    }

    return 0;
}



// ideWriteSectors(uint8_t drive, uint8_t sectorNum, uint32_t lba, uint32_t edi) - Write to an ATA drive.
int ideWriteSectors(uint8_t drive, uint8_t sectorNum, uint32_t lba, uint32_t edi) {
    // Like the above funtion, we follow similar steps.
    // Check if the drive is present.
    if (drive > 3 || ideDevices[drive].reserved == 0) {
        package[0] = 0x1;
        serialPrintf("ideWriteSectors: drive not found - cannot continue.\n");
        return IDE_DRIVE_NOT_FOUND;
    }

    // Check if the inputs are valid.
    else if (((lba + sectorNum) > ideDevices[drive].size) && (ideDevices[drive].type == IDE_ATA)) { 
        package[0] = 0x2; // Seeking to invalid position.
        serialPrintf("ideWriteSectors: LBA address invalid - greater than available sectors.\n");
        return IDE_LBA_INVALID;
    }
    // Now, write in PIO mode through polling and IRQs
    else {
        uint8_t error;
        if (ideDevices[drive].type == IDE_ATA) {
            error = ideAccessATA(ATA_WRITE, drive, lba, sectorNum, edi);
        } else if (ideDevices[drive].type == IDE_ATAPI) error = 4; // Drive is write protected.
        package[0] = idePrintErrors(drive, error);

        return package[0];
    }
}

// Some getter functions..

// ideGetDriveCapacity(uint8_t drive) - Returns drive capacity
int ideGetDriveCapacity(uint8_t drive) {
    // Sanity checks
    if (drive > 3 || ideDevices[drive].reserved == 0) {
        return -1;
    }
    
    return ideDevices[drive].size;
}
