/**
 * @file source/kernel/fs/chardev.c
 * @brief Provides an interface for character devices to use.
 * 
 * This is just a complicated ringbuffer handler used for chardevs like the keyboard/mouse driver.
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/chardev.h>
#include <kernel/ringbuffer.h>
#include <libk_reduced/stdio.h>
#include <libk_reduced/stdint.h>

/* Read/write methods */

uint32_t chardev_read(fsNode_t* node, off_t offset, uint32_t size, uint8_t* buffer) {
    // Make sure to spin the character device's lock first.
    chardev_t* chardev = (chardev_t*)(node->impl_struct);
    if (!chardev || !chardev->lock) return 0;

    spinlock_lock(chardev->lock);
    size_t collected = ringbuffer_read(chardev->ringbuffer, size, buffer);
    spinlock_release(chardev->lock);

    return collected;
}

uint32_t chardev_write(fsNode_t* node, off_t offset, uint32_t size, uint8_t* buffer) {
    // Make sure to spin the character device's lock first.
    chardev_t* chardev = (chardev_t*)(node->impl_struct);
    if (!chardev || !chardev->lock) return 0;

    spinlock_lock(chardev->lock);
    size_t collected = ringbuffer_write(chardev->ringbuffer, size, buffer);
    spinlock_release(chardev->lock);

    return collected;
}

/* Open/close methods */

// Normally, these would just be NULL, but we need to keep chardev handles and free the device if nothing has it
void chardev_open(fsNode_t* node) {
    // Add a handle to it
    chardev_t* chardev = (chardev_t*)(node->impl_struct);
    if (!chardev) return;
    chardev->handles++;
}

void chardev_close(fsNode_t* node) {
    // Remove a handle from it
    chardev_t* chardev = (chardev_t*)(node->impl_struct);
    if (!chardev) return;
    chardev->handles--;

    // If there are no handles left we can free the chardev
    if (chardev->handles == 0) {
        // VFS should handle freeing the node though
        ringbuffer_destroy(chardev->ringbuffer);
        kfree((void*)chardev->lock);
        kfree(chardev);

        // Point impl_struct to NULL, hopefully our sanity checks will catch it.
        node->impl_struct = NULL;
    }
}

/* Exposed functions */

fsNode_t* chardev_create(size_t size, char* name) {
    fsNode_t* out = kmalloc(sizeof(fsNode_t));
    memset(out, 0, sizeof(fsNode_t));
    sprintf(out->name, "%s (pipe)", name);
    out->name[strlen(out->name)] = 0;

    chardev_t* chardev = kmalloc(sizeof(chardev_t));
    chardev->lock = spinlock_init();
    chardev->handles = 1;
    chardev->ringbuffer = ringbuffer_create(size);
    out->impl_struct = (uint32_t*)chardev;

    // Methods
    out->read = chardev_read;
    out->write = chardev_write;
    out->close = chardev_close;
    out->open = chardev_open;

    return out;
}
