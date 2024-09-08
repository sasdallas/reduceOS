/**
 * @file source/kmods/usb/uhci.c
 * @brief UHCI (Universal Host Controller Interface) section of the USB driver.
 * 
 * 
 * @todo Remove useless parts of data structures
 * 
 * @copyright
 * This file is part of the reduceOS C kernel, which is licensed under the terms of GPL.
 * Please credit me if this code is used.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include "uhci.h"
#include "dev.h"
#include "usb.h"
#include <libk_reduced/stdint.h>
#include <libk_reduced/stdio.h>
#include <libk_reduced/sleep.h>
#include <kernel/serial.h>
#include <kernel/pci.h>

/**
 * @brief Debug function
 */
void uhci_printQH(uhci_queue_head_t *qh) {
    serialPrintf("[module usb]\tQH 0x%x - head=0x%08x element=0x%08x\n", qh, qh->head_link_ptr, qh->element_link_ptr);
}

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
    serialPrintf("[module usb] UHCI resetting port 0x%x\n", c->io_addr + reg);
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
            continue;
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
 * @brief Create a transfer descriptor for a device
 * 
 * @param prev The previous TD (accepts NULL if creating the head)
 * @param speed The speed of the transfer
 * @param addr The address of the transfer
 * @param endp The endpoint of the transfer
 * @param toggle Endpoint toggle
 * @param packet_type The type of the packet
 * @param length The length of the transfer
 * @param data The data of the transfer
 * 
 * @return The TD created
 */
static uhci_td_t *uhci_createTD(uhci_td_t *prev,
                            uint32_t speed, uint32_t addr, uint32_t endp, uint32_t toggle, uint32_t packetType,
                            uint32_t length, const void *data) 
{
    // Create the output TD
    uhci_td_t *td = kmalloc(sizeof(uhci_td_t));

    // Update the length to be valid
    length = (length - 1) & 0x7FF;

    // If a previous TD was specified, update its link and tdNext
    if (prev != NULL) {
        prev->link_ptr = (uint32_t)td | UHCI_TD_PTR_DEPTH;
        prev->tdNext = td;
    }                     

    td->link_ptr = UHCI_TD_PTR_TERM;
    td->tdNext = 0; // Signify the end of the TD chain

    td->cs = (3 << TD_CS_ERROR_SHIFT) | TD_CS_ACTIVE;
    if (speed == USB_LOW_SPEED) td->cs |= TD_CS_LOW_SPEED; // CS should specify if the transfer is slow

    // Create the token parameter
    td->token =
        (length << TD_TOK_MAXLEN_SHIFT) |
        (toggle << TD_TOK_D_SHIFT) |
        (endp << TD_TOK_ENDP_SHIFT) |
        (addr << TD_TOK_DEVADDR_SHIFT) |
        packetType;
    
    td->buf_ptr = (uint32_t)data;

    return td;
}

/**
 * @brief Process a queue head
 * @param c UHCI Controller structure
 * @param qh Queue Head
 */
