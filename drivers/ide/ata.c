/**
 * @file drivers/ide/ata.c
 * @brief ATA components of the IDE driver
 * 
 * @warning please for the love of god do not read this i promise you i can code i promise
 * 
 * @see https://wiki.osdev.org/ATA_PIO_Mode for PIO mode information
 * @see https://hddguru.com/documentation/2006.01.27-ATA-ATAPI-7/ for ATA standard documentation
 * @see https://wiki.osdev.org/ATA/ATAPI_using_DMA for DMA information
 * @see https://wiki.osdev.org/ATA_Command_Matrix for a full ATA command matrix
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include "ata.h"
#include <kernel/drivers/x86/pci.h>
#include <kernel/mem/alloc.h>
#include <kernel/misc/spinlock.h>
#include <string.h>

// Architecture-specific
#if defined(__ARCH_I386__)
#include <kernel/arch/i386/hal.h>
#include <kernel/arch/i386/registers.h>
#elif defined(__ARCH_X86_64__)
#include <kernel/arch/x86_64/hal.h>
#include <kernel/arch/x86_64/registers.h>
#endif

/* PCI IDE controller */
uint32_t ide_pci = 0xFFFFFFFF; // Constructed via PCI_ADDR

/* PIO only */
int pio_only = 0;

/* IDE channels */
ide_channel_t channels[2] = {{ .io_base = ATA_PRIMARY_BASE, .control = ATA_PRIMARY_CONTROL }, { .io_base = ATA_SECONDARY_BASE, .control = ATA_SECONDARY_CONTROL }};

/* IDE devices */
ide_device_t devices[4] = { { .exists = 0, .channel = ATA_PRIMARY, .slave = 0 }, 
                            { .exists = 0, .channel = ATA_PRIMARY, .slave = 1 }, 
                            { .exists = 0, .channel = ATA_SECONDARY, .slave = 0 }, 
                            { .exists = 0, .channel = ATA_SECONDARY, .slave = 1 }};

/* I/O wait macro */
#define ATA_IO_WAIT(device) for (int i = 0; i < 4; i++) ide_read(device, ATA_REG_ALTSTATUS)

/* Reorder bytes macro - see ata_device_init */
#define ATA_REORDER_BYTES(buffer, size)     for (int i = 0; i < size-1; i+=2) { uint8_t tmp = ((uint8_t*)buffer)[i+1]; ((uint8_t*)buffer)[i+1] = ((uint8_t*)buffer)[i]; ((uint8_t*)buffer)[i] = tmp; }

/* Spinlock */
static spinlock_t *ata_lock = NULL;

/* Log (device-specific) method - the extra spaces are to make everything look neat */
#define LOG_DEVICE(status, device, ...)     LOG(status, "[DRIVE %s:%s%s%s] ", (device->channel == ATA_PRIMARY) ? "PRIMARY" : "SECONDARY", (device->slave) ? "SLAVE" : "MASTER", (device->channel == ATA_PRIMARY) ? "  " : "", (device->slave) ? " " : ""); \
                                            dprintf(NOHEADER, __VA_ARGS__)

/* WARNING: This needs to be moved into kernel. */
int drive_index = 0;
int cd_index = 0;


/**
 * @brief Find method for the ATA PCI controller
 * 
 * @warning This goes based off of subclass/class ID 
 */
int ata_find(uint8_t bus, uint8_t slot, uint8_t function, uint16_t vendor_id, uint16_t device_id, void *data) {
    if (ide_pci != 0xFFFFFFFF) {
        LOG(WARN, "Additional IDE controller detected: 0x%x 0x%x at bus %i slot %i function %i\n", vendor_id, device_id, bus, slot, function);
        LOG(WARN, "This IDE driver does not support multiple controllers.\n");
        return 0;
    }

    LOG(DEBUG, "IDE controller - vendor 0x%x device 0x%x\n", vendor_id, device_id);

    ide_pci = PCI_ADDR(bus, slot, function, 0); // Bus/slot/function can be extracted using other macros

    return 0; // Temporary while I work some kinks, see ata_initialize
}


