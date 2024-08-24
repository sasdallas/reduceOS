// =========================================================
// floppy.c - Floppy Disk Controller driver
// =========================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/floppy.h> // Main header file
#include <kernel/cmos.h>

// Variables
static int floppyIRQ_fired = 0;
static int DMA_BUFFER = 0x1000;
uint8_t currentFloppyDrive = 0; // Should work
static bool enableFDC = true; // Set to false if version sanity check fails.
static bool primaryFloppy = false;
static bool secondaryFloppy = false;

// floppy_LBAtoCHS(uint32_t lba, int *cylinder, int *head, int *sector)
void floppy_LBAtoCHS(uint32_t lba, int *cylinder, int *head, int *sector) {
    *cylinder = lba / (FLOPPY_HEADS * FLOPPY_144MB_SECTORS_PER_TRACK);
    *head = ((lba % (FLOPPY_HEADS * FLOPPY_144MB_SECTORS_PER_TRACK)) / FLOPPY_144MB_SECTORS_PER_TRACK);
    *sector = lba % FLOPPY_144MB_SECTORS_PER_TRACK + 1;
}

/* IRQ ROUTINES */

// Floppy is kind of slow, so we need a way to wait for the floppy IRQ to fire

// GCC complains that floppy_readDataRegister() and floppy_sendCommand() are "implicitly declared" (stupid proper coding practices, let me make spaghetti)
// So we'll just do an ugly hack:
static uint8_t floppy_readDataRegister();
static void floppy_sendCommand(uint8_t cmd);
static uint8_t floppy_getMSR();

// floppy_IRQ() - Called when floppy IRQ is present
void floppy_IRQ() {
    floppyIRQ_fired = 1;
}

// floppy_waitIRQ() - Waits for an IRQ, then resets the value
void floppy_waitIRQ() {
    while (floppyIRQ_fired == 0);
    floppyIRQ_fired = 0;
}

// floppy_acknowledgeIRQ() - Acknowledge to the FDC we received an interrupt
void floppy_acknowledgeIRQ(uint32_t *st0, uint32_t *cyl) {
    floppy_sendCommand(FLOPPY_CMD_SENSEINT);

    *st0 = floppy_readDataRegister();
    *cyl = floppy_readDataRegister();

}

/* MSR ROUTINES */

// (static) floppy_getMSR() - Returns the main status register
static uint8_t floppy_getMSR() {
    uint8_t msr =  inportb(FLOPPY_MAINSTATUS);
    return msr;
}



/* COMMAND ROUTINES */

// (static) floppy_sendCommand(uint8_t cmd) - Sends a command to the floppy
static void floppy_sendCommand(uint8_t cmd) {
    // Wait until the data register is ready, then send a command.
    for (int i = 0; i < 1000; i++) {
        if (floppy_getMSR() & FLOPPY_MSR_RQM) outportb(FLOPPY_DATA_FIFO, cmd); return;
    }

    serialPrintf("floppy_sendCommand: Command timeout!\n");
}

// (static) floppy_readDataRegister() - Reads the data register. Usually done after a cmd
static uint8_t floppy_readDataRegister() {
    for (int i = 0; i < 1000; i++) {
        if (floppy_getMSR() & FLOPPY_MSR_DIO) return inportb(FLOPPY_DATA_FIFO);
    }

    serialPrintf("floppy_readDataRegister: Reading data register timeout!\n");

    return NULL;
}



/* ACTUAL DRIVE ROUTINES */

// floppy_startMotor(uint8_t drive) - Start the motor on a drive
void floppy_startMotor(uint8_t drive) {
    switch (drive) {
        case 0:
            outportb(FLOPPY_DIGITALOUTPUT, FLOPPY_DIGITALOUTPUT_MOTOR0 | FLOPPY_DIGITALOUTPUT_RESET | FLOPPY_DIGITALOUTPUT_IRQ);
            break;
        case 1:
            outportb(FLOPPY_DIGITALOUTPUT, FLOPPY_DIGITALOUTPUT_MOTOR1 | FLOPPY_DIGITALOUTPUT_RESET | FLOPPY_DIGITALOUTPUT_IRQ);
            break;
        case 2:
            outportb(FLOPPY_DIGITALOUTPUT, FLOPPY_DIGITALOUTPUT_MOTOR2 | FLOPPY_DIGITALOUTPUT_RESET | FLOPPY_DIGITALOUTPUT_IRQ);
            break;
        case 3:
            outportb(FLOPPY_DIGITALOUTPUT, FLOPPY_DIGITALOUTPUT_MOTOR3 | FLOPPY_DIGITALOUTPUT_RESET | FLOPPY_DIGITALOUTPUT_IRQ);
            break;
        default:
            serialPrintf("floppy_startMotor: Attempted to start drive %i motor when drive does not exist", drive);
            return;
    }
    
    sleep(100); // Wait for motor to spin up
}

