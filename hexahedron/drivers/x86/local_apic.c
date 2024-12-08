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
#error "x86_64 is unsupported currently"
#else
#error "Unsupported architecture"
#endif


/* APIC base */
static uintptr_t lapic_base = 0x0;

/* Log method */
#define LOG(status, ...) dprintf_module(status, "LAPIC", __VA_ARGS__)


/**
 * @brief Returns whether the CPU has an APIC
 */
int lapic_available() {
#if defined(__ARCH_I386__)
    // Use CPUID
    uint32_t eax, discard, edx;
    __cpuid(CPUID_GETFEATURES, eax, discard, discard, edx);
    return edx & CPUID_FEAT_EDX_APIC;
#else
    #error "Unimplemented"
#endif

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
    uint8_t lapic_maximum_lvt = (ver_reg & 0xFF0000) >> 16;

    LOG(DEBUG, "Local APIC, version %i maximum LVT entry %i\n", lapic_ver, lapic_maximum_lvt);
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

    LOG(DEBUG, "LAPIC 0x%x SIPI to starting IP 0x%x\n", lapic_id, vector);
}

/**
 * @brief Send INIT signal
 * @param lapic_id The ID of the APIC
 */
void lapic_sendInit(uint8_t lapic_id) {
    // Write the local APIC id to the high ICR
    lapic_write(LAPIC_REGISTER_ICR + 0x10, lapic_id << LAPIC_ICR_HIGH_ID_SHIFT);
    
    // Write the ICR to send the SIPI
    lapic_write(LAPIC_REGISTER_ICR, LAPIC_ICR_INIT | LAPIC_ICR_DESTINATION_PHYSICAL | LAPIC_ICR_INITDEASSERT | LAPIC_ICR_EDGE);

    // Wait for send to be completed
    while (lapic_read(LAPIC_REGISTER_ICR) & LAPIC_ICR_SENDING);

    LOG(DEBUG, "LAPIC 0x%x INIT IPI\n", lapic_id);
}

/**
 * @brief Local APIC spurious IRQ
 * @note This is the same between x86_64 and i386.
 */
int lapic_irq(uint32_t exception_index, uint32_t irq_number, registers_t *registers, extended_registers_t *extended) {
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
    if (hal_registerInterruptHandler(LAPIC_SPUR_INTNO, lapic_irq) != 0) {
        kernel_panic_extended(ASSERTION_FAILED, "lapic", "*** Failed to acquire interrupt 0x%x\n", LAPIC_SPUR_INTNO);
        __builtin_unreachable();
    }

    // Now actually configure the local APIC
    lapic_write(LAPIC_REGISTER_SPURINT, lapic_read(LAPIC_REGISTER_SPURINT) | LAPIC_SPUR_INTNO);
    lapic_setEnabled(1);

    // Get version
    lapic_getVersion(); // Discard, just print to console.

    // Finished!
    return 0;
}