/**
 * @brief IDE IRQ handler
 */
int ide_irqHandler(uintptr_t exception_index, uintptr_t interrupt_no, registers_t *regs, extended_registers_t *extended) {
    return 0;
}


/**
 * @brief Write to an IDE register
 * @param channel The device to write to.
 * @param reg The register to write
 * @param data The data to write
 */
static void ide_write(ide_device_t *device, uint8_t reg, uint8_t data) {
    if (reg > 0x07 && reg < 0x0C) {
        // Set HOB to read back LBA48
        ide_write(device, ATA_REG_CONTROL, 0x80 | channels[device->channel].nIEN);
    }

    if (reg < 0x08) {
        outportb(channels[device->channel].io_base + reg - 0x00, data);
    } else if (reg < 0x0C) {
        outportb(channels[device->channel].io_base + reg - 0x06, data);
    } else if (reg < 0x0E) {
        outportb(channels[device->channel].control + reg - 0x0A, data);
    } else if (reg < 0x16) {
        outportb(channels[device->channel].bmide + reg - 0x0E, data);
    }

    if (reg > 0x07 && reg < 0x0C) {
        // Unset HOB
        ide_write(device, ATA_REG_CONTROL, channels[device->channel].nIEN);
    }
}

/**
 * @brief Read from an IDE register
 * @param device The device to read
 * @param reg The register to read
 */
static uint8_t ide_read(ide_device_t *device, uint8_t reg) {
    if (reg > 0x07 && reg < 0x0C) {
        // Set HOB to read back LBA48
        ide_write(device, ATA_REG_CONTROL, 0x80 | channels[device->channel].nIEN);
    }

    uint8_t return_value;
    if (reg < 0x08) {
        return_value = inportb(channels[device->channel].io_base + reg - 0x00);
    } else if (reg < 0x0C) {
        return_value = inportb(channels[device->channel].io_base + reg - 0x06);
    } else if (reg < 0x0E) {
        return_value = inportb(channels[device->channel].control + reg - 0x0A);
    } else if (reg < 0x16) {
        return_value = inportb(channels[device->channel].bmide + reg - 0x0E);
    }

    if (reg > 0x07 && reg < 0x0C) {
        // Unset HOB
        ide_write(device, ATA_REG_CONTROL, channels[device->channel].nIEN);
    }

    return return_value;
}

/**
 * @brief Wait for BSY to clear, and optionally perform an "advanced check" (which checks ERR bits)
 * 
 * @param device The device to wait for
 * @param advanced Whether to perform an advanced check (check ERR bits)
 * @param timeout The timeout, can be -1 to be ignored
 * 
 * @returns @c IDE_SUCCESS on success
 */
int ide_wait(ide_device_t *device, int advanced, int timeout) {
    // Allow BSY to be set
    ATA_IO_WAIT(device);

    // Now wait for it to be cleared
    if (timeout > 0) {
        while (ide_read(device, ATA_REG_STATUS) & ATA_SR_BSY && timeout > 0) timeout--;
    } else {
        while (ide_read(device, ATA_REG_STATUS) & ATA_SR_BSY);
    }

    if (timeout <= 0) return IDE_TIMEOUT;

    // If advanced check, see if there are any errors
    if (advanced) {
        uint8_t status = ide_read(device, ATA_REG_STATUS);

        if (status & ATA_SR_ERR) return IDE_ERROR;
        if (status & ATA_SR_DF) return IDE_DEVICE_FAULT;
        if (!(status & ATA_SR_DRQ)) return IDE_DRQ_NOT_SET;
    }

    return IDE_SUCCESS;
}

/**
 * @brief Print the error when something happens
 * @param device The device the error happened in
 * @param error The error code returned
 */