// floppy_stopMotor() - Stop the floppy motor
void floppy_stopMotor() {
    outportb(FLOPPY_DIGITALOUTPUT, FLOPPY_DIGITALOUTPUT_RESET); // Reset drive motor
    sleep(100); // Wait for motor to stop
}


// floppy_dmaInit(uint8_t *buffer, size_t length) - Initialize the floppy drive for DMA
void floppy_dmaInit(uint8_t *buffer, size_t length) {
    dma_resetDMA(1); // Reset the master DMA.
    dma_maskChannel(FLOPPY_DMA_CHANNEL); // Mask DMA channel 2
    //outportb(0x0A, 0x06);

    dma_resetFlipFlop(1); // Reset the master flip flop

    // Okay, so apparently here's BrokenThorn's way of doing things:
    union {
        uint8_t byte[4]; // Holds all the bytes for the address
        uint64_t length; // Buffer length ????
    }a, c;

    // This code makes no sense to me, especially the part where byte is never set.
    a.length = (unsigned)buffer;
    c.length = (unsigned)length-1;

    if ((a.length >> 24) || (c.length >> 16) || (((a.length & 0xFFFF) + c.length) >> 16)) {
        serialPrintf("floppy_dmaInit: Can you please not?\n");
        return;
    }

    dma_setStartAddress(FLOPPY_DMA_CHANNEL, a.byte[0], a.byte[1]);
    dma_resetFlipFlop(1); // Reset the master flip flop

    dma_setCount(FLOPPY_DMA_CHANNEL, c.byte[0], c.byte[1]); // Setup the count
    dma_setRead(FLOPPY_DMA_CHANNEL); // Prepare channel for read

    dma_resetMask(1); // Unmask all channels
}





// floppy_readSectorInternal(uint8_t head, uint8_t track, uint8_t sector) - Read a sector (internal)
int floppy_readSectorInternal(uint8_t head, uint8_t track, uint8_t sector) {
    if (!enableFDC) return FLOPPY_ERROR;

    // First, setup the DMA for write transfer
    dma_setRead(FLOPPY_DMA_CHANNEL);

    // Now, we need to read in a sector.
    floppy_sendCommand(FLOPPY_CMD_READDATA | FLOPPY_CMD_EXT_MULTITRACK | FLOPPY_CMD_EXT_SKIP | FLOPPY_CMD_EXT_DENSITY);
    
    // Send the parameters
    floppy_sendCommand(head << 2 | currentFloppyDrive);
    floppy_sendCommand(track);
    floppy_sendCommand(head);
    floppy_sendCommand(sector);
    floppy_sendCommand(FLOPPY_BPS_512);
    floppy_sendCommand((sector + 1) >= FLOPPY_144MB_SECTORS_PER_TRACK ? FLOPPY_144MB_SECTORS_PER_TRACK : sector + 1);
    floppy_sendCommand(FLOPPY_GAP3_3_5);
    floppy_sendCommand(0xFF);

    floppy_waitIRQ();

    // Read in the status (DEFINITELY should sanity check those last values.. but.. too lazy.)
    uint8_t st0 = floppy_readDataRegister();
    uint8_t st1 = floppy_readDataRegister();
    uint8_t st2 = floppy_readDataRegister();
    
    for (int i = 0; i < 4; i++) floppy_readDataRegister();



    // I hate my life
    int ret = FLOPPY_OK;
    if (st0 & 0xC0) {
        char *status[] = {0, "error", "invalid command", "drive not ready"};
        serialPrintf("floppy_readSectorInternal:  status = %s\n", status[st0 >> 6]);
        
        switch (st0 >> 6) {
            case 1:
                ret = FLOPPY_ERROR;
                break;
            case 2:
                ret = FLOPPY_INVALID_CMD;
                break;
            case 3:
                ret = FLOPPY_DRIVE_NOT_READY;
                break;
        }
    }


    if (st1 & 0x80) {
        serialPrintf("floppy_readSectorInternal: End of cylinder.\n");
    }

    if (st0 & 0x08) {
        serialPrintf("floppy_readSectorInternal: Drive not ready\n");
    }

    if (st1 & 0x20) {
        serialPrintf("floppy_readSectorInternal: CRC error\n");
    }

    if (st1 & 0x10) {
        serialPrintf("floppy_readSectorInternal: Controller timeout\n");
    }

    if (st1 & 0x04) {
        serialPrintf("floppy_readSectorInternal: No data found\n");
    }

    if ((st1 | st2) & 0x01) {
        serialPrintf("floppy_readSectorInternal: No address mark found\n");
    }

    if (st2 & 0x40)  {
        serialPrintf("floppy_readSectorInternal: Deleted address mark\n");
    }

    if (st2 & 0x20) {
        serialPrintf("floppy_readSectorInternal: CRC error in data\n");
    }

    if (st2 & 0x04) {
        serialPrintf("floppy_readSectorInternal: uPD765 sector not found\n");
    }

    if (st1 & 0x02) {
        serialPrintf("floppy_readSectorInternal: Your code is bugged. We got a NOT_WRITABLE when reading. What are you smoking?\n");
    }


    uint32_t st0_again, cylinder;
    // Let the FDC know we handled the interrupt
    floppy_acknowledgeIRQ(&st0_again, &cylinder);

    return ret;
}

