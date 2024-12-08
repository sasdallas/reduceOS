/**
 * @file hexahedron/arch/i386/smp.c
 * @brief Symmetric multiprocessor handler
 * 
 * The joys of synchronization primitives are finally here.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/i386/smp.h>
#include <kernel/arch/i386/hal.h>
#include <kernel/arch/i386/interrupt.h>
#include <kernel/arch/i386/cpu.h>
#include <kernel/drivers/x86/local_apic.h>
#include <kernel/drivers/x86/clock.h>

#include <kernel/mem/pmm.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/alloc.h>
#include <kernel/debug.h>
#include <kernel/misc/term.h>

#include <string.h>
#include <stddef.h>
#include <errno.h>

/* SMP data */
static smp_info_t *smp_data = NULL;

/* Local APIC mmio address */
static uintptr_t lapic_remapped = 0;

/* Remapped page for the bootstrap code */
static uintptr_t bootstrap_page_remap = 0;


/* Log method */
#define LOG(status, ...) dprintf_module(status, "SMP", __VA_ARGS__)

/* Trampoline variables */
extern uint32_t _ap_parameters;
extern uint32_t _ap_startup;
extern uint32_t _ap_gdtr;
extern uint32_t _ap_bootstrap_start, _ap_bootstrap_end;
extern uint32_t hal_gdt_base, hal_gdt_end; // !!!: Not sure if this is important, but we should probably be keeping track of each GDT entry in C code. 

/* AP startup flag. This will change when the AP finishes starting */
static int ap_startup_finished = 0;

/* Trampoline rebase macro */
#define TRAMPOLINE_REBASE(section) (void*)(SMP_AP_BOOTSTRAP_PAGE + (uintptr_t)&section - (uintptr_t)&_ap_bootstrap_start)

/**
 * @brief Sleep for a short period of time
 * @param delay How long to sleep for
 */
static void smp_delay(unsigned int delay) {
    uint64_t clock = clock_readTSC();
    while (clock_readTSC() < clock + delay * clock_getTSCSpeed());
}

/**
 * @brief Finish an AP's setup. This is done right after the trampoline code gets to 32-bit mode and sets up a stack
 * @param params The AP parameters set up by @c smp_prepareAP
 */
__attribute__((noreturn)) void smp_finalizeAP(smp_ap_parameters_t *params) {
    // Start by installing the IDT
    hal_installIDT();

    // Setup paging (WARNING: THIS WILL BE REWORKED FOR MULTIPLE APs!)
    mem_switchDirectory(mem_getCurrentDirectory());
    mem_setPaging(true);

    // I guess reinitialize the APIC?
    lapic_initialize(lapic_remapped);

    // Allow BSP to continue
    LOG(DEBUG, "CPU%i: AP online and ready\n", smp_getCurrentCPU());
    ap_startup_finished = 1;

    for (;;);
}

/**
 * @brief Prepare an AP's parameters
 * @param lapic_id The ID of the local APIC
 */
void smp_prepareAP(uint8_t lapic_id) {
    // Setup parameters
    smp_ap_parameters_t parameters;

    // Allocate a stack
    parameters.stack = (uint32_t)kmalloc(4096) + 4096; // Stack grows downward
    
    // Get the page directory & lapic id
    parameters.pagedir = (uint32_t)mem_getCurrentDirectory();
    parameters.lapic_id = lapic_id;

    // Copy!
    memcpy(TRAMPOLINE_REBASE(_ap_parameters), &parameters, sizeof(smp_ap_parameters_t));
}

/**
 * @brief Start an AP
 * @param lapic_id The ID of the local APIC to start
 */
void smp_startAP(uint8_t lapic_id) {
    ap_startup_finished = 0;
    LOG(DEBUG, "AP start 0x%x\n", lapic_id);

    // Copy the bootstrap code. The AP might've messed with it.
    memcpy((void*)bootstrap_page_remap, (void*)&_ap_bootstrap_start, (uintptr_t)&_ap_bootstrap_end - (uintptr_t)&_ap_bootstrap_start);

    // Setup the AP
    smp_prepareAP(lapic_id);

    // Send the INIT signal
    lapic_sendInit(lapic_id);
    smp_delay(5000UL);

    // Send SIPI
    lapic_sendStartup(lapic_id, SMP_AP_BOOTSTRAP_PAGE);

    // Wait for AP to finish
    do { asm volatile ("pause" ::: "memory"); } while (!ap_startup_finished);
}

/**
 * @brief Initialize the SMP system
 * @param info Collected SMP information
 * @returns 0 on success, non-zero is failure
 */
int smp_init(smp_info_t *info) {
    if (!info) return -EINVAL;
    smp_data = info;

    // Local APIC region is finite size - at least I hope.
    lapic_remapped = mem_mapMMIO((uintptr_t)smp_data->lapic_address, PAGE_SIZE);

    // Initialize the local APIC
    int lapic = lapic_initialize(lapic_remapped);
    if (lapic != 0) {
        LOG(ERR, "Failed to initialize local APIC");
        return -EIO;
    }

    // The AP expects its code to be bootstrapped to a page-aligned address (SIPI expects a starting page number)
    // The remapped page for SMP is stored in the variable SMP_AP_BOOTSTRAP_PAGE
    // Assuming that page has some content in it, copy and store it.
    // !!!: This is a bit hacky as we're playing with fire here. What if PMM_BLOCK_SIZE != PAGE_SIZE?
    uintptr_t temp_frame = pmm_allocateBlock();
    uintptr_t temp_frame_remap = mem_remapPhys(temp_frame, PAGE_SIZE);
    bootstrap_page_remap = mem_remapPhys(SMP_AP_BOOTSTRAP_PAGE, PAGE_SIZE);
    memcpy((void*)temp_frame_remap, (void*)bootstrap_page_remap, PAGE_SIZE);

    // Start APs
    // WARNING: Starting CPU0/BSP will triple fault (bad)
    LOG(DEBUG, "%i CPUs available\n", smp_data->processor_count);
    for (int i = 0; i < smp_data->processor_count; i++) {
        if (i != 0) {
            smp_startAP(smp_data->lapic_ids[i]);
        }
    }

    // Finished! Unmap bootstrap code
    memcpy((void*)bootstrap_page_remap, (void*)temp_frame_remap, PAGE_SIZE);
    mem_unmapPhys(temp_frame_remap, PAGE_SIZE);
    mem_unmapPhys(bootstrap_page_remap, PAGE_SIZE);
    pmm_freeBlock(temp_frame);

    return 0;
}

/**
 * @brief Get the amount of CPUs present in the system
 */
int smp_getCPUCount() {
    return smp_data->processor_count;
}

/**
 * @brief Get the current CPU's APIC ID
 */
int smp_getCurrentCPU() {
	uint32_t ebx, unused;
    __cpuid(0x1, unused, ebx, unused, unused);
    return (int)(ebx >> 24);
}