void ide_printError(ide_device_t *device, int error, char *operation) {
    if (error == IDE_SUCCESS) return;

    LOG_DEVICE(ERR, device, "Operation '%s' encountered error: ", operation);

    if (error == IDE_DEVICE_FAULT) LOG(NOHEADER, "Device Fault (IDE_DEVICE_FAULT)\n");
    if (error == IDE_DRQ_NOT_SET) LOG(NOHEADER, "DRQ bit not set (IDE_DRQ_NOT_SET)\n");
    if (error == IDE_TIMEOUT) LOG(NOHEADER, "Timeout (IDE_TIMEOUT)\n");
    if (error == IDE_ERROR) {
        uint8_t st = ide_read(device, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF) LOG(NOHEADER, "Address mark not found (IDE_ERROR)\n");
        if (st & ATA_ER_TK0NF) LOG(NOHEADER, "Track 0 not found (IDE_ERROR)\n");
        if (st & ATA_ER_ABRT) LOG(NOHEADER, "Command aborted (IDE_ERROR)\n");
        if (st & ATA_ER_MCR) LOG(NOHEADER, "Media change request (IDE_ERROR)\n");
        if (st & ATA_ER_MC) LOG(NOHEADER, "Media change (IDE_ERROR)\n");
        if (st & ATA_ER_IDNF) LOG(NOHEADER, "ID mark not found (IDE_ERROR)\n");
        if (st & ATA_ER_UNC) LOG(NOHEADER, "Uncorrectable data error (IDE_ERROR)\n");
        if (st & ATA_ER_BBK) LOG(NOHEADER, "Bad sectors (IDE_ERROR)\n");
    }
}

/**
 * @brief Select an ATA/ATAPI drive
 * @param device The device to select
 */
void ide_select(ide_device_t *device) {
    ide_write(device, ATA_REG_HDDEVSEL, 0xA0 | (device->slave) << 4); // 0xA0 is for bits 5 and 7 which will always be set
    ATA_IO_WAIT(device);
}

/**
 * @brief Soft reset a drive
 * @param device The device to soft reset
 */
void ide_softReset(ide_device_t *device) {
    ide_write(device, ATA_REG_CONTROL, 0x04 | channels[device->channel].nIEN);
    ATA_IO_WAIT(device);
    ide_write(device, ATA_REG_CONTROL, 0x00 | channels[device->channel].nIEN);
}

/**
 * @brief Perform an ATA access
 * 
 * For ATA devices only.
 * @todo DMA in this function would work
 * 
 * @param device The device to perform the access of
 * @param operation @c ATA_READ or @c ATA_WRITE
 * @param lba The LBA to read
 * @param sectors The amount of sectors to read
 * @param buffer The output buffer
 * @returns Error code
 */