// floppy_writeSectorInternal(uint8_t head, uint8_t track, uint8_t sector) - Writes whatever is in the DMA_BUFFER to LBA
int floppy_writeSectorInternal(uint8_t head, uint8_t track, uint8_t sector) {
    if (!enableFDC) return FLOPPY_ERROR;

    // First things first, setup channel 2 for write mode.
    dma_setWrite(FLOPPY_DMA_CHANNEL);

    // Now, we send the write command.
    floppy_sendCommand(FLOPPY_CMD_WRITEDATA | 0xC0);
    
    // Send the parameters
    floppy_sendCommand(head << 2 | currentFloppyDrive);
    floppy_sendCommand(track);
    floppy_sendCommand(head);
    floppy_sendCommand(sector);
    floppy_sendCommand(FLOPPY_BPS_512);
    floppy_sendCommand((sector + 1) >= FLOPPY_144MB_SECTORS_PER_TRACK ? FLOPPY_144MB_SECTORS_PER_TRACK : sector + 1);
    floppy_sendCommand(FLOPPY_GAP3_3_5);
    floppy_sendCommand(0xFF);

    floppy_waitIRQ();
    // Read in the status (DEFINITELY should sanity check those last values.. but.. too lazy.)
    uint8_t st0 = floppy_readDataRegister();
    uint8_t st1 = floppy_readDataRegister();
    uint8_t st2 = floppy_readDataRegister();
    serialPrintf("floppy_writeSectorInternal: st0 = 0x%x\nfloppy_writeSectorInternal: st1 = 0x%x\nfloppy_writeSectorInternal: st2 = 0x%x\n", st0, st1, st2);

    for (int i = 0; i < 4; i++) floppy_readDataRegister();


    // I hate my life
    int ret = FLOPPY_OK;
    if (st0 & 0xC0) {
        char *status[] = {0, "error", "invalid command", "drive not ready"};
        serialPrintf("floppy_writeSectorInternal:  status = %s\n", status[st0 >> 6]);
        
        switch (st0 >> 6) {
            case 1:
                ret = FLOPPY_ERROR;
                break;
            case 2:
                ret = FLOPPY_INVALID_CMD;
                break;
            case 3:
                ret = FLOPPY_DRIVE_NOT_READY;
                break;
        }
    }


    if (st1 & 0x80) {
        serialPrintf("floppy_writeSectorInternal: End of cylinder.\n");
    }

    if (st0 & 0x08) {
        serialPrintf("floppy_writeSectorInternal: Drive not ready\n");
    }

    if (st1 & 0x20) {
        serialPrintf("floppy_writeSectorInternal: CRC error\n");
    }

    if (st1 & 0x10) {
        serialPrintf("floppy_writeSectorInternal: Controller timeout\n");
    }

    if (st1 & 0x04) {
        serialPrintf("floppy_writeSectorInternal: No data found\n");
    }

    if ((st1 | st2) & 0x01) {
        serialPrintf("floppy_writeSectorInternal: No address mark found\n");
    }

    if (st2 & 0x40)  {
        serialPrintf("floppy_writeSectorInternal: Deleted address mark\n");
    }

    if (st2 & 0x20) {
        serialPrintf("floppy_writeSectorInternal: CRC error in data\n");
    }

    if (st2 & 0x04) {
        serialPrintf("floppy_writeSectorInternal: uPD765 sector not found\n");
    }

    if (st1 & 0x02) {
        serialPrintf("floppy_writeSectorInternal: This disk is not writable.\n");
        ret = FLOPPY_DRIVE_READ_ONLY;
    }



    uint32_t st0_again, cylinder;
    // Let the FDC know we handled the interrupt
    floppy_acknowledgeIRQ(&st0_again, &cylinder);

    return ret;
}