void uhci_handleQH(uhci_t *c, uhci_queue_head_t *qh) {
    // We store the transfer structure within the queue head structure (technically not specification-compliant)
    USBTransfer_t *transfer = qh->transfer;

    // Now we'll extract the transfer descriptor from the element link pointer.
    // The first 4 bits of the element link ptr are used for something that's not important.
    // We can fix this by just clearing those bits with a 0xF
    uhci_td_t *td = (uhci_td_t*)(qh->element_link_ptr & ~0xF);

    // If the TD is 0, it was completed and we are done.
    if (!td) {
        // WARNING: Spec says none of this, but this does work sooo
        transfer->success = 1;
        transfer->complete = 1;
    } else if (!(td->cs & TD_CS_ACTIVE)) {
        // The TD was not marked as active, therefore there must've been an error.
        // All fatal errors will have a stalled TD

        if (td->cs & TD_CS_STALLED) {
            // This bit can be s
            serialPrintf("[module usb] UHCI controller detected TD stall\n");
            transfer->success = 0;
            transfer->complete = 1;
        } 
        
        if (td->cs & TD_CS_DATABUFFER) {
            // Data buffer error
            // UHCI docs specify that this is done when the HC can't keep up with incoming/outgoing data requests.
            // The HC will automatically force timeout conditions for overruns (too much incoming data) and will set a CRC failure (and by ext. STALL bit) 
            serialPrintf("[module usb] UHCI controller detected data buffer error\n");
        } 
        
        if (td->cs & TD_CS_BABBLE) {
            // This transfer won't shut up
            serialPrintf("[module usb] UHCI controller detected incessent yapping\n");
        }

        if (td->cs & TD_CS_CRC_TIMEOUT) {
            // This can either be a bad CRC or the target endpoint/device timing out
            serialPrintf("[module usb] UHCI controller detected CRC/Timeout error");
        } 

        if (td->cs & TD_CS_NAK) {
            // The data is not ready to be transmitted yet
            serialPrintf("[module usb] UHCI controller detected NAK\n");
        }

        if (td->cs & TD_CS_BITSTUFF) {
            // Bitstuff errors are when the received data stream had more than 6 ones
            serialPrintf("[module usb] UHCI controller detected bitstuff error\n");
        }
    } 

    // Did the transfer complete?
    if (transfer->complete) {
        serialPrintf("[module usb] UHCI transfer completed\n");

        qh->transfer = NULL; // Remove it

        // Update the endpoint's toggle state
        if (transfer->success && transfer->endp) {
            transfer->endp->toggle ^= 1; // what
        }

        
        // Remove the queue
        node_t *qh_node = list_find(c->qh_async, qh);

        // When removing we need to find the node that points to our QH and then reset some of its pointers.
        uhci_queue_head_t *prev = (uhci_queue_head_t*)qh_node->prev->value;
        serialPrintf("[module usb] QH 0x%x from prev node 0x%x is being updated to use head link ptr 0x%x\n", prev, qh_node->prev, qh->head_link_ptr);
        prev->head_link_ptr = qh->head_link_ptr;
        
        list_delete(c->qh_async, qh_node);

        // Free transfer descriptors
        uhci_td_t *td = (uhci_td_t*)qh->td_head;
        while (td) {
            uhci_td_t *next_td = (uhci_td_t*)td->tdNext;
            kfree(td);
            td = next_td;
        }

        // Free the queue head
        kfree(qh);
    }
}


/**
 * @brief Wait for a queue head to complete
 */
static void uhci_waitQH(uhci_t *hc, uhci_queue_head_t *qh) {
    USBTransfer_t *t = qh->transfer;

    while (!t->complete) {
        uhci_handleQH(hc, qh);
    }
}

/**
 * @brief Handle a device control request
 * 
 * @param dev The device
 * @param t The transfer
 */
