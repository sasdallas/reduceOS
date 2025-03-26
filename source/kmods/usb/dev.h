// dev.h - Provides the standard for USB controllers/devices to conform to

#ifndef _USB_DEV_H
#define _USB_DEV_H

/**** Includes ****/
#include "desc.h"
#include "req.h"
#include <libk_reduced/stdint.h>
#include <kernel/list.h>

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
struct USBController;

typedef void (*hc_control_t)(struct USBDevice *dev, USBTransfer_t *transfer);
typedef void (*hc_interface_t)(struct USBDevice *dev, USBTransfer_t *transfer);

typedef void (*hc_poll_t)(struct USBDevice *dev);

typedef void (*poll_t)(struct USBController *hc);

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

// A minified stripped down controller object that is used in the global USB driver
typedef struct USBController {
    void                *hc;
    poll_t               poll;
} USBController_t;

// Functions
void usb_DevInit();
USBDevice_t *usb_createDevice();
int usb_initDevice(USBDevice_t *dev);
int usb_request(USBDevice_t *dev, uint32_t type, uint32_t request,
                uint32_t value, uint32_t index, uint32_t length, void *data);
list_t *usb_getUSBDeviceList();

#endif 