int ata_access(ide_device_t *device, int operation, uint64_t lba, size_t sectors, uint8_t *buffer) {
    if (!buffer || !device || operation > ATA_WRITE) return IDE_ERROR;

    if (!pio_only) {
        LOG(ERR, "ata_access has DMA unimplemented\n");
        return IDE_ERROR;
    }

    // Disable IRQs
    channels[device->channel].nIEN = 2;
    ide_write(device, ATA_REG_CONTROL, channels[device->channel].nIEN);


    if (!(device->ident.capabilities & 0x200)) {
        // Device does not support LBA, CHS is not implemented
        LOG_DEVICE(ERR, device, "Drive does not support LBA but CHS addressing is not implemented!\n");
        return IDE_ERROR;
    }

    // Decide what type of LBA to use
    int lba48 = 0;
    uint8_t lba_data[6] = { 0 };
    uint8_t sel = 0; // Bits 24-27 of the block number for LBA28

    if (lba >= 0x10000000) {
        // LBA48 addressing
        if (!(device->ident.command_sets & (1 << 26))) {
            // Device does not support LBA48
            LOG_DEVICE(ERR, device, "Attempted to read LBA 0x%llX but drive does not support 48-bit LBA\n", lba);
            return IDE_ERROR;
        }

        lba48 = 1;
        lba_data[3] = (lba & 0x00000000FF000000) >> 24;
        lba_data[4] = (lba & 0x000000FF00000000) >> 32;
        lba_data[5] = (lba & 0x0000FF0000000000) >> 40;
    }

    // Setup the remaining values (or the only ones for LBA28)
    lba_data[0] = (lba & 0x00000000000000FF) >> 0;
    lba_data[1] = (lba & 0x000000000000FF00) >> 8;
    lba_data[2] = (lba & 0x0000000000FF0000) >> 16;
    if (!lba48) sel = (lba & 0x0F000000) >> 24;

    // Poll if busy
    ide_wait(device, 0, -1); // todo: timeout?

    // Select the drive using HDDEVSEL, setting the bit for LBA
    ide_write(device, ATA_REG_HDDEVSEL, 0xE0 | (device->slave << 4) | sel);
    ATA_IO_WAIT(device);

    // Write LBA parameters
    if (lba_data[3]) {
        ide_write(device, ATA_REG_SECCOUNT0, (sectors & 0xFF00) >> 8);
        ide_write(device, ATA_REG_LBA3, lba_data[3]);
        ide_write(device, ATA_REG_LBA4, lba_data[4]);
        ide_write(device, ATA_REG_LBA5, lba_data[5]);
    }
    
    ide_write(device, ATA_REG_SECCOUNT0, sectors & 0xFF);
    ide_write(device, ATA_REG_LBA0, lba_data[0]);
    ide_write(device, ATA_REG_LBA1, lba_data[1]);
    ide_write(device, ATA_REG_LBA2, lba_data[2]);

    // Now decide on the command to use
    // TODO: DMA
    uint8_t cmd = 0x0;
    if (operation == ATA_READ && lba48 == 0) cmd = ATA_CMD_READ_PIO;
    if (operation == ATA_READ && lba48 == 1) cmd = ATA_CMD_READ_PIO_EXT;
    if (operation == ATA_WRITE && lba48 == 0) cmd = ATA_CMD_WRITE_PIO;
    if (operation == ATA_WRITE && lba48 == 1) cmd = ATA_CMD_WRITE_PIO_EXT;

    // Before we do this, poll
    ide_wait(device, 0, -1);

    // Acquire a lock
    spinlock_acquire(ata_lock);

    // Send the command
    ide_write(device, ATA_REG_COMMAND, cmd);
    ATA_IO_WAIT(device);

    // Depending on the operation, handle appropriately
    uint16_t *bufptr = (uint16_t*)buffer; 
    for (size_t sector = 0; sector < sectors; sector++) {
        // Poll first
        int error = ide_wait(device, 1, 1000);
        if (error) {
            spinlock_release(ata_lock);
            ide_printError(device, error, (operation == ATA_READ) ? "ata read" : "ata write");
            return IDE_ERROR;
        }

        // For each word...
        for (int word = 0; word < 256; word++) {
            if (operation == ATA_READ) {
                // If we are reading, copy to the buffer.
                // TODO: rep insw works for this, it's just a little bit ugly
                *bufptr = inportw(channels[device->channel].io_base);
                bufptr++;
            } else {
                // If we are writing, copy from the buffer.
                outportw(channels[device->channel].io_base, *bufptr);
                bufptr++;
            }
        }
    }

    // Now that we're done, send a CACHE_FLUSH command if we were writing
    if (operation == ATA_WRITE) {
        ide_write(device, ATA_REG_COMMAND, (lba48) ? ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH);
        ide_wait(device, 0, -1);
    }

    spinlock_release(ata_lock);
    return IDE_SUCCESS;

}


/**
 * @brief VFS read method for IDE device
 */
ssize_t ide_readFS(fs_node_t *node, off_t offset, size_t size, uint8_t *buffer) {
    return 0;
}

/**
 * @brief VFS write method for IDE device
 */
ssize_t ide_writeFS(fs_node_t *node, off_t offset, size_t size, uint8_t *buffer) {
    return 0;
}


/**
 * @brief Create an IDE node
 * @param device The device to create off of
 */
fs_node_t *ide_createNode(ide_device_t *device) {
    fs_node_t *out = kmalloc(sizeof(fs_node_t));
    memset(out, 0, sizeof(fs_node_t));

    if (device->atapi) {
        snprintf(out->name, 256, "hd%i", drive_index);
    } else {
        snprintf(out->name, 256, "cdrom%i", cd_index);
    }

    out->read = ide_readFS;
    out->write = ide_writeFS;
    out->flags = VFS_BLOCKDEVICE;
    out->mask = 0770;
    out->length = device->size;
    out->dev = (void*)device;

    return out;
}