// floppy_readSector(int lba, uint8_t *buffer) - Reads a sector, using LBA.
int floppy_readSector(int lba, uint8_t *buffer) {

    // First, convert the LBA to CHS
    int head, track, sector;
    floppy_LBAtoCHS(lba, &track, &head, &sector);

    // Turn the motor on and seek to the track
    floppy_startMotor(currentFloppyDrive);
    if (floppy_seek(track, head) != 0) {
        serialPrintf("floppy_readSector: Failed to seek to sector.\n");
        return FLOPPY_SEEK_FAIL;
    }

    // Read the sector and turn off the motor
    int ret = floppy_readSectorInternal(head, track, sector);
    floppy_stopMotor();

    // Check if read sector errored out.
    if (ret != FLOPPY_OK) return ret;

    // I don't even care anymore. I hate pointers with a burning passion, and I always will.
    *buffer = DMA_BUFFER;

    return FLOPPY_OK;
}


// floppy_writeSector(int lba, uint8_t *buffer) - Write a sector in the floppy drive.
int floppy_writeSector(int lba, uint8_t *buffer) {
    if (!enableFDC) return FLOPPY_ERROR;


    // First, convert the LBA to CHS.
    int head, track, sector;
    floppy_LBAtoCHS(lba, &track, &head, &sector);


    // Turn the motor on and seek to the track.
    floppy_startMotor(currentFloppyDrive);
    if (floppy_seek(track, head) != 0) {
        serialPrintf("floppy_writeSector: Failed to write to sector.\n");
        return FLOPPY_SEEK_FAIL;
    } 

    // Copy buffer contents to DMA_BUFFER
    memcpy((uint8_t*)DMA_BUFFER, buffer, 512);

    // Write the sector and disbable the drive motor.
    int ret = floppy_writeSectorInternal(head, track, sector);
    floppy_stopMotor();

    return ret;
}

// floppy_driveData(uint32_t steprate, uint32_t loadtime, uint32_t unloadtime, bool isDMA) - Passes control info to the FDC about the drive
void floppy_driveData(uint32_t steprate, uint32_t loadtime, uint32_t unloadtime, bool isDMA) {
    // Send the SPECIFY command, because we, well, specify data.
    floppy_sendCommand(FLOPPY_CMD_SPECIFY);

    // Do some fancy bitmask magic to put the data into the following form
    // Parameter 1: X X X X Y Y Y Y (x = step rate, y = unload time)
    // Parameter 2: X X X X X X X Y (x = load time, y = DMA mode)
    uint32_t parameter1 = ((steprate & 0xF) << 4) | (unloadtime & 0xF);
    floppy_sendCommand(parameter1);

    uint32_t parameter2 = loadtime << 1 | (isDMA) ? 0 : 1; // NDMA bit has given me so much trouble...
    floppy_sendCommand(parameter2);
}

// floppy_setDrive(uint8_t drive) - Set the current drive
void floppy_setDrive(uint8_t drive) {
    if (drive > 1) {
        serialPrintf("floppy_setDrive: Attempt to set drive %i ignored (>1)\n", drive);
        return;
    }

    if (drive == 0 && primaryFloppy) currentFloppyDrive = drive;
    else if (drive == 1 && secondaryFloppy) currentFloppyDrive = drive;
    else serialPrintf("floppy_setDrive: Refused to set drive %i\n", drive);
}

