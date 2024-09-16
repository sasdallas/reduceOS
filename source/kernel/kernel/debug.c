/**
 * @file source/kernel/kernel/debug.c
 * @brief Debugging system for reduceOS
 * 
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/debug.h>
#include <kernel/serial.h>
#include <libk_reduced/stdio.h>
#include <libk_reduced/stdarg.h>

// STUB FILE! ONLY FOR HEAVY DEBUGGING!


void heavy_dprintf(const char *fmt, ...) {
    uint16_t oldCOM = serial_getCOM();
    if (serial_changeCOM(SERIAL_COM2) == -1) {
        // Must be disabled
        return;
    }

    va_list(ap);
    va_start(ap, fmt);
    xvasprintf((xvas_callback)serialWrite, NULL, fmt, ap);
    va_end(ap);
    
    serial_changeCOM(oldCOM);
}