/**
 * @brief Handle initializing an ATA device
 */
void ata_device_init(ide_device_t *device) {
    // As the IDENTIFY command was already sent, we can just read the buffer in now.
    uint16_t *buffer = (uint16_t*)&device->ident;

    // Read it into ID space
    for (int i = 0; i < 256; i++) {
        buffer[i] = inportw(channels[device->channel].io_base + ATA_REG_DATA);
    }

    // Now we have some cleanup work to do because of the way ATA/ATAPI transfers work
    // Bytes are ordered in words for PIO mode, and for each word the bytes are encoded where byte 1 comes first, then byte 0.
    // So "QEMU HARDDISK " would appear as "EQUMH RADDSI K"
    // For more information, please see the ATA standard section 3.2.9 
    ATA_REORDER_BYTES(&device->ident.model, 40);
    ATA_REORDER_BYTES(&device->ident.serial, 20);
    ATA_REORDER_BYTES(&device->ident.firmware, 8);

    // Now copy this information into the common IDE device structure
    strncpy(device->model, device->ident.model, 40);
    device->model[40] = 0; // Null terminate (we can't pad this as it can have spaces)

    strncpy(device->serial, device->ident.serial, 20);
    device->serial[20] = 0; // Null terminate
    *(strchr(device->serial, ' ')) = 0;

    strncpy(device->firmware, device->ident.firmware, 8);
    device->firmware[8] = 0; // Null terminate
    *(strchr(device->firmware, ' ')) = 0;

    // Print out summary
    LOG_DEVICE(INFO, device, "Model %s - serial %s firmware %s\n", device->model, device->serial, device->firmware);
    
    // Check what type of addressing the device uses
    if ((device->ident.command_sets & (1 << 26))) {
        // LBA48 addressing
        LOG_DEVICE(DEBUG, device, "LBA48-style addressing\n");
        device->size = (device->ident.sectors_lba48 & 0x0000FFFFFFFFFFFF) * 512;
    } else {
        // CHS or LBA28 addressing
        LOG_DEVICE(DEBUG, device, "LBA28/CHS-style addressing detected\n");
        device->size = device->ident.sectors * 512;
    }

    LOG_DEVICE(DEBUG, device, "Capacity: %d MB\n", (device->size) / 1024 / 1024);
}

/**
 * @brief Handle initializing an ATAPI device
 */
