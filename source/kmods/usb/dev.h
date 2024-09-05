// dev.h - Provides the standard for USB controllers/devices to conform to

#ifndef _USB_DEV_H
#define _USB_DEV_H

/**** Includes ****/
#include "desc.h"
#include "req.h"
#include <libk_reduced/stdint.h>

/**** Definitions ****/

// Speeds
#define USB_FULL_SPEED  0x00
#define USB_LOW_SPEED   0x01
#define USB_HIGH_SPEED  0x02

// This is similar to the way the VFS works. It's defined by us

// Endpoint
typedef struct USBEndpoint {
    USBEndpointDescriptor_t desc;   // Actual descriptor
    uint32_t toggle;                // The toggle value is used for bulk data transfers
} USBEndpoint_t;

// Transfer request
typedef struct USBTransfer {
    USBEndpoint_t       *endp;
    USBDeviceRequest_t  *req;
    void                *data;
    uint32_t            length;
    int                 complete;
    int                 success;
} USBTransfer_t;


struct USBDevice; // Prototype

typedef void (*hc_control_t)(struct USBDevice *dev, USBTransfer_t *transfer);
typedef void (*hc_interface_t)(struct USBDevice *dev, USBTransfer_t *transfer);

typedef void (*hc_poll_t)(struct USBDevice *dev);


// Device itself
typedef struct USBDevice {
    struct USBDevice    *parent;        // I hate the fact that we don't have typedefs...
    struct USBDevice    *next;          // Next device in line
    void                *controller;    // Contains a pointer to the HC's device structure (e.g. UHCI)
    
    uint32_t            port;           // Port the device is plugged into
    uint32_t            speed;          // Speed of the device
    uint32_t            addr;           // USB address (enddpoint)
    uint32_t            maxPacketSize;  // Maximum packet size

    USBEndpoint_t       endp;           // USB endpoint
    USBInterfaceDescriptor_t intf;      // USB interface

    // These functions will provide methods for the HC
    hc_control_t        control;        // Starts up a transfer request
    hc_interface_t      interface;      // Starts up a transfer request, with interface
    hc_poll_t           poll;           // USB poll method
} USBDevice_t;

// We'll keep a list for now
extern USBDevice_t *USBDeviceList;



#endif 