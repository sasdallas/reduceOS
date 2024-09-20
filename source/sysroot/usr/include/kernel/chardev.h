// chardev.h - header file for the character device

#ifndef CHARDEV_H
#define CHARDEV_H

// Includes
#include <kernel/ringbuffer.h>
#include <kernel/vfs.h>
#include <libk_reduced/spinlock.h>
#include <libk_reduced/stdint.h>
#include <libk_reduced/stdlib.h>

// Typedefs
typedef struct {
    spinlock_t* lock;         // Character device spinlock
    ringbuffer_t* ringbuffer; // Ringbuffer
    int handles;              // Handles to the character device
} chardev_t;

// Functions
fsNode_t* chardev_create(size_t size, char* name);

#endif