void atapi_device_init(ide_device_t *device) {
    // ATAPI 
    device->atapi = 1;
    
    // Send the identify packet command
    ide_write(device, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);

    // Now we can read it in.
    uint16_t *buffer = (uint16_t*)&device->ident;

    // Read it into ID space
    for (int i = 0; i < 256; i++) {
        buffer[i] = inportw(channels[device->channel].io_base + ATA_REG_DATA);
    }

    // Now we have some cleanup work to do because of the way ATA/ATAPI transfers work
    // Bytes are ordered in words for PIO mode, and for each word the bytes are encoded where byte 1 comes first, then byte 0.
    // So "QEMU HARDDISK " would appear as "EQUMH RADDSI K"
    // For more information, please see the ATA standard section 3.2.9 
    ATA_REORDER_BYTES(&device->ident.model, 40);
    ATA_REORDER_BYTES(&device->ident.serial, 20);
    ATA_REORDER_BYTES(&device->ident.firmware, 8);

    // Now copy this information into the common IDE device structure
    strncpy(device->model, device->ident.model, 40);
    device->model[40] = 0; // Null terminate (we can't pad this as it can have spaces)

    strncpy(device->serial, device->ident.serial, 20);
    device->serial[20] = 0; // Null terminate
    *(strchr(device->serial, ' ')) = 0;

    strncpy(device->firmware, device->ident.firmware, 8);
    device->firmware[8] = 0; // Null terminate
    *(strchr(device->firmware, ' ')) = 0;

    // Print out summary
    LOG_DEVICE(INFO, device, "Model %s - serial %s firmware %s\n", device->model, device->serial, device->firmware);

    // Now we have to detect the medium's capacity.
    // ATAPI uses the SCSI Read Capacity command, which means the capacity can be calculated via:
    // (Last LBA + 1) * Block Size
    atapi_packet_t read_capacity = { 0 };
    read_capacity.bytes[0] = ATAPI_READ_CAPACITY;

    // Set the number of bytes to return
    ide_write(device, ATA_REG_LBA1, 0x08);
    ide_write(device, ATA_REG_LBA2, 0x08);

    // Send the packet command and poll
    ide_write(device, ATA_REG_COMMAND, ATA_CMD_PACKET);

    int err = ide_wait(device, 1, 100); // TODO: test this, we might need to wait for DRDY to set
    if (err != IDE_SUCCESS) {
        ide_printError(device, err, "atapi read capacity");
        device->exists = 0;
        return;
    }

    // Now send packet bytes
    for (int i = 0; i < 6; i++) {
        outportw(channels[device->channel].io_base, read_capacity.words[i]);
    }

    // Poll again
    err = ide_wait(device, 1, 100); 
    if (err != IDE_SUCCESS) {
        ide_printError(device, err, "atapi read capacity");
        device->exists = 0;
        return;
    }

    // Read response
    uint16_t capacity[4];
    for (int i = 0; i < 4; i++) capacity[i] = inportw(channels[device->channel].io_base);

    // htonl source: https://github.com/klange/toaruos/blob/master/modules/ata.c#L686
    #define htonl(l)  ( (((l) & 0xFF) << 24) | (((l) & 0xFF00) << 8) | (((l) & 0xFF0000) >> 8) | (((l) & 0xFF000000) >> 24))
	
    // Convert LBA and block size
    uint32_t lba, block_size;
	memcpy(&lba, &capacity[0], sizeof(uint32_t));
	lba = htonl(lba);
	memcpy(&block_size, &capacity[2], sizeof(uint32_t));
	block_size = htonl(block_size);

    // Calculate capacity and store it
    device->size = (lba + 1) * block_size;

    LOG_DEVICE(INFO, device, "Capacity: %d MB\n", device->size / 1024 / 1024);
}

/**
 * @brief Detect ATA/ATAPI devices
 */
void ide_detectDevice(ide_device_t *device) {
    // First, soft reset the device
    ide_softReset(device);

    // Now select it and wait for BSY to clear
    ide_select(device);
    ide_wait(device, 0, -1);
    
    // We now to send an ATA identify command. 
    ide_write(device, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ATA_IO_WAIT(device);

    int err = 0; // debugging
    int timeout = 0;
    while (1 && timeout < 10000) {
        // ATAPI devices are supposed to set ERR rather than BSY, and then DRQ
        // This is also needed to get CYL_LO and CYL_HI to be reads

        // NOTE: Some ATAPI devices do not set ERR (for some reason). Therefore we do the port check anyways to determine whether the device is ATAPI 
        uint8_t status = ide_read(device, ATA_REG_STATUS);
        if (status & ATA_SR_ERR) {
            // ERR was set
            err = 1;
            break; 
        } else if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
            // DRQ was set and BSY is cleared, likely an ATA device instead
            break;
        }

        timeout++; // Keep waiting...
    }

    // Did it timeout?
    if (timeout >= 10000) {
        LOG_DEVICE(INFO, device, "Timeout while waiting for ATA_CMD_IDENTIFY - assuming dead\n");
        return;
    }

    // Now we can read CYL_LO and CYL_HI (ATA_REG_LBA1 and ATA_REG_LBA2) to determine the type.
    uint8_t cl = ide_read(device, ATA_REG_LBA1);
    uint8_t ch = ide_read(device, ATA_REG_LBA2);

    if ((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96)) {
        // ATAPI device
        LOG_DEVICE(DEBUG, device, "Detected an ATAPI device\n");
        if (!err) {
            LOG_DEVICE(DEBUG, device, "Potentially defective ATA device - ERR was not set during IDENTIFY command\n"); // this is just for funsies, to show the user that their drive is weird
        }

        // Drive does exist
        device->exists = 1;

        // Now initialize it
        atapi_device_init(device);

        if (!device->exists) return; // Init failed

        // Create a VFS node for it
        fs_node_t *node = ide_createNode(device);

        // Mount the node
        char devname[64];
        snprintf(devname, 64, "/device/%s", node->name);
        vfs_mount(node, devname);
        cd_index++;
    } else if ((cl == 0x00 && ch == 0x00) || (cl == 0x3C && ch == 0xC3)) {
        // ATA device
        LOG_DEVICE(DEBUG, device, "Detected an ATA device\n");

        // Drive does exist
        device->exists = 1;

        // Now initialize it
        ata_device_init(device);

        if (!device->exists) return; // Init failed

        // Create a VFS node for it
        fs_node_t *node = ide_createNode(device);

        // Mount the node
        char devname[64];
        snprintf(devname, 64, "/device/%s", node->name);
        vfs_mount(node, devname);
        drive_index++;
    } else if ((cl == 0xFF && ch == 0xFF)) {
        LOG_DEVICE(DEBUG, device, "No device was detected\n");
    } else {
        LOG_DEVICE(WARN, device, "Unimplemented device (cl: 0x%x, ch: 0x%x)\n", cl, ch);
    }
} 

