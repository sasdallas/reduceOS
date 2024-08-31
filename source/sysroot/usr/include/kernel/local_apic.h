// local_apic.h - Initializes the local APIC

#ifndef LOCAL_APIC_H
#define LOCAL_APIC_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations
#include <kernel/hal.h> // Hardware abstraction layer

// Definitions
#define LOCAL_APIC_ID 0x0020
#define LOCAL_APIC_VER 0x0030
#define LOCAL_APIC_TPR 0x0080
#define LOCAL_APIC_APR 0x0090
#define LOCAL_APIC_PPR 0x00A0
#define LOCAL_APIC_EOI 0x00B0
#define LOCAL_APIC_RRD 0x00C0
#define LOCAL_APIC_LDR 0x00D0
#define LOCAL_APIC_DFR 0x00E0
#define LOCAL_APIC_SVR 0x00f0
#define LOCAL_APIC_ISR 0x0100
#define LOCAL_APIC_TMR 0x0180
#define LOCAL_APIC_IRR 0x0200
#define LOCAL_APIC_ESR 0x0280
#define LOCAL_APIC_ICRLO 0x0300
#define LOCAL_APIC_ICRHI 0x0310
#define LOCAL_APIC_TIMER 0x0320
#define LOCAL_APIC_THERMAL 0x0330
#define LOCAL_APIC_PERF 0x0340
#define LOCAL_APIC_LINT0 0x0350
#define LOCAL_APIC_LINT1 0x0360
#define LOCAL_APIC_ERROR 0x0370
#define LOCAL_APIC_TICR 0x0380
#define LOCAL_APIC_TCCR 0x0390
#define LOCAL_APIC_TDCR 0x03e0

// https://github.com/pdoane/osdev/blob/master/intr/local_apic.c
// Delivery Mode
#define ICR_FIXED                       0x00000000
#define ICR_LOWEST                      0x00000100
#define ICR_SMI                         0x00000200
#define ICR_NMI                         0x00000400
#define ICR_INIT                        0x00000500
#define ICR_STARTUP                     0x00000600

// Destination Mode
#define ICR_PHYSICAL                    0x00000000
#define ICR_LOGICAL                     0x00000800

// Delivery Status
#define ICR_IDLE                        0x00000000
#define ICR_SEND_PENDING                0x00001000

// Level
#define ICR_DEASSERT                    0x00000000
#define ICR_ASSERT                      0x00004000

// Trigger Mode
#define ICR_EDGE                        0x00000000
#define ICR_LEVEL                       0x00008000

// Destination Shorthand
#define ICR_NO_SHORTHAND                0x00000000
#define ICR_SELF                        0x00040000
#define ICR_ALL_INCLUDING_SELF          0x00080000
#define ICR_ALL_EXCLUDING_SELF          0x000c0000

// Destination Field
#define ICR_DESTINATION_SHIFT           24

// Functions
void localAPIC_init(); //  Initialize the local APIC
uint8_t localAPIC_getID(); // returns the ID of the local APIC
void localAPIC_sendInit(uint8_t apicID); // Sends an init request to local APIC.
void localAPIC_sendStartup(uint8_t apicID, uint8_t vector); // Sends a startup request to an APIC with a vector.

#endif
