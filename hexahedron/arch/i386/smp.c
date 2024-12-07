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