void uhci_control(USBDevice_t *dev, USBTransfer_t *t) {
    uhci_t *hc = (uhci_t*)dev->controller;
    USBDeviceRequest_t *req = t->req;

    // Determine transfer properties
    uint32_t speed = dev->speed;
    uint32_t addr = dev->addr;
    uint32_t endp = 0;
    uint32_t maxSize = dev->maxPacketSize;
    uint32_t type = req->type;
    uint32_t length = req->length;
    serialPrintf("[module usb]\tUHCI request: speed=%i addr=0x%x endp=0x%x maxSize=0x%x type=0x%x length=0x%x\n", speed, addr, endp, maxSize, type, length);

    // Create the TD queue
    uhci_td_t *td = uhci_createTD(NULL, speed, addr, endp, 0, TD_PACKET_SETUP, sizeof(USBDeviceRequest_t), req);
    uhci_td_t *prev = td;
    uhci_td_t *head = td;

    // Now let's create the data in/out packets
    uint32_t packetType = type & USB_RT_D2H ? TD_PACKET_IN : TD_PACKET_OUT;

    uint8_t *data = t->data;
    uint8_t *end = data + length;
    uint32_t packetSize, toggle;
    while (data < end) {
        // Setup values
        toggle ^= 1;
        packetSize = end - data;
        if (packetSize > maxSize) packetSize = maxSize;

        // Create a new transfer descriptor
        td = uhci_createTD(prev, speed, addr, endp, toggle, packetType, packetSize, data);

        // Update data and prev
        data += packetSize;
        prev = td;
    } 

    // Let's finish off with a status packet
    toggle = 1;
    packetType = type & USB_RT_D2H ? TD_PACKET_OUT : TD_PACKET_IN;
    td = uhci_createTD(prev, speed, addr, endp, toggle, packetType, 0, 0);

    // Initialize a queue head
    uhci_queue_head_t *qh = kmalloc(sizeof(uhci_queue_head_t));
    qh->qh_link = list_create();
    qh->head_link_ptr = td;
    qh->td_head = head;
    qh->transfer = t;
    qh->element_link_ptr = head;

    // Schedule it
    // This is weird, but let me explain:
    // - We told the UHCI controller to have a frame list pointer at some random PMM-allocated address. It doesn't know what the list is.
    // - All entries in the frame list point to a single QH which is the head of our current list.
    // - Therefore, we'll modify the last entry in the list to have a head pointer that points to our qh, then tack on another entry of the list that will point to a TERM pointer.
    // TODO: Better way to do this?
    node_t *last_node;
    foreach(n, hc->qh_async) {
        last_node = n;
    }

    uhci_queue_head_t *last_qh = (uhci_queue_head_t*)last_node->value;
    qh->head_link_ptr = UHCI_TD_PTR_TERM;
    last_qh->head_link_ptr = ((uint32_t)qh) | UHCI_TD_PTR_QH;

    list_insert(hc->qh_async, qh);

    serialPrintf("[module usb] QH order updated:\n");
    foreach(n, hc->qh_async) {
        uhci_printQH((uhci_queue_head_t*)n->value);
    }


    // Wait until the qh is procesed
    serialPrintf("Done, now waiting for queue to process...\n");
    uhci_waitQH(hc, qh);
}






/**
 * @brief The polling method for the UHCI controller
 */
void uhci_poll(USBController_t *controller) {
    uhci_t *c = (uhci_t*)controller->hc;
    //serialPrintf("erm... what the sigma? HC=0x%x QH_ASYNC=0x%x BIO=0x%x\n", c, c->qh_async, c->io_addr);

    foreach(qh_node, c->qh_async) {
        uhci_queue_head_t *qh = (uhci_queue_head_t*)qh_node->value;
        if (qh->transfer) {
            uhci_handleQH(c, qh);
        }
    }

}


/**
 * @brief Handles the interrupts for a UHCI controller
 */
void uhci_interface(USBDevice_t *dev, USBTransfer_t *transfer) {
    uhci_t *hc = (uhci_t*)dev->controller;

    // Find the transfer properties
    uint32_t transfer_speed = dev->speed;
    uint32_t transfer_addr = dev->addr;
    uint32_t transfer_endpoint = dev->endp.desc.addr & 0xF;
    
    // Now we'll setup packet data
    uint32_t toggle = dev->endp.toggle;
    uint32_t packetType = TD_PACKET_IN;
    uint32_t packetSize = transfer->length;

    // Initialize the data packet
    uhci_td_t *td = uhci_createTD(NULL, transfer_speed, transfer_addr, transfer_endpoint, toggle,
                                    packetType, packetSize, transfer->data);

    // Create a new queue head
    uhci_queue_head_t *qh = kmalloc(sizeof(uhci_queue_head_t));
    qh->qh_link = list_create();
    qh->element_link_ptr = td;
    qh->head_link_ptr = td;
    qh->td_head = td;
    qh->transfer = transfer;
    qh->element_link_ptr = td;
    
    // Schedule the queue
    node_t *last_node;
    foreach(n, hc->qh_async) {
        last_node = n;
    }

    uhci_queue_head_t *last_qh = (uhci_queue_head_t*)last_node->value;
    qh->head_link_ptr = UHCI_TD_PTR_TERM;
    last_qh->head_link_ptr = ((uint32_t)qh) | UHCI_TD_PTR_QH;

    list_insert(hc->qh_async, qh);
}


