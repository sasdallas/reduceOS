/**
 * @file drivers/storage/ahci/ahci_port.c
 * @brief AHCI port
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include "ahci.h"
#include <kernel/drivers/clock.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/mem.h>
#include <kernel/debug.h>
#include <string.h>

/* Log method */
#define LOG(status, ...) dprintf_module(status, "DRIVER:AHCI", __VA_ARGS__)

/* Log port */
#define LOG_PORT(status, port, ...) LOG(status, "[PORT%d] ", port->port_num); \
                                    dprintf(NOHEADER, __VA_ARGS__)

/**
 * @brief Disable a port
 * @param port The port to disable
 * @returns 0 on success
 */
static int ahci_portDisable(ahci_port_t *port) {
    // Is port already disabled?
    if (!(port->port->cmd & HBA_PORT_PXCMD_ST)) {
        LOG_PORT(WARN, port, "Tried to disable already disabled port\n");
        return 1;
    }

    // Disable port and wait for CR to clear
    port->port->cmd &= ~(HBA_PORT_PXCMD_ST);

    // Wait for CR to clear
    int timeout = TIMEOUT(!(port->port->cmd & HBA_PORT_PXCMD_CR), 500000);
    if (timeout) {
        // DMA engine didn't stop correcty
        LOG_PORT(ERR, port, "Stopping DMA engine timed out.\n");
        return 1;
    }

    // Success
    return 0;
}

/**
 * @brief Enable a port
 * @param port The port to enable
 * @returns 0 on success
 */
static int ahci_portEnable(ahci_port_t *port) {
    if (port->port->cmd & HBA_PORT_PXCMD_ST) {
        // Port already running?
        LOG_PORT(DEBUG, port, "Port already running, cannot start\n");
        return AHCI_ERROR;
    }

    if (!(port->port->cmd & HBA_PORT_PXCMD_FRE)) {
        // To set ST we need FRE to be set
        LOG_PORT(ERR, port, "Tried to enable port but FRE is not set\n");
        return AHCI_ERROR;
    }

    // Wait until CR is clear
    int cr_clear = TIMEOUT(!(port->port->cmd & HBA_PORT_PXCMD_CR), 100000);
    if (cr_clear) {
        LOG_PORT(ERR, port, "Failed to stop DMA engine\n");
        return AHCI_ERROR;
    }

    port->port->cmd |= HBA_PORT_PXCMD_ST;
    return AHCI_SUCCESS;
}

/**
 * @brief Fill the PRDT of a command header
 * @param port The port to fill the PRDT of
 * @param data The data to fill the PRDT with
 * @param size The size of the data to fill the PRDT with
 * @returns Amount of PRDs filled
 */
static int ahci_portFillPRDT(ahci_port_t *port, ahci_cmd_header_t *header, uintptr_t *data, size_t size) {
    if (!data || !size) return AHCI_ERROR;

    // Get the command table
    ahci_cmd_table_t *table = port->cmd_table;
    
    uintptr_t *buffer = data;
    size_t remaining = size;
    int prds_filled = 0;
    for (int entry = 0; entry < header->prdtl-1; entry++) {
        if (remaining <= 0) break;
        AHCI_SET_ADDRESS(table->prdt_entry[entry].dba, data);
        
        // Calculate amount of bytes
        size_t bytes = remaining;
        if (bytes > AHCI_PRD_MAX_BYTES) bytes = AHCI_PRD_MAX_BYTES;

        // Set the length of the data
        table->prdt_entry[entry].dbc = bytes; 

        // Update
        remaining -= bytes;
        buffer += bytes;
        prds_filled++;
    }

    if (remaining) {
        LOG_PORT(ERR, port, "Failed to fill PRDT - too many bytes (%d bytes left to fill)\n");
        return 0;
    }


    // All filled
    return prds_filled;
}

/**
 * @brief Wait for a transfer to complete
 * @param port The port to wait on
 * @param timeout The timeout to use
 * @returns AHCI_SUCCESS on success, anything else is a failure
 */
static int ahci_portWaitTransfer(ahci_port_t *port, int timeout) {
    while (1) {
        if (!(port->port->ci & (1))) {
            return AHCI_SUCCESS;
        }

        timeout--;
        if (timeout <= 0) break;

        // TODO: ERROR HANDLING!!!!!!
    }
    
    return AHCI_TIMEOUT;   
}

/**
 * @brief Read the identification space of a port
 * @param port The port to read the identification space of
 * @param ident Pointer to identification space (output)
 * @returns AHCI_SUCCESS on success
 */
