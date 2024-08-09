// debugdev.h - Header file for the /device/debug handler

#ifndef DEBUG_DEV_H
#define DEBUG_DEV_H

// Includes
#include <stdio.h>
#include <kernel/vfs.h>
#include <kernel/serial.h>
#include <kernel/rtc.h>


// Functions
void debugdev_init();

#endif