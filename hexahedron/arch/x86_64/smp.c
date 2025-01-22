/**
 * @file hexahedron/arch/x86_64/smp.c
 * @brief Symmetric multiprocessing/processor data handler
 * 
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/arch/x86_64/smp.h>
#include <kernel/arch/x86_64/hal.h>
#include <kernel/arch/x86_64/interrupt.h>
#include <kernel/arch/x86_64/cpu.h>
#include <kernel/arch/x86_64/arch.h>
#include <kernel/processor_data.h>
#include <kernel/drivers/x86/local_apic.h>
#include <kernel/drivers/x86/clock.h>

#include <kernel/mem/pmm.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/alloc.h>
#include <kernel/debug.h>

#include <string.h>
#include <stddef.h>
#include <errno.h>

/* SMP data */
static smp_info_t *smp_data = NULL;

/* CPU data */
processor_t processor_data[MAX_CPUS] = {0};

/* CPU count */
int processor_count = 1;

/* Local APIC mmio address */
uintptr_t lapic_remapped = 0;

/* Remapped page for the bootstrap code */
static uintptr_t bootstrap_page_remap = 0;

/* Core stack - this is used after paging is setup */
uintptr_t _ap_stack_base = 0;

/* Trampoline variables */
extern uintptr_t _ap_bootstrap_start, _ap_bootstrap_end;

/* AP startup flag. This will change when the AP finishes starting */
static int ap_startup_finished = 0;

/* AP shutdown flag. This will change when the AP finishes shutting down. */
static int ap_shutdown_finished = 0;

/* Log method */
#define LOG(status, ...) dprintf_module(status, "SMP", __VA_ARGS__)

/**
 * @brief Finish an AP's setup. This is done right after the trampoline code gets to 32-bit mode and sets up a stack
 * @param params The AP parameters set up by @c smp_prepareAP
 */
__attribute__((noreturn)) void smp_finalizeAP() {
    // Load new stack
    asm volatile ("movq %0, %%rsp" :: "m"(_ap_stack_base));
    
    // Set GSbase
    arch_set_gsbase((uintptr_t)&processor_data[smp_getCurrentCPU()]);

    // We want all cores to have a consistent GDT
    hal_gdtInitCore(smp_getCurrentCPU(), _ap_stack_base);
    
    // Install the IDT
    extern void hal_installIDT();
    hal_installIDT();

    // Initialize FPU
    cpu_fpuInitialize();
\
    // Set current core's directory
    current_cpu->current_dir = mem_getKernelDirectory();

    // Reinitialize the APIC
    lapic_initialize(lapic_remapped);

    // Now collect information
    // smp_collectAPInfo(smp_getCurrentCPU());

    // Allow BSP to continue
    LOG(DEBUG, "CPU%i online and ready\n", smp_getCurrentCPU());
    ap_startup_finished = 1;

    for (;;);
}


/**
 * @brief Sleep for a short period of time
 * @param delay How long to sleep for
 */
static void smp_delay(unsigned int delay) {
    uint64_t clock = clock_readTSC();
    while (clock_readTSC() < clock + delay * clock_getTSCSpeed());
}

/**
 * @brief Start an AP
 * @param lapic_id The ID of the local APIC to start
 */
void smp_startAP(uint8_t lapic_id) {
    ap_startup_finished = 0;

    // Copy the bootstrap code. The AP might've messed with it.
    memcpy((void*)bootstrap_page_remap, (void*)&_ap_bootstrap_start, (uintptr_t)&_ap_bootstrap_end - (uintptr_t)&_ap_bootstrap_start);

    // Allocate a stack for the AP
    if (alloc_canHasValloc()) {
        _ap_stack_base = (uintptr_t)kvalloc(PAGE_SIZE) + PAGE_SIZE;
    } else {
        _ap_stack_base = (uintptr_t)mem_sbrk(PAGE_SIZE * 2) + (PAGE_SIZE);       // !!!: Giving two pages when we're only using one 
                                                                                // !!!: Stack alignment issues - you can also use kvalloc but some allocators don't support it in Hexahedron
    }

    memset((void*)(_ap_stack_base - 4096), 0, PAGE_SIZE);

    // Send the INIT signal
    lapic_sendInit(lapic_id);
    smp_delay(5000UL);

    // Send SIPI
    lapic_sendStartup(lapic_id, SMP_AP_BOOTSTRAP_PAGE);

    // Wait for AP to finish and set the startup flag
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

    processor_count = smp_data->processor_count;
    LOG(INFO, "SMP initialization completed successfully - %i CPUs available to system\n", processor_count);

    return 0;
}

/**
 * @brief Get the amount of CPUs present in the system
 */
int smp_getCPUCount() {
    return processor_count;
}

/**
 * @brief Get the current CPU's APIC ID
 */
int smp_getCurrentCPU() {
	uint32_t ebx, unused;
    __cpuid(0x1, unused, ebx, unused, unused);
    return (int)(ebx >> 24);
}