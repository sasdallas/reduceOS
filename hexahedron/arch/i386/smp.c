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
#include <kernel/drivers/x86/local_apic.h>

#include <kernel/mem/mem.h>
#include <kernel/mem/alloc.h>
#include <kernel/debug.h>

#include <stddef.h>
#include <errno.h>

/* SMP data */
static smp_info_t *smp_data = NULL;

/* Log method */
#define LOG(status, ...) dprintf_module(status, "SMP", __VA_ARGS__)

/**
 * @brief Initialize the SMP system
 * @param info Collected SMP information
 * @returns 0 on success, non-zero is failure
 */
int smp_init(smp_info_t *info) {
    if (!info) return -EINVAL;
    smp_data = info;

    // Local APIC region is finite size - at least I hope.
    uintptr_t lapic_remapped = mem_mapMMIO((uintptr_t)smp_data->lapic_address, PAGE_SIZE);

    // Initialize the local APIC
    int lapic = lapic_initialize(lapic_remapped);
    if (lapic != 0) {
        LOG(ERR, "Failed to initialize local APIC");
        return -EIO;
    }


    return 0;
}