static int ahci_readIdentificationSpace(ahci_port_t *port, ata_ident_t *ident) {
    // Get the command header
    ahci_cmd_header_t *header = (ahci_cmd_header_t*)port->cmd_list;

    // Setup header
    header->cfl = sizeof(ahci_fis_d2h_t) / sizeof(uint32_t);
    header->prdtl = ahci_portFillPRDT(port, header, (uintptr_t*)ident, sizeof(ata_ident_t));
    // NOTE: header->a is only set when cmd_table->acmd is being set, and since ATA_CMD_IDENTIFY_PACKET is technically an ATA command, don't do that

    // Create the FIS
    ahci_fis_h2d_t *fis = (ahci_fis_h2d_t*)(&(port->cmd_table->cfis));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->command = (port->type == AHCI_DEVICE_SATAPI) ? ATA_CMD_IDENTIFY_PACKET : ATA_CMD_IDENTIFY;
    fis->c = 1; // Specify that this FIS is a command

    // Wait for device to not be busy
    int timeout = TIMEOUT(!(port->port->tfd & (ATA_SR_BSY | ATA_SR_DRQ)), 1000000);
    if (timeout) {
        LOG_PORT(ERR, port, "Timeout waiting for existing command to process (BSY/DRQ set)\n");
        return AHCI_ERROR;
    }

    // Send the command!
    port->port->ci = 1;

    // Wait for the transfer to complete
    int transfer = ahci_portWaitTransfer(port, 10000000);

    if (transfer != AHCI_SUCCESS) {
        LOG_PORT(ERR, port, "Failed to read drive identification space\n");
        return AHCI_ERROR;
    }

    // Transfer succeeded
    return AHCI_SUCCESS;
}


/**
 * @brief Initialize a port
 * @param ahci AHCI controller
 * @param port_number The number of the port to initialize
 * @returns @c ahci_port_t structure or NULL
 */
ahci_port_t *ahci_portInitialize(ahci_t *ahci, int port_number) {
    // Allocate port structure
    ahci_port_t *port = kmalloc(sizeof(ahci_port_t));
    port->port_num = port_number;
    port->parent = ahci;
    port->port = &ahci->mem->ports[port_number];

    // Calculate how much memory we need to allocate in total
    size_t memory_amount = sizeof(ahci_received_fis_t) + sizeof(ahci_cmd_table_t);
    memory_amount += (sizeof(ahci_cmd_header_t) * AHCI_CMD_HEADER_COUNT);
    memory_amount += (sizeof(ahci_prdt_entry_t) * AHCI_PRDT_COUNT);

    // Now get that memory from the kernel heap
    uintptr_t port_buffer = mem_allocate(0x0, memory_amount, MEM_ALLOC_CONTIGUOUS | MEM_ALLOC_HEAP, MEM_PAGE_KERNEL | MEM_PAGE_NOT_CACHEABLE);

    memset((void*)port_buffer, 0, memory_amount);

    // Start by allocating the command list
    port->cmd_list = (ahci_cmd_header_t*)port_buffer;
    port_buffer += sizeof(ahci_cmd_header_t) * AHCI_CMD_HEADER_COUNT; // TODO: Checks for alignment

    // Allocate the FIS receive area
    port->fis = (ahci_received_fis_t*)port_buffer;
    port_buffer += sizeof(ahci_received_fis_t);

    // Allocate the command table
    port->cmd_table = (ahci_cmd_table_t*)port_buffer;
    port_buffer += sizeof(ahci_cmd_table_t) + (sizeof(ahci_prdt_entry_t) * AHCI_PRDT_COUNT);

    // Debug
    LOG_PORT(DEBUG, port, "CMDLIST = %p FIS = %p CMDTABLE = %p\n", port->cmd_list, port->fis, port->cmd_table);

    // Now point AHCI registers to our structures
    AHCI_SET_ADDRESS(port->port->clb, port->cmd_list)
    AHCI_SET_ADDRESS(port->port->fb, port->fis);

    // Populate our command list
    for (int i = 0; i < AHCI_CMD_HEADER_COUNT; i++) {
        AHCI_SET_ADDRESS(port->cmd_list[i].ctba, port->cmd_table);
        port->cmd_list[i].prdtl = AHCI_PRDT_COUNT;
    }

    // Then, initialize the port.
    // Clear IRQ status bits and error register
    port->port->is = port->port->is;
    port->port->serr = port->port->serr;

    // Power up device
    port->port->cmd |= HBA_PORT_PXCMD_POD;
    
    // Spin up
    port->port->cmd |= HBA_PORT_PXCMD_SUD;
    
    // ICC active
    port->port->cmd = (port->port->cmd & ~HBA_PORT_PXCMD_ICC) | (1 << 28); // Set 28th bit = ICC active

    // AHCI spec thinks it best that we set FRE now that FB/FBU points to our Receive FIS
    port->port->cmd |= HBA_PORT_PXCMD_FRE;

    // Can't start the port yet, need to enable interrupts and do some more stuff
    // But first the HBA needs to enable its interrupts after probing.
    return port;
}


/**
 * @brief Finish port initialization
 * @param port The port to finish initializing
 * @returns AHCI_SUCCESS on device found, AHCI_NO_DEVICE on device not found, AHCI_ERROR on error
 */