// floppy_calibrateDrive(uint32_t drive) - Calibrate a floppy drive
int floppy_calibrateDrive(uint32_t drive) {
    uint32_t st0, cyl;

    // Turn on the drive's motor
    floppy_startMotor((uint8_t)(drive & 0x3));


    // Now we need to calibrate the drive
    for (int i = 0; i < 10; i++) {
        // Send the calibration commands
        floppy_sendCommand(FLOPPY_CMD_RECALIBRATE);
        floppy_sendCommand(drive);
        // Wait for IRQ 6 and acknowledge it
        floppy_waitIRQ();
        floppy_acknowledgeIRQ(&st0, &cyl);


        // If cylinder 0 found, we are done.
        if (!cyl) {
            floppy_stopMotor();
            return 0;
        }
    } 

    floppy_stopMotor();
    return -1;
}

// floppy_seek(uint32_t cylinder, uint32_t head) - Seek a floppy drive's head to a cylinder
int floppy_seek(uint32_t cylinder, uint32_t head) {
    uint32_t st0, cyl = -1;

    for (int i = 0; i < 10; i++) {
        // Send the command and the parameters 
        floppy_sendCommand(FLOPPY_CMD_SEEK);
        floppy_sendCommand(head << 2 | currentFloppyDrive);
        floppy_sendCommand(cylinder);

        // Wait for IRQ and acknowledge it
        floppy_waitIRQ();
        floppy_acknowledgeIRQ(&st0, &cyl);

        if (cyl == cylinder) return 0; // Seek successful.
    }

    return -1;
}


// floppy_enableFDC() - Enables the FDC
void floppy_enableFDC() {
    outportb(FLOPPY_DIGITALOUTPUT, FLOPPY_DIGITALOUTPUT_RESET | FLOPPY_DIGITALOUTPUT_IRQ);
}

// floppy_disableFDC() - Disables the FDC
void floppy_disableFDC() {
    outportb(FLOPPY_DIGITALOUTPUT, 0); // Disables the FDC (RIP)
}

// floppy_reset() - Resets the floppy drive
void floppy_reset() {
    uint32_t st0, cyl;

    
    // Reset controller
    floppy_disableFDC();
    floppy_enableFDC();

    floppy_waitIRQ(); // Potentially can cause IRQ6 waiting loop.

    // Send a VERSION command to the floppy drive to verify it works.
    floppy_sendCommand(FLOPPY_CMD_VERSION);
    uint8_t version = floppy_readDataRegister();
    if (version != 0x90) {
        serialPrintf("floppy_reset: Invalid version! This driver is ONLY for 82077AA-based FDCs!\n");
        enableFDC = false;
        return;
    }

    // Send the sense interrupt command to all drives
    for (int i = 0; i < 4; i++) {
        floppy_acknowledgeIRQ(&st0, &cyl);
    }

    // Set the transfer speed to 500 KB/S
    outportb(FLOPPY_CONFIGCTRL, 0);

    // Setup the drive's data (steprate = 3ms, unload time = 240ms, load time = 16ms)
    floppy_driveData(3, 16, 240, true);

    // Calibrate the drive
    floppy_calibrateDrive(currentFloppyDrive);
}

// floppy_init() - Initialize the floppy drive
void floppy_init() {
    // BEFORE ANYTHING - WE NEED TO CHECK CMOS TO SEE A FLOPPY EVEN EXISTS
    uint8_t cmosValue = cmos_readRegister(0x10);
    if ((cmosValue & 0xF0) >> 4) {
        serialPrintf("floppy_init: Found primary FDC\n");
        primaryFloppy = true;
    }

    if ((cmosValue & 0x0F)) {
        serialPrintf("floppy_init: Found secondary FDC\n");
        secondaryFloppy = true;
    }

    if (!secondaryFloppy && !primaryFloppy) {
        serialPrintf("floppy_init: Failed to initialize floppy - no controllers were found.\n");
        enableFDC = false;
        return;
    }


    // First, install the IRQ handler.
    isrRegisterInterruptHandler(FLOPPY_IRQ + 32, floppy_IRQ);

    // Setup our DMA buffer
    DMA_BUFFER = dma_allocPool(4096 * 5); // Something like 20 KB

    // We just reset it, because we can't really trust the BIOS to do anything, can we?
    // Proof: A small list of BIOSes with APM bugs - https://lxr.linux.no/#linux+v6.7.1/arch/x86/kernel/apm_32.c#L2055
    floppy_reset();

    // Setup the floppy drive for DMA
    floppy_dmaInit((uint8_t*)DMA_BUFFER, 512);

    // Set the drive information
    floppy_driveData(13, 1, 0xF, true);

}
