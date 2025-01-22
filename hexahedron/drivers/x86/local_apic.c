/**
 * @file hexahedron/drivers/x86/local_apic.c
 * @brief Local APIC driver
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/drivers/x86/local_apic.h>
#include <kernel/mem/mem.h>
#include <kernel/debug.h>
#include <kernel/drivers/x86/clock.h>
#include <kernel/panic.h>
#include <errno.h>

#if defined(__ARCH_I386__)
#include <kernel/arch/i386/cpu.h>
#include <kernel/arch/i386/registers.h>
#include <kernel/arch/i386/hal.h>
#elif defined(__ARCH_X86_64__)
#include <kernel/arch/x86_64/cpu.h>
#include <kernel/arch/x86_64/registers.h>
#include <kernel/arch/x86_64/hal.h>
#else
#error "Unsupported"
#endif

/* APIC base */
uintptr_t lapic_base = 0x0;

/* Log method */
#define LOG(status, ...) dprintf_module(status, "X86:LAPIC", __VA_ARGS__)


/**
 * @brief Returns whether the CPU has an APIC
 */
int lapic_available() {
    // Use CPUID
    uint32_t eax, discard, edx;
    __cpuid(CPUID_GETFEATURES, eax, discard, discard, edx);
    return edx & CPUID_FEAT_EDX_APIC;

    return 0;
}

/**
 * @brief Read register from local APIC
 */
uint32_t lapic_read(uint32_t reg) {
    if (!lapic_base) return 0;
    return *((uint32_t volatile*)(lapic_base + reg));
}

/**
 * @brief Write register to local APIC
 * @param reg Register to write
 * @param data Data to write
 */
void lapic_write(uint32_t reg, uint32_t data) {
    if (!lapic_base) return;

    uint32_t volatile *lapic = ((uint32_t volatile*)(lapic_base + reg));
    *lapic = data;
}

/**
 * @brief Get the local APIC ID
 */
uint8_t lapic_getID() {
    uint8_t id = (lapic_read(LAPIC_REGISTER_ID) >> 24) & 0xF;
    return id;
}

/**
 * @brief Get the local APIC version
 */
uint8_t lapic_getVersion() {
    uint32_t ver_reg = lapic_read(LAPIC_REGISTER_VERSION);
    uint8_t lapic_ver = ((ver_reg & 0xFF));
    
    return lapic_ver;
}

/**
 * @brief Enable/disable local APIC via the spurious-interrupt vector register
 * @param enabled Enable/disable
 * @note Register the interrupt handler and the vector yourself
 */
void lapic_setEnabled(int enabled) {
    if (enabled) {
        lapic_write(LAPIC_REGISTER_SPURINT, lapic_read(LAPIC_REGISTER_SPURINT) | LAPIC_SPUR_ENABLE);
    } else {
        lapic_write(LAPIC_REGISTER_SPURINT, lapic_read(LAPIC_REGISTER_SPURINT) & ~LAPIC_SPUR_ENABLE);
    }
}

/**
 * @brief Send SIPI signal
 * @param lapic_id The ID of the APIC
 * @param vector The starting page number (for SIPI - normally vector number)
 */
void lapic_sendStartup(uint8_t lapic_id, uint32_t vector) {
    // Write the local APIC id to the high ICR
    lapic_write(LAPIC_REGISTER_ICR + 0x10, lapic_id << LAPIC_ICR_HIGH_ID_SHIFT);
    
    // Write the ICR to send the SIPI
    lapic_write(LAPIC_REGISTER_ICR, (vector / PAGE_SIZE) | LAPIC_ICR_STARTUP | LAPIC_ICR_DESTINATION_PHYSICAL | LAPIC_ICR_INITDEASSERT | LAPIC_ICR_EDGE);

    // Wait for send to be completed
    while (lapic_read(LAPIC_REGISTER_ICR) & LAPIC_ICR_SENDING);

    // LOG(DEBUG, "LAPIC 0x%x SIPI to starting IP 0x%x\n", lapic_id, vector);
}


/**
 * @brief Send an NMI to an APIC
 * @param lapic_id The ID of the APIC
 * @param irq_no The IRQ vector to send
 */
void lapic_sendNMI(uint8_t lapic_id, uint8_t irq_no) {
    // Write the local APIC ID to the high ICR
    lapic_write(LAPIC_REGISTER_ICR + 0x10, lapic_id << LAPIC_ICR_HIGH_ID_SHIFT);

    // Write the ICR to send the NMI signal
    lapic_write(LAPIC_REGISTER_ICR, LAPIC_ICR_NMI | LAPIC_ICR_DESTINATION_PHYSICAL | LAPIC_ICR_INITDEASSERT | LAPIC_ICR_EDGE | irq_no);

    // Wait for send to be completed
    while (lapic_read(LAPIC_REGISTER_ICR) & LAPIC_ICR_SENDING);
}

/**
 * @brief Send INIT signal
 * @param lapic_id The ID of the APIC
 */
void lapic_sendInit(uint8_t lapic_id) {
    // Write the local APIC id to the high ICR
    lapic_write(LAPIC_REGISTER_ICR + 0x10, lapic_id << LAPIC_ICR_HIGH_ID_SHIFT);
    
    // Write the ICR to send the INIT signal
    lapic_write(LAPIC_REGISTER_ICR, LAPIC_ICR_INIT | LAPIC_ICR_DESTINATION_PHYSICAL | LAPIC_ICR_INITDEASSERT | LAPIC_ICR_EDGE);

    // Wait for send to be completed
    while (lapic_read(LAPIC_REGISTER_ICR) & LAPIC_ICR_SENDING);

    // LOG(DEBUG, "LAPIC 0x%x INIT IPI\n", lapic_id);
}

/**
 * @brief Local APIC spurious IRQ
 * @note This is the same between x86_64 and i386.
 */
int lapic_irq(uintptr_t exception_index, uintptr_t irq_number, registers_t *registers, extended_registers_t *extended) {
    LOG(DEBUG, "Spurious local APIC IRQ\n");
    return 0;
}

/**
 * @brief Acknowledge local APIC interrupt
 */
void lapic_acknowledge() {
    lapic_write(LAPIC_REGISTER_EOI, LAPIC_EOI);
}

/**
 * @brief Gets the current error state of the local apic
 * @returns Error code
 */
uint8_t lapic_readError() {
    // Update ESR
    lapic_write(LAPIC_REGISTER_ERROR, 0);

    // Read ESR
    uint8_t error = lapic_read(LAPIC_REGISTER_ERROR);

    return error;
}


/**
 * @brief Initialize the local APIC
 * @param lapic_address Address of MMIO-mapped space of the APIC
 * @returns 0 on success, anything else is failure
 * 
 * @warning PIC must be disabled.
 */
int lapic_initialize(uintptr_t lapic_address) {
    if (!lapic_available()) {
        LOG(WARN, "No local APIC available\n");
        return -EINVAL;
    }

    lapic_base = lapic_address;

    // Register the interrupt handler
    hal_registerInterruptHandler(LAPIC_SPUR_INTNO, lapic_irq); // NOTE: This might fail occasionally (BSP will reinitialize APICs for each core)
    
    // Enable spurious vector register
    lapic_write(LAPIC_REGISTER_SPURINT, lapic_read(LAPIC_REGISTER_SPURINT) | LAPIC_SPUR_INTNO);
    lapic_setEnabled(1);

    // Finished!
    return 0;
}