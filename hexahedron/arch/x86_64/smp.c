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

#include <kernel/processor_data.h>
#include <kernel/arch/x86_64/smp.h>

/* CURRENTLY A STUB THAT HOLDS INFORMATION ON CPU */
/* CPU data */
processor_t processor_data[MAX_CPUS] = {0};

/* CPU count */
int processor_count = 1;