/**
 * @file hexahedron/include/kernel/drivers/x86/acpica.h
 * @brief ACPICA kernel interface header
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifdef ACPICA_ENABLED

#ifndef DRIVERS_X86_ACPICA_H
#define DRIVERS_X86_ACPICA_H

/**** INCLUDES ****/
#include <acpica/acpi.h>
#include <stdint.h>

/**** FUNCTIONS ****/

/**
 * @brief Initialize ACPICA
 */
int ACPICA_Initialize();

/**
 * @brief Print the ACPICA namespace to serial (for debug)
 */
void ACPICA_PrintNamespace();

#endif

#endif