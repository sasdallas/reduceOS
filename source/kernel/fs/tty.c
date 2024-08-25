// ==============================================================================
// tty.c - Filesystem driver for the teletype and psuedo-teletype device
// ==============================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/ttydev.h>
#include <kernel/ringbuffer.h>
#include <kernel/vfs.h>
