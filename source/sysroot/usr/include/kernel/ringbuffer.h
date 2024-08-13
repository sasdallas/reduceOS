// ringbuffer.h - do I really need to keep writing headers for HEADER FILES

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

// Includes
#include <stdint.h>
#include <spinlock.h>
#include <stddef.h>
#include <kernel/vfs.h>
#include <kernel/list.h>

// Typedefs
typedef struct {
    unsigned char *buffer;
    size_t write_ptr;
    size_t read_ptr;
    size_t size;
    atomic_flag lock;
    list_t *wait_queue_readers;
    list_t *wait_queue_writers;
    int internal_stop;
    list_t *alert_waiters;
    int discard;
    int soft_stop;
} ringbuffer_t;

// Functions
size_t ring_buffer_unread(ringbuffer_t * ring_buffer);
size_t ring_buffer_size(fsNode_t * node);
size_t ring_buffer_available(ringbuffer_t * ring_buffer);
size_t ring_buffer_read(ringbuffer_t * ring_buffer, size_t size, uint8_t * buffer);
size_t ring_buffer_write(ringbuffer_t * ring_buffer, size_t size, uint8_t * buffer);

ringbuffer_t * ring_buffer_create(size_t size);
void ring_buffer_destroy(ringbuffer_t * ring_buffer);
void ring_buffer_interrupt(ringbuffer_t * ring_buffer);
void ring_buffer_alert_waiters(ringbuffer_t * ring_buffer);
void ring_buffer_select_wait(ringbuffer_t * ring_buffer, void * process);
void ring_buffer_eof(ringbuffer_t * ring_buffer);
void ring_buffer_discard(ringbuffer_t * ring_buffer);


#endif
