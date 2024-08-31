// ttydev.h - Header file for the TTY/PTY driver

#ifndef TTYDEV_H
#define TTYDEV_H

// Includes
#include <kernel/vfs.h>         // Virtual filesystem
#include <kernel/ringbuffer.h>  // Ringbuffer support
#include <libk_reduced/stdint.h>             // Integer declarations
#include <libk_reduced/termios.h>            // Terminal interface
#include <libk_reduced/ioctl.h>              // I/O Control

// Typedefs

// PTY structure
typedef struct _pty {
    intptr_t name;              // PTY number
    fsNode_t *master;           // Master endpoint
    fsNode_t *slave;            // Slave endpoint
    struct winsize size;        // Window size (width/height)
    struct termios tios;        // termios data
    ringbuffer_t *in;           // In pipe
    ringbuffer_t *out;          // Out pipe
    char *canon_buffer;         
    size_t canon_bufsize;
    size_t canon_buflen;
    int ct_proc;                // Controlling process
    int fg_proc;                // Foreground process

    void (*write_in)(struct _pty *, uint8_t);
    void (*write_out)(struct _pty *, uint8_t);

    int next_is_verbatim;

    void (*fill_name)(struct _pty *, uint8_t);

    void *_private;
} pty_t;

// Macros
#define MIN(a,b) ((a) < (b) ? (a) : (b))

// Functions
void tty_output_process_slave(pty_t *pty, uint8_t c);
void tty_output_process(pty_t *pty, uint8_t c);
void tty_input_process(pty_t *pty, uint8_t c);
pty_t *pty_new(struct winsize *size, int index);


#endif
