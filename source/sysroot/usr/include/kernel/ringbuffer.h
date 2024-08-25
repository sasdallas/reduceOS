// ringbuffer.h - Ringbuffer driver for reduceOS
// I do not own this implementation, it was found here: https://github.com/klange/toaruos/blob/master/kernel/misc/ringbuffer.c

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

// Includes
#include <stddef.h>
#include <kernel/list.h>
#include <kernel/vfs.h>
#include <spinlock.h>

// The main ringbuffer structure
typedef struct {
    unsigned char*  buffer;
    size_t          write_ptr;
    size_t          read_ptr;
    size_t          size;
    atomic_flag*    spinlock;
    list_t*         wait_queue_readers;
    list_t*         wait_queue_writers;
    int             internal_stop;
    list_t*         alert_waiters;
    int             discard;
    int             soft_stop;
} ringbuffer_t;

// Functions
size_t ringbuffer_unread(ringbuffer_t *ringbuffer);
size_t ringbuffer_size(fsNode_t *node);
size_t ringbuffer_available(ringbuffer_t *ringbuffer);
size_t ringbuffer_read(ringbuffer_t *ringbuffer, size_t size, uint8_t *buffer);
size_t ringbuffer_write(ringbuffer_t *ringbuffer, size_t size, uint8_t *buffer);

ringbuffer_t *ringbuffer_create(size_t size);
void ringbuffer_destroy(ringbuffer_t *ringbuffer);
void ringbuffer_interrupt(ringbuffer_t *ringbuffer);
void ringbuffer_alertWaiters(ringbuffer_t *ringbuffer);
void ringbuffer_selectWait(ringbuffer_t *ringbuffer, void *process);
void ringbuffer_eof(ringbuffer_t *ringbuffer);
void ringbuffer_discard(ringbuffer_t *ringbuffer);


#endif