/**
 * @brief Initialize the ATA/ATAPI driver logic
 */
int ata_initialize() {
    // First, scan for the ATA controller
    pci_scan(ata_find, NULL, ATA_PCI_TYPE);     // Ignore result for now, just in case there are multiple controllers

    if (ide_pci == 0xFFFFFFFF) {
        LOG(DEBUG, "No IDE controller detected\n");
        return 0; // No IDE controller
    }

    LOG(DEBUG, "ATA controller located\n");
    
    // Initialize the spinlock
    ata_lock = spinlock_create("ata_lock");

    // Let's determine how to program the controller
    uint8_t progif = pci_readConfigOffset(PCI_BUS(ide_pci), PCI_SLOT(ide_pci), PCI_FUNCTION(ide_pci), PCI_PROGIF_OFFSET, 1);
    if (progif == 0xFF) {
        LOG(WARN, "Error attempting to determine ATA controller programming.\n");
        return 0;
    } else {
        LOG(DEBUG, "Primary channel mode: %s\n", (progif & (1 << 0)) ? "PCI native mode" : "Compatibility mode");
        LOG(DEBUG, "Can change primary mode: %s\n", (progif & (1 << 1)) ? "YES" : "NO");
        LOG(DEBUG, "Secondary channel mode: %s\n", (progif & (1 << 2)) ? "PCI native mode" : "Compatibility mode");
        LOG(DEBUG, "Can change secondary mode: %s\n", (progif & (1 << 3)) ? "YES" : "NO");
        LOG(DEBUG, "DMA supported: %s\n", (progif & (1 << 7)) ? "YES" : "NO");
    }

    // Make sure primary and secondary channels are operating in compatibility
    if ((progif & (1 << 0)) || (progif & (1 << 2))) {
        LOG(WARN, "Both channels need to be operating in compatibility mode (switching not implemented).\n");
        return 0;
    } 

    // DMA not supported? Use PIO mode
    if (!(progif & (1 << 7))) pio_only = 1;
    
    // DMA is also not implemented
    pio_only = 1;

    // Read BAR4 and set it in bmide of each channel
    pci_bar_t *bar4 = pci_readBAR(PCI_BUS(ide_pci), PCI_SLOT(ide_pci), PCI_FUNCTION(ide_pci), 4);
    if (bar4) {
        channels[ATA_PRIMARY].bmide = bar4->address + 0;
        channels[ATA_SECONDARY].bmide = bar4->address + 8;
        kfree(bar4);
    }

    // Register IRQ handlers
    hal_registerInterruptHandler(14, ide_irqHandler);
    hal_registerInterruptHandler(15, ide_irqHandler);

    // Detect devices
    for (int i = 0; i < 4; i++) {
        ide_detectDevice(&devices[i]);
    }
    
    return 0;
}