/**
 * @file source/kmods/usb/uhci.c
 * @brief UHCI (Universal Host Controller Interface) section of the USB driver.
 * 
 * 
 * @bug The pool system is unnecessary and draws more memory.
 * 
 * @copyright
 * This file is part of the reduceOS C kernel, which is licensed under the terms of GPL.
 * Please credit me if this code is used.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include "uhci.h"
#include <libk_reduced/stdint.h>
#include <libk_reduced/stdio.h>
#include <libk_reduced/sleep.h>
#include <kernel/serial.h>
#include <kernel/pci.h>

/**
 * @brief Write a value to a controller port
 * @param port The port to write to
 * @param data The data to write
 */
void uhci_writePort(uint32_t port, uint16_t data) {
    // First we have to read the port to make sure that we're not overwriting anything
    uint16_t cur_status = inportw(port);
    cur_status |= data;
    cur_status &= ~UHCI_PORT_RWC; // Clear the change flags
    outportw(port, cur_status);
}

/**
 * @brief Clears a flag controller port (think bitsets)
 * @param port The port to clear
 * @param data The flag to clear
 */
void uhci_clearPort(uint32_t port, uint16_t data) {
    uint16_t cur_status = inportw(port);
    cur_status &= ~UHCI_PORT_RWC; // Clear the change flags
    cur_status &= ~data;
    cur_status |= UHCI_PORT_RWC & data;
    outportw(port, cur_status);
}


/**
 * @brief Reset a port
 * @return The port's status 
 */
uint32_t uhci_resetPort(uhci_t *c, uint32_t port) {
    uint32_t reg = UHCI_REG_PORT1 + port * 2;

    // Reset the port
    uhci_writePort(c->io_addr + reg, UHCI_PORT_RESET);
    sleep(50);
    uhci_clearPort(c->io_addr + reg, UHCI_PORT_RESET);

    // Wait ~100ms for the port to enable
    uint16_t pstat = 0;
    for (int i = 0; i < 10; i++) {
        sleep(10);

        // Read the status and check if a device is attached
        pstat = inportw(c->io_addr + reg);

        if (~pstat & UHCI_PORT_CONNECTION) {
            break;
        }

        // Acknowledge a change in the status
        if (pstat & UHCI_PORT_RWC) {
            uhci_clearPort(c->io_addr + reg, UHCI_PORT_ENABLE_CHANGE | UHCI_PORT_CONNECTION_CHANGE);
        }

        if (pstat & UHCI_PORT_ENABLE) {
            break;
        }

        // Enable the port
        uhci_writePort(c->io_addr + reg, UHCI_PORT_ENABLE);
    }

    return pstat;
}

/**
 * @brief Probe for devices
 */
void uhci_probe(uhci_t *c) {
    uint32_t portCount = 2; // Testing
    for (int port = 0; port < portCount; port++) {
        uint32_t status = uhci_resetPort(c, port);

        if (status & UHCI_PORT_ENABLE) {
            uint32_t speed = (status & UHCI_PORT_LSDA) ? USB_LOW_SPEED : USB_FULL_SPEED;

            serialPrintf("[module usb] UHCI driver found that a USB device on port 0x%x with speed 0x%x is available\n", port, speed);
        }
    }
}


/**
 * @brief Startup the UHCI controller
 * 
 * @bug This function doesn't validate parameters
 */
void uhci_init(uint32_t device) {

    serialPrintf("[module usb] Initializing UHCI controller on PCI device 0x%x...\n", device);

    // Get the base IO address
    uint32_t addr = pciConfigReadField(device, PCI_OFFSET_BAR4, 4);

    // Now let's start up this controller
    uhci_t *c = kmalloc(sizeof(uhci_t));
    c->io_addr = addr;
    c->frame_list = kmalloc(1024 * sizeof(uint32_t)); // 1024 default

    // Setup some of the QH and the frame listt
    uhci_queue_head_t *qh = kmalloc(sizeof(uhci_queue_head_t));
    qh->head_link_ptr = UHCI_TD_PTR_TERM;
    qh->element_link_ptr = UHCI_TD_PTR_TERM;
    qh->transfer = 0;
    qh->qh_link = list_create();

    c->qh_async = qh;
    for (int i = 0; i < 1024; i++) {
        c->frame_list[i] = UHCI_TD_PTR_QH | (uint32_t)qh;
    }

    // Let's write some values to the I/O ports
    // We have to use our outport functions because of differing sizes
    outportw(c->io_addr + UHCI_REG_LEGACYSUP, 0x8F00);      // Disable legacy support
    outportw(c->io_addr + UHCI_REG_INTERRUPT, 0x0);         // Disable interrupts
    outportw(c->io_addr + UHCI_REG_FRAMENUM, 0);            // Assign frame list
    outportl(c->io_addr + UHCI_REG_FLBASEADDR, c->frame_list);
    outportw(c->io_addr + UHCI_REG_STFRAMEMOD, 0x40);
    outportw(c->io_addr + UHCI_REG_STATUS, 0xFFFF);         // Clear status
    outportw(c->io_addr + UHCI_REG_CMD, UHCI_CMD_RS);       // Enable the controller

    // Probe for devices
    uhci_probe(c);
    
    // Register the controller
    
}