int ahci_portFinishInitialization(ahci_port_t *port) {
    LOG_PORT(INFO, port, "Finishing port initialization\n");

    if (ahci_portEnable(port) != AHCI_SUCCESS) {
        LOG_PORT(ERR, port, "Failed to enable port.\n");
        return AHCI_ERROR;
    }

    // Enable port interrupts
    port->port->ie = 0xFFFFFFFF;

    // Ok, now stop the port again.
    if (ahci_portDisable(port) != AHCI_SUCCESS) {
        LOG_PORT(ERR, port, "Failed to disable port\n");
        return AHCI_ERROR;
    }

    // Wait for BSY and DRQ to clear.
    int timeout = TIMEOUT(!(port->port->tfd & (ATA_SR_BSY | ATA_SR_DRQ)), 1000000);
    if (timeout) {
        // AHCI specification says that if this fails to try a COMRESET
        LOG_PORT(INFO, port, "Timeout detected, performing COMRESET\n");

        // Invoke COMRESET (clears other bits)
        port->port->sctl = (HBA_PORT_SCTL_DET_DISABLE | HBA_PORT_SCTL_IPM_PARTIAL | HBA_PORT_SCTL_IPM_SLUMBER);

        // Wait 20ms (spec says 1ms)
        clock_sleep(20);

        // Clear DET
        port->port->sctl = (port->port->sctl & ~HBA_PORT_PXSCTL_DET) | HBA_PORT_SCTL_DET_NONE;
    }

    // Enable
    if (ahci_portEnable(port)) {
        LOG_PORT(ERR, port, "Failed to enable port\n");
        return AHCI_ERROR;
    }

    // Wait for DET to be 3 (HBA_PORT_SSTS_DET_PRESENT)
    timeout = TIMEOUT((port->port->ssts & HBA_PORT_SSTS_DET_PRESENT), 5000);
    if (timeout) {
        LOG_PORT(INFO, port, "No device present on port\n");
        port->type = AHCI_DEVICE_NONE;
        return AHCI_SUCCESS;
    }

    // Now we need to get the type of the device
    switch (port->port->sig) {
        case SATA_SIG_ATA:
            LOG_PORT(DEBUG, port, "Detected a SATA device on port\n");
            port->type = AHCI_DEVICE_SATA;
            break;
        
        case SATA_SIG_ATAPI:
            LOG_PORT(DEBUG, port, "Detected a SATAPI device on port\n");
            port->type = AHCI_DEVICE_SATAPI;
            break;
        
        case SATA_SIG_SEMB:
            LOG_PORT(DEBUG, port, "Detected an enclosure management bridge on port\n");
            port->type = AHCI_DEVICE_SEMB;
            break;
        
        case SATA_SIG_PM:
            LOG_PORT(DEBUG, port, "Detected a port multiplier on port\n");
            port->type = AHCI_DEVICE_PM;
            break;
    }

    // If the device is ATAPI, we should also set the command bit
    if (port->type == AHCI_DEVICE_SATAPI) {
        port->port->cmd |= HBA_PORT_PXCMD_ATAPI;
    } else {
        port->port->cmd &= ~(HBA_PORT_PXCMD_ATAPI);
    }

    // Now we should read the drive information, e.g. sector size and count
    // Allocate identification space
    ata_ident_t *ident = kmalloc(sizeof(ata_ident_t));
    memset(ident, 0, sizeof(ata_ident_t));

    // Read identification space in
    if (ahci_readIdentificationSpace(port, ident) != AHCI_SUCCESS) {
        LOG_PORT(ERR, port, "Failed to read identification space.\n");
        return AHCI_ERROR;
    }

    // Reorder bytes
    ATA_REORDER_BYTES(ident->model, 40);
    ATA_REORDER_BYTES(ident->serial, 20);
    ATA_REORDER_BYTES(ident->firmware, 8);

    // Null-terminate
    ident->model[39] = 0;
    ident->serial[19] = 0;
    ident->firmware[7] = 0;
    LOG_PORT(DEBUG, port, "Model %s - serial %s firmware %s\n", ident->model, ident->serial, ident->firmware);

    // TODO: Mount port as VFS node, main kernel needs a drive "driver" (to assign mountpoints to different drives)

    // All done! Port should be initialized
    return AHCI_SUCCESS;
}

/**
 * @brief Execute a request for a specific port
 * @param port The port to execute the request for
 * @param operation The operation to do
 * @param lba The LBA
 * @param sectors The amount of sectors to operate on
 * @param buffer The buffer to use
 * @returns Error code
 */
int ahci_portOperate(ahci_port_t *port, int operation, uint64_t lba, size_t sectors, uint8_t *buffer) {
    if (!port || !sectors) return AHCI_ERROR;

    // TODO

    return AHCI_SUCCESS;
}


/**
 * @brief Handle a port IRQ
 * @param port The port
 */
void ahci_portIRQ(ahci_port_t *port) {
    // Clear port interrupts
    LOG_PORT(DEBUG, port, "Port IRQ\n");
    uint32_t is = port->port->is;
    port->port->is = is;
}