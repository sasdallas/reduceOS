// ==============================================================================
// tty.c - Filesystem driver for the teletype and psuedo-teletype (PTY) device
// ==============================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/ttydev.h>
#include <kernel/ringbuffer.h>
#include <kernel/vfs.h>
#include <libk_reduced/termios.h>

