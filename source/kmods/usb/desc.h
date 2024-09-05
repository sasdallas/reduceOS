// desc.h - Provides the standard for USB devices to conform their descriptors to

#ifndef _USB_DESC_H
#define _USB_DESC_H

/**** Includes ****/
#include <libk_reduced/stdint.h>

/**** Definitions ****/

// Descriptor types
typedef enum {
    USB_DESC_DEVICE     = 0x01,
    USB_DESC_CONF       = 0x02,
    USB_DESC_STRING     = 0x03,
    USB_DESC_INTF       = 0x04,
    USB_DESC_ENDP       = 0x05,

    // HID types
    USB_DESC_HID        = 0x21,
    USB_DESC_REPORT     = 0x22,
    USB_DESC_PHYSICAL   = 0x23,
    
    // Hub types
    USB_DESC_HUB        = 0x29

} USB_DESC_TYPE;

// USB device descriptor
typedef struct USBDeviceDescriptor {
    uint8_t     length;         // Size of the descriptor in bytes
    uint8_t     type;           // Descriptor type
    uint16_t    spec;           // BCD-encoded specification value
    uint8_t     devclass;       // Device class code
    uint8_t     devsubclass;    // Device subclass code
    uint8_t     protocol;       // Device protocol code
    uint8_t     maxPacketSize;  // Maximum packet size for endpoint 0
    uint16_t    vend_id;        // Vendor ID
    uint16_t    prod_id;        // Product ID
    uint16_t    device_release; // Device release number in BCD
    uint8_t     manuf_idx;      // Manufacturer index in the STRING descriptor
    uint8_t     product_idx;    // Product index in the STRING descriptor
    uint8_t     sn_idx;         // Serial number index in the STRING descriptor.
    uint8_t     num_confs;      // Number of possible configurations          
} USBDeviceDescriptor_t;

// USB device qualifier descriptor
typedef struct USBDeviceQualifierDescriptor {
    uint8_t     length;         // Size of the descriptor in bytes
    uint8_t     type;           // Descriptor type
    uint16_t    spec;           // BCD-encoded specification value
    uint8_t     devclass;       // Device class code
    uint8_t     devsubclass;    // Device subclass code
    uint8_t     protocol;       // Device protocol code
    uint8_t     maxPacketSize;  // Maximum packet size for endpoint 0
    uint8_t     num_confs;      // Number of possible configurations
    uint8_t     reserved;       // Reserved
} USBDeviceQualifierDescriptor_t;

// USB configuration descriptor
typedef struct USBConfigDescriptor {
    uint8_t     length;         // Size of the descriptor in bytes
    uint8_t     type;           // Descriptor type
    uint16_t    totalLength;    // Total combined length, in bytes, of all descriptors returned with the request for this descriptor
    uint8_t     interfaces;     // Number of interfaces supported
    uint8_t     configuration;  // Use this as an argument in SET_CONFIGURATION
    uint8_t     stridx;         // Index of the STRING descriptor describing this descriptor.
    uint8_t     attributes;     // Configuration characteristics (see bitflags for this)
    uint8_t     max_power;      // Maximum power consumption
} USBConfigDescriptor_t;

// USB interface descriptor
typedef struct USBInterfaceDescriptor {
    uint8_t     length;         // Size of the descriptor in bytes
    uint8_t     type;           // Descriptor type
    uint8_t     interface_num;  // Number of this interface
    uint8_t     alternate;      // Alternate settings described by this descriptor.
    uint8_t     num_endpoints;  // Number of endpoints
    uint8_t     ifclass;        // Interface class code
    uint8_t     ifsubclass;     // Interface subclass code
    uint8_t     ifprotocol;     // Interface protocol
    uint8_t     interfaceidx;   // Index of STRING descriptor describing this index
} USBInterfaceDescriptor_t;

// USB endpoint descriptor
typedef struct USBEndpointDescriptor {
    uint8_t     length;         // Size of the descriptor in bytes
    uint8_t     type;           // Descriptor type
    uint8_t     addr;           // Endpoint address
    uint8_t     attributes;     // Endpoint attributes
    uint8_t     maxPacketSize;  // Maximum packet size
    uint8_t     poll_interval;  // Polling interval
} USBEndpointDescriptor_t;

// String descriptor is not provided because its weird

// USB HID descriptor - https://usb.org/sites/default/files/hid1_11.pdf
typedef struct USBHidDescriptor {
    uint8_t     length;         // Size of the descriptor in bytes
    uint8_t     type;           // Descriptor type
    uint16_t    hid_version;    // HID version (BCD)
    uint8_t     country_code;   // Country code of localized hardware
    uint8_t     num_descs;      // Number of descriptors
    uint8_t     class_type;     // Type of class descriptor
    uint16_t    desc_length;    // Total size of the report descriptors
    
    // @todo Implement support for the optional descriptors
} USBHidDescriptor_t;



#endif 