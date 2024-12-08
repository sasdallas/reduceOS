/**
 * @file hexahedron/arch/i386/cpu.c
 * @brief I386 CPU interfacer
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/i386/cpu.h>
#include <stddef.h>

/**
 * @brief Checks whether the RDMSR/WRMSR instructions are supported
 */
int cpu_msrAvailable() {
    static uint32_t eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);
    return edx & CPUID_FEAT_EDX_MSR;
}

/**
 * @brief Get a model-specific register
 * @param msr The MSR to get
 * @param lo The low bits (pointer output)
 * @param hi The high bits (pointer output)
 */
void cpu_getMSR(uint32_t msr, uint32_t *lo, uint32_t *hi) {
    if (!cpu_msrAvailable()) return;
    asm volatile ("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

/**
 * @brief Set a model-specific register
 * @param msr The MSR to set
 * @param lo The low bits to set with
 * @param hi The high bits to set with
 */
void cpu_setMSR(uint32_t msr, uint32_t lo, uint32_t hi) {
    if (!cpu_msrAvailable()) return;
    asm volatile ("wrmsr" :: "a"(lo), "d"(hi), "c"(msr));
}