// uhci.h - header file for the UHCI section of the USB driver

#ifndef _USB_UHCI_H
#define _USB_UHCI_H

/**** Includes ****/
#include "dev.h"
#include <libk_reduced/stdint.h>
#include <kernel/list.h>


// You can find the Intel reference manual on UHCI (rev 1.1) here, it's really helpful esp for TD: https://ftp.netbsd.org/pub/NetBSD/misc/blymn/uhci11d.pdf

/**** Definitions (Frame List Pointer) ****/

// Frame list pointers direct the host controller to the first item in the frame's schedule.
// It's simple. Only the first two and last 27 bits matter.

#define UHCI_FLP_TERMINATE              (1 << 0) // 1 = empty frame (invalid), 0 = valid
#define UHCI_FLP_QHTDSEL                (1 << 1) // 1 = queue head, 0 = transfer descriptor

/**** Definitions (Transfer Descriptor) ****/

// TDs expose characteristics of a transaction requested on USB by a client. 
// There's 4 parts of a TD:
// - Link pointer
// - Control and status
// - Token
// - Buffer Pointer
// All of these are just DWORDs and aligned

typedef struct _uhci_td {
    volatile uint32_t link_ptr;
    volatile uint32_t cs;
    volatile uint32_t token;
    volatile uint32_t buf_ptr;

    // We'll have to have a tdNext object which will define the next TD in the line
    uint32_t tdNext;
} uhci_td_t;

// Link pointer
#define UHCI_TD_PTR_TERM                (1 << 0)
#define UHCI_TD_PTR_QH                  (1 << 1)
#define UHCI_TD_PTR_DEPTH               (1 << 2)

// Control and status
#define TD_CS_ACTLEN                    0x000007ff
#define TD_CS_BITSTUFF                  (1 << 17)     // Bitstuff Error
#define TD_CS_CRC_TIMEOUT               (1 << 18)     // CRC/Time Out Error
#define TD_CS_NAK                       (1 << 19)     // NAK Received
#define TD_CS_BABBLE                    (1 << 20)     // Babble Detected
#define TD_CS_DATABUFFER                (1 << 21)     // Data Buffer Error
#define TD_CS_STALLED                   (1 << 22)     // Stalled
#define TD_CS_ACTIVE                    (1 << 23)     // Active
#define TD_CS_IOC                       (1 << 24)     // Interrupt on Complete
#define TD_CS_IOS                       (1 << 25)     // Isochronous Select
#define TD_CS_LOW_SPEED                 (1 << 26)     // Low Speed Device
#define TD_CS_ERROR_MASK                (3 << 27)     // Error counter
#define TD_CS_ERROR_SHIFT               27
#define TD_CS_SPD                       (1 << 29)     // Short Packet Detect

// TD token
#define TD_TOK_PID_MASK                 0x000000ff    // Packet Identification
#define TD_TOK_DEVADDR_MASK             0x00007f00    // Device Address
#define TD_TOK_DEVADDR_SHIFT            8
#define TD_TOK_ENDP_MASK                0x0078000     // Endpoint
#define TD_TOK_ENDP_SHIFT               15
#define TD_TOK_D                        0x00080000    // Data Toggle
#define TD_TOK_D_SHIFT                  19
#define TD_TOK_MAXLEN_MASK              0xffe00000    // Maximum Length
#define TD_TOK_MAXLEN_SHIFT             21

// TD packet
#define TD_PACKET_IN                    0x69
#define TD_PACKET_OUT                   0xe1
#define TD_PACKET_SETUP                 0x2d

/**** Definitions (Queue Head) ****/

// Queue heads are structures used to support the requirements of Control, Bulk, and Interrupt transfers.

typedef struct _uhci_queue_head {
    // DWORD 0 is the queue head link pointer
    uint32_t head_link_ptr;

    // DWORD 1 is the queue element link pointer
    uint32_t element_link_ptr;

    // Driver specific shenanigans
    USBTransfer_t *transfer;
    list_t *qh_link;
    uint32_t td_head; // Normally, I would use a list, but this is more of a chain situation. A list would fit qh_link better.
    uint32_t active;
    uint32_t padding[24]; // Padding so this can still be typecasted
} uhci_queue_head_t;