/**
 * @brief Probe for devices
 */
void uhci_probe(uhci_t *c) {
    serialPrintf("[module usb]\tProbe for UHCI device start (BIO = 0x%x)...\n", c->io_addr);

    uint32_t portCount = 2; // Testing
    for (int port = 0; port < portCount; port++) {
        uint32_t status = uhci_resetPort(c, port);


        if (status & UHCI_PORT_ENABLE) {
            uint32_t speed = (status & UHCI_PORT_LSDA) ? USB_LOW_SPEED : USB_FULL_SPEED;

            serialPrintf("[module usb] UHCI driver found that a USB device on port 0x%x with speed 0x%x is available\n", port, speed);

            USBDevice_t *dev = usb_createDevice();
            dev->controller = c;
            dev->port = port;
            dev->speed = speed;
            dev->maxPacketSize = 8;

            dev->control = uhci_control;
            dev->interface = uhci_interface;

            if (!usb_initDevice(dev)) {
                // ??
            } 
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
    pciBar_t *bar = pciGetBAR(device, PCI_OFFSET_BAR4);
    if (!(bar->flags & PCI_BAR_IO)) {
        // Only I/O addresses will work
        return;
    }    


    // Now let's start up this controller
    uhci_t *c = kmalloc(sizeof(uhci_t));
    c->io_addr = bar->port;
    c->frame_list = kmalloc(1024 * sizeof(uint32_t)); // 1024 default

    // Setup some of the QH and the frame listt
    uhci_queue_head_t *qh = kmalloc(sizeof(uhci_queue_head_t));
    qh->head_link_ptr = UHCI_TD_PTR_TERM;
    qh->element_link_ptr = UHCI_TD_PTR_TERM;
    qh->transfer = 0;
    qh->qh_link = list_create();

    c->qh_async = list_create();
    list_insert(c->qh_async, qh);

    c->frame_list = pmm_allocateBlocks((1024 * sizeof(uint32_t)) / 4096);
    serialPrintf("Pushing the value 0x%x to 0x%x\n", (UHCI_TD_PTR_QH | ((uint32_t)qh << 4)), c->frame_list);

    for (int i = 0; i < 1024; i++) {
        c->frame_list[i] = UHCI_TD_PTR_QH | ((uint32_t)qh);
    }

    // Let's write some values to the I/O ports
    // We have to use our outport functions because of differing sizes
    outportw(c->io_addr + UHCI_REG_LEGACYSUP, 0x8F00);      // Disable legacy support
    outportw(c->io_addr + UHCI_REG_INTERRUPT, 0x0);         // Disable interrupts
    outportw(c->io_addr + UHCI_REG_FRAMENUM, 0);            // Assign frame list
    outportl(c->io_addr + UHCI_REG_FLBASEADDR, ((uint32_t)c->frame_list));
    outportw(c->io_addr + UHCI_REG_STFRAMEMOD, 0x40);
    outportw(c->io_addr + UHCI_REG_STATUS, 0xFFFF);         // Clear status
    outportw(c->io_addr + UHCI_REG_CMD, UHCI_CMD_RS);       // Enable the controller

    // Probe for devices
    uhci_probe(c);
    
    // Register the controller
    USBController_t *uhci_controller = kmalloc(sizeof(USBController_t));
    uhci_controller->hc = (void*)c;
    uhci_controller->poll = (poll_t*)uhci_poll;
    usb_addController(uhci_controller);
}