// ============================================
// ringbuffer.c - Ringbuffer implementation
// ============================================
// I do not own this implementation, it was sourced here: https://github.com/klange/toaruos/blob/master/kernel/misc/ringbuffer.c

#include <kernel/mem.h>
#include <kernel/process.h>
#include <kernel/ringbuffer.h> // Main header file
#include <kernel/vfs.h>
#include <libk_reduced/stddef.h>
#include <libk_reduced/stdint.h>

size_t ringbuffer_unread(ringbuffer_t* ringbuffer) {
    if (ringbuffer->read_ptr == ringbuffer->write_ptr) return 0;

    if (ringbuffer->read_ptr > ringbuffer->write_ptr) {
        return (ringbuffer->size - ringbuffer->read_ptr) + ringbuffer->write_ptr;
    } else {
        return (ringbuffer->write_ptr - ringbuffer->read_ptr);
    }
}

size_t ringbuffer_size(fsNode_t* node) {
    ringbuffer_t* ringbuffer = (ringbuffer_t*)node->device;
    return ringbuffer_unread(ringbuffer);
}

size_t ringbuffer_available(ringbuffer_t* ringbuffer) {
    if (ringbuffer->read_ptr == ringbuffer->write_ptr) return ringbuffer->size - 1;

    if (ringbuffer->read_ptr > ringbuffer->write_ptr) {
        return ringbuffer->read_ptr - ringbuffer->write_ptr - 1;
    } else {
        return (ringbuffer->size - ringbuffer->write_ptr) + ringbuffer->read_ptr - 1;
    }
}

static inline void ringbuffer_increment_read(ringbuffer_t* ringbuffer) {
    ringbuffer->read_ptr++;
    if (ringbuffer->read_ptr == ringbuffer->size) { ringbuffer->read_ptr = 0; }
}

static inline void ringbuffer_increment_write(ringbuffer_t* ringbuffer) {
    ringbuffer->write_ptr++;
    if (ringbuffer->write_ptr == ringbuffer->size) { ringbuffer->write_ptr = 0; }
}

void ringbuffer_alertWaiters(ringbuffer_t* ringbuffer) {
    if (ringbuffer->alert_waiters) {
        while (ringbuffer->alert_waiters->head) {
            node_t* node = list_dequeue(ringbuffer->alert_waiters); // Pop the top node off of the list
            process_t* proc = (process_t*)node->value;              // Each node is assigned to a process
            process_alert_node(proc, ringbuffer);
            kfree(node);
        }
    }
}

void ringbuffer_selectWait(ringbuffer_t* ringbuffer, void* process) {
    if (!ringbuffer->alert_waiters) { ringbuffer->alert_waiters = list_create(); }

    if (!list_find(ringbuffer->alert_waiters, process)) { list_insert(ringbuffer->alert_waiters, process); }

    list_insert(((process_t*)process)->nodeWaits, ringbuffer);
}

void ringbuffer_discard(ringbuffer_t* ringbuffer) {
    spinlock_lock(ringbuffer->spinlock);
    ringbuffer->read_ptr = ringbuffer->write_ptr;
    spinlock_release(ringbuffer->spinlock);
}

size_t ringbuffer_read(ringbuffer_t* ringbuffer, size_t size, uint8_t* buffer) {
    size_t collected = 0;
    while (collected == 0) {
        spinlock_lock(ringbuffer->spinlock);
        while (ringbuffer_unread(ringbuffer) > 0 && collected < size) {
            buffer[collected] = ringbuffer->buffer[ringbuffer->read_ptr];
            ringbuffer_increment_read(ringbuffer);
            collected++;
        }

        wakeup_queue(ringbuffer->wait_queue_writers);
        if (collected == 0) {
            if (ringbuffer->internal_stop || ringbuffer->soft_stop) {
                ringbuffer->soft_stop = 0;
                spinlock_release(ringbuffer->spinlock);
                return 0;
            }

            if (sleep_on_unlocking(ringbuffer->wait_queue_readers, ringbuffer->spinlock)) {
                return -512; // -ERESTARTSYS
            }
        } else {
            spinlock_release(ringbuffer->spinlock);
        }
    }
    wakeup_queue(ringbuffer->wait_queue_writers);
    return collected;
}

size_t ringbuffer_write(ringbuffer_t* ringbuffer, size_t size, uint8_t* buffer) {
    size_t written = 0;
    while (written < size) {
        spinlock_lock(ringbuffer->spinlock);

        while (ringbuffer_available(ringbuffer) > 0 && written < size) {
            ringbuffer->buffer[ringbuffer->write_ptr] = buffer[written];
            ringbuffer_increment_write(ringbuffer);
            written++;
        }

        wakeup_queue(ringbuffer->wait_queue_readers);
        ringbuffer_alertWaiters(ringbuffer);

        if (written < size) {
            if (ringbuffer->discard) {
                spinlock_release(ringbuffer->spinlock);
                break;
            }
            if (sleep_on_unlocking(ringbuffer->wait_queue_writers, ringbuffer->spinlock)) {
                if (!written) return -512; // -ERESTARTSYS
                break;
            }

            if (ringbuffer->internal_stop) break;
        } else {
            spinlock_release(ringbuffer->spinlock);
        }
    }

    wakeup_queue(ringbuffer->wait_queue_readers);
    ringbuffer_alertWaiters(ringbuffer);
    return written;
}

ringbuffer_t* ringbuffer_create(size_t size) {
    ringbuffer_t* out = kmalloc(sizeof(ringbuffer_t));

    if (size == 4096) {
        // Align it
        out->buffer = pmm_allocateBlock();
        vmm_allocateRegion((uintptr_t)out->buffer, (uintptr_t)out->buffer, 4096);
    } else {
        out->buffer = kmalloc(size);
    }

    out->write_ptr = 0;
    out->read_ptr = 0;
    out->size = size;
    out->alert_waiters = NULL;

    out->spinlock = spinlock_init();

    out->internal_stop = 0;
    out->discard = 0;
    out->soft_stop = 0;

    out->wait_queue_readers = list_create();
    out->wait_queue_writers = list_create();

    return out;
}

void ringbuffer_destroy(ringbuffer_t* ringbuffer) {
    if (ringbuffer->size == 4096) {
        // Destroy it but with alignment
        pmm_freeBlock(ringbuffer->buffer);
        vmm_allocateRegionFlags((uintptr_t)ringbuffer->buffer, (uintptr_t)ringbuffer->buffer, 4096, 0, 0,
                                0); // Force set the page's present flag to 0
    } else {
        kfree(ringbuffer->buffer);
    }

    wakeup_queue(ringbuffer->wait_queue_writers);
    wakeup_queue(ringbuffer->wait_queue_readers);
    ringbuffer_alertWaiters(ringbuffer);

    list_free(ringbuffer->wait_queue_writers);
    list_free(ringbuffer->wait_queue_readers);

    kfree(ringbuffer->wait_queue_writers);
    kfree(ringbuffer->wait_queue_readers);

    if (ringbuffer->alert_waiters) {
        list_free(ringbuffer->alert_waiters);
        kfree(ringbuffer->alert_waiters);
    }
}

void ringbuffer_interrupt(ringbuffer_t* ringbuffer) {
    ringbuffer->internal_stop = 1;
    wakeup_queue(ringbuffer->wait_queue_readers);
    wakeup_queue(ringbuffer->wait_queue_writers);
}

void ringbuffer_eof(ringbuffer_t* ringbuffer) {
    ringbuffer->soft_stop = 1;
    wakeup_queue(ringbuffer->wait_queue_readers);
    wakeup_queue(ringbuffer->wait_queue_writers);
}