#define UHCI_QHLP_TERMINATE             (1 << 0) // 1 = empty frame (invalid), 0 = valid
#define UHCI_QHLP_QHTDSEL               (1 << 1) // 1 = queue head, 0 = transfer descriptor
#define UHCI_QELP_TERMINATE             (1 << 0) // 1 = empty frame (invalid), 0 = valid
#define UHCI_QELP_QHTDSEL               (1 << 1) // 1 = queue head, 0 = transfer descriptor


/**** Definitions (I/O Registers) ****/

// @todo Perhaps we should listen to the UHCI specifications...
#define UHCI_REG_CMD                    0x00        // Command register
#define UHCI_REG_STATUS                 0x02        // Status register
#define UHCI_REG_INTERRUPT              0x04        // Interrupt enable
#define UHCI_REG_FRAMENUM               0x06        // Frame number
#define UHCI_REG_FLBASEADDR             0x08        // Frame list base address
#define UHCI_REG_STFRAMEMOD             0x0C        // Start of frame modify
#define UHCI_REG_PORT1                  0x10        // Port 1 status/control
#define UHCI_REG_PORT2                  0x12        // Port 2 status/control
#define UHCI_REG_LEGACYSUP              0xC0        // Legacy support

/**** Definitions (Command register bitflags) ****/

#define UHCI_CMD_RS                     (1 << 0)    // Run/stop
#define UHCI_CMD_HCRESET                (1 << 1)    // Host controller reset
#define UHCI_CMD_GRESET                 (1 << 2)    // Global reset
#define UHCI_CMD_EGSM                   (1 << 3)    // Enter global suspend resume
#define UHCI_CMD_FGR                    (1 << 4)    // Force global resume
#define UHCI_CMD_SWDBG                  (1 << 5)    // Software Debug
#define UHCI_CMD_CF                     (1 << 6)    // Configure
#define UHCI_CMD_MAXP                   (1 << 7)    // Max packet, 0 = 32, 1 = 64

/**** Definitions (Status register bitflags) ****/

#define UHCI_STS_USBINT                 (1 << 0)    // USB interrupt
#define UHCI_STS_ERROR                  (1 << 1)    // USB error interrupt
#define UHCI_STS_RD                     (1 << 2)    // Resume detect
#define UHCI_STS_HSE                    (1 << 3)    // Host system error
#define UHCI_STS_HCPE                   (1 << 4)    // Host controller process error
#define UHCI_STS_HCH                    (1 << 5)    // HC halted

/**** Definitions (Interrupt register bitflags ) ****/

#define UHCI_INTR_TIMEOUT               (1 << 0)    // Timeout/CRC interrupt enable
#define UHCI_INTR_RESUME                (1 << 1)    // Resume interrupt enable
#define UHCI_INTR_IOC                   (1 << 2)    // Interrupt on Complete Enable
#define UHCI_INTR_SP                    (1 << 3)    // Short packet interrupt enable

/**** Definitions (PORTSC registers) ****/

#define UHCI_PORT_CONNECTION            (1 << 0)    // Current connect status
#define UHCI_PORT_CONNECTION_CHANGE     (1 << 1)    // Connect status change
#define UHCI_PORT_ENABLE                (1 << 2)    // Port enabled
#define UHCI_PORT_ENABLE_CHANGE         (1 << 3)    // Port status change
#define UHCI_PORT_LS                    (1 << 4)    // Line status
#define UHCI_PORT_RD                    (1 << 6)    // Resume detect
#define UHCI_PORT_LSDA                  (1 << 8)    // Low speed device attached
#define UHCI_PORT_RESET                 (1 << 9)    // Port reset
#define UHCI_PORT_SUSP                  (1 << 12)   // Port suspend
#define UHCI_PORT_RWC                   (UHCI_PORT_CONNECTION_CHANGE | UHCI_PORT_ENABLE_CHANGE)

/**** Definitions (UHCI) ****/

#define UHCI_MAX_QH                     8
#define UHCI_MAX_TD                     32 

typedef struct _uhci {
    // WARNING: This implementation was the only *stable* one I could find.
    // This pool system is obsolete and should be replaced.

    uint32_t            io_addr;
    uint32_t            *frame_list;
    uhci_queue_head_t   *qh_pool;
    uhci_td_t           *td_pool;
    list_t              *qh_async;
} uhci_t;


/**** Functions ****/

void uhci_init(uint32_t device);


#endif