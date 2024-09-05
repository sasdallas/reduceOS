// req.h - Provides the standard for USB controllers/devices to conform to

#ifndef _USB_REQ_H
#define _USB_REQ_H

/**** Includes ****/
#include "desc.h"
#include <libk_reduced/stdint.h>

/**** Definitions ****/

// USB request type
#define USB_RT_TRANSFER_MASK            0x80
#define USB_RT_D2H                      0x80
#define USB_RT_H2D                      0x00

#define USB_RT_TYPE_MASK                0x60
#define USB_RT_STANDARD                 0x00
#define USB_RT_CLASS                    0x20
#define USB_RT_VENDOR                   0x40

#define USB_RT_RECIPIENT_MASK           0x1F
#define USB_RT_DEV                      0x00
#define USB_RT_INTF                     0x01
#define USB_RT_ENDP                     0x02
#define USB_RT_OTHER                    0x03

// USB device requests (copy & pasted)
#define USB_REQ_GET_STATUS                  0x00
#define USB_REQ_CLEAR_FEATURE               0x01
#define USB_REQ_SET_FEATURE                 0x03
#define USB_REQ_SET_ADDR                    0x05
#define USB_REQ_GET_DESC                    0x06
#define USB_REQ_SET_DESC                    0x07
#define USB_REQ_GET_CONF                    0x08
#define USB_REQ_SET_CONF                    0x09
#define USB_REQ_GET_INTF                    0x0A
#define USB_REQ_SET_INTF                    0x0B
#define USB_REQ_SYNC_FRAME                  0x0C

// USB standard feature selectors
#define USB_FT_REMOTE_WAKEUP                1   // Device
#define USB_FT_ENDPOINT_HALT                2   // Endpoint
#define USB_FT_TEST_MODE                    3   // Device

// USB device request
typedef struct USBDeviceRequest {
    uint8_t     type;           // Type of the transfer
    uint8_t     request;        // Specific request
    uint16_t    value;          // May or may not serve as parameter
    uint16_t    index;          // May or may not serve as parameter
    uint16_t    length;         // Number of bytes to transfer if there's a DATA stage
} USBDeviceRequest_t;

#endif