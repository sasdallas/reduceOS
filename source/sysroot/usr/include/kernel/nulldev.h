// nulldev.h - header file for the /device/null and /device/zero/ parser

#ifndef NULLDEV_H
#define NULLDEV_H

// Includes
#include <stdint.h>
#include <kernel/vfs.h>

// Functions
void zerodev_init();
void nulldev_init();

#endif