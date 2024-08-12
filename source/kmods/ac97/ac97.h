// ac97.h - Header file for the AC'97 soundcard

#ifndef AC97_H
#define AC97_H

// Includes
#include <kernel/serial.h>
#include <kernel/pci.h>
#include <stdint.h>

// Definitions

// BARs
#define AC97_NAM_BAR                0x10    // Native Audio Mixer Base Address Register
#define AC97_NABM_BAR               0x14    // Native Audio Bus Mastering Base Address Register

// AC97 Mixer I/O Port offsets (BAR0)
#define AC97_RESET                  0x00    // Reset Register/Capabilities
#define AC97_MASTER_VOL             0x02    // Set master output volume
#define AC97_AUX_OUT_VOL            0x04    // Set AUX output volume
#define AC97_MONO_VOL               0x06    // Set mono output volume
#define AC97_SET_PCM_OUT_VOL        0x18    // Set PCM output volume

// Microphone/input device offsets (BAR0)
#define AC97_SET_MICROPHONE         0x0E    // Set microphone volume
#define AC97_SELECT_INPUT           0x1A    // Select input device
#define AC97_SET_INPUT_GAIN         0x1C    // Set input gain
#define AC97_SET_MICROPHONE_GAIN    0x1E    // Set gain of microphone

// Bus mastering I/O port offsets
#define AC97_BM_BDBAR               0x10    // PCM output buffer descriptor BAR
#define AC97_BM_INDEX               0x14    // PCM output current index value
#define AC97_BM_LASTVALIDIDX        0x15    // PCM output last valid index
#define AC97_BM_STATUS              0x16    // PCM output status register
#define AC97_BM_POSINCURB           0x18    // PCM output position in current buffer
#define AC97_BM_CTRL                0x1B    // PCM output control register

// Bus master misc.
#define AC97_BDL_LEN                32      // Buffer descriptor list length
#define AC97_BDL_BUFFER_LEN         0x1000  // Length of buffer in BDL
#define AC97_CL_BUFUNDERRUN         ((uint32_t)1 << 30)     // Buffer underrun policy in CL
#define AC97_CL_INTONCOMP           ((uint32_t)1 << 31)     // Send interrupt on completion

// Status register bitflags
#define AC97_STATUS_DMAHALT         (1 << 0)    // DMAC halted
#define AC97_STATUS_CUREQULV        (1 << 1)    // Current equals last valid
#define AC97_STATUS_LVBUFCOMPINT    (1 << 2)    // Last valid buffer completion interrupt
#define AC97_STATUS_BUFCOMPINT      (1 << 3)    // Buffer completion interrupt status
#define AC97_STATUS_FIFOERR         (1 << 4)    // FIFO error


// PCM output control register bitflags
#define AC97_CONTROL_RUNPAUSE       (1 << 0)    // Run/pause bus master
#define AC97_CONTROL_RESET          (1 << 1)    // Reset registers
#define AC97_CONTROL_LVBUFINT       (1 << 2)    // Enable last valid buffer interrupt
#define AC97_CONTROL_FIFOINT        (1 << 3)    // FIFO error interrupt enable
#define AC97_CONTROL_INTONCOMP      (1 << 4)    // Interrupt on completion enable

// Macros
#define AC97_CL_GET_LENGTH(cl)      ((cl) & 0xFFFF)
#define AC97_CL_SET_LENGTH(cl, v)   ((cl) = (v) & 0xFFFF)

// Typedefs

// Entry in a BDL
typedef struct {
    uint32_t ptr;
    uint32_t cl;
} __attribute__((packed)) ac97_bdlEntry_t;

// AC'97 device structure
typedef struct {
    uint32_t pciDevice;
    uint16_t nabmBAR;
    uint16_t namBAR;
    int IRQ;
    uint8_t lastValidIndex;
    uint8_t volBits;
    ac97_bdlEntry_t *bdl;
    uint16_t *buffers[AC97_BDL_LEN];
    uint32_t bdl_p;
    uint32_t mask;
    volatile char *ioBase;
} ac97_t;




#endif